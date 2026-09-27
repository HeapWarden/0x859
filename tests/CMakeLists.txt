include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.17.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(googletest)

add_executable(scsp_queue_tests TestSCSPQ.cpp)
target_link_libraries(scsp_queue_tests 
    PRIVATE
        scsp_queue
        GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(scsp_queue_tests)