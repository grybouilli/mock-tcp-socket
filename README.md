# Mocking a TCP socket for unit testing in C++
## Introduction

This code is a support material for [an article that you can find here](). It is a demonstration of how to mock a TCP socket in C++ in order to conduct unit testing on smaller parts of a client/server app.

## Build

This project runs with Cmake and [Catch2](https://github.com/catchorg/Catch2).

Run:
```bash
cmake -B build .
cmake --build build
./build/tcparticle.cpp
```
