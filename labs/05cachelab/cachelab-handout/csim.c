#include "cachelab.h"

// Cache数据结构的构建
typedef struct{
    int valid;
    unsigned long tag;
    int lru;
} CacheLine;

typedef struct{
    CacheLine *lines;
} CacheSet;

typedef struct{
    int s;
    int E;
    int b;
    int S;
    CacheSet *sets;
} Cache;

// 全局变量的声明
Cache cache;

int verbose = 0;
int hit_count = 0;
int miss_count = 0;
int eviction_count = 0;

int global_time = 0;




void initCache(int s, int E, int b){
    cache.s = s;
    cache.E = E;
    cache.b = b;
    cache.S = 1 << s;
    
    // 为 Set 分配空间
    cache.sets = (CacheSet *)malloc(cahce.s * sizeof(CacheSet));
    if (cache.sets == NULL){
        fprintf(stderr, "内存分配失败");
        exit(1);
    }

    // 为每个 Set 中的每个 Line 分配空间，并初始化
    for (int i = 0; i < cache.S; i++){
        cache.sets[i].lines = (CacheLine *)malloc(cache.E * sizeof(cacheLine));
        if (cache.sets[i].lines == NULL){
            fprintf(stderr, "内存分配失败");
            exit(1);
        }
            for(int j = 0; j < cache.E, j++){
                cache.sets[i].lines[j].valid = 0;
                cache.sets[i].lines[j].tag = 0;
                cache.sets[i].lines[j].lru = 0;
            }
        
    }
}

void freeCache(){
    for (int i = 0; i < cache.S; i++){
        if (cache.set[i].lines != NULL){
            free(cache.sets[i].lines);
        }
    }
    if (cache.sets != NULL){
        free(cache.sets);
    }
}

void updateCache(unsigned long addr){
    global_time++;

    // 解析地址
    unsigned long set_index = (addr >> cache.b) & ((1 << cache.s) - 1); // 利用了掩码技术 mask，哈哈，用到前面的知识了
    unsigned long tag = addr >> (cache.b + cache.s);

    // 获取set_index的当前 Set;
    CacheSet *current_set = &cache.sets[set_index]; 

    // hit
    for (int i = 0; i < cache.E; i++){
        if (current_set->lines[i].valid && current_set->lines[i].tag == tag){
            hit_count++;
            current_set->lines[i].lru = global_time;
            if (verbose) printf("hit ");
            return;
        }
    }

    // miss
    miss_count++;
    for (int i = 0; i < cache.E; i++){
        // 如果存在空行
        if (!current_set->lines[i].valid){
            // 填充到空行位置
            current_set->lines[i].valid = 1;
            current_set->lines[i].tag = tag;
            current_set->lines[i].lru = global_time;
            return;
        }
    }

    // evict (没有空行)
    eviction_count++;
    //采用lru策略，替换 lru值的最小行
    int min_lru = INT_MAX;
    int evict_index = -1;

    for (int i = 0; i < cache.E, i++){
        if (current_set->lines[i].lru < min_lru){
            min_lru = current_set->lines[i].lru;
            evict_index = i;
        }
    }

    current_set->lines[i].tag = tag;
    current_set->lines[i].lru =global_time;
}