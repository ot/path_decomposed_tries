#pragma once

#include <algorithm>
#include <vector>

#include <boost/range.hpp>

#include "succinct/util.hpp"
#include "bit_strings.hpp"

namespace succinct {
namespace tries {

    template <typename TreeBuilder>
    struct compacted_trie_builder
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
	    
	    stack.push_back(node(0, last_string.size()));
	    
            for (++iter; iter != boost::end(strings); ++iter) {
		char_range cur_string = adaptor(*iter);

                size_t min_len = std::min(boost::size(last_string), boost::size(cur_string));
                std::pair<const uint8_t*, std::vector<uint8_t>::const_iterator> mm = 
                    std::mismatch(cur_string.first,
                                  cur_string.first + min_len,
                                  last_string.begin());
                
                if (mm.first == cur_string.second || mm.second == last_string.end()) {
                    if (mm.first == cur_string.second && mm.second == last_string.end()) {
                        throw std::invalid_argument("Duplicate string found");
                    } else {
			throw std::invalid_argument("Input range are not prefix-free");
                    }
                }
		
                if (*mm.first < *mm.second) {
		    throw std::invalid_argument("Input range is not sorted");
		}
		
                size_t mismatch = mm.first - cur_string.first;
		size_t cur_node_idx = 0;
		    
		// find the node to split
		while (mismatch > stack[cur_node_idx].path_len + stack[cur_node_idx].skip) {
		    assert(cur_node_idx < stack.size());
		    ++cur_node_idx;
		}
		node& cur_node = stack[cur_node_idx];
		
		assert(mismatch >= cur_node.path_len);
		assert(mismatch <= cur_node.path_len + cur_node.skip);

		// close all open nodes below the current branching point
                for (size_t node_idx = stack.size() - 1; node_idx > cur_node_idx; --node_idx) {
                    node& child = stack[node_idx];
                    typename TreeBuilder::representation_type subtrie = 
                        visitor.node(child.children, &last_string[0], child.path_len, child.skip);
                    uint8_t branching_char = last_string[child.path_len - 1];
                    stack[node_idx - 1].children.push_back(std::make_pair(branching_char, subtrie));
                }
                stack.resize(cur_node_idx + 1);

                // if the current string splits the skip, split the current node
                if (mismatch < cur_node.path_len + cur_node.skip) {
                    typename TreeBuilder::representation_type subtrie = 
                        visitor.node(cur_node.children, &last_string[0], mismatch + 1, cur_node.path_len + cur_node.skip - mismatch - 1);
                    uint8_t branching_char = last_string[mismatch];
                    cur_node.children.clear();
                    cur_node.children.push_back(std::make_pair(branching_char, subtrie));
                    cur_node.skip = mismatch - cur_node.path_len;
                }
                
                assert(cur_node.path_len + cur_node.skip == mismatch);
		// open a new leaf with the current suffix
                stack.push_back(node(mismatch + 1, boost::size(cur_string) - mismatch - 1));
                
		// copy the current string (iter could be invalid in next iteration)
		last_string.assign(cur_string.first, cur_string.second);
	    }
	    
	    // close the remaining path
            for (size_t node_idx = stack.size() - 1; node_idx > 0; --node_idx) {
                node& child = stack[node_idx];
                typename TreeBuilder::representation_type subtrie = 
                        visitor.node(child.children, &last_string[0], child.path_len, child.skip);
                uint8_t branching_char = last_string[child.path_len - 1];
                stack[node_idx - 1].children.push_back(std::make_pair(branching_char, subtrie));
            }
            
            typename TreeBuilder::representation_type root = visitor.node(stack[0].children, &last_string[0], stack[0].path_len, stack[0].skip);
	    visitor.root(root);
	    stack.clear();
	}

    private:
	struct node {
	    node() 
                : skip(-1) 
            {}
	    
	    node(size_t path_len_, size_t skip_)
		: path_len(path_len_)
		, skip(skip_)
	    {}

	    size_t path_len;
	    size_t skip;
            
            typedef std::pair<uint8_t, typename TreeBuilder::representation_type> subtrie;
            std::vector<subtrie> children;
	};
    };
}
}


