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

map_record* map_get(map* m, char* key);
void map_add(map* m, char* key, value* val);
map* map_copy(map* source);

#endif  // MAP_H_
