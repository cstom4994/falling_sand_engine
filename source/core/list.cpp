
#include "list.h"

#include <stdlib.h>
#include <string.h>

#include "core/alloc.hpp"
#include "core/core.h"

// --------------- Generic List Functions ---------------

List InitList(unsigned size) {
    List l;
    l.first = NULL;
    l.last = NULL;
    l.elementSize = size;
    l.length = 0;

    return l;
}

void FreeList(List *list) {
    while (list->first) {
        RemoveListStart(list);
    }
}

int IsListEmpty(List list) { return list.first == NULL ? 1 : 0; }

void InsertListEnd(List *list, void *e) {
    ListCellPointer newCell = (ListCellPointer)malloc(sizeof(ListCell));
    newCell->previous = list->last;
    newCell->next = NULL;

    newCell->element = malloc(list->elementSize);
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

void InsertListStart(List *list, void *e) {
    ListCellPointer newCell = (ListCellPointer)malloc(sizeof(ListCell));
    newCell->previous = NULL;
    newCell->next = list->first;

    newCell->element = malloc(list->elementSize);
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

void InsertListIndex(List *list, void *e, int index) {
    int i;

    if (index < 0)  // Support acessing from the end of the list
        index += list->length;

    // Get the element that will go after the element to be inserted
    ListCellPointer current = list->first;
    for (i = 0; i < index; i++) {
        current = GetNextCell(current);
    }

    // If the index is already ocupied
    if (current != NULL) {
        ListCellPointer newCell = (ListCellPointer)malloc(sizeof(ListCell));
        newCell->element = malloc(list->elementSize);
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
        InsertListEnd(list, e);
    }
}

void RemoveListCell(List *list, ListCellPointer cell) {
    if (!cell->previous)
        return RemoveListStart(list);
    else if (!cell->next)
        return RemoveListEnd(list);

    cell->next->previous = cell->previous;
    cell->previous->next = cell->next;

    free(cell->element);
    free(cell);

    list->length -= 1;
}

void RemoveListEnd(List *list) {
    if (list->last->previous) {
        ListCellPointer aux = list->last->previous;
        free(list->last->element);
        free(list->last);

        aux->next = NULL;
        list->last = aux;
    } else {
        free(list->last->element);
        free(list->last);

        list->last = NULL;
        list->first = NULL;
    }

    list->length -= 1;
}

void RemoveListStart(List *list) {
    if (IsListEmpty(*list)) return;

    if (list->first->next) {
        ListCellPointer aux = list->first->next;
        free(list->first->element);
        free(list->first);

        aux->previous = NULL;
        list->first = aux;
    } else {
        free(list->first->element);
        free(list->first);

        list->first = NULL;
        list->last = NULL;
    }

    list->length -= 1;
}

void RemoveListIndex(List *list, int index) {
    int i;

    if (index < 0)  // Support acessing from the end of the list
        index += list->length;

    if (index == 0)
        return RemoveListStart(list);
    else if (index == list->length - 1)
        return RemoveListEnd(list);

    ListCellPointer current = list->first;
    for (i = 0; i < index; i++) {
        current = GetNextCell(current);
    }

    current->next->previous = current->previous;
    current->previous->next = current->next;

    free(current->element);
    free(current);

    list->length -= 1;
}

void *GetElement(ListCell c) { return c.element; }

void *GetLastElement(List list) {
    if (!list.last) return NULL;
    return list.last->element;
}

void *GetFirstElement(List list) {
    if (!list.first) return NULL;
    return list.first->element;
}

void *GetElementAt(List list, int index) {
    int i;

    if (index < 0)  // Support acessing from the end of the list
        index += list.length;

    ListCellPointer current = list.first;
    for (i = 0; i < index; i++) {
        current = GetNextCell(current);
    }

    return current->element;
}

ListCellPointer GetNextCell(ListCellPointer c) {
    if (!c) return NULL;
    return c->next;
}

ListCellPointer GetPreviousCell(ListCellPointer c) {
    if (!c) return NULL;
    return c->previous;
}

ListCellPointer GetFirstCell(List list) { return list.first; }

ListCellPointer GetLastCell(List list) { return list.last; }

ListCellPointer GetCellAt(List list, int index) {
    int i;

    ListCellPointer current = list.first;
    for (i = 0; i < index; i++) {
        current = GetNextCell(current);
    }

    return current;
}

unsigned GetElementSize(List list) { return list.elementSize; }

unsigned GetLength(List list) { return list.length; }