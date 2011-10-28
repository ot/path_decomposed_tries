#define BOOST_TEST_MODULE compressed_string_pool
#include "succinct/test_common.hpp"

#include "succinct/util.hpp"
#include "compressed_string_pool.hpp"

BOOST_AUTO_TEST_CASE(compressed_string_pool)
{

    succinct::util::auto_file f("propernames");
    succinct::util::line_iterator begin(f.get(), true), end;

    std::vector<std::string> strings;
    std::vector<uint8_t> strings_stream;
    
    for (; begin != end; ++begin) {
	strings.push_back(*begin);
        strings_stream.insert(strings_stream.end(), begin->c_str(), begin->c_str() + begin->size() + 1);
    }

    succinct::tries::compressed_string_pool sp(strings_stream);
    BOOST_REQUIRE_EQUAL(strings.size(), sp.size());
    
    for (size_t i = 0; i < strings.size(); ++i) {
        succinct::tries::compressed_string_pool::string_enumerator e = sp.get_string_enumerator(i);
        for (size_t pos = 0; pos < strings[i].size(); ++pos) {
            uint8_t c = e.next();
            MY_REQUIRE_EQUAL(strings[i][pos], c, "i = " << i << " pos = " << pos);
        }
        uint8_t c = e.next();
        MY_REQUIRE_EQUAL(0, c, "i = " << i);

        MY_REQUIRE_EQUAL(strings[i], sp.get_string(i), "i = " << i);
    }
}
