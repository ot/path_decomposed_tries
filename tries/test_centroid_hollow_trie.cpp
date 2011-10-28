#define BOOST_TEST_MODULE centroid_hollow_trie
#include "succinct/test_common.hpp"
#include "test_binary_trie_common.hpp"

#include "succinct/gamma_vector.hpp"
#include "centroid_hollow_trie.hpp"

BOOST_AUTO_TEST_CASE(centroid_hollow_trie)
{
    test_index_binary<succinct::tries::centroid_hollow_trie>();
}
