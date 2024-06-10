#include "memory.h"
#include "string.h"
#include "print.h"
#include "debug.h"
#include "thread.h"
#include "sync.h"
#include "process.h"
#include "interrupt.h"

#define PAGE_SIZE 4096
#define BTMP_START 0xc009a000

#define K_HEAP_START 0xc0100000

#define PDE_IDX(addr) ((addr & 0xffc00000)>>22)
#define PTE_IDX(addr) ((addr & 0x003ff000)>>12)
#define PG_P 1
#define PG_RW_R 0
#define PG_RW_RW 2
#define PG_US_S 0
#define PG_US_U 4

struct mem_block_desc ker_block_desc[DESC_CNT];

struct pool kernel_pool,user_pool;
struct virt_addr ker_vaddr;

void mem_pool_init(uint_32 total_mem)
{
    put_str("  mem pool init start\n");
    uint_32 used_mem = 0x100000 + PAGE_SIZE*256;
    uint_32 free_mem = total_mem - used_mem;

    uint_32 all_free_pages = free_mem / PAGE_SIZE;
    uint_32 ker_free_pages  = all_free_pages / 2;
    uint_32 usr_free_pages  = all_free_pages - ker_free_pages;

    uint_32 kbm_len = ker_free_pages / 8; //管理的区域小于实际区域
    kernel_pool.btmp.bits = (uint_8*)BTMP_START;
    kernel_pool.btmp.map_size = kbm_len;
    kernel_pool.paddr_start = used_mem;
    kernel_pool.pool_size = ker_free_pages * PAGE_SIZE;
    lock_init(&kernel_pool.lock);

    uint_32 ubm_len = usr_free_pages / 8;
    user_pool.btmp.bits = (uint_8*)(BTMP_START + kbm_len);
    user_pool.btmp.map_size = ubm_len;
    user_pool.paddr_start = used_mem + ker_free_pages*PAGE_SIZE;
    user_pool.pool_size = usr_free_pages * PAGE_SIZE;
    lock_init(&user_pool.lock);

    bit_init(&kernel_pool.btmp);
    bit_init(&user_pool.btmp);

    put_str("kernel bitmap start:");
    put_int((uint_32)kernel_pool.btmp.bits);
    put_char('\n');
    put_str("kernel physical addr start:");
    put_int(kernel_pool.paddr_start);
    put_char('\n');

    put_str("user bitmap start:");
    put_int((uint_32)user_pool.btmp.bits);
    put_char('\n');
    put_str("user physical addr start:");
    put_int(user_pool.paddr_start);
    put_char('\n');

    ker_vaddr.btmp.bits = (uint_8*)(BTMP_START + kbm_len + ubm_len);
    ker_vaddr.btmp.map_size = ker_free_pages / 8;
    ker_vaddr.vaddr_start = K_HEAP_START;
    bit_init(&ker_vaddr.btmp);
    put_str("kernel virtural memory bitmap start:");
    put_int((uint_32)ker_vaddr.btmp.bits);
    put_char('\n');
    put_str("kernel virtural memory addr start:");
    put_int(ker_vaddr.vaddr_start);
    put_char('\n');
    put_str("  mem pool init done\n");
}

void mem_init(void)
{
    put_str("memory init start\n");
    uint_32 tot_mem = *((uint_32*)0x800);
    mem_pool_init(tot_mem);
    block_desc_init(ker_block_desc);
    put_str("memory init done\n");
}

static void* get_vaddr(enum pool_flag pf,uint_32 pcnt)
{
    uint_32 vaddr_get;
    if (pf == PF_KERNEL)
    {
        uint_32 bit_start = bit_scan(&ker_vaddr.btmp,pcnt);
        if (bit_start == -1){
            return NULL;
        }

        uint_32 cnt = 0;
        while (cnt < pcnt){
            bitmap_set(&ker_vaddr.btmp,bit_start+cnt++,1);
        }
        vaddr_get = ker_vaddr.vaddr_start + bit_start*PAGE_SIZE;
    }
    else
    {
        struct task_struct* cur = running_thread();
        uint_32 bit_start = bit_scan(&cur->usrprog_vaddr.btmp,pcnt);
        if (bit_start == -1){
            return NULL;
        }

        uint_32 cnt = 0;
        while (cnt < pcnt){
            bitmap_set(&cur->usrprog_vaddr.btmp,bit_start+cnt++,1);
        }
        vaddr_get = cur->usrprog_vaddr.vaddr_start + bit_start*PAGE_SIZE;
    }
    return (void*)vaddr_get;
}

uint_32* pte_ptr(uint_32 vaddr)
{
    uint_32 v = (0xffc00000 + (PDE_IDX(vaddr)<<12) + (PTE_IDX(vaddr)<<2));
    return (uint_32*)v;
}

uint_32* pde_ptr(uint_32 vaddr)
{
    uint_32* v = (uint_32*)(0xfffff000 + (PDE_IDX(vaddr)<<2));
    return v;
}

static void* palloc(struct pool* m_pool)
{
    uint_32 bit_off = bit_scan(&m_pool->btmp,1);
    if (bit_off == -1){
        return NULL;
    }
    bitmap_set(&m_pool->btmp,bit_off,1);

    return (void*)(m_pool->paddr_start + bit_off*PAGE_SIZE);
}

static void page_table_add(void* _vaddr,void* _paddr)
{
    uint_32 vaddr=(uint_32)_vaddr,paddr=(uint_32)_paddr;
    uint_32* pde = pde_ptr(vaddr),*pte = pte_ptr(vaddr);

    if ((*pde & PG_P) == 1)
    {
        ASSERT((*pte & PG_P)==0);
        *pte = paddr | PG_P | PG_RW_RW | PG_US_U;
    }
    else
    {
        void* new_phy_page = palloc(&kernel_pool);
        *pde = (uint_32)new_phy_page | PG_P | PG_RW_RW | PG_US_U;
        memset((void*)((uint_32)pte & 0xfffff000),0,PAGE_SIZE);

        ASSERT((*pte & PG_P)==0);
        *pte = paddr | PG_P | PG_RW_RW | PG_US_U;
    }
}

void* malloc_page(enum pool_flag pf,uint_32 pcnt)
{
    ASSERT(pcnt>0 && pcnt<64000); //按照用户和内核各250MB来计算
    uint_32 cnt = pcnt;
    struct pool* m_pool = pf==PF_KERNEL? &kernel_pool : &user_pool;
    void* vaddr_start = get_vaddr(pf,pcnt);
    if (vaddr_start == NULL){
        return NULL;
    }

    void* vaddr = vaddr_start;
    while (cnt--)
    {
        void* paddr = palloc(m_pool);
        if (paddr == NULL) {
            return NULL;
        }
        page_table_add(vaddr,paddr);
        vaddr =(void*)((uint_32)vaddr + PAGE_SIZE);
    }

    return vaddr_start;
}

void* get_kernel_pages(uint_32 pcnt)
{
    void* vaddr = malloc_page(PF_KERNEL,pcnt);
    if (vaddr != NULL)
        memset(vaddr,0,PAGE_SIZE*pcnt);
    return vaddr;
}

void* get_a_page(enum pool_flag pf,uint_32 vaddr)
{
    struct pool* m_pool = pf==PF_KERNEL? &kernel_pool : &user_pool;
    lock_acquire(&m_pool->lock);

    struct task_struct* cur = running_thread();
    if (pf == PF_KERNEL && cur->pdir == NULL)
    {
        uint_32 bit_idx = (vaddr - ker_vaddr.vaddr_start)/PAGESIZE;
        bitmap_set(&ker_vaddr.btmp,bit_idx,1);
    }
    else if (pf == PF_USER && cur->pdir != NULL)
    {
        uint_32 bit_idx = (vaddr - cur->usrprog_vaddr.vaddr_start)/PAGESIZE;
        bitmap_set(&cur->usrprog_vaddr.btmp,bit_idx,1);
    }
    else {
        PANIC("get a page");
    }

    void* paddr = palloc(m_pool);
    if (paddr == NULL){
        return NULL;
    }
    page_table_add((void*)vaddr,paddr);
    lock_release(&m_pool->lock);
    return (void*)vaddr;
}

void* get_a_page_without_opvaddrbitmap(enum pool_flag pf,uint_32 vaddr)
{
    struct pool* m_pool = pf==PF_KERNEL? &kernel_pool : &user_pool;
    lock_acquire(&m_pool->lock);

    void* paddr = palloc(m_pool);
    if (paddr == NULL){
        lock_release(&m_pool->lock);
        return NULL;
    }
    page_table_add((void*)vaddr,paddr);
    lock_release(&m_pool->lock);
    return (void*)vaddr;
}


uint_32 v2p(void* vaddr)
{
    uint_32 vr = (uint_32)vaddr;
    uint_32* pte = pte_ptr(vr);
    return ((*pte&0xfffff000) | (vr&0xfff));
}

void block_desc_init(struct mem_block_desc* descs)
{
    uint_32 b_sz = 16;
    for (uint_8 desc_idx=0 ; desc_idx < DESC_CNT ; desc_idx++)
    {
        descs[desc_idx].block_size = b_sz;
        descs[desc_idx].blocks_per_arena = (PAGESIZE - sizeof(struct arena))/b_sz;
        list_init(&descs[desc_idx].free_list);
        b_sz *= 2;
    }
}

struct mem_block* arena2block(struct arena* arena,uint_32 idx) {
    uint_32 offset = (uint_32)arena + sizeof(struct arena) + idx*arena->blk_desc->block_size;
    return (struct mem_block*)offset;
}

struct arena* block2arena(struct mem_block* blk) {
    return (struct arena*)((uint_32)blk & 0xfffff000);
}


void* sys_malloc(uint_32 size)
{
    enum pool_flag pf;
    struct pool* m_pool;
    struct mem_block_desc* desc;

    struct task_struct* cur = running_thread();
    if (cur->pdir == NULL) {
        pf = PF_KERNEL;
        m_pool = &kernel_pool;
        desc = ker_block_desc;
    } else {
        pf = PF_USER;
        m_pool = &user_pool;
        desc = cur->usr_block_desc;
    }

    struct arena* arena;
    struct mem_block* blk;
    lock_acquire(&m_pool->lock);
    if (size > m_pool->pool_size)  return NULL;
    if (size > 1024)
    {
        uint_32 page_cnt = DIV_ROUND_UP(size + sizeof(struct arena),PAGESIZE);
        arena = malloc_page(pf,page_cnt);
        ASSERT(arena != NULL);
        memset(arena,0,PAGESIZE*page_cnt);

        arena->blk_desc = NULL;
        arena->cnt      = page_cnt;
        arena->large    = true;

        lock_release(&m_pool->lock);
        return (void*)(arena + 1);
    }

    uint_32 desc_nr;
    for (desc_nr=0 ; desc_nr<DESC_CNT ; desc_nr++) {
        if (size <= desc[desc_nr].block_size) break;
    }
    desc = &desc[desc_nr];
    if (list_empty(&desc->free_list))
    {
        arena = malloc_page(pf,1);
        arena->blk_desc = desc;
        arena->cnt      = desc->blocks_per_arena;
        arena->large    = false;

        enum intr_status old_status = intr_disable();   //操作list需要原子操作
        for (uint_32 idx=0 ; idx < desc->blocks_per_arena ; idx++)
        {
            blk = arena2block(arena,idx);
            list_append(&desc->free_list,&blk->elm);
        }
        intr_set_status(old_status);
    }

    blk = mem2entry(struct mem_block,list_pop(&desc->free_list),elm);
    memset(blk,0,desc->block_size);
    arena = block2arena(blk);
    arena->cnt--;

    lock_release(&m_pool->lock);
    return (void*)blk;
}

void pfree(uint_32 paddr)
{
    uint_32 bit_idx;
    if (paddr >= user_pool.paddr_start) {
        bit_idx = (paddr - user_pool.paddr_start)/PAGESIZE;
        bitmap_set(&user_pool.btmp,bit_idx,0);
    } else {
        bit_idx = (paddr - kernel_pool.paddr_start)/PAGESIZE;
        bitmap_set(&kernel_pool.btmp,bit_idx,0);
    }
}

void remove_pte(void* vaddr)
{
    uint_32 v = (uint_32)vaddr;
    uint_32* pte = pte_ptr(v);
    *pte &= ~PG_P;
}

void vfree(enum pool_flag pf,void* vaddr,uint_32 pcnt)
{
    struct task_struct* cur = running_thread();
    uint_32 bit_idx_start;
    uint_32 cnt = 0;
    if (pf == PF_KERNEL) {
        bit_idx_start = ((uint_32)vaddr - ker_vaddr.vaddr_start)/PAGESIZE;
        while (cnt < pcnt) {
            bitmap_set(&ker_vaddr.btmp,bit_idx_start+cnt++,0);
        }
    } else {
        bit_idx_start = ((uint_32)vaddr - cur->usrprog_vaddr.vaddr_start)/PAGESIZE;
        while (cnt < pcnt) {
            bitmap_set(&cur->usrprog_vaddr.btmp,bit_idx_start+cnt++,0);
        }
    }
}

void mfree_page(enum pool_flag pf,void* _vaddr,uint_32 pcnt)
{
    uint_32 cnt = 0;
    void* vaddr = _vaddr;
    uint_32 paddr = v2p(_vaddr);
    ASSERT((paddr % PAGESIZE)==0 && paddr >= 0x102000);
    while (cnt < pcnt) 
    {
        pfree(paddr);
        remove_pte(vaddr);
        vaddr = (void*)((uint_32)vaddr + PAGESIZE);
        paddr = v2p(vaddr); //物理地址不一定连续，不能直接加PAGESIZE，得根据vaddr求得
        cnt++;
    }

    vfree(pf,_vaddr,pcnt);
}

void sys_free(void* ptr)
{
    if (ptr == NULL){
        return ;
    }
    enum pool_flag pf;
    struct pool* m_pool;
    struct task_struct* cur = running_thread();
    if (cur->pdir == NULL) {
        pf = PF_KERNEL;
        m_pool = &kernel_pool;
    } else {
        pf = PF_USER;
        m_pool = &user_pool;
    }

    lock_acquire(&m_pool->lock);
    struct mem_block* b = ptr;
    struct arena* a = block2arena(b);
    if (a->blk_desc == NULL && a->large == true) {
        mfree_page(pf,a,a->cnt);
        lock_release(&m_pool->lock);
        return ;
    }

    ASSERT(a->blk_desc != NULL && a->large == false);
    struct mem_block_desc* desc = a->blk_desc;
    list_append(&desc->free_list,&b->elm);
    if (++a->cnt == desc->blocks_per_arena)
    {
        uint_32 idx;
        for (idx = 0 ; idx < desc->blocks_per_arena ; idx++)
        {
            struct mem_block* b = arena2block(a,idx);
            list_remove(&b->elm);
        }
        mfree_page(pf,a,1);
    }
    lock_release(&m_pool->lock);
}

