CC=gcc
C_CFLAGS=-c -std=c11 -Wall -Werror -fPIC -O2 -g
LDFLAGS=-g

REPL_LDLIBS=-ledit
TEST_LDLIBS=
LIB_LDLIBS=-lm

SRC_DIR=src
BIN_DIR=bin

C_SRC_DIR=$(SRC_DIR)/c-scheme
C_BIN_DIR=$(BIN_DIR)/c-scheme

C_REPL=$(C_BIN_DIR)/c-scheme-repl
C_TEST=$(C_BIN_DIR)/c-scheme-test
C_LIB=$(C_BIN_DIR)/c-scheme.so
C_SOURCES=$(wildcard $(C_SRC_DIR)/*.c)
C_REPL_SOURCES=$(C_SRC_DIR)/repl.c $(C_SRC_DIR)/edit.c $(C_SRC_DIR)/hist.c
C_REPL_OBJECTS=$(C_REPL_SOURCES:$(C_SRC_DIR)/%.c=$(C_BIN_DIR)/%.o)
C_TEST_SOURCES=$(C_SRC_DIR)/test.c
C_TEST_OBJECTS=$(C_TEST_SOURCES:$(C_SRC_DIR)/%.c=$(C_BIN_DIR)/%.o)
C_LIB_SOURCES=$(filter-out $(C_REPL_SOURCES) $(C_TEST_SOURCES), $(C_SOURCES))
C_LIB_OBJECTS=$(C_LIB_SOURCES:$(C_SRC_DIR)/%.c=$(C_BIN_DIR)/%.o)

all: c-repl c-test c-lib

c-repl: $(C_REPL)
c-test: $(C_TEST)
c-lib: $(C_LIB)

$(C_REPL): $(C_REPL_OBJECTS) $(C_LIB)
	$(CC) $(LDFLAGS) $^ -o $@ $(REPL_LDLIBS)

$(C_REPL_OBJECTS): $(C_BIN_DIR)/%.o: $(C_SRC_DIR)/%.c
	$(CC) $(C_CFLAGS) $< -o $@

$(C_TEST): $(C_TEST_OBJECTS) $(C_LIB)
	$(CC) $(LDFLAGS) $^ -o $@ $(TEST_LDLIBS)

$(C_TEST_OBJECTS): $(C_BIN_DIR)/%.o: $(C_SRC_DIR)/%.c
	$(CC) $(C_CFLAGS) $< -o $@

$(C_LIB): $(C_LIB_OBJECTS)
	$(CC) -shared $(LDFLAGS) $^ -o $@ $(LIB_LDLIBS)

$(C_LIB_OBJECTS): $(C_BIN_DIR)/%.o: $(C_SRC_DIR)/%.c
	$(CC) $(C_CFLAGS) $< -o $@

clean:
	rm -rf $(BIN_DIR)

.PHONY: clean

$(shell mkdir -p $(C_BIN_DIR))
