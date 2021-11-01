#ifndef __LIB_LIST_H__
#define __LIB_LIST_H__

#include <misc/misc.h>

struct list_node {
    struct list_node *prev;
    struct list_node *next;
};

#define __LIST_HEAD_INIT(name) {&(name), &(name)}

#define LIST_HEAD(name) \
    struct list_node name = __LIST_HEAD_INIT(name)

#define LIST_ADD_TAIL(head, node) \
__extension__ ({ \
    struct list_node *__node; \
    for (__node = (head); __node->next != (head); __node = __node->next); \
    __node->next = (node); \
    (node)->prev = __node; \
    (node)->next = (head); \
    (head)->prev = (node); \
})

#define LIST_DEL(node) \
__extension__ ({ \
    (node)->prev->next = (node)->next; \
    (node)->next->prev = (node)->prev; \
    (node)->prev = NULL; \
    (node)->next = NULL; \
})

#define LIST_DEL_HEAD(name) \
__extension__ ({ \
    if ((name)->next != (name)) { \
        struct list_node *__node = (name)->next; \
        __node->next->prev = __node->prev; \
        __node->prev->next = __node->next; \
        __node->next = NULL; \
        __node->prev = NULL; \
    } \
})

#define LIST_FOREACH(head, node) \
    for (node = (head)->next; node != (head); node = node->next)

#define LIST_COUNT(head) \
__extension__ ({ \
    uint32_t __count; \
    struct list_node *__node; \
    for (__count = 0, __node = (head); \
    __node->next != (head); __count++, __node = __node->next); \
    __count; \
})

#define LIST_ENQUEUE(head, node) LIST_ADD_TAIL((head), (node))

#define LIST_DEQUEUE(head) \
__extension__ ({ \
    struct list_node *__node = (head)->next; \
    LIST_DEL_HEAD(head); \
    (__node == (head) ? NULL : __node); \
})

#endif