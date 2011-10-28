#pragma once

#include <algorithm>

#include "succinct/util.hpp"

namespace succinct {
namespace tries {

    using util::char_range;
    using util::stl_string_adaptor;

    template <typename Bytes>
    inline uint8_t get_byte(Bytes const& buf, size_t buf_bit_len, size_t offset)
    {
	assert(offset < buf_bit_len);
	size_t byte_pos = offset / 8;
	size_t byte_offset = offset % 8;
	size_t byte_len = util::ceil_div(buf_bit_len, 8);

	uint8_t ret = buf[byte_pos] << byte_offset;
	if (byte_offset && (byte_pos + 1) < byte_len) {
	    ret |= buf[byte_pos + 1] >> (8 - byte_offset);
	}
	return ret;
    }

    template <typename Bytes>
    inline bool get_bit(Bytes const& buf, size_t offset)
    {
	return (buf[offset / 8] >> (7 - (offset % 8))) & 1;
    }

    namespace detail {
	static const char MSBTable8[256] = 
	    {
#define MSB_LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
		-1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
		MSB_LT(4), MSB_LT(5), MSB_LT(5), MSB_LT(6), MSB_LT(6), MSB_LT(6), MSB_LT(6),
		MSB_LT(7), MSB_LT(7), MSB_LT(7), MSB_LT(7), MSB_LT(7), MSB_LT(7), MSB_LT(7), MSB_LT(7)
#undef MSB_LT
	    };
    }

    inline char MSB8(uint8_t x) {
	assert(x != 0);
	return detail::MSBTable8[x];
    }

    template <typename Bytes1, typename Bytes2>
    inline int64_t find_mismatching_bit(Bytes1 const& buf1, size_t offset1, size_t bit_len1,
					Bytes2 const& buf2, size_t offset2, size_t bit_len2)
    {
	size_t min_len = std::min(bit_len1, bit_len2);
	size_t bytes_len = util::ceil_div(min_len, 8);
  
	for (size_t i = 0; i < bytes_len; ++i) {
	    size_t o1 = offset1 + i * 8;
	    size_t o2 = offset2 + i * 8;
	    uint8_t b1 = get_byte(buf1, offset1 + bit_len1, o1);
	    uint8_t b2 = get_byte(buf2, offset2 + bit_len2, o2);
	    if (b1 != b2) {
		uint8_t x = b1 ^ b2;
		size_t shift = 7 - MSB8(x);
		size_t ret = i * 8 + shift;
		if (ret >= min_len) {
		    return -1;
		} else {
		    return ret;
		}
	    }
	}
	return -1;
    }

    // for debug 
    template <typename Bytes>
    inline std::string binary(Bytes const& buf, size_t offset, size_t bit_len) {
	std::string ret;
	for (size_t i = 0; i < bit_len; ++i) {
	    ret += '0' + (int)get_bit(buf, offset + i);
	}
	return ret;
    }

}
}
