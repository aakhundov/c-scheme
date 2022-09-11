#!/bin/bash

make -j c-test && \
echo && \
./bin/c-scheme/c-scheme-test
