#!/bin/bash

make -j c && \
echo && \
valgrind --leak-check=yes ./bin/c/c-scheme test
