#!/bin/bash

make -j c-test && \
echo && \
valgrind --leak-check=yes ./bin/c-scheme/c-scheme-test
