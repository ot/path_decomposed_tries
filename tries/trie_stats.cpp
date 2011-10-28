#include <iostream>
#include <math.h>

#include "compacted_trie_builder.hpp"
#include "patricia_builder.hpp"
#include "path_decomposed_trie.hpp"
#include "centroid_hollow_trie.hpp"
#include "vbyte_string_pool.hpp"
#include "succinct/util.hpp"
#include "succinct/mapper.hpp"

double mylog2(double x)
{
    return log(x) / log(double(2));
}

double H(size_t n, size_t k)
{
    double p = double(k) / n;
    double q = 1 - p;
    return p * mylog2(1 / p) + q * mylog2(1 / q);
}

struct stats_visitor
{
    stats_visitor()
    {}

    struct subtree {
	size_t n_leaves;
	size_t cum_height;
	size_t n_internal_nodes;
	size_t labels_size;
    };

    typedef subtree representation_type;
    typedef std::vector<std::pair<uint8_t, representation_type> > children_type;

    representation_type node(children_type& children, const uint8_t* buf, size_t offset, size_t skip) 
    {
	representation_type ret;
	ret.labels_size = skip;

	if (children.size()) {
	    ret.n_leaves = 0;
	    ret.cum_height = 0;
	    ret.n_internal_nodes = 1;
	    
	    for (size_t i = 0; i < children.size(); ++i) {
		representation_type const& child = children[i].second;
		ret.n_leaves += child.n_leaves;
		ret.cum_height += child.cum_height + child.n_leaves;
		ret.n_internal_nodes += child.n_internal_nodes;
		ret.labels_size += 1 + child.labels_size;
	    }
	    
	} else {
	    ret.n_leaves = 1;
	    ret.cum_height = 0;
	    ret.n_internal_nodes = 0;
	}
	return ret;
    }

    void root(representation_type& root_node)
    {
	m_root_node = root_node;
    }

    double avg_height() const {
	return double(m_root_node.cum_height) / m_root_node.n_leaves;
    }

    double LT() const {
	size_t E = m_root_node.labels_size;
	return E * 8 + E * H(E, m_root_node.n_internal_nodes + m_root_node.n_leaves - 1);
    }

private:
    representation_type m_root_node;
};

struct binary_stats_visitor
{
    binary_stats_visitor()
    {}

    struct subtree {
	size_t n_leaves;
	size_t cum_height;
	size_t n_internal_nodes;
	size_t labels_size;
    };

    typedef subtree representation_type;
    typedef std::vector<std::pair<uint8_t, representation_type> > children_type;

    representation_type leaf(const uint8_t* buf, size_t offset, size_t skip) 
    {
	representation_type ret;
	ret.n_leaves = 1;
	ret.cum_height = 0;
	ret.n_internal_nodes = 0;
	ret.labels_size = skip;
	return ret;
    }
	
    representation_type node(representation_type& left, representation_type& right, const uint8_t* buf, size_t offset, size_t skip) 
    {
	representation_type ret;
	ret.n_leaves = 0;
	ret.cum_height = 0;
	ret.n_internal_nodes = 1;
	ret.labels_size = skip;
	
	ret.n_leaves += left.n_leaves;
	ret.cum_height += left.cum_height + left.n_leaves;
	ret.n_internal_nodes += left.n_internal_nodes;
	ret.labels_size += 1 + left.labels_size;

	ret.n_leaves += right.n_leaves;
	ret.cum_height += right.cum_height + right.n_leaves;
	ret.n_internal_nodes += right.n_internal_nodes;
	ret.labels_size += 1 + right.labels_size;

	return ret;
    }

    void root(representation_type& root_node)
    {
	m_root_node = root_node;
    }

    double avg_height() const {
	return double(m_root_node.cum_height) / m_root_node.n_leaves;
    }

    double LT() const {
	size_t E = m_root_node.labels_size;
	return E + E * H(E, m_root_node.n_internal_nodes + m_root_node.n_leaves - 1);
    }

private:
    representation_type m_root_node;
};

struct pathdec_tree_stats
{
    size_t n_nodes, cum_height, height;
};
    
pathdec_tree_stats visit_pathdec(succinct::bp_vector const& bp, size_t node)
{
    pathdec_tree_stats ret;
    ret.n_nodes = 1;
    ret.cum_height = 0;

    size_t cur_node = node;
    size_t cur_height = 0;
    while (bp[cur_node]) {
	pathdec_tree_stats child_stats = visit_pathdec(bp, bp.find_close(cur_node) + 1);
	cur_height = std::max(cur_height, child_stats.height);
	ret.cum_height += child_stats.cum_height + child_stats.n_nodes;
	ret.n_nodes += child_stats.n_nodes;
	cur_node += 1;
    }
    ret.height = cur_height;
    ret.cum_height += cur_height;

    return ret;
}

double avg_height(succinct::bp_vector const& bp)
{
    pathdec_tree_stats s = visit_pathdec(bp, 1);
    return double(s.cum_height) / s.n_nodes;
}

int main(int argc, char** argv)
{
    succinct::util::mmap_lines lines(argv[1]);
    { // compacted trie
	stats_visitor v;
	succinct::tries::compacted_trie_builder<stats_visitor> builder;
	builder.build(v, lines, succinct::tries::stl_string_adaptor());
	std::cout << "byte_trie_avg_height\t" << v.avg_height() << std::endl;
	std::cout << "byte_trie_LT\t" << v.LT() << std::endl;
    }

    { // patricia
	binary_stats_visitor v;
	succinct::tries::patricia_builder<binary_stats_visitor> builder;
	builder.build(v, lines, succinct::tries::stl_string_adaptor());
	std::cout << "patricia_avg_height\t" << v.avg_height() << std::endl;
	std::cout << "patricia_LT\t" << v.LT() << std::endl;
    }

    { // centroid trie
	succinct::tries::path_decomposed_trie<succinct::tries::vbyte_string_pool, false> t(lines);
	std::cout << "centroid_byte_avg_height\t" << avg_height(t.get_bp()) << std::endl;
	std::cout << "centroid_byte_bitsize\t" << succinct::mapper::size_of(t) * 8 << std::endl;
    }

    { // lex trie
	succinct::tries::path_decomposed_trie<succinct::tries::vbyte_string_pool, true> t(lines);
	std::cout << "lex_byte_avg_height\t" << avg_height(t.get_bp()) << std::endl;
	std::cout << "lex_byte_bitsize\t" << succinct::mapper::size_of(t) * 8 << std::endl;
    }

    { // centroid hollow trie
	succinct::tries::centroid_hollow_trie t(lines);
	std::cout << "centroid_hollow_avg_height\t" << avg_height(t.get_bp()) << std::endl;
    }

}
