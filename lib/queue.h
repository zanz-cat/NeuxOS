#ifndef __LIB_QUEUE_H__
#define __LIB_QUEUE_H__

#include "list.h"

#define LIST_ENQUEUE(head, node) LIST_ADD_TAIL((head), (node))

#define LIST_DEQUEUE(head) \
__extension__ ({ \
    struct list_node *__node = (head)->next; \
    LIST_DEL_HEAD(head); \
    (__node == (head) ? NULL : __node); \
})

#endif