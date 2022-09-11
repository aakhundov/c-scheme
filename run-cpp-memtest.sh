#!/bin/bash

make -j cpp-test && \
echo && \
valgrind --leak-check=yes ./bin/cpp-scheme/cpp-scheme-test
