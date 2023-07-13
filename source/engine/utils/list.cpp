
#include "list.hpp"

#include "engine/core/base_memory.h"

// --------------- Generic list Functions ---------------

list ME_create_list(unsigned size) {
    list l;
    l.first = NULL;
    l.last = NULL;
    l.elementSize = size;
    l.length = 0;

    return l;
}

void ME_destroy_list(list *list) {
    while (list->first) {
        ME_list_remove_start(list);
    }
}

int ME_list_is_empty(list list) { return list.first == NULL ? 1 : 0; }

void ME_list_insert_end(list *list, void *e) {
    list_cell_pointer newCell = (list_cell_pointer)ME_MALLOC(sizeof(list_cell));
    newCell->previous = list->last;
    newCell->next = NULL;

    newCell->element = ME_MALLOC(list->elementSize);
    memcpy(newCell->element, e, list->elementSize);

    if (list->last) {
        list->last->next = newCell;
        list->last = newCell;
    } else {
        list->first = newCell;
        list->last = newCell;
    }

    list->length += 1;
}

void ME_list_insert_start(list *list, void *e) {
    list_cell_pointer newCell = (list_cell_pointer)ME_MALLOC(sizeof(list_cell));
    newCell->previous = NULL;
    newCell->next = list->first;

    newCell->element = ME_MALLOC(list->elementSize);
    memcpy(newCell->element, e, list->elementSize);

    if (list->first) {
        list->first->previous = newCell;
        list->first = newCell;
    } else {
        list->first = newCell;
        list->last = newCell;
    }

    list->length += 1;
}

void ME_list_insert_index(list *list, void *e, int index) {
    int i;

    if (index < 0)  // Support acessing from the end of the list
        index += list->length;

    // Get the element that will go after the element to be inserted
    list_cell_pointer current = list->first;
    for (i = 0; i < index; i++) {
        current = ME_list_get_cell_next(current);
    }

    // If the index is already ocupied
    if (current != NULL) {
        list_cell_pointer newCell = (list_cell_pointer)ME_MALLOC(sizeof(list_cell));
        newCell->element = ME_MALLOC(list->elementSize);
        memcpy(newCell->element, e, list->elementSize);
        newCell->next = current;

        // Connect the cells to their new parents
        newCell->previous = current->previous;
        current->previous = newCell;

        // If the index is 0 (first), set newCell as first
        if (list->first == current) {
            list->first = newCell;
        }

        // If the index is list length (last), set newCell as last
        if (list->last == current) {
            list->last = newCell;
        }

        // If the previous is not null, point his next to newCell
        if (newCell->previous) {
            newCell->previous->next = newCell;
        }

        list->length += 1;

    } else {
        // Index is list length or off bounds (consider as insertion in the end)
        ME_list_insert_end(list, e);
    }
}

void ME_list_remove_cell(list *list, list_cell_pointer cell) {
    if (!cell->previous)
        return ME_list_remove_start(list);
    else if (!cell->next)
        return ME_list_remove_end(list);

    cell->next->previous = cell->previous;
    cell->previous->next = cell->next;

    ME_FREE(cell->element);
    ME_FREE(cell);

    list->length -= 1;
}

void ME_list_remove_end(list *list) {
    if (list->last->previous) {
        list_cell_pointer aux = list->last->previous;
        ME_FREE(list->last->element);
        ME_FREE(list->last);

        aux->next = NULL;
        list->last = aux;
    } else {
        ME_FREE(list->last->element);
        ME_FREE(list->last);

        list->last = NULL;
        list->first = NULL;
    }

    list->length -= 1;
}

void ME_list_remove_start(list *list) {
    if (ME_list_is_empty(*list)) return;

    if (list->first->next) {
        list_cell_pointer aux = list->first->next;
        ME_FREE(list->first->element);
        ME_FREE(list->first);

        aux->previous = NULL;
        list->first = aux;
    } else {
        ME_FREE(list->first->element);
        ME_FREE(list->first);

        list->first = NULL;
        list->last = NULL;
    }

    list->length -= 1;
}

void ME_list_remove_index(list *list, int index) {
    int i;

    if (index < 0)  // Support acessing from the end of the list
        index += list->length;

    if (index == 0)
        return ME_list_remove_start(list);
    else if (index == list->length - 1)
        return ME_list_remove_end(list);

    list_cell_pointer current = list->first;
    for (i = 0; i < index; i++) {
        current = ME_list_get_cell_next(current);
    }

    current->next->previous = current->previous;
    current->previous->next = current->next;

    ME_FREE(current->element);
    ME_FREE(current);

    list->length -= 1;
}

void *ME_list_get_element(list_cell c) { return c.element; }

void *ME_list_get_last(list list) {
    if (!list.last) return NULL;
    return list.last->element;
}

void *ME_list_get_first(list list) {
    if (!list.first) return NULL;
    return list.first->element;
}

void *ME_list_get_at(list list, int index) {
    int i;

    if (index < 0)  // Support acessing from the end of the list
        index += list.length;

    list_cell_pointer current = list.first;
    for (i = 0; i < index; i++) {
        current = ME_list_get_cell_next(current);
    }

    return current->element;
}

list_cell_pointer ME_list_get_cell_next(list_cell_pointer c) {
    if (!c) return NULL;
    return c->next;
}

list_cell_pointer ME_list_get_cell_previous(list_cell_pointer c) {
    if (!c) return NULL;
    return c->previous;
}

list_cell_pointer ME_list_get_cell_first(list list) { return list.first; }

list_cell_pointer ME_list_get_cell_last(list list) { return list.last; }

list_cell_pointer ME_list_get_cell_at(list list, int index) {
    int i;

    list_cell_pointer current = list.first;
    for (i = 0; i < index; i++) {
        current = ME_list_get_cell_next(current);
    }

    return current;
}

unsigned ME_list_get_element_size(list list) { return list.elementSize; }

unsigned ME_list_get_length(list list) { return list.length; }
