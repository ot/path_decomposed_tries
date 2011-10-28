#define BOOST_TEST_MODULE centroid_trie
#include "succinct/test_common.hpp"
#include "test_binary_trie_common.hpp"

#include "vbyte_string_pool.hpp"
#include "compressed_string_pool.hpp"
#include "path_decomposed_trie.hpp"

BOOST_AUTO_TEST_CASE(path_decomposed_trie)
{
    // Centroid trie only roundtrips
    test_trie_roundtrip<succinct::tries::path_decomposed_trie<succinct::tries::vbyte_string_pool> >();
    test_trie_roundtrip<succinct::tries::path_decomposed_trie<succinct::tries::compressed_string_pool> >();

    // Lexicographic one also has monotone indexes
    test_index_binary<succinct::tries::path_decomposed_trie<succinct::tries::vbyte_string_pool, true> >();
    test_trie_roundtrip<succinct::tries::path_decomposed_trie<succinct::tries::vbyte_string_pool, true> >();
}
