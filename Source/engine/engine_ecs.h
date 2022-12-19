// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_ENGINEECS_H_
#define _METADOT_ENGINEECS_H_

#include "engine_core.h"
#include "utils.h"

// ECS Types
typedef struct ComponentMask {
    unsigned long mask;
} ComponentMask;

typedef int EntityID;

typedef struct Entity {
    ComponentMask mask;
    int isSpawned;
    int isParent;
    int isChild;
    int isPrefab;
    char prefabPath[4096];
    char prefabName[256];

    // If parent: List of his child EntityIDs
    List childs;
    // If child: ID of his parent
    EntityID parent;
} Entity;

typedef struct Component {
    void *data;
} Component;

typedef int ComponentID;

typedef struct ComponentType {
    char name[25];
    void (*constructor)(void **data);
    void (*destructor)(void **data);
    void *(*copy)(void *);

    // cJSON *(*encode)(void **data, cJSON *currentData);
    // void *(*decode)(cJSON **data);
} ComponentType;

typedef int SystemID;

typedef struct System System;
typedef struct System {
    char name[25];
    int priority;
    int enabled;

    ComponentMask required;
    ComponentMask excluded;

    void (*systemInit)();
    void (*systemUpdate)();
    void (*systemFree)();
} System;

typedef struct engine_ecs {
    unsigned maxEntities;
    long int maxUsedIndex;

    // Dinamically allocated array of Entity structs
    Entity *Entities;
    List AvaliableEntitiesIndexes;

    // Array containing dinamicaly allocated arrays with components
    // Each index of the main array is set to be unique to that component type
    Component **Components;
    // Array containing the name and operation functions of the component types
    ComponentType *ComponentTypes;
    unsigned numberOfComponents;

    // List containing all systems
    List SystemList;
} engine_ecs;

// ECS functions
int InitECS(unsigned max_entities);
void FreeECS();

int RegisterNewComponent(char componentName[25], void (*constructorFunc)(void **data), void (*destructorFunc)(void **data), void *(*copyFunc)(void *));
int RegisterNewSystem(char systemName[25], int priority, ComponentMask required, ComponentMask excluded, void (*initFunc)(), void (*updateFunc)(), void (*freeFunc)());

ComponentID GetComponentID(char componentName[25]);

// Receives string
ComponentMask CreateComponentMaskByName(int numComp, ...);
// Receives ComponentID
ComponentMask CreateComponentMaskByID(int numComp, ...);

EntityID CreateEntity();
void DestroyEntity(EntityID entity);
int IsValidEntity(EntityID entity);
int EntityIsPrefab(EntityID entity);
char *GetPrefabPath(EntityID entity);
char *GetPrefabName(EntityID entity);
void AddComponentToEntity(ComponentID component, EntityID entity);
void RemoveComponentFromEntity(ComponentID component, EntityID entity);
EntityID DuplicateEntity(EntityID entity);

ComponentMask GetEntityComponents(EntityID entity);
int IsEmptyComponentMask(ComponentMask mask);
int EntityContainsMask(EntityID entity, ComponentMask mask);
int EntityContainsComponent(EntityID entity, ComponentID component);
int MaskContainsComponent(ComponentMask mask, ComponentID component);
ComponentMask IntersectComponentMasks(ComponentMask mask1, ComponentMask mask2);

int EntityIsParent(EntityID entity);
int EntityIsChild(EntityID entity);
void SetEntityParent(EntityID child, EntityID parent);
EntityID GetEntityParent(EntityID entity);
List *GetChildsList(EntityID parent);
int UnsetParent(EntityID child);

// cJSON *EncodeEntity(EntityID entity, int encodingToPrefab);
// EntityID DecodeEntity(cJSON **entityObj);

// int ExportEntityPrefab(EntityID entity, char path[], char name[]);
// EntityID ImportEntityPrefab(char path[], char name[]);

SystemID GetSystemID(char systemName[25]);
void EnableSystem(SystemID system);
void DisableSystem(SystemID system);
int IsSystemEnabled(SystemID system);

#endif