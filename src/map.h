#ifndef MAP_H_
#define MAP_H_

#include "value.h"

typedef struct map_record map_record;
typedef struct map map;

struct map_record {
    char* key;
    value* val;
    map_record* next;
};

struct map {
    size_t num_buckets;
    map_record** buckets;
};

map* map_new();
void map_dispose(map* m);

int map_has(const map* m, const char* key);
map_record* map_get(const map* m, const char* key);
void map_add(map* m, const char* key, value* val);
map* map_copy(const map* source);

void map_dispose_values(map* m);

#endif  // MAP_H_
