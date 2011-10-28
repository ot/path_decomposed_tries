#define BOOST_TEST_MODULE hollow_trie
#include "succinct/test_common.hpp"
#include "test_binary_trie_common.hpp"

#include "hollow_trie.hpp"

BOOST_AUTO_TEST_CASE(hollow_trie)
{
    test_index_binary<succinct::tries::hollow_trie<succinct::mapper::mappable_vector<uint64_t> > >();
}
