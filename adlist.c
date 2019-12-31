#include <stdlib.h>
#include "zmalloc.h"
#include "adlist.h"

list *listCreate(void)
{
    struct list *list;
    list = zmalloc(sizeof(list));
    list->head = list->tail = NULL;
    list->len = 0;
    list->dup = NULL;
    list->free = NULL;
    list->match = NULL;
    return list;
}

void listEmpty(list *list)
{
    unsigned long len;
    listNode *current, *next;

    current = list->head;
    len = list->len;
    while(len--) {
        next = current->next;
        if (list->free) list->free(current->value);
        zfree(current);
        current = next;
    }
    list->head = list->tail = NULL;
    list->len = 0;
}

void listRelease(list *list)
{
    listEmpty(list);
    zfree(list);
}

list *listAddNodeHead(list *list, void *value)
{
    listNode *node;

    node = zmalloc(sizeof(*node));
    node->value = value;
    if (list->len == 0) {
        list->head = list->tail = node;
        node->prev = node->next = NULL;
    } else {
        node->prev = NULL;
        node->next = list->head;
        list->head->prev = node;
        list->head = node;
    }
    list->len++;
    return list;
}

list *listAddNodeTail(list *list, void *value)
{
    listNode *node;

    node = zmalloc(sizeof(*node));
    node->value = value;
    if (list->len == 0) {
        list->head = list->tail = node;
        node->prev = node->next = NULL;
    } else {
        node->next = NULL;
        node->prev = list->tail;
        list->tail->next = node;
        list->tail = node;
    }
    list->len++;
    return list;
}

list *listInsertNode(list *list, listNode *old_node, void *value, int after)
{
    listNode *node;

    node = zmalloc(sizeof(*node));
    node->value = value;
    if (after) {
        node->prev = old_node;
        node->next = old_node->next;
        if (list->tail == old_node)
            list->tail = node;
    } else {
        node->next = old_node;
        node->prev = old_node->prev;
        if (list->head == old_node)
            list->head = node;
    }
    if (node->prev != NULL)
        node->prev->next = node;
    if (node->next != NULL)
        node->next->prev = node;
    list->len++;
    return list;
}

void listDelNode(list *list, listNode *node)
{
    if (node->prev)
        node->prev->next = node->next;
    else
        list->head = node->next;
    if (node->next)
        node->next->prev = node->prev;
    else
        list->tail = node->prev;
    if (list->free) list->free(node->value);
    zfree(node);
    list->len--;
}

listIter *listGetIterator(list *list, int direction)
{
    listIter *iter;

    iter = zmalloc(sizeof(*iter));
    if (direction == AL_START_HEAD)
        iter->next = list->head;
    else
        iter->next = list->tail;
    iter->direction = direction;
    return iter;
}

void listReleaseIterator(listIter *iter)
{
    zfree(iter);
}

void listRewind(list *list, listIter *li)
{
    li->next = list->head;
    li->direction = AL_START_HEAD;
}

void listRewindTail(list *list, listIter *li)
{
    li->next = list->tail;
    li->direction = AL_START_TAIL;
}

listNode *listNext(listIter *iter)
{
    listNode *current = iter->next;

    if (current != NULL) {
        if (iter->direction == AL_START_HEAD)
            iter->next = current->next;
        else
            iter->next = current->prev;
    }
    return current;
}

listNode *listSearchKey(list *list, void *key)
{
    listIter iter;
    listNode *node;

    listRewind(list, &iter);
    while ((node = listNext(&iter)) != NULL) {
        if (list->match) {
            if (list->match(node->value, key))
                return node;
        } else {
            if (key == node->value)
                return node;
        }
    }
    return NULL;
}
