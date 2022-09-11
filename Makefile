CC=gcc
CPPC=g++
C_CFLAGS=-c -std=c11 -Wall -Werror -fPIC -O2 -g
CPP_CFLAGS=-c -std=c++17 -Wall -Werror -fPIC -O2 -g
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

CPP_SRC_DIR=$(SRC_DIR)/cpp-scheme
CPP_BIN_DIR=$(BIN_DIR)/cpp-scheme

CPP_TEST=$(CPP_BIN_DIR)/cpp-scheme-test
CPP_LIB=$(CPP_BIN_DIR)/cpp-scheme.so

CPP_SOURCES=$(wildcard $(CPP_SRC_DIR)/*.cpp)
CPP_TEST_SOURCES=$(CPP_SRC_DIR)/test.cpp
CPP_TEST_OBJECTS=$(CPP_TEST_SOURCES:$(CPP_SRC_DIR)/%.cpp=$(CPP_BIN_DIR)/%.o)
CPP_LIB_SOURCES=$(filter-out $(CPP_REPL_SOURCES) $(CPP_TEST_SOURCES), $(CPP_SOURCES))
CPP_LIB_OBJECTS=$(CPP_LIB_SOURCES:$(CPP_SRC_DIR)/%.cpp=$(CPP_BIN_DIR)/%.o)

all: c-repl c-test c-lib cpp-test cpp-lib

c-repl: $(C_REPL)
c-test: $(C_TEST)
c-lib: $(C_LIB)

cpp-test: $(CPP_TEST)
cpp-lib: $(CPP_LIB)

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

$(CPP_TEST): $(CPP_TEST_OBJECTS) $(CPP_LIB)
	$(CPPC) $(LDFLAGS) $^ -o $@ $(TEST_LDLIBS)

$(CPP_TEST_OBJECTS): $(CPP_BIN_DIR)/%.o: $(CPP_SRC_DIR)/%.cpp
	$(CPPC) $(CPP_CFLAGS) $< -o $@

$(CPP_LIB): $(CPP_LIB_OBJECTS)
	$(CPPC) -shared $(LDFLAGS) $^ -o $@ $(LIB_LDLIBS)

$(CPP_LIB_OBJECTS): $(CPP_BIN_DIR)/%.o: $(CPP_SRC_DIR)/%.cpp
	$(CPPC) $(CPP_CFLAGS) $< -o $@

clean:
	rm -rf $(BIN_DIR)

.PHONY: clean

$(shell mkdir -p $(C_BIN_DIR))
$(shell mkdir -p $(CPP_BIN_DIR))
