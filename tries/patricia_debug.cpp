#include <iostream>

#include "patricia_builder.hpp"
#include "centroid_hollow_trie.hpp"

struct log_visitor
{
    log_visitor()
	: cur_node(0)
    {}

    typedef size_t representation_type;

    representation_type leaf(const uint8_t* buf, size_t offset, size_t skip) 
    {
	std::cout << cur_node << "\tleaf\t" 
		  << offset << "\t" << skip << "\t" << succinct::tries::binary(buf, offset, skip) << std::endl;
	return cur_node++;
    }

    representation_type node(representation_type& left, representation_type& right, const uint8_t* buf, size_t offset, size_t skip) 
    {
	std::cout << cur_node << "\tnode\t" 
		  << left << "\t" << right << "\t" << offset << "\t" << skip << "\t" << succinct::tries::binary(buf, offset, skip) << std::endl;
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
    std::cout << "Patricia trie:" << std::endl;

    std::string strings[] = {"a", "aa", "aaa", "abac", "bbccd", "cx", "x"};
    size_t n_strings = sizeof(strings) / sizeof(strings[0]);
    
    for (size_t i = 0; i < n_strings; ++i) { 
	std::cout << strings[i] << "\t" << succinct::tries::binary(strings[i].c_str(), 0, (strings[i].size() + 1) * 8) << std::endl;
    }

    succinct::tries::patricia_builder<log_visitor> builder;
    log_visitor v;
    builder.build(v, std::make_pair(strings, strings + n_strings), succinct::tries::stl_string_adaptor());

    {
	std::cout << std::endl
		  << "Centroid hollow trie:"
		  << std::endl;

	succinct::tries::centroid_hollow_trie ct(std::make_pair(strings, strings + n_strings));
	print_sequence(ct.get_bp());
	print_sequence(ct.get_skips(), " ");

	std::cout << std::endl;

	for (size_t i = 0; i < n_strings; ++i) {
	    std::cout << "--------------- " << strings[i] << " " << ct.index(strings[i]) << std::endl;
	}
    }
}
