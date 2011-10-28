#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include "succinct/bp_vector.hpp"
#include "succinct/forward_enumerator.hpp"
#include "succinct/gamma_bit_vector.hpp"

#include "patricia_builder.hpp"

namespace succinct {
namespace tries {

    struct centroid_hollow_trie
    {
        typedef gamma_bit_vector skips_type;

        centroid_hollow_trie()
	{}

	template <typename Range, typename Adaptor>
	centroid_hollow_trie(Range const& strings, Adaptor adaptor = stl_string_adaptor()) 
	{
	    build(strings, adaptor);
	}
	
	template <typename Range>
	centroid_hollow_trie(Range const& strings) 
	{
	    build(strings, stl_string_adaptor());
	}

	template <typename T, typename Adaptor>
	size_t index(T const& val, Adaptor adaptor) const
	{
	    char_range s = adaptor(val);
	    size_t bit_len = boost::size(s) * 8;

	    size_t cur_pos = 0;
	    size_t cur_node_pos = 1;
            size_t right_ancestors = 0;

            size_t first_child_rank = 0;

            while (true) {
                size_t node_end = m_bp.successor0(cur_node_pos); 
                size_t node_deg = node_end - cur_node_pos;

                bool found_mismatch = false;
                size_t taken_directions[2] = {0, 0};

                forward_enumerator<skips_type> skips_enumerator(m_skips, first_child_rank);

                for (size_t i = 0; i < node_deg; ++i) {
		    skips_type::value_type skip_bit = skips_enumerator.next();
                    cur_pos += skip_bit >> 1;
                    bool dir = skip_bit & 1;

                    if (cur_pos >= bit_len) return size_t(-1);
                    bool b = get_bit(s.first, cur_pos);
                    cur_pos += 1;

                    if (b != dir) {
                        found_mismatch = true;
                        size_t child;
                        if (!b) {
                            child = taken_directions[1];
                            right_ancestors += 1;
                        } else {
                            child = node_deg - taken_directions[0] - 1;
                        }
                        assert(child < node_deg);
                        size_t child_open = node_end - child - 1;
                        cur_node_pos = m_bp.find_close(child_open) + 1;
                        assert((cur_node_pos - child_open) % 2 == 0);
                        first_child_rank += (node_deg - child - 1) + (cur_node_pos - child_open) / 2;
                        
                        break;
                    }
                    taken_directions[dir] += 1;
                }
                
                if (!found_mismatch) {
                    size_t rank0 = cur_node_pos - first_child_rank - 1; // == m_bp.rank0(node_end);
                    if (node_deg) {
                        assert(taken_directions[0] != 0); // there is at least a right subtrie
                        size_t first_right_subtrie = node_end - taken_directions[1] - 1;
                        size_t left_leaves = (m_bp.find_close(first_right_subtrie) - first_right_subtrie) / 2;
                        return rank0 + left_leaves - right_ancestors;
                    } else {
                        return rank0 - right_ancestors;
                    }
                }
            }

            assert(false);
        }

	template <typename T>
	size_t index(T const& val) const
	{
	    return index(val, stl_string_adaptor());
	}
        
        size_t size() const
        {
            return m_bp.size() / 2;
        }

	void swap(centroid_hollow_trie& other)
        {
	    m_bp.swap(other.m_bp);
	    m_skips.swap(other.m_skips);
	}

        template <typename Visitor>
        void map(Visitor& visit) {
            visit
                (m_bp, "m_bp")
                (m_skips, "m_skips")
		;
        }

        bp_vector const& get_bp() const
        {
            return m_bp;
        }

        skips_type const& get_skips() const
        {
            return m_skips;
        }
        
    private:

	struct centroid_builder_visitor
	{
	    centroid_builder_visitor()
	    {}

	    struct subtree {
		std::vector<uint64_t> m_centroid_path_skips;

		bit_vector_builder m_bp;
		std::vector<uint64_t> m_skips;

		size_t size() const
		{
		    return m_bp.size() + m_centroid_path_skips.size() + 1;
		}

		void append_to(bool close_path, subtree& tree)
		{
		    if (close_path) {
                        tree.m_skips.insert(tree.m_skips.end(), m_centroid_path_skips.rbegin(), m_centroid_path_skips.rend());
                        tree.m_bp.one_extend(m_centroid_path_skips.size());
			tree.m_bp.push_back(0);
		    }
		    tree.m_bp.append(m_bp);
		    util::dispose(m_bp);
		    tree.m_skips.insert(tree.m_skips.end(), m_skips.begin(), m_skips.end());
		    util::dispose(m_skips);
		}
	    };

	    typedef boost::shared_ptr<subtree> representation_type;

	    representation_type leaf(const uint8_t* buf, size_t offset, size_t skip) const
	    {
		representation_type ret = boost::make_shared<subtree>();
		return ret;
	    }

	    representation_type node(representation_type& left, representation_type& right, const uint8_t* buf, size_t offset, size_t skip) const
	    {
		representation_type ret = boost::make_shared<subtree>();
		
		bool centroid_direction;
		if (left->size() >= right->size()) {
		    std::swap(ret->m_centroid_path_skips, left->m_centroid_path_skips);
		    centroid_direction = 0;
		} else {
		    std::swap(ret->m_centroid_path_skips, right->m_centroid_path_skips);
		    centroid_direction = 1;
		}
		ret->m_centroid_path_skips.push_back((uint64_t(skip) << 1) | uint64_t(centroid_direction));
                
                size_t closing_centroid_path = left->m_centroid_path_skips.size() + right->m_centroid_path_skips.size();
                ret->m_bp.reserve(left->m_bp.size() + right->m_bp.size() + closing_centroid_path + 1);
                ret->m_skips.reserve(left->m_skips.size() + right->m_skips.size() + closing_centroid_path);

		left->append_to(centroid_direction == 1, *ret);
		right->append_to(centroid_direction == 0, *ret);
		
		return ret;
	    }

	    void root(representation_type& root_node)
	    {
		representation_type ret = boost::make_shared<subtree>();

                ret->m_bp.reserve(root_node->m_bp.size() + root_node->m_centroid_path_skips.size() + 2);
                ret->m_skips.reserve(root_node->m_skips.size() + root_node->m_centroid_path_skips.size());
		ret->m_bp.push_back(1); // DFUDS fake root
		root_node->append_to(true, *ret);
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
	    succinct::tries::patricia_builder<centroid_builder_visitor> builder;
	    builder.build(visitor, strings, adaptor);
	    typename centroid_builder_visitor::representation_type root = visitor.get_root();
	    
            bp_vector(&root->m_bp, false, false).swap(m_bp);
            skips_type(root->m_skips).swap(m_skips);
	}

	bp_vector m_bp;
	skips_type m_skips;
    };

}
}
