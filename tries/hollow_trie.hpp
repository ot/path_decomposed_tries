#pragma once

#include <numeric>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include "succinct/bp_vector.hpp"

#include "patricia_builder.hpp"

namespace succinct {
namespace tries {

    template <typename SkipsType>
    struct hollow_trie {

        typedef SkipsType skips_type;

	hollow_trie()
	{}

	template <typename Range, typename Adaptor>
	hollow_trie(Range const& strings, Adaptor adaptor = stl_string_adaptor()) 
	{
	    build(strings, adaptor);
	}
	
	template <typename Range>
	hollow_trie(Range const& strings) 
	{
	    build(strings, stl_string_adaptor());
	}
	
	template <typename T, typename Adaptor>
	size_t index(T const& val, Adaptor adaptor) const
	{
	    char_range s = adaptor(val);
	    size_t bit_len = boost::size(s) * 8;
	    size_t cur_pos = 0;
	    size_t cur_node = 1;
	    size_t cur_rank = 0; // double counting -- to save the ranks
	    while (true) {
		if (!m_bp[cur_node]) {
		    return cur_rank;
		}
		cur_pos += m_skips[cur_node - cur_rank - 1];
		if (cur_pos >= bit_len) return size_t(-1);
		bool b = get_bit(s.first, cur_pos);
		cur_pos += 1;
		if (b) {
		    size_t next_node = m_bp.find_close(cur_node) + 1;
		    cur_rank += (next_node - cur_node) / 2;
		    cur_node = next_node;
		} else {
		    cur_node = cur_node + 1;
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

	void swap(hollow_trie& other)
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

	bp_vector m_bp;
	skips_type m_skips;

	struct bp_builder_visitor
	{
	    bp_builder_visitor()
	    {}

	    struct subtree {
		bit_vector_builder m_bp;
		std::vector<size_t> m_skips;
	    };

	    typedef boost::shared_ptr<subtree> representation_type;

	    representation_type leaf(const uint8_t* buf, size_t offset, size_t skip) const
	    {
		representation_type ret = boost::make_shared<subtree>();
		ret->m_bp.push_back(0);
		return ret;
	    }

	    representation_type node(representation_type& left, representation_type& right, const uint8_t* buf, size_t offset, size_t skip) const
	    {
		representation_type ret = boost::make_shared<subtree>();
		ret->m_bp.push_back(1);
		ret->m_skips.push_back(skip);

		ret->m_bp.append(left->m_bp);
		util::dispose(left->m_bp);
		ret->m_skips.insert(ret->m_skips.end(), left->m_skips.begin(), left->m_skips.end());
		util::dispose(left->m_skips);

		ret->m_bp.append(right->m_bp);
		util::dispose(right->m_bp);
		ret->m_skips.insert(ret->m_skips.end(), right->m_skips.begin(), right->m_skips.end());
		util::dispose(right->m_skips);

		assert(ret->m_bp.size() - 1 == 2 * ret->m_skips.size());
		assert(ret->m_bp.size() % 2 == 1);
		
		return ret;
	    }

	    void root(representation_type& tree)
	    {
                bit_vector_builder bv;
                bv.reserve(tree->m_bp.size() + 1);
                bv.push_back(1); // Add fake root
                bv.append(tree->m_bp);
                bv.swap(tree->m_bp);

		assert(tree->m_bp.size() % 2 == 0);
		m_root_node = tree;		
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
	    bp_builder_visitor visitor;
	    succinct::tries::patricia_builder<bp_builder_visitor> builder;
	    builder.build(visitor, strings, adaptor);
	    typename bp_builder_visitor::representation_type root = visitor.get_root();
	    
	    bp_vector(&root->m_bp, true, false).swap(m_bp);
            skips_type(root->m_skips).swap(m_skips);
	}
	
	
    };
}
}
