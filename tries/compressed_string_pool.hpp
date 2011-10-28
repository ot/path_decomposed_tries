#pragma once

#include <boost/lambda/lambda.hpp>

#include "repair/repair.hpp"
#include "succinct/elias_fano.hpp"
#include "succinct/vbyte.hpp"

namespace succinct {
namespace tries {

    struct compressed_string_pool {
        
        typedef uint16_t char_type;

        compressed_string_pool() {}
        
	template <typename Range>
        compressed_string_pool(Range const& strings_seq)
        {
            using repair::code_type;
            typedef std::vector<char_type> word_type;

            std::vector<code_type> C;
            std::vector<word_type> D;
            
            repair::approximate_repair(strings_seq, C, D, true);

            std::vector<size_t> counts(D.size());
            for (size_t i = 0; i < C.size(); ++i) {
                counts[C[i]] += 1;
            }
            
            std::vector<code_type> sorted_codes(D.size() - 1);
            for (size_t i = 1; i < D.size(); ++i) sorted_codes[i - 1] = code_type(i);
            std::sort(sorted_codes.begin(), sorted_codes.end(),
                      boost::lambda::var(counts)[boost::lambda::_1] > 
		      boost::lambda::var(counts)[boost::lambda::_2]);
            
            std::vector<size_t> code_map(D.size(), -1);
            std::vector<char_type> dictionary;
            std::vector<uint16_t> word_positions;
            word_positions.push_back(0);
            for (size_t i = 0; i < sorted_codes.size(); ++i) {
                code_map[sorted_codes[i]] = i;
                word_type const& word = D[sorted_codes[i]];
                dictionary.insert(dictionary.end(), word.begin(), word.end());
                word_positions.push_back(code_type(dictionary.size()));
            }
            
            m_dictionary.steal(dictionary);
            m_word_positions.steal(word_positions);

            std::vector<uint8_t> byte_streams;
            std::vector<size_t> positions;
            positions.push_back(0);
            
            for (size_t i = 0; i < C.size(); ++i) {
                if (C[i]) {
                    size_t mapped_code = code_map[C[i]];
                    assert(mapped_code != -1);
                    append_vbyte(byte_streams, mapped_code);
                } else {
                    positions.push_back(byte_streams.size());
                }
            }

            elias_fano::elias_fano_builder positions_builder(positions.back() + 1, positions.size());
            for (size_t i = 0; i < positions.size(); ++i) {
                positions_builder.push_back(positions[i]);
            }

            m_byte_streams.steal(byte_streams);
            elias_fano(&positions_builder, false).swap(m_positions);
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
                if (m_word_begin == m_word_end) {
                    if (m_stream_begin == m_stream_end) return 0;

                    size_t code = 0;
                    m_stream_begin += decode_vbyte(m_sp->m_byte_streams, m_stream_begin, code);

                    m_word_begin = m_sp->m_word_positions[code];
                    m_word_end = m_sp->m_word_positions[code + 1];
                }

                return m_sp->m_dictionary[m_word_begin++];
            }

            friend struct compressed_string_pool;
        private:
            string_enumerator(compressed_string_pool const* sp, size_t idx)
                : m_sp(sp)
                , m_word_begin(0)
                , m_word_end(0)
            {
                std::pair<uint64_t, uint64_t> stream_range = m_sp->m_positions.select_range(idx);
                m_stream_begin = stream_range.first;
                m_stream_end = stream_range.second;
                m_sp->m_byte_streams.prefetch(m_stream_begin);
            }
            
            compressed_string_pool const* m_sp;
            size_t m_stream_begin, m_stream_end;
            size_t m_word_begin, m_word_end;
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

        void swap(compressed_string_pool& other)
        {
            m_dictionary.swap(other.m_dictionary);
            m_word_positions.swap(other.m_word_positions);
            m_byte_streams.swap(other.m_byte_streams);
            m_positions.swap(other.m_positions);
        }

        template <typename Visitor>
        void map(Visitor& visit) {
            visit
                (m_dictionary, "m_dictionary")
                (m_word_positions, "m_word_positions")
                (m_byte_streams, "m_byte_streams")
                (m_positions, "m_positions")
                ;
        }
        
    protected:

        mapper::mappable_vector<char_type> m_dictionary;
        mapper::mappable_vector<uint16_t> m_word_positions;
        
        mapper::mappable_vector<uint8_t> m_byte_streams;
        elias_fano m_positions;
    };

}
}
