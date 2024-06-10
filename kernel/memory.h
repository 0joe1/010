#ifndef KERNEL_MEMORY_H
#define KERNEL_MEMORY_H
#include "bitmap.h"
#include "sync.h"
#include "list.h"

#define DESC_CNT 7

enum pool_flag {
    PF_KERNEL,
    PF_USER
};

struct virt_addr {
    struct bitmap btmp;
    uint_32 vaddr_start;
};

struct pool {
    struct bitmap btmp;
    uint_32 paddr_start;
    uint_32 pool_size;
    struct lock lock;
};

struct mem_block {
    struct list_elm elm;
};

struct mem_block_desc {
    uint_32 block_size;
    uint_32 blocks_per_arena;
    struct list free_list;
};

struct arena {
    struct mem_block_desc* blk_desc;
    uint_32 cnt;
    Bool large;
};


void mem_pool_init(uint_32 total_mem);
void mem_init(void);
static void* get_vaddr(enum pool_flag,uint_32 pcnt);
uint_32* pte_ptr(uint_32 vaddr);
uint_32* pde_ptr(uint_32 vaddr);
static void page_table_add(void* _vaddr,void* _paddr);
void* malloc_page(enum pool_flag pf,uint_32 pcnt);
void* get_kernel_pages(uint_32 pcnt);
void* get_a_page(enum pool_flag,uint_32 vaddr);
void* get_a_page_without_opvaddrbitmap(enum pool_flag pf,uint_32 vaddr);
uint_32 v2p(void* vaddr);
void block_desc_init(struct mem_block_desc*);
struct mem_block* arena2block(struct arena* arena,uint_32 idx);
struct arena* block2arena(struct mem_block* blk);
void* sys_malloc(uint_32 size);
void remove_pte(void* vaddr);
void pfree(uint_32 paddr);
void vfree(enum pool_flag pf,void* vaddr,uint_32 pcnt);
void mfree_page(enum pool_flag,void* _vaddr,uint_32 pcnt);
void sys_free(void* ptr);

#endif
