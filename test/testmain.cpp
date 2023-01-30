#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <iostream>
#include <map>
#include <sstream>
#include <thread>

#include "api.h"

std::uint64_t Fibonacci(std::uint64_t number) { return number < 2 ? 1 : Fibonacci(number - 1) + Fibonacci(number - 2); }

// TEST_CASE("T1") {
//     table t = table();
//     int i = 0;

//     uint8_t frame[FRAME_SIZE] = {0};
//     // get long from the frame, big endian
//     int *key = (int *)frame;

//     // now let's benchmark:
//     BENCHMARK("T1 1") {
//         (*key)++;
//         t.put_frame(frame);
//         return 1;
//     };
// }

// TEST_CASE("T2") {
//     OrdinaryTable t = OrdinaryTable();
//     int i = 0;

//     uint8_t frame[FRAME_SIZE] = {0};
//     // get long from the frame, big endian
//     int *key = (int *)frame;

//     // now let's benchmark:
//     BENCHMARK("T2 2") {
//         (*key)++;
//         t.put_frame(frame);
//         return 1;
//     };
// }

struct TimedRequest {
  bool* running;
  uint32_t ms;
};

void* timed_request(void* arg) {
  TimedRequest* tr = (TimedRequest*)arg;
  uint32_t ms = tr->ms;

  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
  *(tr->running) = false;

  delete tr;

  return NULL;
}

void run(bool& running, uint32_t ms) {
  running = true;
  pthread_t thread;
  TimedRequest* tr = new TimedRequest{&running, ms};
  pthread_create(&thread, NULL, timed_request, tr);
}

// TEST_CASE("T0") {
//   Table t = Table();
//   int i = 0;

//   uint8_t frame[FRAME_SIZE] = {0};
//   uint8_t dst[SECONDARY_VALUE_SIZE] = {0};
//   // get long from the frame, big endian
//   KEY_TYPE *key = (KEY_TYPE *)frame;
//   int *key2 = (int *)&(frame[4]);

//   auto start = std::chrono::high_resolution_clock::now();

//   for (int i = 0; i < 100'0000; i++) {
//     (*key) += (1 << (32 - INDEX_BIT_SIZE));
//     (*key2)++;

//     // REQUIRE(t.get_by_header(dst, frame) == 0);

//     t.put_frame(frame);

//     // REQUIRE(t.get_by_header(dst, frame) != 0);
//   }

//   auto end = std::chrono::high_resolution_clock::now();

//   auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
//   printf("T3 took %ld nanoseconds to run \n ", duration.count());

// //   for (int i = 0; i < 10; i++) {
// //     std::cout << (t.get_by_header(dst, frame) != 0) << std::endl;

// //     // sleep 10 seconds
// //     std::this_thread::sleep_for(std::chrono::seconds(10));
// //   }
// }

TEST_CASE("T1") {
  int i = 0;
  bool running = true;

  uint8_t frame[FRAME_SIZE] = {0};
  uint8_t dst[SECONDARY_VALUE_SIZE] = {0};
  // get long from the frame, big endian
  KEY_TYPE* key = (KEY_TYPE*)frame;
  int* key2 = (int*)&(frame[4]);

#pragma region random data, number of inserts per second, (proposed, ordinary, unordered)

  if (0) {
    Table t = Table();
    run(running, 1000);
    i = 0;
    (*key) = 0;
    (*key2) = 0;
    while (running) {
      (*key) += (1 << (32 - INDEX_BIT_SIZE));
      (*key2)++;
      t.put_frame(frame);
      i++;
    }
    std::cout << "random data, number of inserts per second, proposed, " << i << std::endl;
  }

  if (0) {
    OrdinaryTable t = OrdinaryTable();
    run(running, 1000);
    i = 0;
    (*key) = 0;
    (*key2) = 0;
    while (running) {
      (*key) += (1 << (32 - INDEX_BIT_SIZE));
      (*key2)++;
      t.put_frame(frame);
      i++;
    }
    std::cout << "random data, number of inserts per second, ordinary, " << i << std::endl;
  }

  if (0) {
    UnorderedMapTable t = UnorderedMapTable();
    run(running, 1000);
    i = 0;
    (*key) = 0;
    (*key2) = 0;
    while (running) {
      (*key) += (1 << (32 - INDEX_BIT_SIZE));
      (*key2)++;
      t.put_frame(frame);
      i++;
    }
    std::cout << "random data, number of inserts per second, unordered, " << i << std::endl;
  }

  if (0) {
    AvlTreeMapTable t = AvlTreeMapTable();
    run(running, 1000);
    i = 0;
    (*key) = 0;
    (*key2) = 0;
    while (running) {
      (*key) += (1 << (32 - INDEX_BIT_SIZE));
      (*key2)++;
      t.put_frame(frame);
      i++;
    }
    std::cout << "random data, number of inserts per second, avl tree, " << i << std::endl;
  }

#pragma endregion

#pragma region ascending data, number of inserts per second, (proposed, ordinary)

  if (0) {
    Table t = Table();
    run(running, 1000);
    i = 0;
    (*key) = 0;
    (*key2) = 0;
    while (running) {
      (*key)++;
      (*key2)++;
      t.put_frame(frame);
      i++;
    }
    std::cout << "ascending data, number of inserts per second, proposed, " << i << std::endl;
  }

  if (0) {
    OrdinaryTable t = OrdinaryTable();
    run(running, 1000);
    i = 0;
    (*key) = 0;
    (*key2) = 0;
    while (running) {
      (*key)++;
      (*key2)++;
      t.put_frame(frame);
      i++;
    }
    std::cout << "ascending data, number of inserts per second, ordinary, " << i << std::endl;
  }

  if (0) {
    UnorderedMapTable t = UnorderedMapTable();
    run(running, 1000);
    i = 0;
    (*key) = 0;
    (*key2) = 0;
    while (running) {
      (*key)++;
      (*key2)++;
      t.put_frame(frame);
      i++;
    }
    std::cout << "ascending data, number of inserts per second, unordered, " << i << std::endl;
  }

  if (0) {
    AvlTreeMapTable t = AvlTreeMapTable();
    run(running, 1000);
    i = 0;
    (*key) = 0;
    (*key2) = 0;
    while (running) {
      (*key)++;
      (*key2)++;
      t.put_frame(frame);
      i++;
    }
    std::cout << "ascending data, number of inserts per second, avl tree, " << i << std::endl;
  }

#pragma endregion

#pragma region finishing time of one million random data in microsecond, (insert, lookup), proposed

  if (0) {
    Table t = Table();
    i = 0;
    (*key) = 0;
    (*key2) = 0;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100'0000; i++) {
      (*key) += (1 << (32 - INDEX_BIT_SIZE));
      (*key2)++;
      t.put_frame(frame);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "finishing time of one million random data in microsecond, insert, proposed(" << INDEX_BIT_SIZE << "), " << duration.count()
              << std::endl;

    i = 0;
    (*key) = 0;
    (*key2) = 0;

    start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100'0000; i++) {
      (*key) += (1 << (32 - INDEX_BIT_SIZE));
      (*key2)++;
      t.get_by_header(dst, frame);
    }

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "finishing time of one million random data in microsecond, lookup, proposed(" << INDEX_BIT_SIZE << "), " << duration.count()
              << std::endl;

    // NO DELETE DATA
  }

#pragma endregion

#pragma region finishing time of one million random data in microsecond, (insert, lookup), ordinary

  if (0) {
    OrdinaryTable t = OrdinaryTable();
    i = 0;
    (*key) = 0;
    (*key2) = 0;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100'0000; i++) {
      (*key) += (1 << (32 - INDEX_BIT_SIZE));
      (*key2)++;
      t.put_frame(frame);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "finishing time of one million random data in microsecond, insert, ordinary, " << duration.count()
              << std::endl;

    i = 0;
    (*key) = 0;
    (*key2) = 0;

    start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100'0000; i++) {
      (*key) += (1 << (32 - INDEX_BIT_SIZE));
      (*key2)++;
      t.get_by_header(dst, frame);
    }

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "finishing time of one million random data in microsecond, lookup, ordinary, " << duration.count()
              << std::endl;

    // NO DELETE DATA
  }

#pragma endregion

#pragma region finishing time of one million random data in microsecond, (insert, lookup), unordered

  if (0) {
    UnorderedMapTable t = UnorderedMapTable();
    i = 0;
    (*key) = 0;
    (*key2) = 0;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100'0000; i++) {
      (*key) += (1 << (32 - INDEX_BIT_SIZE));
      (*key2)++;
      t.put_frame(frame);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "finishing time of one million random data in microsecond, insert, unordered, " << duration.count()
              << std::endl;

    i = 0;
    (*key) = 0;
    (*key2) = 0;

    start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100'0000; i++) {
      (*key) += (1 << (32 - INDEX_BIT_SIZE));
      (*key2)++;
      t.get_by_header(dst, frame);
    }

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "finishing time of one million random data in microsecond, lookup, unordered, " << duration.count()
              << std::endl;

    // NO DELETE DATA
  }

#pragma endregion

#pragma region finishing time of one million random data in microsecond, (insert, lookup), avl tree

  if (0) {
    AvlTreeMapTable t = AvlTreeMapTable();
    i = 0;
    (*key) = 0;
    (*key2) = 0;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100'0000; i++) {
      (*key) += (1 << (32 - INDEX_BIT_SIZE));
      (*key2)++;
      t.put_frame(frame);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "finishing time of one million random data in microsecond, insert, avl tree, " << duration.count()
              << std::endl;

    i = 0;
    (*key) = 0;
    (*key2) = 0;

    start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100'0000; i++) {
      (*key) += (1 << (32 - INDEX_BIT_SIZE));
      (*key2)++;
      t.get_by_header(dst, frame);
    }

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "finishing time of one million random data in microsecond, lookup, avl tree, " << duration.count()
              << std::endl;

    // NO DELETE DATA
  }

#pragma endregion

#pragma region finishing time of one million ascending data in microsecond, (insert, lookup), proposed

  if (0) {
    Table t = Table();
    i = 0;
    (*key) = 0;
    (*key2) = 0;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100'0000; i++) {
      (*key)++;
      (*key2)++;
      t.put_frame(frame);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "finishing time of one million ascending data in microsecond, insert, proposed, " << duration.count()
              << std::endl;

    i = 0;
    (*key) = 0;
    (*key2) = 0;

    start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100'0000; i++) {
      (*key)++;
      (*key2)++;
      t.get_by_header(dst, frame);
    }

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "finishing time of one million ascending data in microsecond, lookup, proposed, " << duration.count()
              << std::endl;

    // NO DELETE DATA
  }

#pragma endregion

#pragma region finishing time of one million ascending data in microsecond, (insert, lookup), ordinary
  if (0) {
    OrdinaryTable t = OrdinaryTable();
    i = 0;
    (*key) = 0;
    (*key2) = 0;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100'0000; i++) {
      (*key)++;
      (*key2)++;
      t.put_frame(frame);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "finishing time of one million ascending data in microsecond, insert, ordinary, " << duration.count()
              << std::endl;

    i = 0;
    (*key) = 0;
    (*key2) = 0;

    start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100'0000; i++) {
      (*key)++;
      (*key2)++;
      t.get_by_header(dst, frame);
    }

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "finishing time of one million ascending data in microsecond, lookup, ordinary, " << duration.count()
              << std::endl;

    // NO DELETE DATA
  }
#pragma endregion

#pragma region finishing time of one million ascending data in microsecond, (insert, lookup), unordered

  if (0) {
    UnorderedMapTable t = UnorderedMapTable();
    i = 0;
    (*key) = 0;
    (*key2) = 0;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100'0000; i++) {
      (*key) += (1 << (32 - INDEX_BIT_SIZE));
      (*key2)++;
      t.put_frame(frame);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "finishing time of one million ascending data in microsecond, insert, unordered, " << duration.count()
              << std::endl;

    i = 0;
    (*key) = 0;
    (*key2) = 0;

    start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100'0000; i++) {
      (*key) += (1 << (32 - INDEX_BIT_SIZE));
      (*key2)++;
      t.get_by_header(dst, frame);
    }

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "finishing time of one million ascending data in microsecond, lookup, unordered, " << duration.count()
              << std::endl;

    // NO DELETE DATA
  }

#pragma endregion

#pragma region finishing time of one million ascending data in microsecond, (insert, lookup), avl tree

  if (0) {
    AvlTreeMapTable t = AvlTreeMapTable();
    i = 0;
    (*key) = 0;
    (*key2) = 0;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100'0000; i++) {
      (*key) += (1 << (32 - INDEX_BIT_SIZE));
      (*key2)++;
      t.put_frame(frame);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "finishing time of one million ascending data in microsecond, insert, avl tree, " << duration.count()
              << std::endl;

    i = 0;
    (*key) = 0;
    (*key2) = 0;

    start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100'0000; i++) {
      (*key) += (1 << (32 - INDEX_BIT_SIZE));
      (*key2)++;
      t.get_by_header(dst, frame);
    }

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "finishing time of one million ascending data in microsecond, lookup, avl tree, " << duration.count()
              << std::endl;

    // NO DELETE DATA
  }

#pragma endregion

}

// TEST_CASE("T4") {
//   Table t = Table();
//   int i = 0;

//   uint8_t frame[FRAME_SIZE] = {0};
//   // get long from the frame, big endian
//   int* key = (int*)frame;

//   auto start = std::chrono::high_resolution_clock::now();

//   for (int i = 0; i < 100'0000; i++) {
//     (*key)++;
//     // t.put_frame(frame);
//   }

//   auto end = std::chrono::high_resolution_clock::now();

//   auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
//   printf("T4 took %ld nanoseconds to run \n ", duration.count());
// }

// TEST_CASE("T5") {
//   ListenerTable t = ListenerTable();
//   int i = 0;

//   uint8_t frame[FRAME_SIZE] = {0}; // 32 * 4
//   for (int i = 0; i < FRAME_SIZE; i++) frame[i] = rand();

//   // get long from the frame, big endian
//   int* key = (int*)frame;

//   auto start = std::chrono::high_resolution_clock::now();

//   t.listen(0, frame, 1);
//   t.listen(123, frame + 32, 2);
//   t.listen(65535, frame + 32 + 32 + 32, 1);

//   REQUIRE(t.pull(frame) == 0);
//   REQUIRE(t.pull(frame) == -1);
//   REQUIRE(t.pull(frame + 32 + 32) == 123);
//   REQUIRE(t.pull(frame + 32 + 32) == -1);
//   REQUIRE(t.pull(frame + 32) == 123);
//   REQUIRE(t.pull(frame + 32) == -1);
//   REQUIRE(t.pull(frame + 32 + 32 + 32) == 65535);
//   REQUIRE(t.pull(frame + 32 + 32 + 32) == -1);

//   // for (int i = 0; i < 65536; i++) {
//   //   // (*key)++;
//   //   t.remove(i);
//   // }

//   auto end = std::chrono::high_resolution_clock::now();

//   auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
//   printf("T4 took %ld nanoseconds to run \n ", duration.count());
// }
