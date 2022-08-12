#!/bin/bash

make && \
echo && \
valgrind --leak-check=yes ./bin/c-scheme test
