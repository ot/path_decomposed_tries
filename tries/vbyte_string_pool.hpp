#pragma once

#include <sstream>

#include "succinct/elias_fano.hpp"
#include "succinct/vbyte.hpp"

namespace succinct {
namespace tries {

    struct vbyte_string_pool {

        typedef size_t char_type;
        
        vbyte_string_pool() {}
        
	template <typename Range>
        vbyte_string_pool(Range const& strings_seq)
        {
	    typedef typename boost::range_const_iterator<Range>::type iterator_t;
            size_t n = 0, sum = 0;
            for (iterator_t iter = strings_seq.begin(); iter != strings_seq.end(); ++iter) {
                if (!*iter) ++n;
                else sum += vbyte_size(*iter);
            }

            std::vector<uint8_t> byte_streams;
            byte_streams.reserve(sum);

            elias_fano::elias_fano_builder positions(sum + 1, n + 1);
            positions.push_back(0);
            
            char_type c;
            for (iterator_t iter = strings_seq.begin(); iter != strings_seq.end(); ++iter) {
                c = *iter;
                if (c) {
                    append_vbyte(byte_streams, c);
                } else {
                    positions.push_back(byte_streams.size());
                }
            }
            
            assert(!c); // check last char is 0

            m_byte_streams.steal(byte_streams);
            elias_fano(&positions, false).swap(m_positions);
        }

        size_t size() const
        {
            return m_positions.num_ones() - 1;
        }

        struct string_enumerator
        {
            string_enumerator()
                : m_sp(0)
            {}

            char_type next()
            {
                assert(m_sp);
                if (m_begin == m_end) return 0;
                char_type val;
                m_begin += decode_vbyte(m_sp->m_byte_streams, m_begin, val);
                return val;
            }

            friend struct vbyte_string_pool;
        private:
            string_enumerator(vbyte_string_pool const* sp, size_t idx)
                : m_sp(sp)
            {
                std::pair<uint64_t, uint64_t> string_range = m_sp->m_positions.select_range(idx);
                m_begin = string_range.first;
                m_end = string_range.second;
                m_sp->m_byte_streams.prefetch(m_begin);
            }
            
            vbyte_string_pool const* m_sp;
            size_t m_begin, m_end;
        };

        string_enumerator get_string_enumerator(size_t idx) const
        {
            return string_enumerator(this, idx);
        }

        std::string get_string(size_t idx) const
        {
            // only for debug 
            std::ostringstream os;
            string_enumerator e = get_string_enumerator(idx);
            size_t c;
            while ((c = e.next()) != 0) {
                if (c >= 32 && c < 256) {
                    os << (char)c;
                } else {
                    os << '[' << c << ']';
                }
            }
            return os.str();
        }

        void swap(vbyte_string_pool& other)
        {
            m_byte_streams.swap(other.m_byte_streams);
            m_positions.swap(other.m_positions);
        }

        template <typename Visitor>
        void map(Visitor& visit) {
            visit
                (m_byte_streams, "m_byte_streams")
                (m_positions, "m_positions")
                ;
        }
        
    protected:
        mapper::mappable_vector<uint8_t> m_byte_streams;
        elias_fano m_positions;
    };

}
}
