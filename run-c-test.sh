#!/bin/bash

make -j c-test && \
echo && \
./bin/c/scheme-test
