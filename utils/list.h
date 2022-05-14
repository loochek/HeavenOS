#ifndef LIST_H
#define LIST_H

#include "common.h"

typedef struct list_node
{
    struct list_node *next;
    struct list_node *prev;
} list_node_t;

/**
 * \param node List node
 * 
 * \return True if associated list is empty, false otherwise
 */
bool list_empty(list_node_t *node);

/**
 * Initializes an empty list
 * 
 * \param node List node
 */
void list_init(list_node_t *node);

/**
 * Inserts new node after specified one
 * 
 * \param node List node
 * \param node New node
 */
void list_insert_after(list_node_t *node, list_node_t *new);

/**
 * Extracts node from the list. Be careful to not extract list head
 * 
 * \param node List node
 */
void list_extract(list_node_t *node);

#endif
