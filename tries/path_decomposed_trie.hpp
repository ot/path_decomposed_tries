#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/static_assert.hpp>

#include "succinct/bp_vector.hpp"
#include "succinct/elias_fano.hpp"
#include "succinct/forward_enumerator.hpp"

#include "compacted_trie_builder.hpp"

namespace succinct {
namespace tries {
 
    // When Lexicographic is false, centroid path decomposition is used
    template <typename LabelsPoolType, bool Lexicographic = false>
    struct path_decomposed_trie
    {
        typedef LabelsPoolType labels_pool_type;
        typedef mapper::mappable_vector<uint8_t> branching_chars_type;

        path_decomposed_trie()
	{
            BOOST_STATIC_ASSERT(sizeof(typename labels_pool_type::char_type) >= sizeof(label_char_type));
        }

	template <typename Range, typename Adaptor>
	path_decomposed_trie(Range const& strings, Adaptor adaptor = stl_string_adaptor()) 
	{
	    build(strings, adaptor);
	}
	
	template <typename Range>
	path_decomposed_trie(Range const& strings) 
	{
	    build(strings, stl_string_adaptor());
	}

	template <typename T, typename Adaptor>
	size_t index(T const& val, Adaptor adaptor) const
	{
	    char_range s = adaptor(val);
            size_t len = boost::size(s);

	    size_t cur_pos = 0;
	    size_t cur_node_pos = 1;

            size_t first_child_rank = 0;

            while (true) {
                size_t rank0 = cur_node_pos - first_child_rank - 1; // == m_bp.rank0(node_end);
                if (cur_pos == len) return rank0; // assume the string is null-terminated

                m_branching_chars.prefetch(first_child_rank);
                typename labels_pool_type::string_enumerator label_enumerator = m_labels.get_string_enumerator(rank0);
                
                size_t branching_chars_begin = 0;
                size_t branching_chars = 0;
                size_t last_branching_point = -1;
                while (true) {
                    if (cur_pos == len) return -1;

                    typename labels_pool_type::char_type label = label_enumerator.next();
                    if (label >= branching_point) {
                        branching_chars_begin += branching_chars;
                        branching_chars = label - branching_point + 1;
                        last_branching_point = cur_pos;
                    } else {
                        uint8_t c = s.first[cur_pos];
                        if (label != c) {
                            if (last_branching_point != cur_pos) return -1;
                            break;
                        }
                        cur_pos += 1;
                        if (!label) {
                            if (cur_pos == len) return rank0;
                            else return -1;
                        }
                    }
                }
                    
                bool found_child = false;
                
                for (size_t i = branching_chars_begin; i < branching_chars_begin + branching_chars; ++i) {
                    uint8_t c = m_branching_chars[first_child_rank + i];
                    if (s.first[cur_pos] == c) {
                        cur_pos += 1;
                        found_child = true;

                        size_t child = i;
                        assert(child < m_bp.successor0(cur_node_pos) - cur_node_pos);
                        size_t child_open = cur_node_pos + child;
                        cur_node_pos = m_bp.find_close(child_open) + 1;
                        assert((cur_node_pos - child_open) % 2 == 0);
                        first_child_rank += child + (cur_node_pos - child_open) / 2;
                        break;
                    }
                }
                
                if (!found_child) return -1;
            }
            assert(false);
            return 0;
        }

	template <typename T>
	size_t index(T const& val) const
	{
	    return index(val, stl_string_adaptor());
	}

        std::string operator[](size_t idx) const
        {
            std::string ret;
            ret.reserve(256); // reasonable tradeoff

            size_t rank0 = idx;
            size_t cur_node_pos = idx ? m_bp.select0(idx - 1) : 0;
	    size_t last_rank0 = rank0;
	    size_t next_opener = cur_node_pos ? m_bp.find_open(cur_node_pos) : 0;

	    typename labels_pool_type::string_enumerator label_enumerator;
            
            while (cur_node_pos) {
                size_t opener_pos = next_opener;
                rank0 = rank0 - (cur_node_pos - opener_pos + 1) / 2;
                assert(m_bp.select0(rank0) == m_bp.successor0(opener_pos));

                size_t parent_pos = rank0 ? m_bp.predecessor0(opener_pos) : 0;
                size_t child_idx = opener_pos - parent_pos - 1;

                cur_node_pos = parent_pos;

                m_branching_chars.prefetch(opener_pos - rank0 - 1);
                label_enumerator = m_labels.get_string_enumerator(rank0);

		// while the prefetcher is going get the branching
		// char and the labels, we can find the next node
		if (cur_node_pos) {
		    next_opener = m_bp.find_open(cur_node_pos);
		}

                uint8_t branching_char = m_branching_chars[opener_pos - rank0 - 1];
		if (branching_char) {
		    ret.push_back(branching_char);
		}

                size_t cur_suffix_size = ret.size();
                size_t branching_chars_begin = 0;

                while (true) {
                    typename labels_pool_type::char_type c = label_enumerator.next();
                    assert(c);
                    if (c < 256) {
                        ret.push_back((char)c);
                    } else {
                        size_t branching_chars = c - branching_point + 1;
                        if (child_idx < branching_chars_begin + branching_chars) break;
                        branching_chars_begin += branching_chars;
                    }
                }

                std::reverse(ret.begin() + cur_suffix_size, ret.end());
            }
            
            assert(rank0 == 0);
            std::reverse(ret.begin(), ret.end());

	    // append the string tail
            label_enumerator = m_labels.get_string_enumerator(last_rank0);
            while (true) {
                typename labels_pool_type::char_type c = label_enumerator.next();
                if (!c) break;
                if (c < 256) {
                    ret.push_back((char)c);
                } else { 
                    // ignore branching points
                }
            }
            
            return ret;
        }
        
        size_t size() const
        {
            return m_bp.size() / 2;
        }

	void swap(path_decomposed_trie& other)
        {
	    m_bp.swap(other.m_bp);
	    m_branching_chars.swap(other.m_branching_chars);
            m_labels.swap(other.m_labels);
	}

        template <typename Visitor>
        void map(Visitor& visit) {
            visit
                (m_bp, "m_bp")
                (m_branching_chars, "m_branching_chars")
                (m_labels, "m_labels")
		;
        }

        bp_vector const& get_bp() const
        {
            return m_bp;
        }

        branching_chars_type const& get_branching_chars() const
        {
            return m_branching_chars;
        }
        
        labels_pool_type const& get_labels() const
        {
            return m_labels;
        }
        
    private:

        typedef uint16_t label_char_type;
        static const size_t branching_point = 256;
        
	struct centroid_builder_visitor
	{
	    centroid_builder_visitor()
	    {}

	    struct subtree {
		std::vector<label_char_type> m_centroid_path_string;
                std::vector<uint8_t> m_centroid_path_branches;

		bit_vector_builder m_bp;
                std::vector<uint8_t> m_branching_chars;
		std::vector<label_char_type> m_labels;

		size_t size() const
		{
		    return (m_bp.size() + 1) / 2 + m_centroid_path_branches.size();
		}

		void append_to(subtree& tree)
		{
                    if (m_centroid_path_string.size()) {
                        tree.m_labels.insert(tree.m_labels.end(), m_centroid_path_string.rbegin(), m_centroid_path_string.rend());
                    } else {
                        // we need this to obtain the right number of strings in the pool, and we have to special-case 0s anyway
                        tree.m_labels.push_back(0); 
                    }
		    assert(tree.m_labels.back() == 0);

		    tree.m_bp.one_extend(m_centroid_path_branches.size());
		    tree.m_bp.push_back(0);

                    tree.m_branching_chars.insert(tree.m_branching_chars.end(), m_centroid_path_branches.rbegin(), m_centroid_path_branches.rend());

		    tree.m_bp.append(m_bp);
		    util::dispose(m_bp);
		    tree.m_branching_chars.insert(tree.m_branching_chars.end(), m_branching_chars.begin(), m_branching_chars.end());
		    util::dispose(m_branching_chars);
		    tree.m_labels.insert(tree.m_labels.end(), m_labels.begin(), m_labels.end());
		    util::dispose(m_labels);
		}
	    };

	    typedef boost::shared_ptr<subtree> representation_type;
            typedef std::vector<std::pair<uint8_t, representation_type> > children_type;

            representation_type node(children_type& children, const uint8_t* buf, size_t offset, size_t skip) 
	    {
		representation_type ret;

                if (children.size()) {
                    assert(children.size() > 1);
                    
                    size_t largest_child = -1;
                    if (Lexicographic) {
                        largest_child = 0;
                    } else {
                        size_t largest_child_size = 0;
                    
                        for (size_t i = 0; i < children.size(); ++i) {
                            if (i == 0 || children[i].second->size() > largest_child_size) {
                                largest_child = i;
                                largest_child_size = children[i].second->size();
                            }
                        }
                    }
                    assert(largest_child != -1);

                    children[largest_child].second.swap(ret);
                    size_t n_branches = children.size() - 1;
                    assert(n_branches > 0);
                    assert(n_branches <= std::numeric_limits<label_char_type>::max());
                    ret->m_centroid_path_string.push_back(children[largest_child].first);
                    ret->m_centroid_path_string.push_back(label_char_type(branching_point + n_branches - 1));

                    // append children (note that branching chars are reversed)
                    for (size_t i = 0; i < children.size(); ++i) {
                        if (i != largest_child) {
                            ret->m_centroid_path_branches.push_back(children[i].first);
                            children[i].second->append_to(*ret);
                        }
                    }
                } else {
                    ret = boost::make_shared<subtree>();
                }

                // append in reverse order
                for (size_t i = offset + skip - 1; i != offset - 1; --i) {
                    ret->m_centroid_path_string.push_back(buf[i]);
                }

                return ret;
	    }

	    void root(representation_type& root_node)
	    {
		representation_type ret = boost::make_shared<subtree>();

                ret->m_bp.reserve(root_node->m_bp.size() + root_node->m_centroid_path_branches.size() + 2);
		ret->m_bp.push_back(1); // DFUDS fake root
		root_node->append_to(*ret);
		assert(ret->m_bp.size() % 2 == 0);

		m_root_node = ret;
	    }

	    representation_type get_root() const 
	    {
		return m_root_node;
	    }

	private:
	    representation_type m_root_node;
	};


	template <typename Range, typename Adaptor>
	void build(Range const& strings, Adaptor adaptor) 
	{
	    centroid_builder_visitor visitor;
	    succinct::tries::compacted_trie_builder<centroid_builder_visitor> builder;
	    builder.build(visitor, strings, adaptor);
	    typename centroid_builder_visitor::representation_type root = visitor.get_root();
	    
            bp_vector(&root->m_bp, false, true).swap(m_bp);
            branching_chars_type(root->m_branching_chars).swap(m_branching_chars);
            labels_pool_type(root->m_labels).swap(m_labels);
            assert(m_labels.size() == m_bp.size() / 2);
	}

	bp_vector m_bp;
        branching_chars_type m_branching_chars;
        
        labels_pool_type m_labels;
        
    };

}
}
