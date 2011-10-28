#include <cassert>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <algorithm>

#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include "succinct/mapper.hpp"

#include "succinct/elias_fano_list.hpp"
#include "succinct/gamma_vector.hpp"

#include "tries/hollow_trie.hpp"
#include "tries/centroid_hollow_trie.hpp"
#include "tries/path_decomposed_trie.hpp"

#include "tries/vbyte_string_pool.hpp"
#include "tries/compressed_string_pool.hpp"

#include "perftest_common.hpp"


#define SLOW_BENCHMARK 0

class benchmark
{
public:
    virtual ~benchmark() {}
    virtual int prepare(std::string benchmark_name, std::string strings_filename, std::string output_filename, std::vector<std::string> args) = 0;
    virtual int measure(std::string benchmark_name, std::string filename, std::string sample_filename, std::vector<std::string> args) = 0;
};

class sample : public benchmark
{
    virtual int prepare(std::string benchmark_name, std::string strings_filename, std::string output_filename, std::vector<std::string> args)
    { 
        const size_t sample_size = 1000000;
        srand(42);

        std::vector<std::string> strings_sample;
        strings_sample.reserve(sample_size);

        size_t n = 0;
        
        std::cerr << "Sampling input... ";
        BOOST_FOREACH(std::string const& line, succinct::util::mmap_lines(strings_filename)) {
            ++n;
        
            if (strings_sample.size() < sample_size) {
                strings_sample.push_back(line);
            } else {
                unsigned int r = (rand() * RAND_MAX + rand()) % (n - 1);
                if (r < sample_size) {
                    strings_sample[r] = line;
                }
            }
        }
        std::cerr << n << " strings." << std::endl << std::endl;

        std::random_shuffle(strings_sample.begin(), strings_sample.end());

        std::ofstream fout(output_filename.c_str(), std::ios::binary);
        std::copy(strings_sample.begin(), strings_sample.end(), std::ostream_iterator<std::string>(fout, "\n"));
        return 0;
    }

    virtual int measure(std::string benchmark_name, std::string filename, std::string sample_filename, std::vector<std::string> args)
    {
        std::cerr << "No 'measure' on 'sample'" << std::endl;
        return 1;
    }
};

template <typename Trie>
class benchmark_trie_index : public benchmark
{
public:
    virtual int prepare(std::string benchmark_name, std::string strings_filename, std::string output_filename, std::vector<std::string> args)
    {
        Trie trie;
        TIMEIT(benchmark_name + " - construction", 1) {
            Trie(succinct::util::mmap_lines(strings_filename)).swap(trie);
        }

        succinct::mapper::size_tree_of(trie)->dump();
        std::cerr <<
            "bits per string " << succinct::mapper::size_of(trie) * 8.0 / trie.size() << std::endl;

        succinct::mapper::freeze(trie, output_filename.c_str());
        return 0;
    }

    virtual int measure(std::string benchmark_name, std::string filename, std::string sample_filename, std::vector<std::string> args)
    {
	succinct::util::mmap_lines sample_lines(sample_filename);
	std::vector<std::string> strings_sample(sample_lines.begin(), sample_lines.end());
        
        boost::iostreams::mapped_file_source m(filename);
        Trie trie;
        succinct::mapper::map(trie, m, succinct::mapper::map_flags::warmup);
        
        volatile size_t foo;
        TIMEIT(benchmark_name + " - random queries - warmup", strings_sample.size()) {
            for (size_t i = 0; i < strings_sample.size(); ++i) {
                if (i + 1 < strings_sample.size()) {
                    succinct::intrinsics::prefetch(&strings_sample[i + 1][0]);
                }
                foo = trie.index(strings_sample[i]);
            }
        }
        
#if SLOW_BENCHMARK
        TIMEIT(benchmark_name + " - random queries - 10 repetitions", strings_sample.size() * 10) {
            for (size_t round = 0; round < 10; ++round) {
                for (size_t i = 0; i < strings_sample.size(); ++i) {
                    foo = trie.index(strings_sample[i]);
                }
            }
        }
#endif
        return 0;
    }
};

template <typename Trie>
class benchmark_trie_2way : public benchmark_trie_index<Trie>
{
public:
    virtual int measure(std::string benchmark_name, std::string filename, std::string sample_filename, std::vector<std::string> args)
    {
        benchmark_trie_index<Trie>::measure(benchmark_name, filename, sample_filename, args);
        boost::iostreams::mapped_file_source m(filename);
        Trie trie;
        succinct::mapper::map(trie, m, succinct::mapper::map_flags::warmup);
        
        size_t sample_size = 1000000;

        std::vector<size_t> indices(sample_size);
        for (size_t i = 0; i < sample_size; ++i) {
            indices[i] = (rand() * RAND_MAX + rand()) % trie.size();
        }

        volatile size_t foo;
        TIMEIT(benchmark_name + " - random queries - warmup", indices.size()) {
            for (size_t i = 0; i < indices.size(); ++i) {
                foo = trie[indices[i]].size();
            }
        }
        
#if SLOW_BENCHMARK
        TIMEIT(benchmark_name + " - random queries - 10 repetitions", indices.size() * 10) {
            for (size_t round = 0; round < 10; ++round) {
                for (size_t i = 0; i < indices.size(); ++i) {
                    foo = trie[indices[i]].size();
                }
            }
        }
#endif
        return 0;

    }
};


typedef std::map<std::string, boost::shared_ptr<benchmark> > benchmarks_type;

void print_benchmarks(benchmarks_type const& benchmarks)
{
    std::cerr << "Available benchmarks: " << std::endl;
    for (benchmarks_type::const_iterator iter = benchmarks.begin();
         iter != benchmarks.end();
         ++iter) {
        std::cerr << iter->first << std::endl;
    }
}

int main(int argc, char** argv)
{
    using boost::shared_ptr;
    using boost::make_shared;

    benchmarks_type benchmarks;

    benchmarks["sample"] = make_shared<sample>();

    benchmarks["hollow_gamma"] = make_shared<benchmark_trie_index<succinct::tries::hollow_trie<succinct::gamma_vector> > >();
    benchmarks["hollow_elias"] = make_shared<benchmark_trie_index<succinct::tries::hollow_trie<succinct::elias_fano_list> > >();
    benchmarks["hollow_vector"] = make_shared<benchmark_trie_index<succinct::tries::hollow_trie<succinct::mapper::mappable_vector<uint16_t> > > >();

    benchmarks["centroid"] = make_shared<benchmark_trie_2way<succinct::tries::path_decomposed_trie<succinct::tries::vbyte_string_pool > > >();
    benchmarks["centroid_repair"] = make_shared<benchmark_trie_2way<succinct::tries::path_decomposed_trie<succinct::tries::compressed_string_pool > > >();

    benchmarks["lex"] = make_shared<benchmark_trie_2way<succinct::tries::path_decomposed_trie<succinct::tries::vbyte_string_pool, true> > >();
    benchmarks["lex_repair"] = make_shared<benchmark_trie_2way<succinct::tries::path_decomposed_trie<succinct::tries::compressed_string_pool, true> > >();

    if (argc == 1) {
        print_benchmarks(benchmarks);
        return 1;
    } else if (argc < 1 + 1 + 1 + 2) {
        std::cerr << "Missing arguments" << std::endl;
        return 1;
    } else {
        const char* b = argv[1];
        if (!benchmarks.count(b)) {
            std::cerr << "No benchmark " << b << std::endl;
            print_benchmarks(benchmarks);
            return 1;
        } else {
            shared_ptr<benchmark> inst = benchmarks[b];
            if (std::string(argv[2]) == "prepare") {
                return inst->prepare(b, argv[3], argv[4], std::vector<std::string>(argv + 5, argv + argc));
            } else if (std::string(argv[2]) == "measure") {
                return inst->measure(b, argv[3], argv[4], std::vector<std::string>(argv + 5, argv + argc));
            } else { 
                std::cerr << "Invalid command" << std::endl;
                return 1;
            }
        }
    }
}
