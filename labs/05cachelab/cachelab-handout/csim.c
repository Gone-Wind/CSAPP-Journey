#include "cachelab.h"
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

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
char *trace_file = NULL;

int hit_count = 0;
int miss_count = 0;
int eviction_count = 0;

int global_time = 0;

void initCache(int s, int E, int b);
void freeCache();
void updateCache(unsigned long addr);
void printUsage(char *argv[]);

int main(int argc, char *argv[]){
    int opt;
    int s = 0, E = 0, b = 0;

    // 解析命令行参数
    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1){
        switch(opt){
            case 'h':
                printUsage(argv);
                exit(0);
            case 'v':
                verbose = 1;
                break;
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                trace_file = optarg;
                break;
            default:
                printUsage(argv);
                exit(1);
        }
    }

    //初始化缓存
    initCache(s, E, b);

    //打开测试文件
    FILE *pFile = fopen(trace_file, "r");
    if (pFile == NULL){
        printf("Error: Cannot open the trace file %s\n", trace_file);
        exit(1);
    }
  
    char operation;
    unsigned long address;
    int size;

    while(fscanf(pFile, " %c %lx, %d", &operation, &address, &size) > 0){
        if (operation == 'I'){
            continue;
        }

        updateCache(address);

        if (operation == 'M'){
            hit_count++;
            if (verbose) printf("hit ");
        }
        if (verbose) printf("\n");
    }

    fclose(pFile);

    freeCache();

    printSummary(hit_count, miss_count, eviction_count);

}

void initCache(int s, int E, int b){
    cache.s = s;
    cache.E = E;
    cache.b = b;
    cache.S = 1 << s;
    
    // 为 Set 分配空间
    cache.sets = (CacheSet *)malloc(cache.S * sizeof(CacheSet));
    if (cache.sets == NULL){
        fprintf(stderr, "内存分配失败");
        exit(1);
    }

    // 为每个 Set 中的每个 Line 分配空间，并初始化
    for (int i = 0; i < cache.S; i++){
        cache.sets[i].lines = (CacheLine *)malloc(cache.E * sizeof(CacheLine));
        if (cache.sets[i].lines == NULL){
            fprintf(stderr, "内存分配失败");
            exit(1);
        }
            for(int j = 0; j < cache.E; j++){
                cache.sets[i].lines[j].valid = 0;
                cache.sets[i].lines[j].tag = 0;
                cache.sets[i].lines[j].lru = 0;
            }
        
    }
}

void freeCache(){
    for (int i = 0; i < cache.S; i++){
        if (cache.sets[i].lines != NULL){
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

    for (int i = 0; i < cache.E; i++){
        if (current_set->lines[i].lru < min_lru){
            min_lru = current_set->lines[i].lru;
            evict_index = i;
        }
    }

    current_set->lines[evict_index].tag = tag;
    current_set->lines[evict_index].lru = global_time;
}

void printUsage(char* argv[]) {
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv[0]);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
}