CC=gcc
C_CFLAGS=-c -std=c11 -Wall -Werror -fPIC -O2 -g
LDFLAGS=-g
LDLIBS=-ledit -lm

SRC_DIR=src
BIN_DIR=bin

C_SRC_DIR=$(SRC_DIR)/c
C_BIN_DIR=$(BIN_DIR)/c

C_DRIVER=$(C_BIN_DIR)/c-scheme
C_LIBRARY=$(C_DRIVER).so
C_SOURCES=$(wildcard $(C_SRC_DIR)/*.c)
C_DRIVER_SOURCES=$(C_SRC_DIR)/main.c $(C_SRC_DIR)/test.c $(C_SRC_DIR)/repl.c $(C_SRC_DIR)/edit.c $(C_SRC_DIR)/hist.c
C_DRIVER_OBJECTS=$(C_DRIVER_SOURCES:$(C_SRC_DIR)/%.c=$(C_BIN_DIR)/%.o)
C_LIBRARY_SOURCES=$(filter-out $(C_DRIVER_SOURCES), $(C_SOURCES))
C_LIBRARY_OBJECTS=$(C_LIBRARY_SOURCES:$(C_SRC_DIR)/%.c=$(C_BIN_DIR)/%.o)

c: $(C_DRIVER) $(C_LIBRARY)

$(C_DRIVER): $(C_DRIVER_OBJECTS) $(C_LIBRARY)
	$(CC) $(LDFLAGS) $(LDLIBS) $^ -o $@

$(C_DRIVER_OBJECTS): $(C_BIN_DIR)/%.o: $(C_SRC_DIR)/%.c
	$(CC) $(C_CFLAGS) $< -o $@

$(C_LIBRARY): $(C_LIBRARY_OBJECTS)
	$(CC) -shared $(LDFLAGS) $^ $(LDLIBS) -o $@

$(C_LIBRARY_OBJECTS): $(C_BIN_DIR)/%.o: $(C_SRC_DIR)/%.c
	$(CC) $(C_CFLAGS) $< -o $@

clean:
	rm -rf $(BIN_DIR)

.PHONY: clean

$(shell mkdir -p $(C_BIN_DIR))
