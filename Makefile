CC=gcc
CFLAGS=-c -std=c99 -Wall -Werror -fPIC -g
LDFLAGS=-g
LDLIBS=-ledit -lm

SRC_DIR=src
BIN_DIR=bin

APP=$(BIN_DIR)/c-scheme
LIB=$(APP).so
SOURCES=$(wildcard $(SRC_DIR)/*.c)
OBJECTS=$(SOURCES:$(SRC_DIR)/%.c=$(BIN_DIR)/%.o)
DRIVER_SOURCE=$(SRC_DIR)/main.c
DRIVER_OBJECT=$(BIN_DIR)/main.o
LIB_SOURCES=$(filter-out $(DRIVER_SOURCE), $(SOURCES))
LIB_OBJECTS=$(filter-out $(DRIVER_OBJECT), $(OBJECTS))

$(APP): $(DRIVER_OBJECT) $(LIB)
	$(CC) $(LDFLAGS) $(LDLIBS) $^ -o $@

$(DRIVER_OBJECT): $(DRIVER_SOURCE) $(LIB_OBJECTS)
	$(CC) $(CFLAGS) $(DRIVER_SOURCE) -o $@

$(LIB): $(LIB_OBJECTS)
	$(CC) -shared $(LDFLAGS) $^ $(LDLIBS) -o $@

$(LIB_OBJECTS): $(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BIN_DIR)

.PHONY: clean

$(shell mkdir -p $(BIN_DIR))
