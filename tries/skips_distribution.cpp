#include <iostream>

#include "patricia_builder.hpp"
#include "succinct/broadword.hpp"
#include <boost/date_time/posix_time/posix_time_types.hpp>

struct skip_counter_visitor
{
    skip_counter_visitor()
        : skip_bits_counts(32)
        , n_strings(0)
    {}

    typedef size_t representation_type;

    representation_type leaf(const uint8_t* buf, size_t offset, size_t skip) 
    {
        n_strings += 1;
        return 0;
    }

    representation_type node(representation_type& left, representation_type& right, const uint8_t* buf, size_t offset, size_t skip) 
    {
        unsigned long i;
        if (succinct::broadword::msb(skip, i)) {
            skip_bits_counts[i + 1] += 1;
        } else {
            skip_bits_counts[0] += 1;
        }
        return 0;
    }

    void root(representation_type& tree)
    {}

    std::vector<size_t> skip_bits_counts;
    size_t n_strings;
};

int main(int argc, char** argv)
{
    using boost::posix_time::ptime;
    using boost::posix_time::microsec_clock;

    succinct::util::auto_file strings_file(argv[1]);
    succinct::tries::patricia_builder<skip_counter_visitor> builder;
    skip_counter_visitor v;

    ptime m_tick = microsec_clock::universal_time();
    builder.build(v, succinct::util::lines(strings_file.get(), true), succinct::tries::stl_string_adaptor());
    double elapsed = double((microsec_clock::universal_time() - m_tick).total_microseconds());

    std::cerr << v.n_strings << " strings processed, " << int(elapsed / 1000) / 1000.0 << " seconds elapsed, " << 
        elapsed / v.n_strings << " us per string" << std::endl;

    for (size_t i = 0; i < v.skip_bits_counts.size(); ++i) {
        std::cout << i << "\t" << v.skip_bits_counts[i] << std::endl;
    }
}
