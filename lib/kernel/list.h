#ifndef LIB_KERNEL_LIST_H
#define LIB_KERNEL_LIST_H
#include "stdint.h"

#define offset(struct_type,member) (uint_32)(&((struct_type*)0)->member)
#define mem2entry(struct_type,mem_addr,member) (struct_type*)((uint_32)mem_addr-offset(struct_type,member))

struct list_elm {
    struct list_elm* prev;
    struct list_elm* next;
};

struct list {
    struct list_elm head;
    struct list_elm tail;
};

typedef Bool (function) (struct list_elm* ,int arg);

void list_init(struct list*);
void list_insert_before(struct list_elm* before,struct list_elm* elm);
void list_push(struct list* plist,struct list_elm* elm);
void list_append(struct list* plist,struct list_elm* elm);
void list_remove(struct list_elm* elm);
struct list_elm* list_pop(struct list* plist);
Bool list_empty(struct list* plist);
uint_32 list_len(struct list* plist);
struct list_elm* list_traversal(struct list* plist,function func,int arg);
Bool elem_find(struct list* plist,struct list_elm* obj_elem);

#endif
