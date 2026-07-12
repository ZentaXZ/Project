// cJSON.c - Simplified stub (full cJSON source should be downloaded from github.com/DaveGamble/cJSON)
// This is a placeholder. Replace with the actual cJSON.c from the repository.

#include "cJSON.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <float.h>
#include <ctype.h>
#include <stddef.h>

#ifndef CJSON_NOINLINE
#define CJSON_NOINLINE
#endif

static void* CJSON_CDECL cJSON_malloc_default(size_t size) {
    return malloc(size);
}

static void CJSON_CDECL cJSON_free_default(void *p) {
    free(p);
}

static cJSON_Hooks global_hooks = {
    cJSON_malloc_default,
    cJSON_free_default
};

void cJSON_InitHooks(cJSON_Hooks *hooks) {
    if (!hooks) {
        global_hooks.malloc_fn = cJSON_malloc_default;
        global_hooks.free_fn = cJSON_free_default;
        return;
    }
    global_hooks.malloc_fn = hooks->malloc_fn ? hooks->malloc_fn : cJSON_malloc_default;
    global_hooks.free_fn = hooks->free_fn ? hooks->free_fn : cJSON_free_default;
}

void *cJSON_malloc(size_t size) {
    return global_hooks.malloc_fn(size);
}

void cJSON_free(void *object) {
    global_hooks.free_fn(object);
}

static cJSON *cJSON_New_Item(void) {
    cJSON *node = (cJSON*)cJSON_malloc(sizeof(cJSON));
    if (node) {
        memset(node, 0, sizeof(cJSON));
    }
    return node;
}

void cJSON_Delete(cJSON *item) {
    cJSON *next;
    while (item) {
        next = item->next;
        if (!(item->type & cJSON_IsReference)) {
            cJSON_Delete(item->child);
            if (item->valuestring) cJSON_free(item->valuestring);
            if (item->string) cJSON_free(item->string);
        }
        cJSON_free(item);
        item = next;
    }
}

cJSON *cJSON_CreateNull(void) {
    cJSON *item = cJSON_New_Item();
    if (item) item->type = cJSON_NULL;
    return item;
}

cJSON *cJSON_CreateTrue(void) {
    cJSON *item = cJSON_New_Item();
    if (item) item->type = cJSON_True;
    return item;
}

cJSON *cJSON_CreateFalse(void) {
    cJSON *item = cJSON_New_Item();
    if (item) item->type = cJSON_False;
    return item;
}

cJSON *cJSON_CreateBool(int b) {
    cJSON *item = cJSON_New_Item();
    if (item) item->type = b ? cJSON_True : cJSON_False;
    return item;
}

cJSON *cJSON_CreateNumber(double num) {
    cJSON *item = cJSON_New_Item();
    if (item) {
        item->type = cJSON_Number;
        item->valuedouble = num;
        item->valueint = (int)num;
    }
    return item;
}

cJSON *cJSON_CreateString(const char *string) {
    cJSON *item = cJSON_New_Item();
    if (!item) return NULL;
    item->type = cJSON_String;
    if (string) {
        item->valuestring = (char*)cJSON_malloc(strlen(string) + 1);
        if (!item->valuestring) {
            cJSON_Delete(item);
            return NULL;
        }
        strcpy(item->valuestring, string);
    }
    return item;
}

cJSON *cJSON_CreateArray(void) {
    cJSON *item = cJSON_New_Item();
    if (item) item->type = cJSON_Array;
    return item;
}

cJSON *cJSON_CreateObject(void) {
    cJSON *item = cJSON_New_Item();
    if (item) item->type = cJSON_Object;
    return item;
}

int cJSON_GetArraySize(const cJSON *array) {
    cJSON *c = array->child;
    int i = 0;
    while (c) {
        i++;
        c = c->next;
    }
    return i;
}

cJSON *cJSON_GetArrayItem(const cJSON *array, int index) {
    cJSON *c = array ? array->child : NULL;
    while (c && index > 0) {
        index--;
        c = c->next;
    }
    return c;
}

cJSON *cJSON_GetObjectItem(const cJSON *object, const char *string) {
    cJSON *c = object ? object->child : NULL;
    while (c) {
        if (c->string && strcmp(c->string, string) == 0) return c;
        c = c->next;
    }
    return NULL;
}

int cJSON_HasObjectItem(const cJSON *object, const char *string) {
    return cJSON_GetObjectItem(object, string) != NULL;
}

void cJSON_AddItemToArray(cJSON *array, cJSON *item) {
    if (!array || !item) return;
    if (!array->child) {
        array->child = item;
    } else {
        cJSON *c = array->child;
        while (c->next) c = c->next;
        c->next = item;
        item->prev = c;
    }
}

void cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item) {
    if (!item) return;
    if (item->string) cJSON_free(item->string);
    item->string = (char*)cJSON_malloc(strlen(string) + 1);
    if (!item->string) return;
    strcpy(item->string, string);
    cJSON_AddItemToArray(object, item);
}

void cJSON_AddNullToObject(cJSON *object, const char *name) {
    cJSON_AddItemToObject(object, name, cJSON_CreateNull());
}

void cJSON_AddTrueToObject(cJSON *object, const char *name) {
    cJSON_AddItemToObject(object, name, cJSON_CreateTrue());
}

void cJSON_AddFalseToObject(cJSON *object, const char *name) {
    cJSON_AddItemToObject(object, name, cJSON_CreateFalse());
}

void cJSON_AddBoolToObject(cJSON *object, const char *name, int b) {
    cJSON_AddItemToObject(object, name, cJSON_CreateBool(b));
}

void cJSON_AddNumberToObject(cJSON *object, const char *name, double n) {
    cJSON_AddItemToObject(object, name, cJSON_CreateNumber(n));
}

void cJSON_AddStringToObject(cJSON *object, const char *name, const char *string) {
    cJSON_AddItemToObject(object, name, cJSON_CreateString(string));
}

void cJSON_AddArrayToObject(cJSON *object, const char *name) {
    cJSON_AddItemToObject(object, name, cJSON_CreateArray());
}

void cJSON_AddObjectToObject(cJSON *object, const char *name) {
    cJSON_AddItemToObject(object, name, cJSON_CreateObject());
}

static void print_value(cJSON *item, char *p, int format) {
    // Simplified print implementation
    if (!item) return;
    if (item->type == cJSON_NULL) strcat(p, "null");
    else if (item->type == cJSON_True) strcat(p, "true");
    else if (item->type == cJSON_False) strcat(p, "false");
    else if (item->type == cJSON_Number) sprintf(p + strlen(p), "%g", item->valuedouble);
    else if (item->type == cJSON_String) {
        strcat(p, "\"");
        strcat(p, item->valuestring);
        strcat(p, "\"");
    }
}

char *cJSON_Print(cJSON *item) {
    char *out = (char*)cJSON_malloc(256); // TODO: proper buffer sizing
    if (!out) return NULL;
    out[0] = 0;
    print_value(item, out, 1);
    return out;
}

char *cJSON_PrintUnformatted(cJSON *item) {
    return cJSON_Print(item);
}

static const char *parse_value(cJSON *item, const char *value);

cJSON *cJSON_Parse(const char *value) {
    if (!value) return NULL;
    cJSON *item = cJSON_New_Item();
    if (!item) return NULL;
    parse_value(item, value);
    return item;
}

static const char *parse_value(cJSON *item, const char *value) {
    if (!value) return NULL;
    // Minimal parser stub
    if (!strncmp(value, "null", 4)) {
        item->type = cJSON_NULL;
        return value + 4;
    }
    if (!strncmp(value, "true", 4)) {
        item->type = cJSON_True;
        return value + 4;
    }
    if (!strncmp(value, "false", 5)) {
        item->type = cJSON_False;
        return value + 5;
    }
    return value;
}

int cJSON_GetNumberValue(const cJSON *item) {
    if (!item || item->type != cJSON_Number) return 0;
    return item->valueint;
}

char *cJSON_GetStringValue(const cJSON *item) {
    if (!item || item->type != cJSON_String) return NULL;
    return item->valuestring;
}

cJSON *cJSON_Duplicate(cJSON *item, int recurse) {
    if (!item) return NULL;
    cJSON *newitem = cJSON_New_Item();
    if (!newitem) return NULL;
    newitem->type = item->type;
    if (item->valuestring) {
        newitem->valuestring = (char*)cJSON_malloc(strlen(item->valuestring) + 1);
        strcpy(newitem->valuestring, item->valuestring);
    }
    if (item->string) {
        newitem->string = (char*)cJSON_malloc(strlen(item->string) + 1);
        strcpy(newitem->string, item->string);
    }
    if (recurse && item->child) {
        newitem->child = cJSON_Duplicate(item->child, recurse);
    }
    return newitem;
}

int cJSON_Version(void) {
    return (CJSON_VERSION_MAJOR << 16) | (CJSON_VERSION_MINOR << 8) | CJSON_VERSION_PATCH;
}
