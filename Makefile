CC=gcc
CFLAGS=-c -std=c11 -Wall -Werror -fPIC -O2 -g
LDFLAGS=-g
LDLIBS=-ledit -lm

SRC_DIR=src
BIN_DIR=bin

APP=$(BIN_DIR)/c-scheme
LIBRARY=$(APP).so
SOURCES=$(wildcard $(SRC_DIR)/*.c)
DRIVER_SOURCES=$(SRC_DIR)/main.c $(SRC_DIR)/test.c $(SRC_DIR)/repl.c $(SRC_DIR)/edit.c $(SRC_DIR)/hist.c
DRIVER_OBJECTS=$(DRIVER_SOURCES:$(SRC_DIR)/%.c=$(BIN_DIR)/%.o)
LIBRARY_SOURCES=$(filter-out $(DRIVER_SOURCES), $(SOURCES))
LIBRARY_OBJECTS=$(LIBRARY_SOURCES:$(SRC_DIR)/%.c=$(BIN_DIR)/%.o)

$(APP): $(DRIVER_OBJECTS) $(LIBRARY)
	$(CC) $(LDFLAGS) $(LDLIBS) $^ -o $@

$(DRIVER_OBJECTS): $(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $< -o $@

$(LIBRARY): $(LIBRARY_OBJECTS)
	$(CC) -shared $(LDFLAGS) $^ $(LDLIBS) -o $@

$(LIBRARY_OBJECTS): $(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BIN_DIR)

.PHONY: clean

$(shell mkdir -p $(BIN_DIR))
