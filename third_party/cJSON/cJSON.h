/*
  Copyright (c) 2009-2017 Dave Gamble and cJSON contributors

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#ifndef cJSON__h
#define cJSON__h

#ifdef __cplusplus
extern "C"
{
#endif

#if !defined(__WINDOWS__) && (defined(WIN32) || defined(WIN64) || defined(_MSC_VER) || defined(_WIN32))
#define __WINDOWS__
#endif

#ifdef __WINDOWS__
#define CJSON_STDCALL __stdcall
#else
#define CJSON_STDCALL
#endif

if defined(_MSC_VER)
#define CJSON_NOINLINE __declspec(noinline)
#elif (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4))
#define CJSON_NOINLINE __attribute__((noinline))
#else
#define CJSON_NOINLINE
#endif

if !defined(CJSON_API_VISIBILITY)
#if defined(_WIN32) && defined(CJSON_HIDE_SYMBOLS)
#define CJSON_PUBLIC(type) type CJSON_STDCALL
#elif defined(_WIN32) && defined(CJSON_EXPORT_SYMBOLS)
#define CJSON_PUBLIC(type) __declspec(dllexport) type CJSON_STDCALL
#elif defined(_WIN32)
#define CJSON_PUBLIC(type) __declspec(dllimport) type CJSON_STDCALL
#else
#define CJSON_PUBLIC(type) type
#endif
#define CJSON_API_VISIBILITY
#endif

#include <stddef.h>

/* project version */
#define CJSON_VERSION_MAJOR 1
#define CJSON_VERSION_MINOR 7
#define CJSON_VERSION_PATCH 14

#include <stddef.h>
#include <string.h>

typedef struct cJSON {
    struct cJSON *next;
    struct cJSON *prev;
    struct cJSON *child;
    int type;
    char *valuestring;
    double valueint;
    double valuedouble;
    char *string;
} cJSON;

typedef struct cJSON_Hooks {
    void* (CJSON_STDCALL *malloc_fn)(size_t sz);
    void (CJSON_STDCALL *free_fn)(void *p);
} cJSON_Hooks;

if defined(__GNUC__)
#define CJSON_CONST const
#else
#define CJSON_CONST
#endif

enum cJSON_Type {
    cJSON_Invalid = ((1 << 28) + 0),
    cJSON_False = ((1 << 28) + 1),
    cJSON_True = ((1 << 28) + 2),
    cJSON_NULL = ((1 << 28) + 3),
    cJSON_Number = ((1 << 28) + 4),
    cJSON_String = ((1 << 28) + 5),
    cJSON_Array = ((1 << 28) + 6),
    cJSON_Object = ((1 << 28) + 7)
};

#define cJSON_IsInvalid(item) ((item)->type & cJSON_Invalid)
#define cJSON_IsFalse(item) ((item)->type & cJSON_False)
#define cJSON_IsTrue(item) ((item)->type & cJSON_True)
#define cJSON_IsBool(item) (((item)->type & (cJSON_True | cJSON_False)) != 0)
#define cJSON_IsNull(item) ((item)->type & cJSON_NULL)
#define cJSON_IsNumber(item) ((item)->type & cJSON_Number)
#define cJSON_IsString(item) ((item)->type & cJSON_String)
#define cJSON_IsArray(item) ((item)->type & cJSON_Array)
#define cJSON_IsObject(item) ((item)->type & cJSON_Object)

CJSON_PUBLIC(cJSON *) cJSON_Parse(CJSON_CONST char *value);
CJSON_PUBLIC(cJSON *) cJSON_ParseWithLength(CJSON_CONST char *value, size_t buffer_length);
CJSON_PUBLIC(cJSON *) cJSON_ParseWithOpts(CJSON_CONST char *value, CJSON_CONST char **return_parse_end, int require_null_terminated);
CJSON_PUBLIC(cJSON *) cJSON_ParseWithLengthOpts(CJSON_CONST char *value, size_t buffer_length, CJSON_CONST char **return_parse_end, int require_null_terminated);

CJSON_PUBLIC(char *) cJSON_Print(cJSON *item);
CJSON_PUBLIC(char *) cJSON_PrintUnformatted(cJSON *item);
CJSON_PUBLIC(char *) cJSON_PrintBuffered(cJSON *item, int prebuffer, int fmt);
CJSON_PUBLIC(char *) cJSON_PrintPreallocated(cJSON *item, char *buf, const int length, const int format);

CJSON_PUBLIC(void) cJSON_Delete(cJSON *item);
CJSON_PUBLIC(int) cJSON_GetArraySize(CJSON_CONST cJSON *array);
CJSON_PUBLIC(cJSON *) cJSON_GetArrayItem(CJSON_CONST cJSON *array, int index);
CJSON_PUBLIC(cJSON *) cJSON_GetObjectItem(CJSON_CONST cJSON * const object, CJSON_CONST char * const string);
CJSON_PUBLIC(cJSON *) cJSON_GetObjectItemCaseSensitive(CJSON_CONST cJSON * const object, CJSON_CONST char * const string);
CJSON_PUBLIC(int) cJSON_HasObjectItem(CJSON_CONST cJSON *object, CJSON_CONST char *string);

CJSON_PUBLIC(cJSON *) cJSON_CreateNull(void);
CJSON_PUBLIC(cJSON *) cJSON_CreateTrue(void);
CJSON_PUBLIC(cJSON *) cJSON_CreateFalse(void);
CJSON_PUBLIC(cJSON *) cJSON_CreateBool(int b);
CJSON_PUBLIC(cJSON *) cJSON_CreateNumber(double num);
CJSON_PUBLIC(cJSON *) cJSON_CreateString(CJSON_CONST char *string);
CJSON_PUBLIC(cJSON *) cJSON_CreateRaw(CJSON_CONST char *raw);
CJSON_PUBLIC(cJSON *) cJSON_CreateArray(void);
CJSON_PUBLIC(cJSON *) cJSON_CreateObject(void);

CJSON_PUBLIC(cJSON *) cJSON_CreateStringReference(CJSON_CONST char *string);
CJSON_PUBLIC(cJSON *) cJSON_CreateObjectReference(CJSON_CONST cJSON *child);
CJSON_PUBLIC(cJSON *) cJSON_CreateArrayReference(CJSON_CONST cJSON *child);

CJSON_PUBLIC(void) cJSON_AddItemToArray(cJSON *array, cJSON *item);
CJSON_PUBLIC(void) cJSON_AddItemToObject(cJSON *object, CJSON_CONST char *string, cJSON *item);
CJSON_PUBLIC(void) cJSON_AddItemToObjectCS(cJSON *object, CJSON_CONST char *string, cJSON *item);
CJSON_PUBLIC(void) cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item);
CJSON_PUBLIC(void) cJSON_AddItemReferenceToObject(cJSON *object, CJSON_CONST char *string, cJSON *item);

CJSON_PUBLIC(cJSON *) cJSON_DetachItemViaPointer(cJSON *parent, cJSON * const item);
CJSON_PUBLIC(cJSON *) cJSON_DetachItemFromArray(cJSON *array, int which);
CJSON_PUBLIC(void) cJSON_DeleteItemFromArray(cJSON *array, int which);
CJSON_PUBLIC(cJSON *) cJSON_DetachItemFromObject(cJSON *object, CJSON_CONST char *string);
CJSON_PUBLIC(cJSON *) cJSON_DetachItemFromObjectCaseSensitive(cJSON *object, CJSON_CONST char *string);
CJSON_PUBLIC(void) cJSON_DeleteItemFromObject(cJSON *object, CJSON_CONST char *string);
CJSON_PUBLIC(void) cJSON_DeleteItemFromObjectCaseSensitive(cJSON *object, CJSON_CONST char *string);

CJSON_PUBLIC(void) cJSON_InsertItemInArray(cJSON *array, int which, cJSON *newitem);
CJSON_PUBLIC(cJSON *) cJSON_ReplaceItemViaPointer(cJSON * const parent, cJSON * const item, cJSON * const replacement);
CJSON_PUBLIC(cJSON *) cJSON_ReplaceItemInArray(cJSON *array, int which, cJSON *newitem);
CJSON_PUBLIC(cJSON *) cJSON_ReplaceItemInObject(cJSON *object,CJSON_CONST char *string,cJSON *newitem);
CJSON_PUBLIC(cJSON *) cJSON_ReplaceItemInObjectCaseSensitive(cJSON *object,CJSON_CONST char *string,cJSON *newitem);

CJSON_PUBLIC(void) cJSON_ReplaceItemViaPointer(cJSON * const parent, cJSON * const item, cJSON * const replacement);

CJSON_PUBLIC(void) cJSON_Minify(char *json);
CJSON_PUBLIC(void) cJSON_AddNullToObject(cJSON * const object, CJSON_CONST char * const name);
CJSON_PUBLIC(void) cJSON_AddTrueToObject(cJSON * const object, CJSON_CONST char * const name);
CJSON_PUBLIC(void) cJSON_AddFalseToObject(cJSON * const object, CJSON_CONST char * const name);
CJSON_PUBLIC(void) cJSON_AddBoolToObject(cJSON * const object, CJSON_CONST char * const name, int b);
CJSON_PUBLIC(void) cJSON_AddNumberToObject(cJSON * const object, CJSON_CONST char * const name, double n);
CJSON_PUBLIC(void) cJSON_AddStringToObject(cJSON * const object, CJSON_CONST char * const name, CJSON_CONST char * const string);
CJSON_PUBLIC(void) cJSON_AddRawToObject(cJSON * const object, CJSON_CONST char * const name, CJSON_CONST char * const raw);
CJSON_PUBLIC(void) cJSON_AddObjectToObject(cJSON * const object, CJSON_CONST char * const name);
CJSON_PUBLIC(void) cJSON_AddArrayToObject(cJSON * const object, CJSON_CONST char * const name);
CJSON_PUBLIC(void) cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item);

CJSON_PUBLIC(cJSON *) cJSON_Duplicate(cJSON *item, int recurse);
CJSON_PUBLIC(void) cJSON_Duplicate(cJSON *item, int recurse);

CJSON_PUBLIC(int) cJSON_Compare(cJSON * const a, cJSON * const b, const int case_sensitive);
CJSON_PUBLIC(void *) cJSON_malloc(size_t size);
CJSON_PUBLIC(void) cJSON_free(void *object);
CJSON_PUBLIC(void) cJSON_InitHooks(cJSON_Hooks* hooks);

CJSON_PUBLIC(char*) cJSON_GetStringValue(CJSON_CONST cJSON * const item);
CJSON_PUBLIC(double) cJSON_GetNumberValue(CJSON_CONST cJSON * const item);
CJSON_PUBLIC(int) cJSON_IsNumber(CJSON_CONST cJSON * const item);

CJSON_PUBLIC(int) cJSON_Version(void);

#define cJSON_AddItemToArray(array, item) cJSON_InsertItemInArray(array, -1, item)
#define cJSON_SetNumberHelper(object,number) ((object)->valuedouble=(number),(object)->valueint=(int)(number),(object)->type=cJSON_Number)
#define cJSON_SetNumberValue(object, number) ((object)->valuedouble = (number), (object)->valueint = (int)(number))

#if defined(__GNUC__)
#define cJSON_ArrayForEach(element, array) for(element = (array)->child; element; element = element->next)
#endif

#ifdef __cplusplus
}
#endif

#endif
