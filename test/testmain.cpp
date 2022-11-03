#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <cstdint>

#include <sstream>
#include <iostream>
#include <map>
#include "api.h"

// TEST_CASE( "hello() says hello" ) {
//     std::ostringstream out;
// 	std::streambuf* coutbuf = std::cout.rdbuf();
	
//     std::cout.rdbuf(out.rdbuf()); //redirect cout to out
//     hello("tester");
// 	hello("you");
//     std::cout.rdbuf(coutbuf);

// 	REQUIRE( out.str() == "Hello, tester!\nHello, you!\n");
// }

std::uint64_t Fibonacci(std::uint64_t number) {
    return number < 2 ? 1 : Fibonacci(number - 1) + Fibonacci(number - 2);
}

TEST_CASE("T1") {
    table t = table();
    int i = 0;

    uint8_t frame[FRAME_SIZE] = {0};
    // get long from the frame, big endian
    int *key = (int *)frame;

    // now let's benchmark:
    BENCHMARK("T1 1") {
        (*key)++;
        t.put_frame(frame);
        return 1;
    };
}

TEST_CASE("T2") {
    ordinary_table t = ordinary_table();
    int i = 0;

    uint8_t frame[FRAME_SIZE] = {0};
    // get long from the frame, big endian
    int *key = (int *)frame;

    // now let's benchmark:
    BENCHMARK("T2 2") {
        (*key)++;
        t.put_frame(frame);
        return 1;
    };
}
