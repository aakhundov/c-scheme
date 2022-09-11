#!/bin/bash

make -j c-test && \
echo && \
valgrind --leak-check=yes ./bin/c-scheme/test
