## Usage

```
cmake --build build; ./build/src/server
```

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
