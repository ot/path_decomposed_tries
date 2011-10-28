add_executable(repair_file repair_file.cpp)
target_link_libraries(repair_file
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
