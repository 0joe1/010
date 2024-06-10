#include "list.h"
#include "string.h"
#include "interrupt.h"
#include "print.h"
#include "debug.h"

void list_init(struct list* list)
{
    list->head.prev = NULL;
    list->head.next = &list->tail;
    list->tail.prev = &list->head;
    list->tail.next = NULL;
}

void show_list(struct list* list)
{
    struct list_elm* p = &list->head;
    while (p != &list->tail){
        put_int((uint_32)p);
        if (p != NULL){
            put_str(" -> ");
            p = p->next;
        }
    }
}

void list_insert_before(struct list_elm* before,struct list_elm* elm)
{
    enum intr_status old_status = intr_disable();
    ASSERT(elm != NULL);
    before->prev->next = elm;
    elm->prev = before->prev;
    elm->next = before;
    before->prev = elm;
    intr_set_status(old_status);
}

void list_push(struct list* plist,struct list_elm* elm)
{
    list_insert_before(plist->head.next,elm);
}

void list_append(struct list* plist,struct list_elm* elm)
{
    list_insert_before(&plist->tail,elm);
}

void list_remove(struct list_elm* elm)
{
    enum intr_status old_status = intr_disable();
    elm->prev->next = elm->next;
    elm->next->prev = elm->prev;
    intr_set_status(old_status);
}

struct list_elm* list_pop(struct list* plist)
{
    struct list_elm* ret = plist->head.next;
    list_remove(plist->head.next);
    return ret;
}

Bool list_empty(struct list* plist)
{
    if (plist->head.next == &plist->tail)
        return 1;
    return 0;
}

uint_32 list_len(struct list* plist)
{
    uint_32 len = 0;
    struct list_elm* elm = plist->head.next;
    while (elm != &plist->tail)
    {
        len++;
        elm = elm->next;
    }

    return len;
}

struct list_elm* list_traversal(struct list* plist,function func,int arg)
{
    struct list_elm* ret = plist->head.next;
    while (ret != &plist->tail)
    {
        if (func(ret,arg)){
            return ret;
        }
        ret = ret->next;
    }

    return NULL;
}

Bool elem_find(struct list* plist,struct list_elm* obj_elem)
{
    struct list_elm* ret = plist->head.next;
    while (ret != &plist->tail)
    {
        if (ret == obj_elem){
            return 1;
        }
        ret = ret->next;
    }
    return 0;
}
