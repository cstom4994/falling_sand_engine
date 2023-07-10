#ifndef ME_LIST_HPP
#define ME_LIST_HPP

// Generic list implementation
// In this implementation, every new element added is copied to the list, not just referenced
// If there is the need to use this list to reference an variable, initialize the list as a pointers list

typedef struct list_cell *list_cell_pointer;
typedef struct list_cell {
    void *element;
    list_cell_pointer next;
    list_cell_pointer previous;
} list_cell;

typedef struct list {
    unsigned elementSize;
    list_cell_pointer first;
    list_cell_pointer last;
    unsigned length;
} list;

list ME_create_list(unsigned size);
void ME_destroy_list(list *list);

int ME_list_is_empty(list list);

void ME_list_insert_end(list *list, void *e);
void ME_list_insert_start(list *list, void *e);
void ME_list_insert_index(list *list, void *e, int index);

void ME_list_remove_cell(list *list, list_cell_pointer cell);
void ME_list_remove_end(list *list);
void ME_list_remove_start(list *list);
void ME_list_remove_index(list *list, int index);

void *ME_list_get_element(list_cell c);
void *ME_list_get_last(list list);
void *ME_list_get_first(list list);
void *ME_list_get_at(list list, int index);

list_cell_pointer ME_list_get_cell_next(list_cell_pointer c);
list_cell_pointer ME_list_get_cell_previous(list_cell_pointer c);
list_cell_pointer ME_list_get_cell_last(list list);
list_cell_pointer ME_list_get_cell_first(list list);
list_cell_pointer ME_list_get_cell_at(list list, int index);

unsigned ME_list_get_element_size(list list);
unsigned ME_list_get_length(list list);

#define ListForEach(cellPointer, list) for (cellPointer = ME_list_get_cell_first(list); cellPointer != NULL; cellPointer = ME_list_get_cell_next(cellPointer))
#define GetElementAsType(cellPointer, type) (*((type *)ME_list_get_element(*cellPointer)))

#endif