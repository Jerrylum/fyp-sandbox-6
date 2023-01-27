# Hello CMake!

This repository contains the walkthrough code for my CMake related blog posts.


## Development

Install Catch2
https://github.com/catchorg/Catch2/blob/devel/docs/cmake-integration.md#installing-catch2-from-git-repository

Compile
```
cmake -Bbuild
cmake --build build
cmake --build build && ./build/test/tests
cmake --build build && ./build/test/tests --benchmark-samples 1000
cmake --build build && ./build/test/tests --durations yes
export CFLAGS="-m32"; cmake --build build && valgrind --leak-check=yes ./build/test/tests
```

## Network

### Frame structure
```
|Key(4)--------|Value(124)---------------------|
|Index(24bit)|-|SecKey(28)|SecValue(96)--------|
|Frame(128)------------------------------------|
|Header(32)---------------|Encrypted Packet(96)|
```

```
random data:
 - number of inserts per second
 - number of lookups per second
 - number of deletes per second

ascending data:
 - number of inserts per second
 - number of lookups per second
 - number of deletes per second 
 
finishing time of one million random data in millisecond:
 - insert
 - lookup
 - delete
 
finishing time of one million ascending data in millisecond:
 - insert
 - lookup
 - delete

response time of the proposed system with 1000 inserts and lookups per second over time (random data)
 - 1 min
 - 5 min
 - 30 min

response time of the proposed system with increasing lookups (random data)
 - 100
 - 1000
 - 10000
 - 100000
```

## Links to the posts:

1. [Hello CMake!](https://arne-mertz.de/2018/05/hello-cmake/)
2. [Another Target and the Project](https://arne-mertz.de/2018/05/cmake-another-target/)
3. [CMake Project Structure](https://arne-mertz.de/2018/06/cmake-project-structure)
4. [Properties and Options](https://arne-mertz.de/2018/07/cmake-properties-options)
