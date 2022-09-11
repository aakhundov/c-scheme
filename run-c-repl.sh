#!/bin/bash

make -j c-repl && \
echo && \
./bin/c-scheme/c-scheme-repl
