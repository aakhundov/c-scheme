#include "map.h"

#include <stdlib.h>
#include <string.h>

#include "value.h"

#define INITIAL_NUM_BUCKETS 5

static size_t get_bucket_id(map* m, char* key) {
    // http://www.cse.yorku.ca/~oz/hash.html

    int c;
    unsigned long hash = 5381;
    while ((c = *key++) != '\0') {
        hash = ((hash << 5) + hash) + c;
    }

    return hash % m->num_buckets;
}

static void initialize_map(map* m, size_t num_buckets) {
    m->num_buckets = num_buckets;
    m->buckets = calloc(m->num_buckets, sizeof(map_record*));
}

static void expand_buckets(map* m) {
    size_t old_num_buckets = m->num_buckets;
    map_record** old_buckets = m->buckets;

    size_t new_num_buckets = old_num_buckets * 2 + 1;
    initialize_map(m, new_num_buckets);

    for (size_t i = 0; i < old_num_buckets; i++) {
        // iterate over the old buckets
        map_record** old_bucket = old_buckets + i;
        while (*old_bucket != NULL) {
            // pop a record from the old bucket
            map_record* r = *old_bucket;
            *old_bucket = r->next;

            // push the record to the new bucket
            size_t new_id = get_bucket_id(m, r->key);
            map_record** new_bucket = m->buckets + new_id;
            r->next = *new_bucket;
            *new_bucket = r;
        }
    }

    free(old_buckets);
}

static map_record* record_new(char* key, value* val, map_record* next) {
    map_record* r = malloc(sizeof(map_record));

    r->key = malloc(strlen(key) + 1);
    strcpy(r->key, key);

    r->val = val;
    r->next = next;

    return r;
}

static void record_dispose(map_record* r) {
    free(r->key);
    free(r);
}

static map_record* record_copy(map_record* source) {
    if (source == NULL) {
        return NULL;
    } else {
        return record_new(
            source->key,
            source->val,
            record_copy(source->next));
    }
}

map* map_new() {
    map* m = malloc(sizeof(map));

    initialize_map(m, INITIAL_NUM_BUCKETS);

    return m;
}

void map_dispose(map* m) {
    for (size_t i = 0; i < m->num_buckets; i++) {
        // iterate over the buckets
        map_record* r = *(m->buckets + i);
        while (r != NULL) {
            // dispose each record in a chain
            map_record* next = r->next;
            record_dispose(r);
            r = next;
        }
    }

    free(m->buckets);
    free(m);
}

map_record* map_get(map* m, char* key) {
    // find the bucket for the key
    size_t id = get_bucket_id(m, key);
    map_record** bucket = m->buckets + id;

    map_record* r = *bucket;
    while (r != NULL) {
        // search for the key in the bucket
        if (strcmp(r->key, key) == 0) {
            return r;
        }
        r = r->next;
    }

    return NULL;
}

void map_add(map* m, char* key, value* val) {
    // find the bucket for the key
    size_t id = get_bucket_id(m, key);
    map_record** bucket = m->buckets + id;

    if (*bucket != NULL) {
        // collision -> expand
        expand_buckets(m);

        // find the new bucket for the key
        id = get_bucket_id(m, key);
        bucket = m->buckets + id;
    }

    // add a new record to the chain
    *bucket = record_new(key, val, *bucket);
}

map* map_copy(map* source) {
    if (source == NULL) {
        return NULL;
    } else {
        map* m = malloc(sizeof(map));

        initialize_map(m, source->num_buckets);

        for (size_t i = 0; i < source->num_buckets; i++) {
            // iterate over the buckets and
            // copy each bucket's chain recursively
            *(m->buckets + i) = record_copy(*(source->buckets + i));
        }

        return m;
    }
}
