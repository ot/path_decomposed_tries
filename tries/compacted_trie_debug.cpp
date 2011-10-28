#include <iostream>

#include "compacted_trie_builder.hpp"
#include "path_decomposed_trie.hpp"
#include "vbyte_string_pool.hpp"

struct log_visitor
{
    log_visitor()
	: cur_node(0)
    {}

    typedef size_t representation_type;
    typedef std::vector<std::pair<uint8_t, representation_type> > children_type;

    representation_type node(children_type& children, const uint8_t* buf, size_t offset, size_t skip) 
    {
	std::cout << cur_node << "\t";
        if (children.size()) {
            std::cout << "node\t";
        } else { 
            std::cout << "leaf\t";
        }
        for (children_type::iterator iter = children.begin(); iter != children.end(); ++iter) {
            std::cout << "'" << (char)iter->first << "'\t" << iter->second << "\t";
        }
        std::cout << offset << "\t" << skip << "\t'" << std::string((const char*)(buf + offset), skip) << "'" << std::endl;

	return cur_node++;
    }

    void root(representation_type& tree)
    {
	std::cout << tree << " root" << std::endl;
    }

private:
    representation_type cur_node;
};

template <typename T>
void print_sequence(T const& seq, std::string const& sep = "")
{
    for (size_t i = 0; i < seq.size(); ++i) {
        std::cout << seq[i] << sep;
    }
    std::cout << std::endl;
}

int main()
{
    std::cout << "Compacted trie:" << std::endl;

    std::string strings[] = {"a", "aa", "aaa", "abac", "bbccd", "bbcce", "bbcd", "bbce","ccx", "cx", "x"};
    size_t n_strings = sizeof(strings) / sizeof(strings[0]);
    
    for (size_t i = 0; i < n_strings; ++i) { 
	std::cout << strings[i] << std::endl;
    }

    succinct::tries::compacted_trie_builder<log_visitor> builder;
    log_visitor v;
    builder.build(v, std::make_pair(strings, strings + n_strings), succinct::tries::stl_string_adaptor());

    {
	std::cout << std::endl
		  << "Centroid byte trie:"
		  << std::endl;

	succinct::tries::path_decomposed_trie<succinct::tries::vbyte_string_pool> ct(std::make_pair(strings, strings + n_strings));
	print_sequence(ct.get_bp());
	print_sequence(ct.get_branching_chars());
	
	succinct::tries::vbyte_string_pool const& sp = ct.get_labels();
	for (size_t i = 0; i < sp.size(); ++i) {
	    std::cout << sp.get_string(i) << "|";
	}

	std::cout << std::endl;

	for (size_t i = 0; i < n_strings; ++i) {
            size_t idx = ct.index(strings[i]);
            std::cout << "--------------- " << strings[i] << " " << idx;
            assert(idx != -1);
            std::cout << " '" << ct[idx] << "'";
            std::cout << std::endl;
        }
    }

    {
	std::cout << std::endl
		  << "Lexicographic trie:"
		  << std::endl;

	succinct::tries::path_decomposed_trie<succinct::tries::vbyte_string_pool, true> ct(std::make_pair(strings, strings + n_strings));
	print_sequence(ct.get_bp());
	print_sequence(ct.get_branching_chars());
	
	succinct::tries::vbyte_string_pool const& sp = ct.get_labels();
	for (size_t i = 0; i < sp.size(); ++i) {
	    std::cout << sp.get_string(i) << "|";
	}

	std::cout << std::endl;

	for (size_t i = 0; i < n_strings; ++i) {
            size_t idx = ct.index(strings[i]);
            std::cout << "--------------- " << strings[i] << " " << idx;
            assert(idx != -1);
            std::cout << " '" << ct[idx] << "'";
            std::cout << std::endl;
        }
    }

}
