add_executable(tries_perftest tries_perftest.cpp)
target_link_libraries(tries_perftest
    succinct
    ${Boost_LIBRARIES}
    )
