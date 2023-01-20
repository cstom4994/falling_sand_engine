
#ifndef _METADOT_C_LIST_H_
#define _METADOT_C_LIST_H_

// Generic list implementation
// In this implementation, every new element added is copied to the list, not just referenced
// If there is the need to use this list to reference an variable, initialize the list as a pointers list

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct ListCell* ListCellPointer;
typedef struct ListCell {
    void* element;
    ListCellPointer next;
    ListCellPointer previous;
} ListCell;

typedef struct List {
    unsigned elementSize;
    ListCellPointer first;
    ListCellPointer last;
    unsigned length;
} List;

List InitList(unsigned size);
void FreeList(List* list);

int IsListEmpty(List list);

void InsertListEnd(List* list, void* e);
void InsertListStart(List* list, void* e);
void InsertListIndex(List* list, void* e, int index);

void RemoveListCell(List* list, ListCellPointer cell);
void RemoveListEnd(List* list);
void RemoveListStart(List* list);
void RemoveListIndex(List* list, int index);

void* GetElement(ListCell c);
void* GetLastElement(List list);
void* GetFirstElement(List list);
void* GetElementAt(List list, int index);

ListCellPointer GetNextCell(ListCellPointer c);
ListCellPointer GetPreviousCell(ListCellPointer c);
ListCellPointer GetLastCell(List list);
ListCellPointer GetFirstCell(List list);
ListCellPointer GetCellAt(List list, int index);

unsigned GetElementSize(List list);
unsigned GetLength(List list);

#define ListForEach(cellPointer, list) for (cellPointer = GetFirstCell(list); cellPointer != NULL; cellPointer = GetNextCell(cellPointer))
#define GetElementAsType(cellPointer, type) (*((type*)GetElement(*cellPointer)))

#if defined(__cplusplus)
}
#endif

#endif