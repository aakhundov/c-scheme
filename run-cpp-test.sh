#!/bin/bash

make -j cpp-test && \
echo && \
./bin/cpp-scheme/cpp-scheme-test
