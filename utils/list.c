#include "kernel/panic.h"
#include "utils/list.h"

bool list_empty(list_node_t *node)
{
    kassert_dbg(node != NULL);
    return node->next == node;
}

void list_init(list_node_t *node)
{
    kassert_dbg(node != NULL);
    node->next = node;
    node->prev = node;
}

void list_insert_after(list_node_t *node, list_node_t *new)
{
    kassert_dbg(node != NULL);
    kassert_dbg(new != NULL);
    
    list_node_t *old_list_next = node->next;
    node->next = new;

    new->prev = node;
    new->next = old_list_next;

    old_list_next->prev = new;
}

void list_extract(list_node_t *node)
{
    kassert_dbg(node != NULL);
    kassert_dbg(!list_empty(node));

    list_node_t *prev_elem = node->prev;
    list_node_t *next_elem = node->next;

    prev_elem->next = next_elem;
    next_elem->prev = prev_elem;
}
