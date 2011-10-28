#pragma once

#include <algorithm>
#include <vector>

#include <boost/range.hpp>

#include "succinct/util.hpp"
#include "bit_strings.hpp"

namespace succinct {
namespace tries {

    template <typename TreeBuilder>
    struct patricia_builder
    {
	template <typename Range, typename Adaptor>
	void build(TreeBuilder& visitor, Range const& strings, Adaptor adaptor)
	{
	    if (boost::empty(strings)) return;

	    typedef typename boost::range_const_iterator<Range>::type iterator_t;
	    iterator_t iter = boost::begin(strings);
	    
	    std::vector<node> stack;
	    char_range first_string = adaptor(*iter);
	    std::vector<uint8_t> last_string(boost::begin(first_string), boost::end(first_string));
	    
	    stack.push_back(node(0, last_string.size() * 8));
	    
            for (++iter; iter != boost::end(strings); ++iter) {
		char_range cur_string = adaptor(*iter);

		size_t cur_bit_len = boost::size(cur_string) * 8;
		size_t last_bit_len = last_string.size() * 8;

		int64_t mismatch = find_mismatching_bit(cur_string.first, 0, cur_bit_len,
							last_string, 0, last_bit_len);

		if (mismatch == -1) {
		    if (last_bit_len == cur_bit_len) {
			throw std::invalid_argument("Duplicate string found");
		    } else {
			throw std::invalid_argument("Input range are not prefix-free");
		    }
		}
		
		if (get_bit(cur_string.first, mismatch) != 1) {
		    throw std::invalid_argument("Input range is not sorted");
		}
		    
		size_t cur_node_idx = 0;
		    
		// find the node to split
		while (size_t(mismatch) > stack[cur_node_idx].path_len + stack[cur_node_idx].skip) {
		    assert(cur_node_idx < stack.size());
		    ++cur_node_idx;
		}
		node& cur_node = stack[cur_node_idx];
		
		assert(size_t(mismatch) >= cur_node.path_len);
		assert(size_t(mismatch) < cur_node.path_len + cur_node.skip);

		// close all open nodes up to the current branching point
		typename TreeBuilder::representation_type left_subtree;
		if (cur_node_idx == stack.size() - 1) {
		    left_subtree = visitor.leaf(&last_string[0], mismatch + 1, last_bit_len - mismatch - 1);
		} else {
		    typename TreeBuilder::representation_type right_subtree = visitor.leaf(&last_string[0], stack.back().path_len, last_bit_len - stack.back().path_len);
		    for (size_t node_idx = stack.size() - 2; node_idx > cur_node_idx; --node_idx) {
			right_subtree = visitor.node(stack[node_idx].left_subtree, right_subtree, 
						     &last_string[0], stack[node_idx].path_len, stack[node_idx].skip);
		    }
		    left_subtree = visitor.node(cur_node.left_subtree, right_subtree, 
						&last_string[0], mismatch + 1, cur_node.path_len + cur_node.skip - mismatch - 1);
		}
		// cut the stack at position cur_node_idx and push the splitted node
		size_t cur_path_len = cur_node.path_len;
		stack.resize(cur_node_idx);
		stack.push_back(node(cur_path_len, mismatch - cur_path_len, left_subtree));
		
		// open a new leaf with the current suffix
		stack.push_back(node(mismatch + 1, cur_bit_len - mismatch - 1));
		
		// copy the current string (iter could be invalid in next iteration)
		last_string.assign(cur_string.first, cur_string.second);
	    }
	    
	    // close the remaining path
	    typename TreeBuilder::representation_type right_subtree = visitor.leaf(&last_string[0], stack.back().path_len, stack.back().skip);
	    if (stack.size() >= 2) {
		for (size_t node_idx = stack.size() - 2; node_idx + 1 >= 1; --node_idx) {
		    right_subtree = visitor.node(stack[node_idx].left_subtree, right_subtree, 
						 &last_string[0], stack[node_idx].path_len, stack[node_idx].skip);
		}
	    }
	    visitor.root(right_subtree);
	    stack.clear();
	}
	
    private:
	struct node {
	    node() : skip(-1) {}
	    
	    node(size_t path_len_, size_t skip_, typename TreeBuilder::representation_type left_subtree_)
		: path_len(path_len_)
		, skip(skip_)
		, left_subtree(left_subtree_)
	    {}

	    node(size_t path_len_, size_t skip_)
		: path_len(path_len_)
		, skip(skip_)
	    {}

	    size_t path_len;
	    size_t skip;
	    typename TreeBuilder::representation_type left_subtree;
	};
    };
}
}


