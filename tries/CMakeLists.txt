add_executable(patricia_debug patricia_debug.cpp)
target_link_libraries(patricia_debug
    succinct
    ${Boost_LIBRARIES}
    )

add_executable(compacted_trie_debug compacted_trie_debug.cpp)
target_link_libraries(compacted_trie_debug
    succinct
    ${Boost_LIBRARIES}
    )

add_executable(skips_distribution skips_distribution.cpp)
target_link_libraries(skips_distribution
    succinct
    ${Boost_LIBRARIES}
    )

add_executable(trie_stats trie_stats.cpp)
target_link_libraries(trie_stats
    succinct
    ${Boost_LIBRARIES}
    )

file(GLOB TRIES_TEST_SOURCES test_*.cpp)
foreach(TEST_SRC ${TRIES_TEST_SOURCES})
  get_filename_component (TEST_SRC_NAME ${TEST_SRC} NAME_WE)
  add_executable(${TEST_SRC_NAME} ${TEST_SRC})
  target_link_libraries(${TEST_SRC_NAME}
    succinct
    ${Boost_LIBRARIES}
    )
  add_test(${TEST_SRC_NAME} ${TEST_SRC_NAME})
endforeach(TEST_SRC)
