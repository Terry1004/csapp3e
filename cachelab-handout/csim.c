#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>
#include "cachelab.h"

const int INIT_CACHE_CAP = 8;

typedef struct
{
    FILE *trace_file;
    int set_bits;
    int block_bits;
    int associativity;
    bool verbose;
} CLIArgs;

typedef enum
{
    Load,
    Store,
    Modify,
    Unknown,
} CacheOpType;

typedef struct
{
    unsigned long set_index;
    unsigned long tag;
} CacheID;

typedef struct
{
    unsigned long addr;
    CacheID id;
    int last_access;
} CacheEntry;

typedef struct
{
    int hits;
    int misses;
    int evictions;
} CacheResult;

typedef struct
{
    int set_bits;
    int block_bits;
    int associativity;
} CacheConfig;

typedef struct
{
    CacheEntry *store;
    CacheConfig conf;
    int cap;
    int len;
} LRUCache;

LRUCache new_lru_cache(CacheConfig conf, int cap);
CacheResult access_entry(LRUCache *cache, unsigned long addr, int access_time);
CacheResult modify_entry(LRUCache *cache, unsigned long addr, int access_time);
CacheResult process_line(LRUCache *cache, char *line, int line_num);

void run(CLIArgs args);

void print_help();
void print_debug_line(char *line, CacheResult result);
CacheID compute_id(LRUCache *cache, unsigned long addr);

LRUCache new_lru_cache(CacheConfig conf, int cap)
{
    LRUCache cache = {NULL, conf, cap, 0};
    if (cap > 0)
    {
        cache.store = malloc(sizeof(CacheEntry) * cap);
        if (cache.store == NULL)
        {
            printf("OOM creating lru cache with cap=%d\n", cap);
            exit(EXIT_FAILURE);
        }
    }
    return cache;
}

CacheResult access_entry(LRUCache *cache, unsigned long addr, int access_time)
{
    CacheID id = compute_id(cache, addr);

    int found = -1;
    int num_set_entries = 0, earliest_access = INT_MAX, earliest_index = 0;
    for (int i = 0; i < cache->len; i++)
    {
        CacheEntry entry = cache->store[i];
        if (entry.id.set_index == id.set_index)
        {
            if (entry.last_access < earliest_access)
            {
                earliest_access = entry.last_access;
                earliest_index = i;
            }
            if (entry.id.tag == id.tag)
            {
                found = i;
            }
            num_set_entries++;
        }
    }

    CacheResult result = {0, 0, 0};
    if (found >= 0)
    {
        cache->store[found].last_access = access_time;
        result.hits++;
        return result;
    }

    result.misses++;
    if (num_set_entries == cache->conf.associativity)
    {
        cache->store[earliest_index].addr = addr;
        cache->store[earliest_index].id = id;
        cache->store[earliest_index].last_access = access_time;
        result.evictions++;
        return result;
    }

    if (cache->len == cache->cap)
    {
        int cap = 2 * cache->cap + 1;
        cache->store = realloc(cache->store, sizeof(CacheEntry) * cap);
        if (cache->store == NULL)
        {
            printf("OOM expanding from cap=%d to cap=%d\n", cache->cap, cap);
            exit(EXIT_FAILURE);
        }
        cache->cap = cap;
    }
    CacheEntry new_entry = {addr, id, access_time};
    cache->store[cache->len++] = new_entry;
    return result;
}

CacheResult modify_entry(LRUCache *cache, unsigned long addr, int access_time)
{
    CacheResult access_result = access_entry(cache, addr, access_time);
    access_result.hits++; // Load after Store always yields a hits.
    return access_result;
}

CacheResult process_line(LRUCache *cache, char *line, int line_num)
{
    char type;
    unsigned long addr;
    sscanf(line, " %c %lx", &type, &addr);

    CacheResult result;
    switch (type)
    {
    case 'L':
        result = access_entry(cache, addr, line_num);
        break;
    case 'S':
        result = access_entry(cache, addr, line_num);
        break;
    case 'M':
        result = modify_entry(cache, addr, line_num);
        break;
    }
    return result;
}

void run(CLIArgs args)
{
    CacheConfig conf = {args.set_bits, args.block_bits, args.associativity};
    LRUCache cache = new_lru_cache(conf, INIT_CACHE_CAP);

    char buffer[1024];
    int line_num = 0;
    CacheResult total_result = {0, 0, 0};
    while (!feof(args.trace_file))
    {
        fgets(buffer, sizeof(buffer), args.trace_file);

        size_t line_len = strlen(buffer);
        if (line_len > 0 && buffer[line_len - 1] == '\n')
        {
            buffer[line_len - 1] = '\0';
            if (strlen(buffer) == 0 || buffer[0] == 'I')
            {
                continue;
            }
            CacheResult result = process_line(&cache, buffer, line_num++);
            total_result.hits += result.hits;
            total_result.misses += result.misses;
            total_result.evictions += result.evictions;
            if (args.verbose)
            {
                print_debug_line(buffer, result);
            }
        }
    }
    printSummary(total_result.hits, total_result.misses, total_result.evictions);
}

void print_help()
{
    printf("Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n");
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\n");
    printf("Examples:\n");
    printf("  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n");
    printf("  linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

void print_debug_line(char *line, CacheResult result)
{
    printf("%s", line + 1);
    for (int i = 0; i < result.misses; i++)
    {
        printf(" miss");
    }
    for (int i = 0; i < result.evictions; i++)
    {
        printf(" eviction");
    }
    for (int i = 0; i < result.hits; i++)
    {
        printf(" hit");
    }
    printf("\n");
}

CacheID compute_id(LRUCache *cache, unsigned long addr)
{
    int tag_bits = 64 - cache->conf.block_bits - cache->conf.set_bits;
    unsigned long set_index = addr << tag_bits >> (64 - cache->conf.set_bits);
    unsigned long tag = addr >> (64 - tag_bits);

    CacheID id = {set_index, tag};
    return id;
}

int main(int argc, char **argv)
{
    char c;
    char *trace_file_name = "";
    CLIArgs args = {NULL, -1, -1, -1, false};

    while ((c = getopt(argc, argv, "hvs:E:b:t:")) != -1)
    {
        switch (c)
        {
        case 'h':
            print_help();
            return 0;
        case 'v':
            args.verbose = true;
            break;
        case 's':
            sscanf(optarg, "%d", &args.set_bits);
            if (args.set_bits < 0)
            {
                printf("invalid set bits=%s\n", optarg);
                return 0;
            }
            break;
        case 'E':
            sscanf(optarg, "%d", &args.associativity);
            if (args.associativity < 0)
            {
                printf("invalid assocativity=%s\n", optarg);
                return 0;
            }
            break;
        case 'b':
            sscanf(optarg, "%d", &args.block_bits);
            if (args.block_bits < 0)
            {
                printf("invalid block bits=%s\n", optarg);
                return 0;
            }
            break;
        case 't':
            trace_file_name = optarg;
            if (strlen(trace_file_name) == 0)
            {
                printf("invalid trace file name=%s\n", trace_file_name);
                return 0;
            }
            break;
        }
    }
    args.trace_file = fopen(trace_file_name, "r");
    if (args.trace_file == NULL)
    {
        printf("unable to open trace file: %s\n", trace_file_name);
        return 0;
    }

    run(args);
    return 0;
}
