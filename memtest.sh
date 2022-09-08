#!/bin/bash

make -j && \
echo && \
valgrind --leak-check=yes ./bin/c-scheme test
