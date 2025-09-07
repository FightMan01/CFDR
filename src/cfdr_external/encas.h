#ifndef ENCAS_H
#define ENCAS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <float.h>
//#define ENCAS_API extern "C"
//#define ENCAS_API static
#define ENCAS_API

#ifdef __unix__
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

//-----Types-----
#include <stdint.h>

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#ifndef __cplusplus
#include <stdbool.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#define PATH_MAX MAX_PATH
#else
#include <limits.h>
#endif

#ifdef _MSC_VER
#define force_inline __forceinline
#elif defined (__GNUC__)
#define force_inline inline __attribute__((always_inline))
#endif
//---------------

//-----Alloc-----
#ifdef ENCAS_CUSTOM_ALLOC
#define ENCAS_MALLOC(size) ENCAS_CUSTOM_ALLOC(size)
#define ENCAS_REALLOC(ptr, size) ENCAS_CUSTOM_REALLOC(ptr, size)
#define ENCAS_FREE(ptr) ENCAS_CUSTOM_FREE(ptr)
#else
#define ENCAS_MALLOC(size) malloc(size)
#define ENCAS_REALLOC(ptr, size) realloc(ptr, size)
#define ENCAS_FREE(ptr) free(ptr)
#endif
//---------------

typedef enum {
    ENCAS_LOG_LEVEL_INFO,
    ENCAS_LOG_LEVEL_WARNING,
    ENCAS_LOG_LEVEL_ERROR,
} Encas_Log_Level;

#define ENCAS_LOG_CALLBACK(proc_name) void proc_name(Encas_Log_Level level, const char *format, va_list args)
typedef ENCAS_LOG_CALLBACK(encas_log_callback);

#ifdef ENCAS_IMPLEMENTATION
encas_log_callback *global_logger = NULL;
#endif


#define LOAD_FACTOR 0.75f

typedef struct Encas_FaceKey {
    u32 v[3];
} Encas_FaceKey;

typedef struct Encas_FaceKeyMap {
    Encas_FaceKey *keys;
    u8 *values;
    u32 *global_indices;
    u32 cap;
    u32 len;
} Encas_FaceKeyMap;

#define ENCAS_TABLE_SIZE 1024

typedef struct Encas_HashEntry {
    s32 key;
    s32 value;
    struct Encas_HashEntry* next;
} Encas_HashEntry;

typedef struct {
    Encas_HashEntry** table;
} Encas_HashTable;

typedef struct Encas_File {
    u8 *buffer;
    u64 size; // Size of file
    u64 cur;  // Current position of cursor in the file
} Encas_File;

#define IS_ENCAS_EOF(f) (f->cur >= f->size)

// Encas String type
// -----------------
typedef struct Encas_Str {
    u8 *buffer;
    u32 len;
} Encas_Str;

// Mutable Encas String type
#define ENCAS_MUTABLE_STRING_SIZE 128
typedef struct Encas_MutStr {
    u8 buffer[ENCAS_MUTABLE_STRING_SIZE];
    u32 len;
} Encas_MutStr;

force_inline Encas_Str Encas_Str_from_zstr(char *txt, u32 len) { Encas_Str res = { (u8*)txt, len }; return res; }
#define Encas_Str_Lit(s) Encas_Str_from_zstr((char *)s, sizeof(s) - 1)

typedef enum {
    ENCAS_NOSECTION, // Not a section name
    ENCAS_FORMAT,
    ENCAS_GEOMETRY,
    ENCAS_VARIABLE,
    ENCAS_TIME,
    ENCAS_FILE,
    ENCAS_MATERIAL,
} Encas_SectionType;

typedef enum Encas_Elem_Type
{
    ENCAS_ELEM_POINT,
    ENCAS_ELEM_BAR2,
    ENCAS_ELEM_BAR3,
    ENCAS_ELEM_TRIA3,
    ENCAS_ELEM_TRIA6,
    ENCAS_ELEM_QUAD4,
    ENCAS_ELEM_QUAD8,
    ENCAS_ELEM_TETRA4,
    ENCAS_ELEM_TETRA10,
    ENCAS_ELEM_PYRAMID5,
    ENCAS_ELEM_PYRAMID13,
    ENCAS_ELEM_PENTA6,
    ENCAS_ELEM_PENTA15,
    ENCAS_ELEM_HEXA8,
    ENCAS_ELEM_HEXA20,
    ENCAS_ELEM_NSIDED,
    ENCAS_ELEM_NFACED,
    ENCAS_ELEM_UNKNOWN,
} Encas_Elem_Type;


// Encas String Array type
// -----------------
#define MAX_STRARRAY_ELEMS 16
typedef struct Encas_StrArray {
    Encas_Str elems[MAX_STRARRAY_ELEMS];
    u32 len;
    u32 cap; // For later usage
} Encas_StrArray;

typedef struct Encas_HashTableArray {
    Encas_HashTable **elems;
    u32 len;
    u32 cap;
} Encas_HashTableArray;

typedef struct Encas_MeshInfoPart {
    s32 *elem_sizes;
    u32 *elem_offsets;
    s32 len;
    u64 elem_vert_map_array_size;
    s32 num_of_coords;
} Encas_MeshInfoPart;

typedef struct Encas_MeshInfo {
    Encas_MeshInfoPart *parts;
    u32 len;
    Encas_HashTable *part_num_lookup;
} Encas_MeshInfo;

typedef struct Encas_MeshInfoArray {
    Encas_MeshInfo *elems;
    u32 len;
} Encas_MeshInfoArray;


typedef struct Encas_GeometryElem {
    Encas_MutStr filename;
    s32 ts, fs; // OPTIONAL
    bool change_coords_only; // OPTIONAL
    bool ts_set, fs_set, change_coords_only_set;
    Encas_MeshInfoArray mesh_info_array;
    u32 num_of_files;
} Encas_GeometryElem;

typedef struct Encas_Geometry {
    Encas_GeometryElem *model;
    Encas_GeometryElem *measured;
    Encas_GeometryElem *match;
    Encas_GeometryElem *boundary;
} Encas_Geometry;

typedef enum Encas_VariableType {
    ENCAS_VARIABLE_SCALAR_PER_NODE,
    ENCAS_VARIABLE_VECTOR_PER_NODE,
    ENCAS_VARIABLE_SCALAR_PER_ELEMENT,
    ENCAS_VARIABLE_VECTOR_PER_ELEMENT,
} Encas_VariableType;

// type of: [ts] [fs] description filename
// e.g.: scalar per node, vector per element... etc.
typedef struct Encas_DescFile {
    Encas_VariableType type;
    s32 ts, fs; // OPTIONAL
    bool ts_set, fs_set;
    Encas_MutStr description;
    Encas_MutStr filename;
} Encas_DescFile;

typedef struct Encas_VariableArray {
    Encas_DescFile **elems;
    u32 len;
    u32 cap;
} Encas_VariableArray;

typedef struct Encas_Time {
    // time set
    s32 time_set_number;
    Encas_MutStr time_set_description; // Empty string if not specified

    // number of steps
    s32 number_of_steps;

    // NOTE: I assume that only this format is given
    // If both filename_start_number and filename_increment are zero then only
    // time values given
    s32 filename_start_number;
    s32 filename_increment;

    // time values
    float *time_values;


} Encas_Time;

#define MAX_TIMEARRAY_ELEMS 16
typedef struct Encas_TimeArray {
    Encas_Time *elems[MAX_TIMEARRAY_ELEMS];
    u32 len;
} Encas_TimeArray;

typedef struct Encas_Case {
    Encas_Geometry      *geometry;
    Encas_VariableArray *variable;
    Encas_TimeArray     *times;
    char                 dirname[PATH_MAX + 1];
} Encas_Case;

typedef enum {
    ENCAS_MODE_GIVEN,
    ENCAS_MODE_IGNORE,
    ENCAS_MODE_OFF,
    ENCAS_MODE_ASSIGN,
    ENCAS_MODE_UNKNOWN,
} Encas_Mode;

typedef struct Encas_Vertex
{
    float x, y, z;
} Encas_Vertex;

typedef struct Encas_Vertices
{
    float *x;
    float *y;
    float *z;
} Encas_Vertices;

typedef struct Encas_Elem
{
    Encas_Elem_Type  type;
    u8               elem_size; // Size of one elem
    u32              elem_vert_map_size;    // num_of_elems * size_of_elem_type
    u32              elem_vert_map_entry;
} Encas_Elem;

typedef struct Encas_Mesh
{
    s32            part_number;
    Encas_Vertices vert_array;
    u64            vert_array_size;
    Encas_Elem     *elem_array;
    u64            elem_array_size;

    u32            *elem_vert_map_array;
} Encas_Mesh;

#define DEFAULT_MESHARRAY_CAP 16
typedef struct Encas_MeshArray {
    Encas_Mesh **elems;
    u32 len;
    u32 cap;
} Encas_MeshArray;

typedef struct Encas_FlatMesh {
    Encas_Vertex *vertices;
    u64 vertices_size;

    u64 *elem_vert_map;
    u64 elem_vert_map_size;

    u32 num_variables; // Number of variables
    float **data; // All variables converted to node values
    u32 *data_sizes; // Size of each variable data in bytes
} Encas_FlatMesh;

typedef struct Encas_ShellParams {
    float *vbo;
    u32 vbo_size;
    u32 *vbo_orig_idx;
    u32 *ebo;
    u32 ebo_size;

    u32 *tria_global_idx;

    u32 global_ebo_size;
} Encas_ShellParams;

//------------------------------------

static inline u32 _encas_hash(s32 key) {
    return key % ENCAS_TABLE_SIZE;
}


ENCAS_API void Encas_Init(encas_log_callback *logger);
ENCAS_API void Encas_Log(Encas_Log_Level level, const char *format, ...);
ENCAS_API void Encas_DeleteMeshInfo(Encas_MeshInfo *info);
ENCAS_API void Encas_CreateMeshInfoArray(Encas_MeshInfoArray *arr, u32 len);
ENCAS_API void Encas_DeleteMeshInfoArray(Encas_MeshInfoArray *arr);
ENCAS_API Encas_HashTable* Encas_CreateHashTable();
ENCAS_API void Encas_InsertHashTable(Encas_HashTable* hashTable, s32 key, s32 value);
ENCAS_API bool Encas_SearchHashTable(Encas_HashTable* hashTable, s32 key, s32 *value);
ENCAS_API void Encas_DeleteFromHashTable(Encas_HashTable* hashTable, s32 key);
ENCAS_API void Encas_DeleteHashTable(Encas_HashTable* hashTable);
ENCAS_API Encas_File *Encas_SlurpFile(char *filename);
ENCAS_API bool Encas_FileAdvace(Encas_File *f, u32 n);
ENCAS_API void Encas_FreeFile(Encas_File *file);
ENCAS_API bool Encas_Copy_Str_To_MutStr(Encas_Str str, Encas_MutStr *mutstr);
ENCAS_API Encas_Str Encas_ReadLine(Encas_File *f);
ENCAS_API Encas_Str Encas_ReadBinaryLine(Encas_File *f);
ENCAS_API s32 Encas_ReadS32(Encas_File *f);
ENCAS_API bool Encas_Str_Equals(Encas_Str lhs, Encas_Str rhs);
ENCAS_API bool Encas_Str_StartsWith(Encas_Str str, Encas_Str pattern);
ENCAS_API Encas_SectionType Encas_GetSectionType(Encas_Str str);
ENCAS_API s32 Encas_Str_FindChar(Encas_Str str, char c);
ENCAS_API void Encas_SplitKeyValue(Encas_Str str, Encas_Str *key, Encas_Str *value);
ENCAS_API s32 Encas_Str_to_S32(Encas_Str str);
ENCAS_API u32 Encas_Str_to_U32(Encas_Str str);
ENCAS_API float Encas_Str_to_F32(Encas_Str str);
ENCAS_API bool Encas_Str_IsNumber(Encas_Str str);
ENCAS_API Encas_StrArray *Encas_CreateStrArray();
ENCAS_API bool Encas_PushStrArray(Encas_StrArray *arr, Encas_Str str);
ENCAS_API void Encas_DeleteStrArray(Encas_StrArray *arr);
ENCAS_API Encas_StrArray *Encas_Str_Split(Encas_Str str);
ENCAS_API Encas_HashTableArray *Encas_CreateHashTableArray();
ENCAS_API bool Encas_PushHashTableArray(Encas_HashTableArray *arr, Encas_HashTable *elem);
ENCAS_API void Encas_DeleteHashTableArray(Encas_HashTableArray *arr);
ENCAS_API Encas_GeometryElem *Encas_CreateGeometryElem();
ENCAS_API Encas_Geometry *Encas_CreateGeometry();
ENCAS_API void Encas_DeleteGeometry(Encas_Geometry *geo);
ENCAS_API Encas_DescFile *Encas_CreateDescFile();
ENCAS_API void Encas_DeleteDescFile(Encas_DescFile *df);
ENCAS_API Encas_VariableArray *Encas_CreateVariableArray();
ENCAS_API bool Encas_PushVariableArray(Encas_VariableArray *arr, Encas_DescFile *elem);
ENCAS_API void Encas_DeleteVariableArray(Encas_VariableArray *arr);
ENCAS_API Encas_Time *Encas_CreateTime();
ENCAS_API void Encas_DeleteTime(Encas_Time *time);
ENCAS_API Encas_TimeArray *Encas_CreateTimeArray();
ENCAS_API bool Encas_PushTimeArray(Encas_TimeArray *arr, Encas_Time *time);
ENCAS_API void Encas_DeleteTimeArray(Encas_TimeArray *arr);
ENCAS_API Encas_Case *Encas_CreateCase();
ENCAS_API void Encas_DeleteCase(Encas_Case *encase);
ENCAS_API Encas_Elem_Type Encas_ReadElemType(Encas_Str str, bool *is_ghost);
ENCAS_API bool Encas_ParseMeshInfo(Encas_MeshInfo *info, char *filename);
ENCAS_API Encas_HashTable *Encas_ParseGeoFileLookup(char *filename);
ENCAS_API void Encas_Dirname(char *path, char *dest);
ENCAS_API Encas_Case *Encas_ReadCase(char *filename);
ENCAS_API Encas_Mesh *Encas_CreateMesh();
ENCAS_API void Encas_DeleteMesh(Encas_Mesh *mesh);
ENCAS_API Encas_MeshArray *Encas_CreateMeshArray();
ENCAS_API Encas_MeshArray *Encas_CreateMeshArrayWithCap(u32 cap);
ENCAS_API bool Encas_PushMeshArray(Encas_MeshArray *arr, Encas_Mesh *mesh);
ENCAS_API void Encas_DeleteMeshArray(Encas_MeshArray *arr);
ENCAS_API const char *Encas_ElemToCstr(Encas_Elem_Type elem);
ENCAS_API Encas_MeshArray *Encas_ReadGeometry(Encas_MeshInfo *mesh_info, char *filename);
ENCAS_API Encas_MeshArray *Encas_LoadGeometry(Encas_Case *encase, u32 time_value_idx);
ENCAS_API u32 Encas_GetCellTrianglesCount(Encas_Elem_Type cell_type);
ENCAS_API void Encas_TriangulateTria3s(u32 *elem_vert_map_array, u32 num_cells, u32 *faces, u64 *faces_offset, u64 vert_offset);
ENCAS_API void Encas_TriangulateTetra4s(u32 *elem_vert_map_array, u32 num_cells, u32 *faces, u64 *faces_offset, u64 vert_offset);
//ENCAS_API void Encas_LoadGeometryShell(Encas_Case *encase, Encas_MeshArray *mesh, float **vbo_out, u32 *vbo_size, u32 **vbo_orig_idx_out, u32 **ebo_out, u32 *ebo_size);
ENCAS_API void Encas_LoadGeometryShell(Encas_Case *encase, Encas_MeshArray *mesh, Encas_ShellParams *params);
ENCAS_API bool Encas_LoadVariableOnShell_Vertices(Encas_Case *encase, Encas_MeshArray *mesh, u32 variable_idx, u32 time_value_idx, Encas_ShellParams *params, float **var_vbo_out);
ENCAS_API bool Encas_LoadVariableOnShell_Elements(Encas_Case *encase, Encas_MeshArray *mesh, u32 variable_idx, u32 time_value_idx, Encas_ShellParams *params, float **var_vbo_out);
ENCAS_API void Encas_DeleteFloatArrParts(float **data, u32 num_of_parts);
ENCAS_API float **Encas_ReadVariableDataPerElement(Encas_Case *encase, Encas_MeshInfo *mesh_info, char *filename, u32 num_of_data);
ENCAS_API float *Encas_ReadVariableDataPerElementPart(Encas_Case *encase, Encas_MeshInfo *mesh_info, char *filename, u32 part_idx, u32 num_of_data);
ENCAS_API float **Encas_ReadVariableDataPerNode(Encas_Case *encase, Encas_MeshInfo *mesh_info, char *filename, u32 num_of_data);
ENCAS_API float *Encas_ReadVariableDataPerNodePart(Encas_Case *encase, Encas_MeshInfo *mesh_info, char *filename, u32 part_idx, u32 num_of_data);
ENCAS_API float *Encas_LoadVariableDataPart(Encas_Case *encase, u32 time_value_idx, u32 variable_idx, u32 part_idx);
ENCAS_API float **Encas_LoadVariableData(Encas_Case *encase, u32 time_value_idx, u32 variable_idx);
ENCAS_API void Encas_MeshArray_To_FlatMesh(Encas_Case *encas, Encas_MeshArray *mesh, Encas_FlatMesh *flat, u32 time_idx, u32 variable_idx);
ENCAS_API void Encas_DeleteFlatMesh(Encas_FlatMesh *flat);
ENCAS_API bool Encas_EqualFaceKey(const Encas_FaceKey *a, const Encas_FaceKey *b);
ENCAS_API void Encas_CreateFaceKeyMap(Encas_FaceKeyMap *map, u32 cap);
ENCAS_API void Encas_DeleteFaceKeyMap(Encas_FaceKeyMap *map);
ENCAS_API void Encas_RehashFaceKeyMap(Encas_FaceKeyMap *map, u32 new_cap);
ENCAS_API void Encas_SetFaceKeyMap(Encas_FaceKeyMap *map, Encas_FaceKey key, u8 value, u32 global_idx);
ENCAS_API bool Encas_GetFaceKeyMap(Encas_FaceKeyMap *map, Encas_FaceKey key, u8 *out_value);
ENCAS_API void Encas_RehashFaceKeyMap(Encas_FaceKeyMap *map, u32 new_cap);

#ifdef ENCAS_IMPLEMENTATION
ENCAS_API void Encas_Init(encas_log_callback *logger) {
    global_logger = logger;
}

ENCAS_API void Encas_Log(Encas_Log_Level level, const char *format, ...) {
    va_list args;
    va_start (args, format);

    if (global_logger) {
        global_logger(level, format, args);
    }
    // Default logger
    else {
        vprintf(format, args);
        printf("\n");
    }
    va_end(args);
}

ENCAS_API void Encas_DeleteMeshInfo(Encas_MeshInfo *info) {
    for (u32 i = 0; i < info->len; ++i) {
        ENCAS_FREE(info->parts[i].elem_sizes);
        ENCAS_FREE(info->parts[i].elem_offsets);
    }

    ENCAS_FREE(info->parts);
}

ENCAS_API void Encas_CreateMeshInfoArray(Encas_MeshInfoArray *arr, u32 len) {
    arr->len = len;
    arr->elems = (Encas_MeshInfo *)ENCAS_MALLOC(len * sizeof(Encas_MeshInfo));
}

ENCAS_API void Encas_DeleteMeshInfoArray(Encas_MeshInfoArray *arr) {
    for (u32 i = 0; i < arr->len; ++i)
        Encas_DeleteMeshInfo(&arr->elems[i]);

    ENCAS_FREE(arr->elems);
}

ENCAS_API Encas_HashTable* Encas_CreateHashTable() {
    Encas_HashTable* hashTable = (Encas_HashTable*)ENCAS_MALLOC(sizeof(Encas_HashTable));
    hashTable->table = (Encas_HashEntry**)ENCAS_MALLOC(ENCAS_TABLE_SIZE * sizeof(Encas_HashEntry*));
    if(hashTable->table == NULL) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Couln't allocate memory for hash table!\n");
        return NULL;
    }

    for (int i = 0; i < ENCAS_TABLE_SIZE; i++)
        hashTable->table[i] = NULL;

    return hashTable;
}

static Encas_HashEntry* _encas_create_entry(s32 key, s32 value) {
    Encas_HashEntry* newEntry = (Encas_HashEntry*)ENCAS_MALLOC(sizeof(Encas_HashEntry));
    newEntry->key = key;
    newEntry->value = value;
    newEntry->next = NULL;
    return newEntry;
}

ENCAS_API void Encas_InsertHashTable(Encas_HashTable* hashTable, s32 key, s32 value) {
    u32 slot = _encas_hash(key);
    Encas_HashEntry* entry = hashTable->table[slot];
    Encas_HashEntry* newEntry = _encas_create_entry(key, value);

    if (entry == NULL) {
        hashTable->table[slot] = newEntry;
    } else {
        Encas_HashEntry* prev = NULL;
        while (entry != NULL) {
            if (entry->key == key) {
                entry->value = value;
                ENCAS_FREE(newEntry);
                return;
            }
            prev = entry;
            entry = entry->next;
        }
        prev->next = newEntry;
    }
}

ENCAS_API bool Encas_SearchHashTable(Encas_HashTable* hashTable, s32 key, s32 *value) {
    u32 slot = _encas_hash(key);
    Encas_HashEntry* entry = hashTable->table[slot];

    while (entry != NULL) {
        if (entry->key == key) {
            *value = entry->value;
            return true;
        }
        entry = entry->next;
    }
    return false;
}

ENCAS_API void Encas_DeleteFromHashTable(Encas_HashTable* hashTable, s32 key) {
    u32 slot = _encas_hash(key);
    Encas_HashEntry* entry = hashTable->table[slot];
    Encas_HashEntry* prev = NULL;

    while (entry != NULL && entry->key != key) {
        prev = entry;
        entry = entry->next;
    }

    if (entry == NULL) {
        return;
    }

    if (prev == NULL) {
        hashTable->table[slot] = entry->next;
    } else {
        prev->next = entry->next;
    }

    ENCAS_FREE(entry);
}

ENCAS_API void Encas_DeleteHashTable(Encas_HashTable* hashTable) {
    if (hashTable == NULL)
        return;

    for (u32 i = 0; i < ENCAS_TABLE_SIZE; i++) {
        Encas_HashEntry* entry = hashTable->table[i];
        while (entry != NULL) {
            Encas_HashEntry* temp = entry;
            entry = entry->next;
            ENCAS_FREE(temp);
        }
    }

    ENCAS_FREE(hashTable->table);
    ENCAS_FREE(hashTable);
}

static bool check_if_file_exists(char *filename) {
#ifdef __unix__
    return (access(filename, F_OK) == 0);
#else
    FILE *fp = fopen(filename, "r");
    bool is_exist = false;
    if (fp != NULL)
    {
        is_exist = true;
        fclose(fp); // close the file
    }
    return is_exist;
#endif
}

ENCAS_API Encas_File *Encas_SlurpFile(char *filename) {
    Encas_File *file = (Encas_File *)ENCAS_MALLOC(sizeof(Encas_File));

#ifdef __unix__
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "open");
        ENCAS_FREE(file);
        return NULL;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "fstat");
        close(fd);
        ENCAS_FREE(file);
        return NULL;
    }

    file->size = sb.st_size;

    file->buffer = (u8 *)mmap(NULL, file->size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file->buffer == MAP_FAILED) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "mmap");
        close(fd);
        ENCAS_FREE(file);
        return NULL;
    }

    close(fd);

#else
    FILE *f = fopen(filename, "rb");
    if (!f) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "fopen");
        ENCAS_FREE(file);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    file->size = ftell(f);
    rewind(f);

    file->buffer = ENCAS_MALLOC(file->size);
    if (!file->buffer) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "malloc");
        fclose(f);
        ENCAS_FREE(file);
        return NULL;
    }

    size_t bytes_read = fread(file->buffer, 1, file->size, f);
    if (bytes_read != file->size) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "fread");
        ENCAS_FREE(file->buffer);
        fclose(f);
        ENCAS_FREE(file);
        return NULL;
    }

    fclose(f);
#endif

    file->cur = 0;

    return file;
}

ENCAS_API bool Encas_FileAdvace(Encas_File *f, u32 n) {
    if (f->cur + n > f->size) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Cannot advance file by %d bytes!\n", n);
        return false;
    }

    f->cur += n;
    return true;
}

ENCAS_API void Encas_FreeFile(Encas_File *file) {
#ifdef __unix__
    munmap(file->buffer, file->size);
#endif
#ifdef _WIN32
    ENCAS_FREE(file->buffer);
#endif

    ENCAS_FREE(file);
}

ENCAS_API bool Encas_Copy_Str_To_MutStr(Encas_Str str, Encas_MutStr *mutstr) {
    if (mutstr == NULL) {
        return false;
    }

    if (str.len >= ENCAS_MUTABLE_STRING_SIZE) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "String too long to copy to MutStr (len=%u, max=%u)",
                  str.len, ENCAS_MUTABLE_STRING_SIZE-1);
        return false;
    }

    mutstr->len = str.len;
    memcpy(mutstr->buffer, str.buffer, str.len);
    // Add null termination for safety when using C string functions
    if (str.len < ENCAS_MUTABLE_STRING_SIZE) {
        mutstr->buffer[str.len] = '\0';
    }
    return true;
}

// Reads a line of a file
// Encas_Str is ummutable!!!
ENCAS_API Encas_Str Encas_ReadLine(Encas_File *f) {
    Encas_Str str;

    if (f->cur >= f->size) {
        str.buffer = NULL;
        return str;
    }

    str.buffer = f->buffer + f->cur;

    u32 len = 0;
    s32 comment_len = -1;

    // cur = character | '\r' | '\n' | "\r\n" | '#'
    while (f->cur < f->size) {
        u8 ch = f->buffer[f->cur];

        if (ch == '\r') {
            if (comment_len != -1)
                str.len = comment_len;
            else
                str.len = len;

            if (f->cur + 1 < f->size && f->buffer[f->cur + 1] == '\n')
                f->cur += 2;
            else
                f->cur++;

            return str;
        }

        if (ch == '\n') {
            if (comment_len != -1)
                str.len = comment_len;
            else
                str.len = len;

            f->cur++;
            return str;
        }

        if (ch == '#')
            comment_len = len;

        len++;
        f->cur++;
    }

    // Error
    str.buffer = NULL;
    return str;
}

ENCAS_API Encas_Str Encas_ReadBinaryLine(Encas_File *f) {
    Encas_Str str;

    if (f->cur + 80 > f->size) {
        str.buffer = NULL;
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Cannot read binary line!\n");
        return str;
    }

    str.buffer = f->buffer + f->cur;
    str.len = 80;
    f->cur += 80;

    return str;
}

ENCAS_API s32 Encas_ReadS32(Encas_File *f) {
    if (f->cur + sizeof(s32) > f->size) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Cannot read S32!\n");
        return 0;
    }

    s32 ret = *(s32*)(f->buffer + f->cur);
    f->cur += sizeof(s32);
    return ret;
}

ENCAS_API bool Encas_Str_Equals(Encas_Str lhs, Encas_Str rhs) {
    if (lhs.len != rhs.len)
        return false;

    for (u32 i = 0; i < lhs.len; ++i)
        if (lhs.buffer[i] != rhs.buffer[i])
            return false;

    return true;
}

ENCAS_API bool Encas_Str_StartsWith(Encas_Str str, Encas_Str pattern) {
    if (str.len < pattern.len)
        return false;

    for (u32 i = 0; i < pattern.len; ++i)
        if (str.buffer[i] != pattern.buffer[i])
            return false;

    return true;
}

// TODO: optimize this function
ENCAS_API Encas_SectionType Encas_GetSectionType(Encas_Str str) {
    if (Encas_Str_StartsWith(str, Encas_Str_Lit("FORMAT")))
        return ENCAS_FORMAT;
    if (Encas_Str_StartsWith(str, Encas_Str_Lit("GEOMETRY")))
        return ENCAS_GEOMETRY;
    if (Encas_Str_StartsWith(str, Encas_Str_Lit("VARIABLE")))
        return ENCAS_VARIABLE;
    if (Encas_Str_StartsWith(str, Encas_Str_Lit("TIME")))
        return ENCAS_TIME;
    if (Encas_Str_StartsWith(str, Encas_Str_Lit("FILE")))
        return ENCAS_FILE;
    if (Encas_Str_StartsWith(str, Encas_Str_Lit("MATERIAL")))
        return ENCAS_MATERIAL;

    return ENCAS_NOSECTION;
}

// Finds a char in an Encas_Str then returns the first occurrence index
// or -1 if string not contains that char
ENCAS_API s32 Encas_Str_FindChar(Encas_Str str, char c) {
    for (u32 i = 0; i < str.len; ++i)
        if (str.buffer[i] == c)
            return i;

    return -1;
}

inline static s32 Encas_MutStr_FindChar(Encas_MutStr *mutstr, char c) {
    Encas_Str str = { .buffer = mutstr->buffer, .len = mutstr->len };
    return Encas_Str_FindChar(str, c);
}

ENCAS_API void Encas_SplitKeyValue(Encas_Str str, Encas_Str *key, Encas_Str *value) {
    if (str.len <= 1) {
        key->buffer   = NULL;
        value->buffer = NULL;
        return;
    }

    key->buffer = str.buffer;
    key->len = 0;

    for (u32 i = 0; i < str.len; ++i) {
        if (str.buffer[i] == ':') {
            key->len = i;
            value->buffer = str.buffer + i + 1;
            value->len = str.len - i - 1;
            if (key->len == 0) {
                key->buffer   = NULL;
                value->buffer = NULL;
            }
            return;
        }
    }

    // If no ':' found
    key->buffer   = NULL;
    value->buffer = NULL;
}

ENCAS_API s32 Encas_Str_to_S32(Encas_Str str) {
    s32 n=0, sign=1;
    u32 len = str.len;

    if (len > 0)
        switch(str.buffer[0]) {
            case '-':   sign=-1;
            case '+':   --len, ++str.buffer;
        }

    while (len-- && isdigit(*str.buffer))
        n = n*10 + *str.buffer++ - '0';

    return n*sign;
}

ENCAS_API u32 Encas_Str_to_U32(Encas_Str str) {
    u32 n=0;
    u32 len = str.len;

    while (len-- && isdigit(*str.buffer))
        n = n*10 + *str.buffer++ - '0';

    return n;
}

ENCAS_API float Encas_Str_to_F32(Encas_Str str) {
    if (!str.buffer || str.len == 0)
        return 0.0f;

    double result = 0.0;
    double fraction = 0.0;
    int exponent = 0;
    int sign = 1;
    double frac_divisor = 1;
    int i = 0;

    if (i < str.len && (str.buffer[i] == '-' || str.buffer[i] == '+')) {
        if (str.buffer[i] == '-') {
            sign = -1;
        }
        i++;
    }

    while (i < str.len && isdigit((u8)str.buffer[i])) {
        result = result * 10.0 + (str.buffer[i] - '0');
        i++;
    }

    if (i < str.len && str.buffer[i] == '.') {
        i++;
        while (i < str.len && isdigit(str.buffer[i])) {
            fraction = fraction * 10.0 + (str.buffer[i] - '0');
            frac_divisor *= 10.0;
            i++;
        }
    }

    result += fraction / frac_divisor;

    if (i < str.len && (str.buffer[i] == 'e' || str.buffer[i] == 'E')) {
        i++;
        int exp_sign = 1;
        int exp_value = 0;

        if (i < str.len && (str.buffer[i] == '-' || str.buffer[i] == '+')) {
            if (str.buffer[i] == '-') {
                exp_sign = -1;
            }
            i++;
        }

        while (i < str.len && isdigit((u8)str.buffer[i])) {
            exp_value = exp_value * 10 + (str.buffer[i] - '0');
            i++;
        }

        exponent = exp_sign * exp_value;
    }

#if 0
    while (exponent > 0) {
        result *= 10.0f;
        exponent--;
    }

    while (exponent < 0) {
        result /= 10.0f;
        exponent++;
    }
#endif
    result *= pow(10.0f, exponent);

    return (float)(sign * result);
}

ENCAS_API bool Encas_Str_IsNumber(Encas_Str str) {
    for (u32 i = 0; i < str.len; ++i)
        if (!(str.buffer[i] >= '0' && str.buffer[i] <= '9'))
            return false;
    return true;
}

ENCAS_API Encas_StrArray *Encas_CreateStrArray() {
    Encas_StrArray *arr = (Encas_StrArray *)ENCAS_MALLOC(sizeof(Encas_StrArray));
    arr->len = 0;
    arr->cap = MAX_STRARRAY_ELEMS;
    return arr;
}

ENCAS_API bool Encas_PushStrArray(Encas_StrArray *arr, Encas_Str str) {
    if (arr->len >= arr->cap) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Cannot push to StrArray!");
        return false;
    }

    arr->elems[arr->len++] = str;

    return true;
}

ENCAS_API void Encas_DeleteStrArray(Encas_StrArray *arr) {
    ENCAS_FREE(arr);
}

ENCAS_API Encas_StrArray *Encas_Str_Split(Encas_Str str) {
    Encas_StrArray *arr = Encas_CreateStrArray();

    Encas_Str tmp = {NULL, 0};

    bool new_word = true;
    for (u32 i = 0; i < str.len; ++i) {
        u8 ch = str.buffer[i];

        if (isspace(ch)) {
            if (!new_word) {
                Encas_PushStrArray(arr, tmp);
                tmp.len = 0;
            }
            new_word = true;
            continue;
        }

        if (new_word) {
            tmp.buffer = str.buffer + i;
            tmp.len++;
            new_word = false;
        } else {
            tmp.len++;
        }
    }

    if (!new_word)
        Encas_PushStrArray(arr, tmp);

    return arr;
}

ENCAS_API Encas_HashTableArray *Encas_CreateHashTableArray() {
    Encas_HashTableArray *arr = (Encas_HashTableArray *)ENCAS_MALLOC(sizeof(Encas_HashTableArray));
    arr->len = 0;
    arr->cap = 16;
    arr->elems = (Encas_HashTable **)ENCAS_MALLOC(arr->cap * sizeof(Encas_HashTable *));
    if (arr->elems == NULL) {
        ENCAS_FREE(arr);
        return NULL;
    }
    memset(arr->elems, 0, arr->cap * sizeof(Encas_HashTable *));
    return arr;
}

ENCAS_API bool Encas_PushHashTableArray(Encas_HashTableArray *arr, Encas_HashTable *elem) {
    if (arr->len + 1 > arr->cap) {
        arr->cap *= 2;
        arr->elems = (Encas_HashTable **)ENCAS_REALLOC(arr->elems, arr->cap * sizeof(Encas_HashTable *));

        if (arr->elems == NULL) {
            Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Cannot allocate memory for HashTableArray push!\n");
            return false;
        }
    }

    arr->elems[arr->len++] = elem;
    return true;
}

ENCAS_API void Encas_DeleteHashTableArray(Encas_HashTableArray *arr) {
    if (arr == NULL)
        return;

    for (u32 i = 0; i < arr->len; ++i)
        Encas_DeleteHashTable(arr->elems[i]);

    ENCAS_FREE(arr->elems);
    ENCAS_FREE(arr);
}

ENCAS_API Encas_GeometryElem *Encas_CreateGeometryElem() {
    Encas_GeometryElem *gelem = (Encas_GeometryElem *)ENCAS_MALLOC(sizeof(Encas_GeometryElem));
    memset(gelem, 0, sizeof(Encas_GeometryElem));
    return gelem;
}

static inline void Encas_DeleteGeometryElem(Encas_GeometryElem *gelem) {
    if (gelem == NULL)
        return;

    Encas_DeleteMeshInfoArray(&gelem->mesh_info_array);
    ENCAS_FREE(gelem);
}

ENCAS_API Encas_Geometry *Encas_CreateGeometry() {
    Encas_Geometry *geo = (Encas_Geometry *)ENCAS_MALLOC(sizeof(Encas_Geometry));
    memset(geo, 0, sizeof(Encas_Geometry));
    return geo;
}

ENCAS_API void Encas_DeleteGeometry(Encas_Geometry *geo) {
    if (geo == NULL)
        return;

    Encas_DeleteGeometryElem(geo->model);
    Encas_DeleteGeometryElem(geo->measured);
    Encas_DeleteGeometryElem(geo->match);
    Encas_DeleteGeometryElem(geo->boundary);

    ENCAS_FREE(geo);
}

ENCAS_API Encas_DescFile *Encas_CreateDescFile() {
    Encas_DescFile *df = (Encas_DescFile *)ENCAS_MALLOC(sizeof(Encas_DescFile));
    memset(df, 0, sizeof(Encas_DescFile));

    return df;
}

ENCAS_API void Encas_DeleteDescFile(Encas_DescFile *df) {
    ENCAS_FREE(df);
}

ENCAS_API Encas_VariableArray *Encas_CreateVariableArray() {
    Encas_VariableArray *arr = (Encas_VariableArray *)ENCAS_MALLOC(sizeof(Encas_VariableArray));
    arr->cap = 8;
    arr->len = 0;
    arr->elems = (Encas_DescFile **)ENCAS_MALLOC(arr->cap * sizeof(Encas_DescFile *));
    return arr;
}

ENCAS_API bool Encas_PushVariableArray(Encas_VariableArray *arr, Encas_DescFile *elem) {
    if (arr->len + 1 > arr->cap) {
        arr->cap *= 2;
        arr->elems = (Encas_DescFile **)ENCAS_REALLOC(arr->elems, arr->cap * sizeof(Encas_DescFile *));

        if (arr->elems == NULL) {
            Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Cannot allocate memory for VariableArray push!\n");
            return false;
        }
    }

    arr->elems[arr->len++] = elem;
    return true;
}

ENCAS_API void Encas_DeleteVariableArray(Encas_VariableArray *arr) {
    if (arr == NULL)
        return;

    for (u32 i = 0; i < arr->len; ++i)
        Encas_DeleteDescFile(arr->elems[i]);

    ENCAS_FREE(arr->elems);
    ENCAS_FREE(arr);
}

ENCAS_API Encas_Time *Encas_CreateTime() {
    Encas_Time *time = (Encas_Time *)ENCAS_MALLOC(sizeof(Encas_Time));
    memset(time, 0, sizeof(Encas_Time));
    return time;
}

ENCAS_API void Encas_DeleteTime(Encas_Time *time) {
    if (time == NULL)
        return;

    ENCAS_FREE(time->time_values);
    ENCAS_FREE(time);
}


ENCAS_API Encas_TimeArray *Encas_CreateTimeArray() {
    Encas_TimeArray *arr = (Encas_TimeArray *)ENCAS_MALLOC(sizeof(Encas_TimeArray));
    arr->len = 0;
    return arr;
}

ENCAS_API bool Encas_PushTimeArray(Encas_TimeArray *arr, Encas_Time *time) {
    if (arr->len >= MAX_TIMEARRAY_ELEMS) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Cannot push to TimeArray!");
        return false;
    }

    arr->elems[arr->len++] = time;

    return true;
}

ENCAS_API void Encas_DeleteTimeArray(Encas_TimeArray *arr) {
    for (u32 i = 0; i < arr->len; ++i)
        Encas_DeleteTime(arr->elems[i]);

    ENCAS_FREE(arr);
}

ENCAS_API Encas_Case *Encas_CreateCase() {
    Encas_Case *encase = (Encas_Case *)ENCAS_MALLOC(sizeof(Encas_Case));
    memset(encase, 0, sizeof(Encas_Case));
    return encase;
}

ENCAS_API void Encas_DeleteCase(Encas_Case *encase) {
    Encas_DeleteGeometry(encase->geometry);
    Encas_DeleteVariableArray(encase->variable);
    Encas_DeleteTimeArray(encase->times);
    ENCAS_FREE(encase);
}

static inline void _free_case_and_file(Encas_Case *encase, Encas_File *f) {
    Encas_DeleteCase(encase);
    Encas_FreeFile(f);
}

static Encas_Mode _get_mode(Encas_Str str) {
    if (Encas_Str_StartsWith(str, Encas_Str_Lit("off")))
        return ENCAS_MODE_OFF;
    else if (Encas_Str_StartsWith(str, Encas_Str_Lit("given")))
        return ENCAS_MODE_GIVEN;
    else if (Encas_Str_StartsWith(str, Encas_Str_Lit("assign")))
        return ENCAS_MODE_ASSIGN;
    else if (Encas_Str_StartsWith(str, Encas_Str_Lit("ignore")))
        return ENCAS_MODE_IGNORE;
    else
        return ENCAS_MODE_UNKNOWN;
}

static Encas_Elem_Type _get_elem_type(Encas_Str str) {
    Encas_Str tmp;
    tmp.buffer = str.buffer;
    tmp.len = str.len;

    switch (str.buffer[0]) {
        case 'b':
            if (str.buffer[1] == 'a' && str.buffer[2] == 'r') {
                // bar2
                if (str.buffer[3] == '2')
                    return ENCAS_ELEM_BAR2;
                // bar3
                if (str.buffer[3] == '3')
                    return ENCAS_ELEM_BAR3;
            }
            break;
        case 'h':
            if (str.buffer[1] == 'e' && str.buffer[2] == 'x' && str.buffer[3] == 'a') {
                // hexa8
                if (str.buffer[4] == '8')
                    return ENCAS_ELEM_HEXA8;
                // hexa20
                if (str.buffer[4] == '2' && str.buffer[5] == '0')
                    return ENCAS_ELEM_HEXA20;
            }
            break;
        case 'n':
            --tmp.len;
            ++tmp.buffer;

            // nfaced
            if (Encas_Str_StartsWith(tmp, Encas_Str_Lit("faced")))
                return ENCAS_ELEM_NFACED;
            // nsided
            if (Encas_Str_StartsWith(tmp, Encas_Str_Lit("sided")))
                return ENCAS_ELEM_NSIDED;
            break;
        case 'p':
            // penta/point/pyramid
            switch (str.buffer[1]) {
                // penta
                case 'e':
                    if (str.buffer[2] == 'n' && str.buffer[3] == 't' && str.buffer[4] == 'a') {
                        // penta6
                        if (str.buffer[5] == '6')
                            return ENCAS_ELEM_PENTA6;
                        // penta15
                        if (str.buffer[5] == '1' && str.buffer[6] == '5')
                            return ENCAS_ELEM_PENTA15;
                    }
                    break;
                case 'o':
                    tmp.len    -= 2;
                    tmp.buffer += 2;

                    // point
                    if (Encas_Str_StartsWith(tmp, Encas_Str_Lit("int")))
                        return ENCAS_ELEM_POINT;
                    break;
                case 'y':
                    tmp.len = 5;
                    tmp.buffer += 2;

                    if (Encas_Str_StartsWith(tmp, Encas_Str_Lit("ramid"))) {
                        if (str.buffer[7] == '5')
                            return ENCAS_ELEM_PYRAMID5;
                        if (str.buffer[7] == '1' && str.buffer[8] == '3')
                            return ENCAS_ELEM_PYRAMID13;
                    }
                    break;
                default:
                    return ENCAS_ELEM_UNKNOWN; 
            }
            break;
        case 'q':
            if (str.buffer[1] == 'u' && str.buffer[2] == 'a' && str.buffer[3] == 'd') {
                if (str.buffer[4] == '4')
                    return ENCAS_ELEM_QUAD4;
                if (str.buffer[4] == '8')
                    return ENCAS_ELEM_QUAD8;
            }
            break;
        case 't':
            // tetra
            if (str.buffer[1] == 'e' && str.buffer[2] == 't' && str.buffer[3] == 'r' && str.buffer[4] == 'a') {
                if (str.buffer[5] == '4')
                    return ENCAS_ELEM_TETRA4;
                if (str.buffer[5] == '1' && str.buffer[6] == '0')
                    return ENCAS_ELEM_TETRA10;
            }
            // tria
            else if (str.buffer[1] == 'r' && str.buffer[2] == 'i' && str.buffer[3] == 'a') {
                if (str.buffer[4] == '3')
                    return ENCAS_ELEM_TRIA3;
                if (str.buffer[4] == '6')
                    return ENCAS_ELEM_TRIA6;
            }
            break;
        default:
            return ENCAS_ELEM_UNKNOWN;
    }

    return ENCAS_ELEM_UNKNOWN;
}

ENCAS_API Encas_Elem_Type Encas_ReadElemType(Encas_Str str, bool *is_ghost) {
    if (str.buffer[0] == 'g' && str.buffer[1] == '_') {
        str.buffer += 2;
        str.len -= 2;
        Encas_Elem_Type type = _get_elem_type(str);
        *is_ghost = (type != ENCAS_ELEM_UNKNOWN);

        return type;
    }

    *is_ghost = false;
    return _get_elem_type(str);
}

// Returns the number of vertices that a type defines
static u32 _get_elem_vert_count(Encas_Elem_Type type) {
    switch (type) {
        case ENCAS_ELEM_POINT:
            return 1;
        case ENCAS_ELEM_BAR2:
            return 2;
        case ENCAS_ELEM_BAR3:
            return 3;
        case ENCAS_ELEM_TRIA3:
            return 3;
        case ENCAS_ELEM_TRIA6:
            return 6;
        case ENCAS_ELEM_QUAD4:
            return 4;
        case ENCAS_ELEM_QUAD8:
            return 8;
        case ENCAS_ELEM_TETRA4:
            return 4;
        case ENCAS_ELEM_TETRA10:
            return 10;
        case ENCAS_ELEM_PYRAMID5:
            return 5;
        case ENCAS_ELEM_PYRAMID13:
            return 13;
        case ENCAS_ELEM_PENTA6:
            return 6;
        case ENCAS_ELEM_PENTA15:
            return 15;
        case ENCAS_ELEM_HEXA8:
            return 8;
        case ENCAS_ELEM_HEXA20:
            return 20;
        // TODO: implement
        case ENCAS_ELEM_NSIDED:
        case ENCAS_ELEM_NFACED:
            return 0;
        default:
            return 0;
    }
}

ENCAS_API bool Encas_ParseMeshInfo(Encas_MeshInfo *info, char *filename) {
    Encas_Log(ENCAS_LOG_LEVEL_INFO, "Loading %s geometry file\n", filename);
    if (!check_if_file_exists(filename)) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Cannot open %s geometry file\n", filename);
        return false;
    }

    Encas_File *f = Encas_SlurpFile(filename);
    Encas_Mode node_id, element_id;

    Encas_Str line = Encas_ReadBinaryLine(f);

    if (!Encas_Str_StartsWith(line, Encas_Str_Lit("C Binary"))) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'%s' is not in C Binary form!\n", filename);
        Encas_FreeFile(f);
        return false;
    }

    // Skip the two description line
    if (!Encas_FileAdvace(f, 160)) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'%s' file is not contains description!\n", filename);
        Encas_FreeFile(f);
        return false;
    }

    line = Encas_ReadBinaryLine(f);
    // node id
    if (!Encas_Str_StartsWith(line, Encas_Str_Lit("node id "))) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'%s' file is not contains node id!\n", filename);
        Encas_FreeFile(f);
        return false;
    }

    // Substring in a hacky way :)
    line.buffer += 8;
    line.len -= 8;

    if ((node_id = _get_mode(line)) == ENCAS_MODE_UNKNOWN) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'%s' file: Unkown mode found! (not in: <off/given/assign/ignore>) at node id\n", filename);
        Encas_FreeFile(f);
        return false;
    }

    // element id
    line = Encas_ReadBinaryLine(f);
    if (!Encas_Str_StartsWith(line, Encas_Str_Lit("element id "))) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'%s' file is not contains element id!\n", filename);
        Encas_FreeFile(f);
        return false;
    }

    // Substring in a hacky way :)
    line.buffer += 11;
    line.len -= 11;

    if ((element_id = _get_mode(line)) == ENCAS_MODE_UNKNOWN) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'%s' file: Unkown mode found! (not in: <off/given/assign/ignore>) at element id\n", filename);
        Encas_FreeFile(f);
        return false;
    }

    // extents?
    line = Encas_ReadBinaryLine(f);
    if (Encas_Str_StartsWith(line, Encas_Str_Lit("extents"))) {
        // Skips extents for now
        // maybe implement later
        Encas_FileAdvace(f, 6 * sizeof(float));
        line = Encas_ReadBinaryLine(f);
    }

    u64 before_parts = f->cur;
    Encas_Str before_parts_line = { .buffer = line.buffer, .len = line.len }; 
    u64 num_of_parts = 0;

    // Count how many parts are in the file
    while (Encas_Str_StartsWith(line, Encas_Str_Lit("part"))) {
        num_of_parts++;
        Encas_ReadS32(f);
        Encas_FileAdvace(f, 80);

        while (!IS_ENCAS_EOF(f)) {
            line = Encas_ReadBinaryLine(f);
            bool is_ghost = false;
            Encas_Elem_Type elem_type;

            if (Encas_Str_StartsWith(line, Encas_Str_Lit("coordinates"))) {
                s32 num_of_nodes = Encas_ReadS32(f);

                // Skip node ids
                if (node_id == ENCAS_MODE_GIVEN || node_id == ENCAS_MODE_IGNORE)
                    Encas_FileAdvace(f, num_of_nodes * sizeof(s32));

                Encas_FileAdvace(f, 3 * num_of_nodes * sizeof(float));

            }

            else if (Encas_Str_StartsWith(line, Encas_Str_Lit("block"))) {
                Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Structured data is not implemented yet! (block keyword)\n");
                Encas_FreeFile(f);
                return false;
            }

            // Element type
            else if ((elem_type = Encas_ReadElemType(line, &is_ghost)) != ENCAS_ELEM_UNKNOWN) {
                s32 num_of_elements = Encas_ReadS32(f);

                // Skip element ids
                if (element_id == ENCAS_MODE_GIVEN || element_id == ENCAS_MODE_IGNORE)
                    Encas_FileAdvace(f, num_of_elements * sizeof(s32));

                u32 elem_vert_count = _get_elem_vert_count(elem_type);
                Encas_FileAdvace(f, num_of_elements * elem_vert_count * sizeof(s32));
            }

            else {
                break;
            }
        }
    }

    if (!num_of_parts) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "there are no parts in '%s' file!", filename);
        Encas_FreeFile(f);
        return false;
    }

    f->cur = before_parts;

    line.buffer = before_parts_line.buffer;
    line.len = before_parts_line.len;

    info->parts = (Encas_MeshInfoPart *)ENCAS_MALLOC(num_of_parts * sizeof(Encas_MeshInfoPart));
    info->len = num_of_parts;
    info->part_num_lookup = Encas_CreateHashTable();

    u32 part_idx = 0;
    // Parts
    while (Encas_Str_StartsWith(line, Encas_Str_Lit("part"))) {
        s32 part_number = Encas_ReadS32(f);

        // Skip description line
        Encas_FileAdvace(f, 80);

        u64 begin = f->cur;
        u32 num_of_elemtypes = 0;

        // First only count the size of the memory to be allocated
        while (!IS_ENCAS_EOF(f)) {
            line = Encas_ReadBinaryLine(f);
            bool is_ghost = false;
            Encas_Elem_Type elem_type;

            if (Encas_Str_StartsWith(line, Encas_Str_Lit("coordinates"))) {
                s32 num_of_nodes = Encas_ReadS32(f);

                // Skip node ids
                if (node_id == ENCAS_MODE_GIVEN || node_id == ENCAS_MODE_IGNORE)
                    Encas_FileAdvace(f, num_of_nodes * sizeof(s32));

                Encas_FileAdvace(f, 3 * num_of_nodes * sizeof(float));

            }

            // Element type
            else if ((elem_type = Encas_ReadElemType(line, &is_ghost)) != ENCAS_ELEM_UNKNOWN) {
                s32 num_of_elements = Encas_ReadS32(f);

                // Skip element ids
                if (element_id == ENCAS_MODE_GIVEN || element_id == ENCAS_MODE_IGNORE)
                    Encas_FileAdvace(f, num_of_elements * sizeof(s32));


                num_of_elemtypes++;
                u32 elem_vert_count = _get_elem_vert_count(elem_type);
                Encas_FileAdvace(f, num_of_elements * elem_vert_count * sizeof(s32));
            }

            else {
                break;
            }
        }

        f->cur = begin;

        info->parts[part_idx].len = num_of_elemtypes;
        info->parts[part_idx].elem_sizes = (s32 *)ENCAS_MALLOC(num_of_elemtypes * sizeof(s32));
        info->parts[part_idx].elem_offsets = (u32 *)ENCAS_MALLOC(num_of_elemtypes * sizeof(u32));
        if (info->parts[part_idx].elem_sizes == NULL) {
            Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Couldn't allocate memory!");
            Encas_FreeFile(f);
            return false;
        }
        info->parts[part_idx].elem_vert_map_array_size = 0;

        // Then store the data

        u32 elem_idx = 0;

        while (!IS_ENCAS_EOF(f)) {
            line = Encas_ReadBinaryLine(f);
            bool is_ghost = false;
            Encas_Elem_Type elem_type;

            if (Encas_Str_StartsWith(line, Encas_Str_Lit("coordinates"))) {
                s32 num_of_nodes = Encas_ReadS32(f);

                // Skip node ids
                if (node_id == ENCAS_MODE_GIVEN || node_id == ENCAS_MODE_IGNORE)
                    Encas_FileAdvace(f, num_of_nodes * sizeof(s32));

                info->parts[part_idx].num_of_coords = num_of_nodes;
                Encas_FileAdvace(f, 3 * num_of_nodes * sizeof(float));

            }

            // Element type
            else if ((elem_type = Encas_ReadElemType(line, &is_ghost)) != ENCAS_ELEM_UNKNOWN) {
                s32 num_of_elements = Encas_ReadS32(f);

                // Skip element ids
                if (element_id == ENCAS_MODE_GIVEN || element_id == ENCAS_MODE_IGNORE)
                    Encas_FileAdvace(f, num_of_elements * sizeof(s32));

                u32 elem_vert_count = _get_elem_vert_count(elem_type);

                // ghost elems
                if (!is_ghost) {
                    info->parts[part_idx].elem_vert_map_array_size += num_of_elements * elem_vert_count;

                }

                info->parts[part_idx].elem_sizes[elem_idx] = num_of_elements;
                info->parts[part_idx].elem_offsets[elem_idx] = (elem_idx == 0) ? 0 : info->parts[part_idx].elem_offsets[elem_idx - 1] + num_of_elements;
                Encas_FileAdvace(f, num_of_elements * elem_vert_count * sizeof(s32));
            }

            else {
                break;
            }
        }

        // Part end
        Encas_InsertHashTable(info->part_num_lookup, part_number, part_idx);
        part_idx++;
    }

    Encas_FreeFile(f);
    return true;

    Encas_FreeFile(f);
    Encas_DeleteMeshInfo(info);
    return false;
}

// Reads a geometry file then parse the part numbers into a hashtable
ENCAS_API Encas_HashTable *Encas_ParseGeoFileLookup(char *filename) {
    Encas_File *f = Encas_SlurpFile(filename);
    Encas_Mode node_id, element_id;

    Encas_Str line = Encas_ReadBinaryLine(f);

    if (!Encas_Str_StartsWith(line, Encas_Str_Lit("C Binary"))) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'%s' is not in C Binary form!\n", filename);
        Encas_FreeFile(f);
        return NULL;
    }

    // Skip the two description line
    if (!Encas_FileAdvace(f, 160)) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'%s' file is not contains description!\n", filename);
        Encas_FreeFile(f);
        return NULL;
    }

    line = Encas_ReadBinaryLine(f);
    // node id
    if (!Encas_Str_StartsWith(line, Encas_Str_Lit("node id "))) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'%s' file is not contains node id!\n", filename);
        Encas_FreeFile(f);
        return NULL;
    }

    // Substring in a hacky way :)
    line.buffer += 8;
    line.len -= 8;

    if ((node_id = _get_mode(line)) == ENCAS_MODE_UNKNOWN) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'%s' file: Unkown mode found! (not in: <off/given/assign/ignore>) at node id\n", filename);
        Encas_FreeFile(f);
        return NULL;
    }

    // element id
    line = Encas_ReadBinaryLine(f);
    if (!Encas_Str_StartsWith(line, Encas_Str_Lit("element id "))) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'%s' file is not contains element id!\n", filename);
        Encas_FreeFile(f);
        return NULL;
    }

    // Substring in a hacky way :)
    line.buffer += 11;
    line.len -= 11;

    if ((element_id = _get_mode(line)) == ENCAS_MODE_UNKNOWN) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'%s' file: Unkown mode found! (not in: <off/given/assign/ignore>) at element id\n", filename);
        Encas_FreeFile(f);
        return NULL;
    }

    // extents?
    line = Encas_ReadBinaryLine(f);
    if (Encas_Str_StartsWith(line, Encas_Str_Lit("extents"))) {
        // Skips extents for now
        Encas_FileAdvace(f, 6 * sizeof(float));
        line = Encas_ReadBinaryLine(f);
    }

    // Parts
    u32 part_count = 0;
    Encas_HashTable *h = Encas_CreateHashTable();

    while (Encas_Str_StartsWith(line, Encas_Str_Lit("part"))) {
        s32 part_num = Encas_ReadS32(f);
        Encas_InsertHashTable(h, part_num, part_count);
        part_count++;

        // Skip description line
        Encas_FileAdvace(f, 80);

        while (!IS_ENCAS_EOF(f)) {
            line = Encas_ReadBinaryLine(f);
            bool is_ghost = false;
            Encas_Elem_Type elem_type;

            if (Encas_Str_StartsWith(line, Encas_Str_Lit("coordinates"))) {
                s32 num_of_nodes = Encas_ReadS32(f);

                // Skip node ids
                if (node_id == ENCAS_MODE_GIVEN || node_id == ENCAS_MODE_IGNORE)
                    Encas_FileAdvace(f, num_of_nodes * sizeof(s32));

                // Store coordinates
                /*
                for (u32 vert_idx = 0; vert_idx < num_of_nodes; ++vert_idx) {
                    //f->buffer + f->cur + vert_idx * sizeof(float)
                }
                */
                Encas_FileAdvace(f, 3 * num_of_nodes * sizeof(float));

            }

            else if (Encas_Str_StartsWith(line, Encas_Str_Lit("block"))) {
                Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Structured data is not implemented yet! (block keyword)\n");
                Encas_FreeFile(f);
                return NULL;
            }

            // Element type
            else if ((elem_type = Encas_ReadElemType(line, &is_ghost)) != ENCAS_ELEM_UNKNOWN) {
                s32 num_of_elements = Encas_ReadS32(f);

                // Skip element ids
                if (element_id == ENCAS_MODE_GIVEN || element_id == ENCAS_MODE_IGNORE)
                    Encas_FileAdvace(f, num_of_elements * sizeof(s32));

                u32 elem_vert_count = _get_elem_vert_count(elem_type);

                Encas_FileAdvace(f, num_of_elements * elem_vert_count * sizeof(s32));
            }

            else {
                break;
            }
        }
    }

    if (part_count == 0) {
        Encas_DeleteHashTable(h);
        return NULL;
    }

    return h;
}

// path is read only, everything will be written to dest
ENCAS_API void Encas_Dirname(char *path, char *dest) {
    size_t path_len = strlen(path);
    if (path_len > PATH_MAX)
        return;

    memcpy(dest, path, path_len);
    dest[path_len] = '\0';

    char *last_slash;

    // Find the last occurace of '/' or '\\' char
    #ifdef _WIN32
        last_slash = strrchr(dest, '\\');
        if (last_slash == NULL) {
            last_slash = strrchr(dest, '/');
        }
    #else
        last_slash = strrchr(dest, '/');
    #endif

    // If found one then replace with null char
    if (last_slash != NULL) {
        *last_slash = '\0';
    } else {
        strcpy(dest, ".");
    }
}

ENCAS_API Encas_Case *Encas_ReadCase(char *filename) {
    if (!check_if_file_exists(filename)) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Couldn't open '%s' case file!\n", filename);
        return NULL;
    }

    Encas_File *f = Encas_SlurpFile(filename);
    Encas_SectionType type = ENCAS_NOSECTION;
    Encas_Time *cur_time_elem = NULL;

    Encas_Case *encase = Encas_CreateCase();
    Encas_Dirname(filename, encase->dirname);

    while(!IS_ENCAS_EOF(f)) {
        Encas_Str line = Encas_ReadLine(f);
        if (line.buffer == NULL) {
            Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Error while reading a line");
        }

        Encas_SectionType temp_type = Encas_GetSectionType(line);
        if (temp_type != ENCAS_NOSECTION) {
            if (type == ENCAS_TIME && temp_type != ENCAS_TIME && cur_time_elem != NULL) {
                // Hozzáfűzzük az encase->times -hoz a cur_time_elem -et
                Encas_PushTimeArray(encase->times, cur_time_elem);
                cur_time_elem = NULL;
            }
            type = temp_type;
            continue;
        }

        if (type == ENCAS_NOSECTION || line.len == 0)
            continue;


        Encas_Str key, value;

        Encas_SplitKeyValue(line, &key, &value);
        if (key.buffer == NULL || value.buffer == NULL)
            continue;

        /*
        printf("Key: %.*s\n", key.len, key.buffer);
        printf("Value: %.*s\n", value.len, value.buffer);
        */
        Encas_StrArray *arr = Encas_Str_Split(value);

        switch(type) {
            case ENCAS_FORMAT: {
                if (!(Encas_Str_Equals(key, Encas_Str_Lit("type"))
                    && arr->len == 2
                    && Encas_Str_Equals(arr->elems[0], Encas_Str_Lit("ensight"))
                    && Encas_Str_Equals(arr->elems[1], Encas_Str_Lit("gold")))) {
                    Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Not a valid Ensight Gold Case file format!\n");
                    _free_case_and_file(encase, f);
                    return NULL;
                }
                break;
            }
            case ENCAS_GEOMETRY: {
                if (encase->geometry == NULL)
                    encase->geometry = Encas_CreateGeometry();
                if (arr->len > 4) {
                    Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Invalid geometry parameter: %.*s\n", value.len, value.buffer);
                    _free_case_and_file(encase, f);
                    return NULL;
                }

                Encas_GeometryElem *geometry_elem = Encas_CreateGeometryElem();

                // [ts] [fs] filename [change_coords_only]
                switch (arr->len) {
                    // filename
                    case 1: {
                        // Copy the filename temporary string to persistent string
                        Encas_Copy_Str_To_MutStr(arr->elems[0], &geometry_elem->filename);
                        break;
                    }
                    // [ts] filename
                    // filename [change_coords_only]
                    case 2: {
                        // [ts] filename
                        if (Encas_Str_IsNumber(arr->elems[0])) {
                            geometry_elem->ts_set = true;
                            geometry_elem->ts = Encas_Str_to_S32(arr->elems[0]);
                            Encas_Copy_Str_To_MutStr(arr->elems[1], &geometry_elem->filename);

                        }
                        // filename [change_coords_only]
                        else {
                            Encas_Copy_Str_To_MutStr(arr->elems[0], &geometry_elem->filename);
                            geometry_elem->change_coords_only_set = true;
                            geometry_elem->change_coords_only = Encas_Str_to_S32(arr->elems[1]);
                        }
                        break;
                    }

                    // [ts] [fs] filename
                    // [ts] filename [change_coords_only]
                    case 3: {
                        // [ts] [fs] filename
                        if (Encas_Str_IsNumber(arr->elems[1])) {
                            geometry_elem->ts_set = true;
                            geometry_elem->ts = Encas_Str_to_S32(arr->elems[0]);

                            geometry_elem->fs_set = true;
                            geometry_elem->fs = Encas_Str_to_S32(arr->elems[1]);

                            Encas_Copy_Str_To_MutStr(arr->elems[2], &geometry_elem->filename);
                        }
                        // [ts] filename [change_coords_only]
                        else {
                            geometry_elem->ts_set = true;
                            geometry_elem->ts = Encas_Str_to_S32(arr->elems[0]);

                            Encas_Copy_Str_To_MutStr(arr->elems[1], &geometry_elem->filename);

                            geometry_elem->change_coords_only_set = true;
                            geometry_elem->change_coords_only = Encas_Str_to_S32(arr->elems[2]);
                        }
                        break;
                    }

                    // ts fs filename change_coords_only
                    case 4: {
                        geometry_elem->ts_set = true;
                        geometry_elem->ts = Encas_Str_to_S32(arr->elems[0]);

                        geometry_elem->fs_set = true;
                        geometry_elem->fs = Encas_Str_to_S32(arr->elems[1]);

                        Encas_Copy_Str_To_MutStr(arr->elems[2], &geometry_elem->filename);

                        geometry_elem->change_coords_only_set = true;
                        geometry_elem->change_coords_only = Encas_Str_to_S32(arr->elems[3]);
                        break;
                    }
                    default: {
                        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Invalid geometry parameter: %.*s\n", value.len, value.buffer);
                        break;
                    }
                }

                if (Encas_Str_Equals(key, Encas_Str_Lit("model"))) {
                    encase->geometry->model = geometry_elem;

                } else if (Encas_Str_Equals(key, Encas_Str_Lit("measured"))) {
                    encase->geometry->measured = geometry_elem;

                } else if (Encas_Str_Equals(key, Encas_Str_Lit("match"))) {
                    encase->geometry->match = geometry_elem;

                } else if (Encas_Str_Equals(key, Encas_Str_Lit("boundary"))) {
                    encase->geometry->boundary = geometry_elem;
                }
                else {
                    Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Invalid key in GEOMETRY section: %.*s\n", key.len, key.buffer);
                    Encas_DeleteGeometryElem(geometry_elem);
                    _free_case_and_file(encase, f);
                    return NULL;
                }

                break;
             }
            case ENCAS_VARIABLE: {
                if (encase->variable == NULL)
                    encase->variable = Encas_CreateVariableArray();

                if (arr->len > 4 || arr->len < 2) {
                    Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Invalid variable parameter: %.*s\n", value.len, value.buffer);
                    _free_case_and_file(encase, f);
                    return NULL;
                }

                Encas_DescFile *df = Encas_CreateDescFile();

                switch (arr->len) {
                    // description filename
                    case 2: {
                        Encas_Copy_Str_To_MutStr(arr->elems[0], &df->description);
                        Encas_Copy_Str_To_MutStr(arr->elems[1], &df->filename);
                        break;
                    }
                    // [ts] description filename
                    case 3: {
                        df->ts_set = true;
                        df->ts = Encas_Str_to_S32(arr->elems[0]);
                        Encas_Copy_Str_To_MutStr(arr->elems[1], &df->description);
                        Encas_Copy_Str_To_MutStr(arr->elems[2], &df->filename);
                        break;
                    }
                    // [ts] [fs] description filename
                    case 4: {
                        df->ts_set = true;
                        df->ts = Encas_Str_to_S32(arr->elems[0]);
                        df->fs_set = true;
                        df->fs = Encas_Str_to_S32(arr->elems[1]);
                        Encas_Copy_Str_To_MutStr(arr->elems[2], &df->description);
                        Encas_Copy_Str_To_MutStr(arr->elems[3], &df->filename);
                        break;
                    }
                    default: {
                        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Invalid variable parameter: %.*s\n", value.len, value.buffer);
                        break;
                    }
                }

                // [ts] description const_value(s)
                if (Encas_Str_Equals(key, Encas_Str_Lit("constant per case"))) {
                    Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'constant per case' is not implemented yet\n");
                    Encas_DeleteDescFile(df);
                    _free_case_and_file(encase, f);
                    return NULL;
                }
                // [ts] description cvfilename
                else if (Encas_Str_Equals(key, Encas_Str_Lit("constant per case file"))) {
                    Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'constant per case file' is not implemented yet\n");
                    Encas_DeleteDescFile(df);
                    _free_case_and_file(encase, f);
                    return NULL;
                }

                // ----------------------------
                // [ts] [fs] description filename
                else if (Encas_Str_Equals(key, Encas_Str_Lit("scalar per node"))) {
                    df->type = ENCAS_VARIABLE_SCALAR_PER_NODE;
                    Encas_PushVariableArray(encase->variable, df);
                }
                // [ts] [fs] description filename
                else if (Encas_Str_Equals(key, Encas_Str_Lit("vector per node"))) {
                    df->type = ENCAS_VARIABLE_VECTOR_PER_NODE;
                    Encas_PushVariableArray(encase->variable, df);
                }
                // [ts] [fs] description filename
                else if (Encas_Str_Equals(key, Encas_Str_Lit("tensor symm per node"))) {
                    Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'tensor symm per node' is not implemented yet\n");
                    Encas_DeleteDescFile(df);
                    _free_case_and_file(encase, f);
                    return NULL;
                }
                // [ts] [fs] description filename
                else if (Encas_Str_Equals(key, Encas_Str_Lit("tensor asymm per node"))) {
                    Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'tensor asymm per node' is not implemented yet\n");
                    Encas_DeleteDescFile(df);
                    _free_case_and_file(encase, f);
                    return NULL;
                }
                // [ts] [fs] description filename
                else if (Encas_Str_Equals(key, Encas_Str_Lit("scalar per element"))) {
                    df->type = ENCAS_VARIABLE_SCALAR_PER_ELEMENT;
                    Encas_PushVariableArray(encase->variable, df);
                }
                // [ts] [fs] description filename
                else if (Encas_Str_Equals(key, Encas_Str_Lit("vector per element"))) {
                    df->type = ENCAS_VARIABLE_VECTOR_PER_ELEMENT;
                    Encas_PushVariableArray(encase->variable, df);
                }
                // [ts] [fs] description filename
                else if (Encas_Str_Equals(key, Encas_Str_Lit("tensor symm per element"))) {
                    Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'tensor symm per element' is not implemented yet\n");
                    Encas_DeleteDescFile(df);
                    _free_case_and_file(encase, f);
                    return NULL;
                }
                // [ts] [fs] description filename
                else if (Encas_Str_Equals(key, Encas_Str_Lit("tensor asymm per element"))) {
                    Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'tensor asymm per element' is not implemented yet\n");
                    Encas_DeleteDescFile(df);
                    _free_case_and_file(encase, f);
                    return NULL;
                }
                // [ts] [fs] description filename
                else if (Encas_Str_Equals(key, Encas_Str_Lit("scalar per measured node"))) {
                    Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'scalar per measured node' is not implemented yet\n");
                    Encas_DeleteDescFile(df);
                    _free_case_and_file(encase, f);
                    return NULL;
                }
                // [ts] [fs] description filename
                else if (Encas_Str_Equals(key, Encas_Str_Lit("vector per measured node"))) {
                    Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'vector per measured node' is not implemented yet\n");
                    Encas_DeleteDescFile(df);
                    _free_case_and_file(encase, f);
                    return NULL;
                }
                // ----------------------------

                // [ts] [fs] description Re_fn Im_fn freq
                else if (Encas_Str_Equals(key, Encas_Str_Lit("complex scalar per node"))) {
                    Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'complex scalar per node' is not implemented yet\n");
                    Encas_DeleteDescFile(df);
                    _free_case_and_file(encase, f);
                    return NULL;
                }
                // [ts] [fs] description Re_fn Im_fn freq
                else if (Encas_Str_Equals(key, Encas_Str_Lit("complex vector per node"))) {
                    Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'complex vector per node' is not implemented yet\n");
                    Encas_DeleteDescFile(df);
                    _free_case_and_file(encase, f);
                    return NULL;
                }
                // [ts] [fs] description Re_fn Im_fn freq
                else if (Encas_Str_Equals(key, Encas_Str_Lit("complex scalar per element"))) {
                    Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'complex scalar per element' is not implemented yet\n");
                    Encas_DeleteDescFile(df);
                    _free_case_and_file(encase, f);
                    return NULL;
                }
                // [ts] [fs] description Re_fn Im_fn freq
                else if (Encas_Str_Equals(key, Encas_Str_Lit("complex vector per element"))) {
                    Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'complex vector per element' is not implemented yet\n");
                    Encas_DeleteDescFile(df);
                    _free_case_and_file(encase, f);
                    return NULL;
                }
                else {
                    Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Invalid key in VARIABLE section: %.*s\n", key.len, key.buffer);
                    Encas_DeleteDescFile(df);
                    _free_case_and_file(encase, f);
                    return NULL;
                }

                break;
            }
            case ENCAS_TIME: {
                if (encase->times == NULL) {
                    encase->times = Encas_CreateTimeArray();
                }

                if (Encas_Str_Equals(key, Encas_Str_Lit("time set"))) {
                    if (cur_time_elem != NULL)
                        Encas_PushTimeArray(encase->times, cur_time_elem);

                    cur_time_elem = Encas_CreateTime();
                    cur_time_elem->time_set_number = Encas_Str_to_S32(arr->elems[0]);

                    if (arr->len == 2) {
                        Encas_Copy_Str_To_MutStr(arr->elems[1], &cur_time_elem->time_set_description);
                    }
                }
                else if (cur_time_elem == NULL) {
                    Encas_Log(ENCAS_LOG_LEVEL_ERROR, "no time set number specified\n");
                    _free_case_and_file(encase, f);
                    return NULL;
                }
                else if (Encas_Str_Equals(key, Encas_Str_Lit("number of steps"))) {
                    cur_time_elem->number_of_steps = Encas_Str_to_S32(arr->elems[0]);
                    cur_time_elem->time_values = (float *)ENCAS_MALLOC(cur_time_elem->number_of_steps * sizeof(float));
                }
                else if (Encas_Str_Equals(key, Encas_Str_Lit("filename start number"))) {
                    cur_time_elem->filename_start_number = Encas_Str_to_S32(arr->elems[0]);
                }
                else if (Encas_Str_Equals(key, Encas_Str_Lit("filename increment"))) {
                    cur_time_elem->filename_increment = Encas_Str_to_S32(arr->elems[0]);
                }
                else if (Encas_Str_Equals(key, Encas_Str_Lit("time values"))) {
                    // Beolvasott time értékek száma
                    u32 num_time_values = 0;

                    for(u32 i = 0; i < arr->len; ++i) {
                        cur_time_elem->time_values[num_time_values++] = Encas_Str_to_F32(arr->elems[i]);
                    }
                    // Több sorban vannak az adatok
                    // Data continues on the next lines
                    if (cur_time_elem->number_of_steps != num_time_values) {
                        while (!(IS_ENCAS_EOF(f) || num_time_values == cur_time_elem->number_of_steps)) {
                            Encas_Str next_line = Encas_ReadLine(f);
                            if (next_line.buffer == NULL) {
                                Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Error while reading a line");
                            }

                            Encas_StrArray *time_values = Encas_Str_Split(next_line);
                            for(u32 i = 0; i < time_values->len; ++i) {
                                cur_time_elem->time_values[num_time_values++] = Encas_Str_to_F32(time_values->elems[i]);
                            }

                            Encas_DeleteStrArray(time_values);
                        }
                    }
                }
                break;
            }
            case ENCAS_FILE:
                break;
            case ENCAS_MATERIAL:
                break;
            default:
                Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Shouldn't see this! :D\n");
                break;
        }

        Encas_DeleteStrArray(arr);
        //printf("%d: %.*s\n", line.len, line.len, line.buffer);
    }


    if (cur_time_elem != NULL)
        Encas_PushTimeArray(encase->times, cur_time_elem);


    /*
    printf("MODEL ts: %d, filename: %.*s\n", encase->geometry->model->ts, encase->geometry->model->filename.len, encase->geometry->model->filename.buffer);
    printf("MEASURED ts: %d, filename: %.*s\n", encase->geometry->measured->ts, encase->geometry->measured->filename.len, encase->geometry->measured->filename.buffer);
    */
    /*
    printf("SCALAR_PER_ELEMENT description: %.*s, filename: %.*s\n", encase->variable->scalar_per_element->description.len, encase->variable->scalar_per_element->description.buffer, encase->variable->scalar_per_element->filename.len, encase->variable->scalar_per_element->filename.buffer); 
    printf("SCALAR_PER_NODE description: %.*s, filename: %.*s\n", encase->variable->scalar_per_node->description.len, encase->variable->scalar_per_node->description.buffer, encase->variable->scalar_per_node->filename.len, encase->variable->scalar_per_node->filename.buffer); 
    */

    Encas_FreeFile(f);

    if (encase->geometry->model == NULL) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "No model specified in case file!\n");
        Encas_DeleteCase(encase);
        return NULL;
    }

    Encas_GeometryElem *gelem = encase->geometry->model;

    if (encase->geometry->model->ts_set) {
        Encas_Time *time = NULL;

        for (u32 time_idx = 0; time_idx < encase->times->len && time == NULL; ++time_idx)
            if (encase->times->elems[time_idx]->time_set_number == gelem->ts)
                time = encase->times->elems[time_idx];

        if (time == NULL) {
            Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Time with 'time set number = %d' not found!\n", gelem->ts);
            Encas_DeleteCase(encase);
            return NULL;
        }

        if (time->filename_start_number == 0 && time->filename_increment == 0) {
            Encas_Log(ENCAS_LOG_LEVEL_ERROR, "filename_start_number and filename_increment is not specified!\n");
            Encas_DeleteCase(encase);
            return NULL;
        }

        s32 asterisk_idx = Encas_MutStr_FindChar(&gelem->filename, '*');
        if (asterisk_idx == -1) {
            Encas_CreateMeshInfoArray(&gelem->mesh_info_array, 1);

            u32 dirname_length = strlen(encase->dirname);
            u32 filename_length = 0;
            char geo_filename[PATH_MAX + 1];
            memcpy(geo_filename, encase->dirname, dirname_length);
            memcpy(geo_filename + dirname_length, "/", 1);
            filename_length += dirname_length + 1;
            memcpy(geo_filename + filename_length, encase->geometry->model->filename.buffer, encase->geometry->model->filename.len);
            filename_length += encase->geometry->model->filename.len;
            geo_filename[filename_length] = '\0';

            if(!Encas_ParseMeshInfo(&gelem->mesh_info_array.elems[0], geo_filename)) {
                Encas_DeleteCase(encase);
                return NULL;
            }
            gelem->num_of_files = 1;

            return encase;
        }

        u32 asterisk_count = 1;
        for (u32 i = asterisk_idx + 1; i < gelem->filename.len && gelem->filename.buffer[i] == '*'; ++i)
            ++asterisk_count;

        gelem->num_of_files = time->number_of_steps;
        Encas_CreateMeshInfoArray(&gelem->mesh_info_array, time->number_of_steps);
        u32 to = time->filename_start_number + time->filename_increment * (time->number_of_steps - 1);
        u32 idx = 0;

        char geo_filename[PATH_MAX + 1];
        u32 dirname_length = strlen(encase->dirname);
        u32 filename_length = 0;
        memcpy(geo_filename, encase->dirname, dirname_length);
        geo_filename[dirname_length] = '/';
        filename_length += dirname_length + 1;

        for (u32 file_num = time->filename_start_number; file_num <= to; file_num += time->filename_increment) {

            memcpy(geo_filename + filename_length, gelem->filename.buffer, gelem->filename.len);
            geo_filename[filename_length + gelem->filename.len] = '\0';

            char tmp[256];
            snprintf(tmp, 256, "%0*d", asterisk_count, file_num);
            u32 tmp_len = strlen(tmp);
            if (tmp_len != asterisk_count) {
                Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Pattern '*' is shorter than the generated number!\n");
                Encas_DeleteCase(encase);
                return NULL;
            }

            memcpy(geo_filename + filename_length + asterisk_idx, tmp, tmp_len);

            //printf("filename: %s\n", filename);
            if(!Encas_ParseMeshInfo(&gelem->mesh_info_array.elems[idx++], geo_filename)) {
                Encas_DeleteCase(encase);
                return NULL;
            }
        }
    }
    // Only one geometry file
    else {
        // load a single geometry file
        Encas_CreateMeshInfoArray(&gelem->mesh_info_array, 1);

        /*
        char geo_filename[256];
        memcpy(geo_filename, encase->geometry->model->filename.buffer, encase->geometry->model->filename.len);
        geo_filename[encase->geometry->model->filename.len] = '\0';
        */
        u32 dirname_length = strlen(encase->dirname);
        u32 filename_length = 0;
        char geo_filename[PATH_MAX + 1];
        memcpy(geo_filename, encase->dirname, dirname_length);
        memcpy(geo_filename + dirname_length, "/", 1);
        filename_length += dirname_length + 1;
        memcpy(geo_filename + filename_length, encase->geometry->model->filename.buffer, encase->geometry->model->filename.len);
        filename_length += encase->geometry->model->filename.len;
        geo_filename[filename_length] = '\0';

        if(!Encas_ParseMeshInfo(&gelem->mesh_info_array.elems[0], geo_filename)) {
            Encas_DeleteCase(encase);
            return NULL;
        }
        gelem->num_of_files = 1;
    }

    return encase;
}

ENCAS_API Encas_Mesh *Encas_CreateMesh() {
    Encas_Mesh *mesh = (Encas_Mesh *)ENCAS_MALLOC(sizeof(Encas_Mesh));
    memset(mesh, 0, sizeof(Encas_Mesh));

    return mesh;
}

ENCAS_API void Encas_DeleteMesh(Encas_Mesh *mesh) {
    ENCAS_FREE(mesh->vert_array.x);
    ENCAS_FREE(mesh->vert_array.y);
    ENCAS_FREE(mesh->vert_array.z);
    ENCAS_FREE(mesh->elem_array);
    ENCAS_FREE(mesh->elem_vert_map_array);

    ENCAS_FREE(mesh);
}

ENCAS_API Encas_MeshArray *Encas_CreateMeshArray() {
    Encas_MeshArray *arr = (Encas_MeshArray *)ENCAS_MALLOC(sizeof(Encas_MeshArray));
    memset(arr, 0, sizeof(Encas_MeshArray));

    arr->cap = DEFAULT_MESHARRAY_CAP;
    arr->elems = (Encas_Mesh **)ENCAS_MALLOC(arr->cap * sizeof(Encas_Mesh *));
    if (arr->elems == NULL) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Cannot allocate memory for MeshArray!\n");
        ENCAS_FREE(arr);
        return NULL;
    }

    for (u32 i = 0; i < arr->cap; ++i)
        arr->elems[i] = NULL;

    return arr;
}

ENCAS_API Encas_MeshArray *Encas_CreateMeshArrayWithCap(u32 cap) {
    Encas_MeshArray *arr = (Encas_MeshArray *)ENCAS_MALLOC(sizeof(Encas_MeshArray));
    memset(arr, 0, sizeof(Encas_MeshArray));

    arr->cap = cap;
    arr->elems = (Encas_Mesh **)ENCAS_MALLOC(arr->cap * sizeof(Encas_Mesh *));
    if (arr->elems == NULL) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Cannot allocate memory for MeshArray!\n");
        ENCAS_FREE(arr);
        return NULL;
    }

    for (u32 i = 0; i < arr->cap; ++i)
        arr->elems[i] = NULL;

    return arr;
}

ENCAS_API bool Encas_PushMeshArray(Encas_MeshArray *arr, Encas_Mesh *mesh) {
    if (arr->len + 1 > arr->cap) {
        arr->cap *= 2;
        arr->elems = (Encas_Mesh **)ENCAS_REALLOC(arr->elems, arr->cap * sizeof(Encas_Mesh *));

        if (arr->elems == NULL) {
            Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Cannot allocate memory for MeshArray push!\n");
            return false;
        }
    }

    arr->elems[arr->len++] = mesh;
    return true;
}

ENCAS_API void Encas_DeleteMeshArray(Encas_MeshArray *arr) {
    // loop over elems
    for(u32 mesh_idx = 0; mesh_idx < arr->len; ++mesh_idx)
        Encas_DeleteMesh(arr->elems[mesh_idx]);

    ENCAS_FREE(arr->elems);
    ENCAS_FREE(arr);
}

ENCAS_API const char *Encas_ElemToCstr(Encas_Elem_Type elem) {
    switch (elem) {
    case ENCAS_ELEM_POINT:
        return "ENCAS_ELEM_POINT";
    case ENCAS_ELEM_BAR2:
        return "ENCAS_ELEM_BAR2";
    case ENCAS_ELEM_BAR3:
        return "ENCAS_ELEM_BAR3";
    case ENCAS_ELEM_TRIA3:
        return "ENCAS_ELEM_TRIA3";
    case ENCAS_ELEM_TRIA6:
        return "ENCAS_ELEM_TRIA6";
    case ENCAS_ELEM_QUAD4:
        return "ENCAS_ELEM_QUAD4";
    case ENCAS_ELEM_QUAD8:
        return "ENCAS_ELEM_QUAD8";
    case ENCAS_ELEM_TETRA4:
        return "ENCAS_ELEM_TETRA4";
    case ENCAS_ELEM_TETRA10:
        return "ENCAS_ELEM_TETRA10";
    case ENCAS_ELEM_PYRAMID5:
        return "ENCAS_ELEM_PYRAMID5";
    case ENCAS_ELEM_PYRAMID13:
        return "ENCAS_ELEM_PYRAMID13";
    case ENCAS_ELEM_PENTA6:
        return "ENCAS_ELEM_PENTA6";
    case ENCAS_ELEM_PENTA15:
        return "ENCAS_ELEM_PENTA15";
    case ENCAS_ELEM_HEXA8:
        return "ENCAS_ELEM_HEXA8";
    case ENCAS_ELEM_HEXA20:
        return "ENCAS_ELEM_HEXA20";
    case ENCAS_ELEM_NSIDED:
        return "ENCAS_ELEM_NSIDED";
    case ENCAS_ELEM_NFACED:
        return "ENCAS_ELEM_NFACED";
    case ENCAS_ELEM_UNKNOWN:
        return "ENCAS_ELEM_UNKNOWN";
    default:
        break;
    }

    return "(null)";
}

ENCAS_API Encas_MeshArray *Encas_ReadGeometry(Encas_MeshInfo *mesh_info, char *filename) {
    if (!check_if_file_exists(filename)) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Cannot open %s geometry file\n", filename);
        return NULL;
    }

    Encas_File *f = Encas_SlurpFile(filename);
    Encas_Mode node_id, element_id;
    Encas_MeshArray *mesh_arr = Encas_CreateMeshArrayWithCap(mesh_info->len); // Geometry

    Encas_Str line = Encas_ReadBinaryLine(f);

    if (!Encas_Str_StartsWith(line, Encas_Str_Lit("C Binary"))) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'%s' is not in C Binary form!\n", filename);
        Encas_DeleteMeshArray(mesh_arr);
        Encas_FreeFile(f);
        return NULL;
    }

    // Skip the two description line
    if (!Encas_FileAdvace(f, 160)) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'%s' file is not contains description!\n", filename);
        Encas_DeleteMeshArray(mesh_arr);
        Encas_FreeFile(f);
        return NULL;
    }

    line = Encas_ReadBinaryLine(f);
    // node id
    if (!Encas_Str_StartsWith(line, Encas_Str_Lit("node id "))) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'%s' file is not contains node id!\n", filename);
        Encas_DeleteMeshArray(mesh_arr);
        Encas_FreeFile(f);
        return NULL;
    }

    // Substring in a hacky way :)
    line.buffer += 8;
    line.len -= 8;

    if ((node_id = _get_mode(line)) == ENCAS_MODE_UNKNOWN) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'%s' file: Unkown mode found! (not in: <off/given/assign/ignore>) at node id\n", filename);
        Encas_DeleteMeshArray(mesh_arr);
        Encas_FreeFile(f);
        return NULL;
    }

    // element id
    line = Encas_ReadBinaryLine(f);
    if (!Encas_Str_StartsWith(line, Encas_Str_Lit("element id "))) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'%s' file is not contains element id!\n", filename);
        Encas_DeleteMeshArray(mesh_arr);
        Encas_FreeFile(f);
        return NULL;
    }

    // Substring in a hacky way :)
    line.buffer += 11;
    line.len -= 11;

    if ((element_id = _get_mode(line)) == ENCAS_MODE_UNKNOWN) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'%s' file: Unkown mode found! (not in: <off/given/assign/ignore>) at element id\n", filename);
        Encas_DeleteMeshArray(mesh_arr);
        Encas_FreeFile(f);
        return NULL;
    }

    // extents?
    line = Encas_ReadBinaryLine(f);
    if (Encas_Str_StartsWith(line, Encas_Str_Lit("extents"))) {
        // Skips extents for now
        // maybe implement later
        Encas_FileAdvace(f, 6 * sizeof(float));
        line = Encas_ReadBinaryLine(f);
    }

    u32 part_idx = 0;
    while (Encas_Str_StartsWith(line, Encas_Str_Lit("part"))) {
        Encas_Mesh *mesh = Encas_CreateMesh();

        mesh->part_number = Encas_ReadS32(f);

        // Skip description line
        Encas_FileAdvace(f, 80);

        mesh->elem_array_size = mesh_info->parts[part_idx].len;
        u64 elem_vert_map_array_size = mesh_info->parts[part_idx].elem_vert_map_array_size;

        // Then store the data
        mesh->elem_array          = (Encas_Elem *)ENCAS_MALLOC(mesh->elem_array_size * sizeof(Encas_Elem));
        mesh->elem_vert_map_array = (u32 *)ENCAS_MALLOC(elem_vert_map_array_size * sizeof(u32));

        u32 elem_idx = 0;
        u32 elem_vert_map_entry_ptr = 0;

        while (!IS_ENCAS_EOF(f)) {
            line = Encas_ReadBinaryLine(f);
            bool is_ghost = false;
            Encas_Elem_Type elem_type;

            if (Encas_Str_StartsWith(line, Encas_Str_Lit("coordinates"))) {
                s32 num_of_nodes = Encas_ReadS32(f);

                // Skip node ids
                if (node_id == ENCAS_MODE_GIVEN || node_id == ENCAS_MODE_IGNORE)
                    Encas_FileAdvace(f, num_of_nodes * sizeof(s32));

                mesh->vert_array_size = num_of_nodes;
                mesh->vert_array.x = (float *)ENCAS_MALLOC(mesh->vert_array_size * sizeof(float));
                mesh->vert_array.y = (float *)ENCAS_MALLOC(mesh->vert_array_size * sizeof(float));
                mesh->vert_array.z = (float *)ENCAS_MALLOC(mesh->vert_array_size * sizeof(float));

                // Store coordinates
/* DEPRECATED
                for (u32 vert_idx = 0; vert_idx < num_of_nodes; ++vert_idx) {
                    mesh->vert_array[vert_idx].x = *(float *)(f->buffer + f->cur + vert_idx * sizeof(float));
                    mesh->vert_array[vert_idx].y = *(float *)(f->buffer + f->cur + num_of_nodes * sizeof(float) + vert_idx * sizeof(float));
                    mesh->vert_array[vert_idx].z = *(float *)(f->buffer + f->cur + 2 * num_of_nodes * sizeof(float) + vert_idx * sizeof(float));
                }
*/

                memcpy(mesh->vert_array.x, f->buffer + f->cur, num_of_nodes * sizeof(float));
                memcpy(mesh->vert_array.y, f->buffer + f->cur + num_of_nodes * sizeof(float), num_of_nodes * sizeof(float));
                memcpy(mesh->vert_array.z, f->buffer + f->cur + 2 * num_of_nodes * sizeof(float), num_of_nodes * sizeof(float));

                Encas_FileAdvace(f, 3 * num_of_nodes * sizeof(float));
            }

            else if (Encas_Str_StartsWith(line, Encas_Str_Lit("block"))) {
                Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Structured data is not implemented yet! (block keyword)\n");
                Encas_DeleteMeshArray(mesh_arr);
                Encas_FreeFile(f);
                return NULL;
            }

            // Element type
            else if ((elem_type = Encas_ReadElemType(line, &is_ghost)) != ENCAS_ELEM_UNKNOWN) {
                s32 num_of_elements = Encas_ReadS32(f);

                // Skip element ids
                if (element_id == ENCAS_MODE_GIVEN || element_id == ENCAS_MODE_IGNORE)
                    Encas_FileAdvace(f, num_of_elements * sizeof(s32));

                u32 elem_vert_count = _get_elem_vert_count(elem_type);

                // ghost elems
                if (!is_ghost) {
                    mesh->elem_array[elem_idx].type = elem_type;
                    mesh->elem_array[elem_idx].elem_size = elem_vert_count;
                    mesh->elem_array[elem_idx].elem_vert_map_size = num_of_elements * elem_vert_count;
                    mesh->elem_array[elem_idx].elem_vert_map_entry = elem_vert_map_entry_ptr;
                    elem_vert_map_entry_ptr += num_of_elements * elem_vert_count;

                    /*
                    memcpy(mesh->elem_vert_map_array + mesh->elem_array[elem_idx].elem_vert_map_entry,
                           f->buffer + f->cur,
                           mesh->elem_array[elem_idx].elem_vert_map_size * sizeof(u32));
                           */

                    for (u32 i = 0; i < num_of_elements * elem_vert_count; ++i) {
                        mesh->elem_vert_map_array[mesh->elem_array[elem_idx].elem_vert_map_entry + i] = *((u32 *)(f->buffer + f->cur + sizeof(u32) * i)) - 1;
                    }

                    ++elem_idx;
                }

                Encas_FileAdvace(f, num_of_elements * elem_vert_count * sizeof(s32));
            }

            else {
                break;
            }
        }
        // Part end
        //u32 part_idx = mesh_arr->len;
        //Encas_InsertHashTable(mesh_arr->part_num_lookup, mesh->part_number, part_idx);
        Encas_PushMeshArray(mesh_arr, mesh);
        part_idx++;
    }

    Encas_FreeFile(f);
    return mesh_arr;
}

ENCAS_API Encas_MeshArray *Encas_LoadGeometry(Encas_Case *encase, u32 time_value_idx) {
    if (time_value_idx > encase->geometry->model->num_of_files - 1)
        time_value_idx = 0;

    Encas_GeometryElem *gelem = encase->geometry->model;
    Encas_MeshInfo *mesh_info = NULL;

    u32 dirname_length = strlen(encase->dirname);
    char geo_filename[PATH_MAX + 1];
    memcpy(geo_filename, encase->dirname, dirname_length);
    memcpy(geo_filename + dirname_length, "/", 1);

    if (!gelem->ts_set) {
        // load a single geometry file
        if (encase->times != NULL && encase->times->len >= 1) {
            gelem->ts = encase->times->elems[0]->time_set_number;
        } else {
            memcpy(geo_filename + dirname_length + 1, gelem->filename.buffer, gelem->filename.len);
            geo_filename[dirname_length + 1 + gelem->filename.len] = '\0';
            mesh_info = &gelem->mesh_info_array.elems[0];
            Encas_Log(ENCAS_LOG_LEVEL_INFO, "Geometry filename: %s\n", geo_filename);
            return Encas_ReadGeometry(mesh_info, geo_filename);
        }
    }

    Encas_Time *time = NULL;

    for (u32 time_idx = 0; time_idx < encase->times->len && time == NULL; ++time_idx)
        if (encase->times->elems[time_idx]->time_set_number == gelem->ts)
            time = encase->times->elems[time_idx];

    if (time == NULL) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Encas_LoadGeometry: Time with 'time set number = %d' not found!\n", gelem->ts);
        return NULL;
    }

    if (time->filename_start_number == 0 && time->filename_increment == 0) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "filename_start_number and filename_increment is not specified!\n");
        return NULL;
    }

    if (time_value_idx > time->number_of_steps - 1) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "time_value_idx is out of range! (%d > %d)\n", time_value_idx, time->number_of_steps - 1);
        return NULL;
    }

    s32 asterisk_idx = Encas_MutStr_FindChar(&gelem->filename, '*');
    if (asterisk_idx == -1) {
        memcpy(geo_filename + dirname_length + 1, gelem->filename.buffer, gelem->filename.len);
        geo_filename[dirname_length + 1 + gelem->filename.len] = '\0';
        mesh_info = &gelem->mesh_info_array.elems[0];
        return Encas_ReadGeometry(mesh_info, geo_filename);
    }

    u32 asterisk_count = 1;
    for (u32 i = asterisk_idx + 1; i < gelem->filename.len && gelem->filename.buffer[i] == '*'; ++i)
        ++asterisk_count;

    u32 file_num = time->filename_start_number + time->filename_increment * time_value_idx;
    mesh_info = &gelem->mesh_info_array.elems[time_value_idx];
    //for (u32 file_num = time->filename_start_number; file_num <= to; file_num += time->filename_increment) {

    memcpy(geo_filename + dirname_length + 1, gelem->filename.buffer, gelem->filename.len);
    geo_filename[dirname_length + 1 + gelem->filename.len] = '\0';

    char tmp[256];
    snprintf(tmp, 256, "%0*d", asterisk_count, file_num);
    u32 tmp_len = strlen(tmp);
    if (tmp_len != asterisk_count) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Pattern '*' is shorter than the generated number!\n");
        return NULL;
    }

    memcpy(geo_filename + dirname_length + 1 + asterisk_idx, tmp, tmp_len);

    return Encas_ReadGeometry(mesh_info, geo_filename);
}

ENCAS_API u32 Encas_GetCellTrianglesCount(Encas_Elem_Type cell_type) {
    switch (cell_type) {
        case ENCAS_ELEM_TRIA3:
            return 1;
        case ENCAS_ELEM_TRIA6:
            return 4;
        case ENCAS_ELEM_QUAD4:
            return 2;
        case ENCAS_ELEM_QUAD8:
            return 6;
        case ENCAS_ELEM_TETRA4:
            return 4;
        case ENCAS_ELEM_TETRA10:
            return 16;
        case ENCAS_ELEM_PYRAMID5:
            return 6;
        case ENCAS_ELEM_PYRAMID13:
            return 22;
        case ENCAS_ELEM_PENTA6:
            return 8;
        case ENCAS_ELEM_PENTA15:
            return 26;
        case ENCAS_ELEM_HEXA8:
            return 12;
        case ENCAS_ELEM_HEXA20:
            return 36;

        case ENCAS_ELEM_NSIDED:
        case ENCAS_ELEM_NFACED:
        case ENCAS_ELEM_UNKNOWN:
        default:
            return 0;
    }
}

ENCAS_API void Encas_TriangulateTria3s(u32 *elem_vert_map_array, u32 num_cells, u32 *faces, u64 *faces_offset, u64 vert_offset) {
    u64 local_faces_offset = *faces_offset;

    for (u32 cell_idx = 0; cell_idx < num_cells; ++cell_idx) {
        faces[local_faces_offset + 0] = elem_vert_map_array[cell_idx * 3 + 0] + vert_offset;
        faces[local_faces_offset + 1] = elem_vert_map_array[cell_idx * 3 + 1] + vert_offset;
        faces[local_faces_offset + 2] = elem_vert_map_array[cell_idx * 3 + 2] + vert_offset;

        local_faces_offset += 3;
    }

    *faces_offset = local_faces_offset;
}

ENCAS_API void Encas_TriangulateTetra4s(u32 *elem_vert_map_array, u32 num_cells, u32 *faces, u64 *faces_offset, u64 vert_offset) {
    u64 local_faces_offset = *faces_offset;

    for (u32 cell_idx = 0; cell_idx < num_cells; ++cell_idx) {
        faces[local_faces_offset + 0] = elem_vert_map_array[cell_idx * 4 + 0] + vert_offset;
        faces[local_faces_offset + 1] = elem_vert_map_array[cell_idx * 4 + 1] + vert_offset;
        faces[local_faces_offset + 2] = elem_vert_map_array[cell_idx * 4 + 2] + vert_offset;

        local_faces_offset += 3;

        faces[local_faces_offset + 0] = elem_vert_map_array[cell_idx * 4 + 0] + vert_offset;
        faces[local_faces_offset + 1] = elem_vert_map_array[cell_idx * 4 + 1] + vert_offset;
        faces[local_faces_offset + 2] = elem_vert_map_array[cell_idx * 4 + 3] + vert_offset;

        local_faces_offset += 3;

        faces[local_faces_offset + 0] = elem_vert_map_array[cell_idx * 4 + 1] + vert_offset;
        faces[local_faces_offset + 1] = elem_vert_map_array[cell_idx * 4 + 2] + vert_offset;
        faces[local_faces_offset + 2] = elem_vert_map_array[cell_idx * 4 + 3] + vert_offset;

        local_faces_offset += 3;

        faces[local_faces_offset + 0] = elem_vert_map_array[cell_idx * 4 + 0] + vert_offset;
        faces[local_faces_offset + 1] = elem_vert_map_array[cell_idx * 4 + 2] + vert_offset;
        faces[local_faces_offset + 2] = elem_vert_map_array[cell_idx * 4 + 3] + vert_offset;

        local_faces_offset += 3;
    }

    *faces_offset = local_faces_offset;
}

force_inline void sort3(u32 *a, u32 *b, u32 *c) {
    if (*a > *b) { u32 t = *a; *a = *b; *b = t; }
    if (*b > *c) { u32 t = *b; *b = *c; *c = t; }
    if (*a > *b) { u32 t = *a; *a = *b; *b = t; }
}

force_inline u32 next_power_of_two(u32 x) {
    if (x == 0) return 1;
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
}

ENCAS_API void Encas_LoadGeometryShell(Encas_Case *encase, Encas_MeshArray *mesh, Encas_ShellParams *params) {
    u32 *faces; // triangles
    u64 num_faces = 0;

    Encas_Vertex *vertices;

    u64 vertices_size = 0;
    for (u32 part_idx = 0; part_idx < mesh->len; ++part_idx)
        vertices_size += mesh->elems[part_idx]->vert_array_size;

    vertices = (Encas_Vertex *)ENCAS_MALLOC(vertices_size * sizeof(Encas_Vertex));


    u64 vert_offset = 0;
    for (u32 part_idx = 0; part_idx < mesh->len; ++part_idx) {
        Encas_Mesh *mesh_part = mesh->elems[part_idx];

        for (u32 elem_idx = 0; elem_idx < mesh_part->elem_array_size; ++elem_idx) {
            Encas_Elem_Type type = mesh_part->elem_array[elem_idx].type;
            u32 num_of_cells = mesh_part->elem_array[elem_idx].elem_vert_map_size / (u32)mesh_part->elem_array[elem_idx].elem_size;

            num_faces += num_of_cells * Encas_GetCellTrianglesCount(type);
        }

        for (u64 vert_idx = 0; vert_idx < mesh_part->vert_array_size; ++vert_idx) {
            Encas_Vertex *vs = vertices + vert_offset + vert_idx;

            vs->x = mesh_part->vert_array.x[vert_idx];
            vs->y = mesh_part->vert_array.y[vert_idx];
            vs->z = mesh_part->vert_array.z[vert_idx];
        }

        vert_offset += mesh_part->vert_array_size;
    }

    faces = (u32 *)ENCAS_MALLOC(3 * num_faces * sizeof(u32));

    // Fill the face array
    vert_offset = 0;

    u64 face_offset = 0;
    for (u32 part_idx = 0; part_idx < mesh->len; ++part_idx) {
        Encas_Mesh *mesh_part = mesh->elems[part_idx];

        for (u32 elem_idx = 0; elem_idx < mesh_part->elem_array_size; ++elem_idx) {
            Encas_Elem_Type type = mesh_part->elem_array[elem_idx].type;
            u32 num_of_cells = mesh_part->elem_array[elem_idx].elem_vert_map_size / (u32)mesh_part->elem_array[elem_idx].elem_size;

            switch (type) {
                case ENCAS_ELEM_TRIA3:
                    Encas_TriangulateTria3s(mesh_part->elem_vert_map_array + mesh_part->elem_array[elem_idx].elem_vert_map_entry,
                                             num_of_cells, faces, &face_offset, vert_offset);
                    break;
                case ENCAS_ELEM_TETRA4:
                    Encas_TriangulateTetra4s(mesh_part->elem_vert_map_array + mesh_part->elem_array[elem_idx].elem_vert_map_entry,
                                             num_of_cells, faces, &face_offset, vert_offset);
                    break;
                default:
                    break;
            }
        }


        vert_offset += mesh_part->vert_array_size;
    }

    Encas_FaceKeyMap m;
    Encas_CreateFaceKeyMap(&m, next_power_of_two(2 * num_faces));

    for (u64 tria_idx = 0; tria_idx < num_faces; ++tria_idx) {
        Encas_FaceKey f;
        f.v[0] = faces[3 * tria_idx + 0];
        f.v[1] = faces[3 * tria_idx + 1];
        f.v[2] = faces[3 * tria_idx + 2];

        sort3(f.v, f.v + 1, f.v + 2);

        u8 num_tria;
        if (Encas_GetFaceKeyMap(&m, f, &num_tria))
            Encas_SetFaceKeyMap(&m, f, num_tria + 1, tria_idx);
        else
            Encas_SetFaceKeyMap(&m, f, 1, tria_idx);
    }

    u8 *used_vertices = (u8 *)ENCAS_MALLOC(vertices_size * sizeof(u8));
    memset(used_vertices, 0, vertices_size * sizeof(u8));

    u32 new_triangle_count = 0;

    for (u32 i = 0; i < m.cap; ++i) {
        if (m.values[i] == 1) {
            Encas_FaceKey face = m.keys[i];
            used_vertices[face.v[0]] = 1;
            used_vertices[face.v[1]] = 1;
            used_vertices[face.v[2]] = 1;
            ++new_triangle_count;
        }
    }

    u32 *remap = (u32 *)ENCAS_MALLOC(sizeof(u32) * vertices_size);
    u32 new_vertex_count = 0;

    for (u32 i = 0; i < vertices_size; ++i) {
        if (used_vertices[i])
            remap[i] = new_vertex_count++;
    }

    u32 *vbo_orig_idx = (u32 *)ENCAS_MALLOC(new_vertex_count * sizeof(u32));
    new_vertex_count = 0;
    for (u32 i = 0; i < vertices_size; ++i) {
        if (used_vertices[i])
            vbo_orig_idx[new_vertex_count++] = i;
    }

    float *vbo = (float *)ENCAS_MALLOC(3 * new_vertex_count * sizeof(float));
    for (u32 i = 0; i < vertices_size; ++i) {
        if (used_vertices[i]) {
            vbo[3 * remap[i] + 0] = vertices[i].x;
            vbo[3 * remap[i] + 1] = vertices[i].y;
            vbo[3 * remap[i] + 2] = vertices[i].z;
        }
    }

    u32 *visible_triangle_indices = (u32 *)ENCAS_MALLOC(sizeof(u32) * 3 * new_triangle_count);
    params->tria_global_idx = (u32 *)ENCAS_MALLOC(new_triangle_count * sizeof(u32));

    u32 j = 0;

    for (u32 i = 0; i < m.cap; ++i) {
        if (m.values[i] == 1) {
            Encas_FaceKey face = m.keys[i];
            params->tria_global_idx[j / 3] = m.global_indices[i];
            visible_triangle_indices[j++] = remap[face.v[0]];
            visible_triangle_indices[j++] = remap[face.v[1]];
            visible_triangle_indices[j++] = remap[face.v[2]];
        }
    }

    printf("new_vertex_count = %d\n", new_vertex_count);
    printf("new_triangle_count = %d\n", new_triangle_count);

    FILE *f = fopen("ki.obj", "w");
    for (u32 vert_idx = 0; vert_idx < new_vertex_count; ++vert_idx)
        fprintf(f, "v %f %f %f\n", vbo[vert_idx * 3 + 0], vbo[vert_idx * 3 + 1], vbo[vert_idx * 3 + 2]);
    for (u32 tria_idx = 0; tria_idx < new_triangle_count; ++tria_idx)
        fprintf(f, "f %d %d %d\n", visible_triangle_indices[tria_idx * 3 + 0] + 1, visible_triangle_indices[tria_idx * 3 + 1] + 1, visible_triangle_indices[tria_idx * 3 + 2] + 1);
    fclose(f);

    params->vbo = vbo;
    params->vbo_size = new_vertex_count;
    params->vbo_orig_idx = vbo_orig_idx;
    params->ebo = visible_triangle_indices;
    params->ebo_size = new_triangle_count * 3;
    params->global_ebo_size = num_faces;

    ENCAS_FREE(used_vertices);
    ENCAS_FREE(remap);

    Encas_DeleteFaceKeyMap(&m);
    ENCAS_FREE(vertices);
    ENCAS_FREE(faces);
}

ENCAS_API bool Encas_LoadVariableOnShell_Vertices(Encas_Case *encase, Encas_MeshArray *mesh, u32 variable_idx, u32 time_value_idx, Encas_ShellParams *params, float **var_vbo_out) {
    Encas_DescFile *variable = encase->variable->elems[variable_idx];

    // TODO: Load variable data from FlatVariable

    float **var_data = Encas_LoadVariableData(encase, time_value_idx, variable_idx);
    if (var_data == NULL) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Could't load ensight gold variable!\n");
        return false;
    }

    u32 dimension_count = (variable->type == ENCAS_VARIABLE_SCALAR_PER_ELEMENT
                           || variable->type == ENCAS_VARIABLE_SCALAR_PER_NODE) ? 1 : 3;

    // Flatten the variable matrix

    u32 mesh_info_array_idx = time_value_idx;
    if (time_value_idx > encase->geometry->model->num_of_files - 1)
        mesh_info_array_idx = 0;

    Encas_MeshInfo *mesh_info = &encase->geometry->model->mesh_info_array.elems[mesh_info_array_idx];

    u64 vertices_size = 0;
    u64 var_offset = 0;
    for (u32 part_idx = 0; part_idx < mesh_info->len; ++part_idx) {
        Encas_MeshInfoPart *part = mesh_info->parts + part_idx;
        vertices_size += (u64)part->num_of_coords;
    }

    float *local_var_vbo_out = (float *)ENCAS_MALLOC(dimension_count * params->vbo_size * sizeof(float));
    if (variable->type == ENCAS_VARIABLE_SCALAR_PER_NODE) {
        float *vertex_data = (float *)ENCAS_MALLOC(vertices_size * sizeof(float));

        for (u32 part_idx = 0; part_idx < mesh_info->len; ++part_idx) {
            Encas_MeshInfoPart *part = mesh_info->parts + part_idx;
            u64 num_of_coords = part->num_of_coords;

            memcpy(vertex_data + var_offset, var_data[part_idx], num_of_coords * sizeof(float));
            var_offset += num_of_coords;
        }

        for (u32 i = 0; i < params->vbo_size; ++i)
            local_var_vbo_out[i] = vertex_data[params->vbo_orig_idx[i]];


        ENCAS_FREE(vertex_data);
    } else if (variable->type == ENCAS_VARIABLE_VECTOR_PER_NODE) {
        float *vertex_data = (float *)ENCAS_MALLOC(3 * vertices_size * sizeof(float));

        for (u32 part_idx = 0; part_idx < mesh_info->len; ++part_idx) {
            Encas_MeshInfoPart *part = mesh_info->parts + part_idx;
            u64 num_of_coords = part->num_of_coords;

            for (u64 i = 0; i < num_of_coords; ++i) {
                vertex_data[var_offset + vertices_size * 0] = var_data[part_idx][i + 0 * num_of_coords];
                vertex_data[var_offset + vertices_size * 1] = var_data[part_idx][i + 1 * num_of_coords];
                vertex_data[var_offset + vertices_size * 2] = var_data[part_idx][i + 2 * num_of_coords];

                ++var_offset;
            }
        }

        for (u32 i = 0; i < params->vbo_size; ++i) {
            u32 idx = params->vbo_orig_idx[i];
            local_var_vbo_out[i + params->vbo_size * 0] = vertex_data[idx + vertices_size * 0];
            local_var_vbo_out[i + params->vbo_size * 1] = vertex_data[idx + vertices_size * 1];
            local_var_vbo_out[i + params->vbo_size * 2] = vertex_data[idx + vertices_size * 2];
        }

        ENCAS_FREE(vertex_data);
    } else {
        // ENCAS_VARIABLE_SCALAR_PER_ELEMENT or ENCAS_VARIABLE_VECTOR_PER_ELEMENT
        const u32 pool_size = dimension_count * vertices_size * sizeof(float) + vertices_size * sizeof(u32);
        u8 *pool = (u8 *)ENCAS_MALLOC(pool_size);
        float *node_values = (float *)pool;
        u32 *node_counts = (u32 *)(pool + dimension_count * vertices_size * sizeof(float));

        memset(pool, 0, pool_size);

        u32 *coords_offset = (u32 *)ENCAS_MALLOC(mesh_info->len * sizeof(u32));
        coords_offset[0] = 0;
        for (u32 part_idx = 1; part_idx < mesh_info->len; ++part_idx)
            coords_offset[part_idx] = coords_offset[part_idx - 1] + mesh_info->parts[part_idx - 1].num_of_coords;

        if (variable->type == ENCAS_VARIABLE_SCALAR_PER_ELEMENT) {
            for (u32 part_idx = 0; part_idx < mesh_info->len; ++part_idx) {
                float *part_data = var_data[part_idx];
                Encas_Mesh *mpart = mesh->elems[part_idx];

                Encas_MeshInfoPart *mesh_part = &mesh_info->parts[part_idx];

                for (u32 elem_idx = 0; elem_idx < mesh_part->len; ++elem_idx) {
                    u32 vert_cnt = _get_elem_vert_count(mpart->elem_array[elem_idx].type);

                    for (u32 cell_idx = 0; cell_idx < mesh_part->elem_sizes[elem_idx]; ++cell_idx) {
                        const u32 val_idx = mesh_part->elem_offsets[elem_idx] + cell_idx;
                        for (u32 node_idx = 0; node_idx < vert_cnt; ++node_idx) {
                            u32 vert_idx = mpart->elem_vert_map_array[mpart->elem_array[elem_idx].elem_vert_map_entry + cell_idx * vert_cnt + node_idx];
                            const u32 final_idx = vert_idx + coords_offset[part_idx];

                            node_values[final_idx] += part_data[val_idx];
                            ++node_counts[final_idx];
                        }
                    }
                }

            }

            for (u32 i = 0; i < vertices_size; ++i) {
                if (node_counts[i] > 0)
                    node_values[i] /= node_counts[i];
                else
                    node_values[i] = 0.f;
            }

            for (u32 i = 0; i < params->vbo_size; ++i)
                local_var_vbo_out[i] = node_values[params->vbo_orig_idx[i]];
        } else {
            for (u32 part_idx = 0; part_idx < mesh_info->len; ++part_idx) {
                float *part_data = var_data[part_idx];
                Encas_MeshInfoPart *mesh_part = &mesh_info->parts[part_idx];
                u32 num_of_total_cells = mesh_part->elem_offsets[mesh_part->len - 1] + mesh_part->elem_sizes[mesh_part->len - 1];

                Encas_Mesh *mpart = mesh->elems[part_idx];


                for (u32 elem_idx = 0; elem_idx < mesh_part->len; ++elem_idx) {
                    u32 vert_cnt = _get_elem_vert_count(mpart->elem_array[elem_idx].type);

                    for (u32 cell_idx = 0; cell_idx < mesh_part->elem_sizes[elem_idx]; ++cell_idx) {
                        const u32 val_idx = mesh_part->elem_offsets[elem_idx] + cell_idx;
                        for (u32 node_idx = 0; node_idx < vert_cnt; ++node_idx) {
                            u32 vert_idx = mpart->elem_vert_map_array[mpart->elem_array[elem_idx].elem_vert_map_entry + cell_idx * vert_cnt + node_idx];
                            const u32 final_idx = vert_idx + coords_offset[part_idx];

                            node_values[final_idx + vertices_size * 0] += part_data[val_idx + num_of_total_cells * 0];
                            node_values[final_idx + vertices_size * 1] += part_data[val_idx + num_of_total_cells * 1];
                            node_values[final_idx + vertices_size * 2] += part_data[val_idx + num_of_total_cells * 2];
                            ++node_counts[final_idx];
                        }
                    }
                }

            }

            for (u32 i = 0; i < vertices_size; ++i) {
                if (node_counts[i] > 0) {
                    node_values[i + vertices_size * 0] /= node_counts[i];
                    node_values[i + vertices_size * 1] /= node_counts[i];
                    node_values[i + vertices_size * 2] /= node_counts[i];
                }
                else {
                    node_values[i] = 0.f;
                }
            }

            for (u32 i = 0; i < params->vbo_size; ++i) {
                u32 idx = params->vbo_orig_idx[i];
                local_var_vbo_out[i + params->vbo_size * 0] = node_values[idx + vertices_size * 0];
                local_var_vbo_out[i + params->vbo_size * 1] = node_values[idx + vertices_size * 1];
                local_var_vbo_out[i + params->vbo_size * 2] = node_values[idx + vertices_size * 2];
            }

        }


        ENCAS_FREE(coords_offset);
        ENCAS_FREE(pool);
    }

    for (u32 part_idx = 0; part_idx < mesh_info->len; ++part_idx)
        ENCAS_FREE(var_data[part_idx]);

    ENCAS_FREE(var_data);

    *var_vbo_out = local_var_vbo_out;
    return true;
}

ENCAS_API bool Encas_LoadVariableOnShell_Elements(Encas_Case *encase, Encas_MeshArray *mesh, u32 variable_idx, u32 time_value_idx, Encas_ShellParams *params, float **var_vbo_out) {
    Encas_DescFile *variable = encase->variable->elems[variable_idx];

    // TODO: Load variable data from FlatVariable

    float **var_data = Encas_LoadVariableData(encase, time_value_idx, variable_idx);
    if (var_data == NULL) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Could't load ensight gold variable!\n");
        return false;
    }

    u32 dimension_count = (variable->type == ENCAS_VARIABLE_SCALAR_PER_ELEMENT
                           || variable->type == ENCAS_VARIABLE_SCALAR_PER_NODE) ? 1 : 3;

    u32 mesh_info_array_idx = time_value_idx;
    if (time_value_idx > encase->geometry->model->num_of_files - 1)
        mesh_info_array_idx = 0;

    Encas_MeshInfo *mesh_info = &encase->geometry->model->mesh_info_array.elems[mesh_info_array_idx];


    u32 tria_count = params->ebo_size / 3;
    float *local_var_vbo = (float *)ENCAS_MALLOC(dimension_count * tria_count * sizeof(float));
    if (variable->type == ENCAS_VARIABLE_SCALAR_PER_ELEMENT || variable->type == ENCAS_VARIABLE_VECTOR_PER_ELEMENT) {
        u32 num_all_tria = params->global_ebo_size;

        float *tria_var_lookup = (float *)ENCAS_MALLOC(dimension_count * num_all_tria * sizeof(float));

        u32 g_tria_idx = 0;

        if (variable->type == ENCAS_VARIABLE_SCALAR_PER_ELEMENT) {
            for (u32 part_idx = 0; part_idx < mesh->len; ++part_idx) {
                float *part_data = var_data[part_idx];
                Encas_Mesh *mesh_part = mesh->elems[part_idx];
                Encas_MeshInfoPart *minfo_part = &mesh_info->parts[part_idx];

                for (u32 elem_idx = 0; elem_idx < mesh_part->elem_array_size; ++elem_idx) {
                    Encas_Elem *elem = &mesh_part->elem_array[elem_idx];
                    u32 num_cells = elem->elem_vert_map_size / elem->elem_size;
                    u32 num_tria_cell = Encas_GetCellTrianglesCount(elem->type);

                    for (u32 cell_idx = 0; cell_idx < num_cells; ++cell_idx) {
                        u32   val_idx = minfo_part->elem_offsets[elem_idx] + cell_idx;
                        float val = part_data[val_idx];

                        for (u32 tria_idx = 0; tria_idx < num_tria_cell; ++tria_idx)
                            tria_var_lookup[g_tria_idx++] = val;
                    }
                }
            }

            for (u32 tria_idx = 0; tria_idx < tria_count; ++tria_idx) {
                float value = tria_var_lookup[params->tria_global_idx[tria_idx]];
                local_var_vbo[tria_idx] = value;
            }
        } else {
            for (u32 part_idx = 0; part_idx < mesh->len; ++part_idx) {
                float *part_data = var_data[part_idx];
                Encas_Mesh *mesh_part = mesh->elems[part_idx];
                Encas_MeshInfoPart *minfo_part = &mesh_info->parts[part_idx];
                const u32 num_of_total_cells = minfo_part->elem_offsets[minfo_part->len - 1] + minfo_part->elem_sizes[minfo_part->len - 1];

                for (u32 elem_idx = 0; elem_idx < mesh_part->elem_array_size; ++elem_idx) {
                    Encas_Elem *elem = &mesh_part->elem_array[elem_idx];
                    u32 num_cells = elem->elem_vert_map_size / elem->elem_size;
                    u32 num_tria_cell = Encas_GetCellTrianglesCount(elem->type);

                    for (u32 cell_idx = 0; cell_idx < num_cells; ++cell_idx) {
                        u32   val_idx = minfo_part->elem_offsets[elem_idx] + cell_idx;

                        float val_x = part_data[val_idx + num_of_total_cells * 0];
                        float val_y = part_data[val_idx + num_of_total_cells * 1];
                        float val_z = part_data[val_idx + num_of_total_cells * 2];

                        for (u32 tria_idx = 0; tria_idx < num_tria_cell; ++tria_idx) {
                            tria_var_lookup[g_tria_idx * 3 + 0] = val_x;
                            tria_var_lookup[g_tria_idx * 3 + 1] = val_y;
                            tria_var_lookup[g_tria_idx * 3 + 2] = val_z;
                            ++g_tria_idx;
                        }
                    }
                }
            }

            for (u32 tria_idx = 0; tria_idx < tria_count; ++tria_idx) {
                float value_x = tria_var_lookup[params->tria_global_idx[tria_idx] * 3 + 0];
                float value_y = tria_var_lookup[params->tria_global_idx[tria_idx] * 3 + 1];
                float value_z = tria_var_lookup[params->tria_global_idx[tria_idx] * 3 + 2];

                local_var_vbo[tria_idx + tria_count * 0] = value_x;
                local_var_vbo[tria_idx + tria_count * 1] = value_y;
                local_var_vbo[tria_idx + tria_count * 2] = value_z;
            }
        }

        ENCAS_FREE(tria_var_lookup);
    } else {
        u64 vertices_size = 0;
        u64 var_offset = 0;
        for (u32 part_idx = 0; part_idx < mesh_info->len; ++part_idx) {
            Encas_MeshInfoPart *part = mesh_info->parts + part_idx;
            vertices_size += (u64)part->num_of_coords;
        }

        float *vertex_data = (float *)ENCAS_MALLOC(dimension_count * vertices_size * sizeof(float));

        if (variable->type == ENCAS_VARIABLE_SCALAR_PER_NODE) {
            for (u32 part_idx = 0; part_idx < mesh_info->len; ++part_idx) {
                Encas_MeshInfoPart *part = mesh_info->parts + part_idx;
                u64 num_of_coords = part->num_of_coords;

                memcpy(vertex_data + var_offset, var_data[part_idx], num_of_coords * sizeof(float));
                var_offset += num_of_coords;
            }

            for (u32 tria_idx = 0; tria_idx < tria_count; ++tria_idx) {
                local_var_vbo[tria_idx] = (
                    vertex_data[params->vbo_orig_idx[params->ebo[tria_idx * 3 + 0]]] +
                    vertex_data[params->vbo_orig_idx[params->ebo[tria_idx * 3 + 1]]] +
                    vertex_data[params->vbo_orig_idx[params->ebo[tria_idx * 3 + 2]]]
                ) / 3;
            }
        } else {
            for (u32 part_idx = 0; part_idx < mesh_info->len; ++part_idx) {
                Encas_MeshInfoPart *part = mesh_info->parts + part_idx;
                u64 num_of_coords = part->num_of_coords;

                for (u64 i = 0; i < num_of_coords; ++i) {
                    vertex_data[var_offset * 3 + 0] = var_data[part_idx][i + 0 * num_of_coords];
                    vertex_data[var_offset * 3 + 1] = var_data[part_idx][i + 1 * num_of_coords];
                    vertex_data[var_offset * 3 + 2] = var_data[part_idx][i + 2 * num_of_coords];

                    ++var_offset;
                }
            }

            for (u32 tria_idx = 0; tria_idx < tria_count; ++tria_idx) {
                local_var_vbo[tria_idx + tria_count * 0] = (
                    vertex_data[params->vbo_orig_idx[params->ebo[tria_idx * 3 + 0]] * 3 + 0] +
                    vertex_data[params->vbo_orig_idx[params->ebo[tria_idx * 3 + 1]] * 3 + 0] +
                    vertex_data[params->vbo_orig_idx[params->ebo[tria_idx * 3 + 2]] * 3 + 0]
                ) / 3;

                local_var_vbo[tria_idx + tria_count * 1] = (
                    vertex_data[params->vbo_orig_idx[params->ebo[tria_idx * 3 + 0]] * 3 + 1] +
                    vertex_data[params->vbo_orig_idx[params->ebo[tria_idx * 3 + 1]] * 3 + 1] +
                    vertex_data[params->vbo_orig_idx[params->ebo[tria_idx * 3 + 2]] * 3 + 1]
                ) / 3;

                local_var_vbo[tria_idx + tria_count * 2] = (
                    vertex_data[params->vbo_orig_idx[params->ebo[tria_idx * 3 + 0]] * 3 + 2] +
                    vertex_data[params->vbo_orig_idx[params->ebo[tria_idx * 3 + 1]] * 3 + 2] +
                    vertex_data[params->vbo_orig_idx[params->ebo[tria_idx * 3 + 2]] * 3 + 2]
                ) / 3;
            }
        }

        ENCAS_FREE(vertex_data);
    }

    for (u32 part_idx = 0; part_idx < mesh_info->len; ++part_idx)
        ENCAS_FREE(var_data[part_idx]);

    ENCAS_FREE(var_data);

    *var_vbo_out = local_var_vbo;
    return true;
}

ENCAS_API void Encas_DeleteFloatArrParts(float **data, u32 num_of_parts) {
    for (u32 i = 0; i < num_of_parts; ++i)
        ENCAS_FREE(data[i]);

    ENCAS_FREE(data);
}

// num_of_data: 1 for scalar
//              3 for vector
ENCAS_API float **Encas_ReadVariableDataPerElement(Encas_Case *encase, Encas_MeshInfo *mesh_info, char *filename, u32 num_of_data) {
    if (!check_if_file_exists(filename)) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'%s' variable file doesn't exists!\n", filename);
        return NULL;
    }

    Encas_File *f = Encas_SlurpFile(filename);
    float **parts = (float **)ENCAS_MALLOC(mesh_info->len * sizeof(float *));
    if (!parts) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Failed to allocate memory for variable data\n");
        Encas_FreeFile(f);
        return NULL;
    }

    // Initialize parts
    for (u32 i = 0; i < mesh_info->len; ++i) {
        parts[i] = NULL;
    }

    // Skip the description line
    Encas_FileAdvace(f, 80);

    // Parts
    Encas_Str line = Encas_ReadBinaryLine(f);
    while (Encas_Str_StartsWith(line, Encas_Str_Lit("part"))) {
        s32 part_num = Encas_ReadS32(f);

        s32 part_num_idx;
        if (!Encas_SearchHashTable(mesh_info->part_num_lookup, part_num, &part_num_idx)) {
            Encas_Log(ENCAS_LOG_LEVEL_ERROR, "invalid part number found!\n");
            Encas_DeleteFloatArrParts(parts, mesh_info->len);
            Encas_FreeFile(f);
            return NULL;
        }

        u32 alloc_size = 0;
        for (u32 elem_idx = 0; elem_idx < mesh_info->parts[part_num_idx].len; ++elem_idx)
            alloc_size += mesh_info->parts[part_num_idx].elem_sizes[elem_idx];

        alloc_size *= num_of_data;
        parts[part_num_idx] = (float *)ENCAS_MALLOC(alloc_size * sizeof(float));

        u32 data_ptr = 0;
        //parts_data[part_num_idx] = Encas_CreateScalarPerElementArray(mesh_arr->elems[part_num_idx]->elem_array_size);

        Encas_Elem_Type elem_type;
        bool is_ghost = false;

        u32 elem_idx = 0;

        // element type
        while (!IS_ENCAS_EOF(f)) {
            line = Encas_ReadBinaryLine(f);

            if ((elem_type = Encas_ReadElemType(line, &is_ghost)) != ENCAS_ELEM_UNKNOWN) {
                if (is_ghost) {
                    Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Something unexpected happended: ghost elem type found in scalar per element file ('%s')!\n", filename);
                    Encas_DeleteFloatArrParts(parts, mesh_info->len);
                    Encas_FreeFile(f);
                    return NULL;
                }

                if (elem_idx > mesh_info->parts[part_num_idx].len - 1) {
                    Encas_Log(ENCAS_LOG_LEVEL_ERROR, "elem_idx out of range!\n");
                    Encas_DeleteFloatArrParts(parts, mesh_info->len);
                    Encas_FreeFile(f);
                    return NULL;
                }

                u32 num_of_elems = mesh_info->parts[part_num_idx].elem_sizes[elem_idx];
                memcpy(parts[part_num_idx] + data_ptr, f->buffer + f->cur, num_of_elems * num_of_data * sizeof(float));
                Encas_FileAdvace(f, num_of_elems * num_of_data * sizeof(float));

                data_ptr += num_of_elems * num_of_data;
                ++elem_idx;

            } else { break; }
        }
    }

    Encas_FreeFile(f);

    return parts;
}

// num_of_data: 1 for scalar
//              3 for vector
ENCAS_API float *Encas_ReadVariableDataPerElementPart(Encas_Case *encase, Encas_MeshInfo *mesh_info, char *filename, u32 part_idx, u32 num_of_data) {
    if (!encase || !mesh_info || !filename) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Null parameters passed to Encas_ReadVariableDataPerElementPart\n");
        return NULL;
    }

    if (!check_if_file_exists(filename)) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'%s' variable file doesn't exists!\n", filename);
        return NULL;
    }

    if (part_idx > mesh_info->len - 1) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "part_idx is out of range!\n");
        return NULL;
    }

    Encas_File *f = Encas_SlurpFile(filename);
    if (f == NULL) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Failed to open file '%s'\n", filename);
        return NULL;
    }

    u32 alloc_size = 0;
    
    for (u32 i = 0; i < mesh_info->parts[part_idx].len; ++i)
        alloc_size += mesh_info->parts[part_idx].elem_sizes[i];

    alloc_size *= num_of_data;
    float *data = (float *)ENCAS_MALLOC(alloc_size * sizeof(float));
    if (!data) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Failed to allocate memory for variable data\n");
        Encas_FreeFile(f);
        return NULL;
    }

    // Skip the description line
    if (!Encas_FileAdvace(f, 80)) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Failed to skip description line\n");
        ENCAS_FREE(data);
        Encas_FreeFile(f);
        return NULL;
    }

    u32 part_idx_inner = 0;
    
    // Parts
    Encas_Str line = Encas_ReadBinaryLine(f);
    while (Encas_Str_StartsWith(line, Encas_Str_Lit("part"))) {
        s32 part_num = Encas_ReadS32(f);
        bool store = part_idx == part_idx_inner;

        s32 part_num_idx;
        if (!Encas_SearchHashTable(mesh_info->part_num_lookup, part_num, &part_num_idx)) {
            Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Invalid part number %d found!\n", part_num);
            ENCAS_FREE(data);
            Encas_FreeFile(f);
            return NULL;
        }

        u32 data_ptr = 0;

        Encas_Elem_Type elem_type;
        bool is_ghost = false;

        u32 elem_idx = 0;

        // element type
        while (!IS_ENCAS_EOF(f)) {
            line = Encas_ReadBinaryLine(f);
            if (!line.buffer) {
                break;
            }

            if ((elem_type = Encas_ReadElemType(line, &is_ghost)) != ENCAS_ELEM_UNKNOWN) {
                if (is_ghost) {
                    Encas_Log(ENCAS_LOG_LEVEL_ERROR,
                              "Ghost elem type found in scalar per element file '%s'!\n",
                              filename);
                    ENCAS_FREE(data);
                    Encas_FreeFile(f);
                    return NULL;
                }

                if (elem_idx > mesh_info->parts[part_num_idx].len - 1) {
                    Encas_Log(ENCAS_LOG_LEVEL_ERROR, "elem_idx out of range!\n");
                    ENCAS_FREE(data);
                    Encas_FreeFile(f);
                    return NULL;
                }

                u32 num_of_elems = mesh_info->parts[part_num_idx].elem_sizes[elem_idx];
                if (store) {
                    memcpy(data + data_ptr, f->buffer + f->cur, num_of_elems * num_of_data * sizeof(float));
                    data_ptr += num_of_elems * num_of_data;
                }

                Encas_FileAdvace(f, num_of_elems * num_of_data * sizeof(float));

                ++elem_idx;

            } else { break; }
        }

        if (store) {
            Encas_FreeFile(f);
            return data;
        }
    }

    Encas_FreeFile(f);

    Encas_Log(ENCAS_LOG_LEVEL_ERROR, "part with part_idx = %d not found!\n", part_idx);
    return NULL;
}

ENCAS_API float **Encas_ReadVariableDataPerNode(Encas_Case *encase, Encas_MeshInfo *mesh_info, char *filename, u32 num_of_data) {
    if (!check_if_file_exists(filename)) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'%s' variable file doesn't exists!\n", filename);
        return NULL;
    }

    Encas_File *f = Encas_SlurpFile(filename);
    float **parts = (float **)ENCAS_MALLOC(mesh_info->len * sizeof(float *));
    if (!parts) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Failed to allocate memory for variable data\n");
        Encas_FreeFile(f);
        return NULL;
    }

    // Initialize parts
    for (u32 i = 0; i < mesh_info->len; ++i) {
        parts[i] = NULL;
    }

    // Skip the description line
    Encas_FileAdvace(f, 80);

    // Parts
    Encas_Str line = Encas_ReadBinaryLine(f);
    while (Encas_Str_StartsWith(line, Encas_Str_Lit("part"))) {
        s32 part_num = Encas_ReadS32(f);

        s32 part_num_idx;
        if (!Encas_SearchHashTable(mesh_info->part_num_lookup, part_num, &part_num_idx)) {
            Encas_Log(ENCAS_LOG_LEVEL_ERROR, "invalid part number found!\n");
            Encas_DeleteFloatArrParts(parts, mesh_info->len);
            Encas_FreeFile(f);
            return NULL;
        }

        u32 alloc_size = mesh_info->parts[part_num_idx].num_of_coords;
        alloc_size *= num_of_data;
        parts[part_num_idx] = (float *)ENCAS_MALLOC(alloc_size * sizeof(float));

        while (!IS_ENCAS_EOF(f)) {
            line = Encas_ReadBinaryLine(f);
            if (Encas_Str_StartsWith(line, Encas_Str_Lit("coordinates"))) {
                memcpy(parts[part_num_idx], f->buffer + f->cur, alloc_size * sizeof(float));
                Encas_FileAdvace(f, alloc_size * sizeof(float));

            }
            else if (Encas_Str_StartsWith(line, Encas_Str_Lit("block"))) {
                Encas_Log(ENCAS_LOG_LEVEL_ERROR, "block type is not implemented yet\n");
                Encas_DeleteFloatArrParts(parts, mesh_info->len);
                Encas_FreeFile(f);
                return NULL;
            } else { break; }
        }
    }

    Encas_FreeFile(f);
    return parts;
}

ENCAS_API float *Encas_ReadVariableDataPerNodePart(Encas_Case *encase, Encas_MeshInfo *mesh_info, char *filename, u32 part_idx, u32 num_of_data) {
    if (!encase || !mesh_info || !filename) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Null parameters passed to Encas_ReadVariableDataPerNodePart\n");
        return NULL;
    }

    if (!check_if_file_exists(filename)) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "'%s' variable file doesn't exists!\n", filename);
        return NULL;
    }

    if (part_idx > mesh_info->len - 1) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "part_idx is out of range!\n");
        return NULL;
    }

    Encas_File *f = Encas_SlurpFile(filename);
    u32 alloc_size = 0;
    
    alloc_size += mesh_info->parts[part_idx].num_of_coords;

    alloc_size *= num_of_data;
    float *data = (float *)ENCAS_MALLOC(alloc_size * sizeof(float));
    if (!data) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Failed to allocate memory for variable data\n");
        Encas_FreeFile(f);
        return NULL;
    }

    // Skip the description line
    if (!Encas_FileAdvace(f, 80)) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Failed to skip description line\n");
        ENCAS_FREE(data);
        Encas_FreeFile(f);
        return NULL;
    }

    u32 part_idx_inner = 0;
    
    // Parts
    Encas_Str line = Encas_ReadBinaryLine(f);
    while (Encas_Str_StartsWith(line, Encas_Str_Lit("part"))) {
        s32 part_num = Encas_ReadS32(f);
        bool store = part_idx == part_idx_inner;

        s32 part_num_idx;
        if (!Encas_SearchHashTable(mesh_info->part_num_lookup, part_num, &part_num_idx)) {
            Encas_Log(ENCAS_LOG_LEVEL_ERROR, "invalid part number found!\n");
            ENCAS_FREE(data);
            Encas_FreeFile(f);
            return NULL;
        }

        // element type
        while (!IS_ENCAS_EOF(f)) {
            line = Encas_ReadBinaryLine(f);

            if (Encas_Str_StartsWith(line, Encas_Str_Lit("coordinates"))) {
                if (store)
                    memcpy(data, f->buffer + f->cur, alloc_size * sizeof(float));
                Encas_FileAdvace(f, alloc_size * sizeof(float));

            }
            else if (Encas_Str_StartsWith(line, Encas_Str_Lit("block"))) {
                Encas_Log(ENCAS_LOG_LEVEL_ERROR, "block type is not implemented yet\n");
                ENCAS_FREE(data);
                Encas_FreeFile(f);
                return NULL;
            } else { break; }
        }

        if (store) {
            Encas_FreeFile(f);
            return data;
        }
    }

    Encas_FreeFile(f);

    Encas_Log(ENCAS_LOG_LEVEL_ERROR, "part with part_idx = %d not found!\n", part_idx);
    return NULL;
}

ENCAS_API float *Encas_LoadVariableDataPart(Encas_Case *encase, u32 time_value_idx, u32 variable_idx, u32 part_idx) {
    if (variable_idx > encase->variable->len - 1) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "variable_idx out of range (%d > %d)", variable_idx, encase->variable->len - 1);
        return NULL;
    }

    Encas_DescFile *df = encase->variable->elems[variable_idx];
    Encas_MeshInfo *mesh_info = NULL;

    if (encase->geometry->model->num_of_files - 1 < time_value_idx)
        mesh_info = &encase->geometry->model->mesh_info_array.elems[0];
    else
        mesh_info = &encase->geometry->model->mesh_info_array.elems[time_value_idx];

    u32 dirname_length = strlen(encase->dirname);

    char filename[PATH_MAX + 1];
    u32 filename_length = 0;

    memcpy(filename, encase->dirname, dirname_length);
    filename[dirname_length] = '/';
    filename_length += dirname_length + 1;

    if (!df->ts_set) {
        memcpy(filename + filename_length, df->filename.buffer, df->filename.len);
        filename[filename_length + df->filename.len] = '\0';
    }
    else {
        Encas_Time *time = NULL;

        for (u32 time_idx = 0; time_idx < encase->times->len && time == NULL; ++time_idx)
            if (encase->times->elems[time_idx]->time_set_number == df->ts)
                time = encase->times->elems[time_idx];

        if (time == NULL) {
            Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Encas_LoadVariableDataPart: Time with 'time set number = %d' not found!\n", df->ts);
            return NULL;
        }

        s32 asterisk_idx = Encas_MutStr_FindChar(&df->filename, '*');
        if (asterisk_idx == -1) {
            memcpy(filename + filename_length, df->filename.buffer, df->filename.len);
            filename[filename_length + df->filename.len] = '\0';
        }
        else {
            u32 asterisk_count = 1;
            for (u32 i = asterisk_idx + 1; i < df->filename.len && df->filename.buffer[i] == '*'; ++i)
                ++asterisk_count;

            u32 file_num = time->filename_start_number + time->filename_increment * time_value_idx;

            // Copy the first part of the filename (before the asterisks)
            memcpy(filename + filename_length, df->filename.buffer, asterisk_idx);

            // Format the number with leading zeros based on asterisk count
            char tmp[256];
            snprintf(tmp, 256, "%0*d", asterisk_count, file_num);
            u32 tmp_len = strlen(tmp);
            if (tmp_len != asterisk_count) {
                Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Pattern '*' is shorter than the generated number!\n");
                return NULL;
            }

            // Copy the formatted number
            memcpy(filename + filename_length + asterisk_idx, tmp, tmp_len);

            // Copy the rest of the filename (after the asterisks)
            u32 remaining_len = df->filename.len - (asterisk_idx + asterisk_count);
            if (remaining_len > 0) {
                memcpy(filename + filename_length + asterisk_idx + tmp_len,
                       df->filename.buffer + asterisk_idx + asterisk_count,
                       remaining_len);
            }

            // Null-terminate the filename
            filename[filename_length + asterisk_idx + tmp_len + remaining_len] = '\0';
        }
    }

    switch (df->type) {
        case ENCAS_VARIABLE_SCALAR_PER_NODE: {
            return Encas_ReadVariableDataPerNodePart(encase, mesh_info, filename, part_idx, 1);
            break;
        }
        case ENCAS_VARIABLE_VECTOR_PER_NODE: {
            return Encas_ReadVariableDataPerNodePart(encase, mesh_info, filename, part_idx, 3);
            break;
        }
        case ENCAS_VARIABLE_SCALAR_PER_ELEMENT: {
            return Encas_ReadVariableDataPerElementPart(encase, mesh_info, filename, part_idx, 1);
            break;
        }
        case ENCAS_VARIABLE_VECTOR_PER_ELEMENT: {
            return Encas_ReadVariableDataPerElementPart(encase, mesh_info, filename, part_idx, 3);
            break;
        }
        default: {
            break;
        }
    }
    return NULL;
}

ENCAS_API float **Encas_LoadVariableData(Encas_Case *encase, u32 time_value_idx, u32 variable_idx) {
    if (variable_idx > encase->variable->len - 1) {
        Encas_Log(ENCAS_LOG_LEVEL_ERROR, "variable_idx out of range (%d > %d)", variable_idx, encase->variable->len - 1);
        return NULL;
    }

    Encas_DescFile *df = encase->variable->elems[variable_idx];
    Encas_MeshInfo *mesh_info = NULL;

    if (encase->geometry->model->num_of_files - 1 < time_value_idx)
        mesh_info = &encase->geometry->model->mesh_info_array.elems[0];
    else
        mesh_info = &encase->geometry->model->mesh_info_array.elems[time_value_idx];

    u32 dirname_length = strlen(encase->dirname);

    char filename[PATH_MAX + 1];
    u32 filename_length = 0;

    memcpy(filename, encase->dirname, dirname_length);
    filename[dirname_length] = '/';
    filename_length += dirname_length + 1;

    if (!df->ts_set) {
        df->ts = encase->times->elems[0]->time_set_number;
    }

    Encas_Time *time = NULL;

    if (encase->times != NULL)
        for (u32 time_idx = 0; time_idx < encase->times->len && time == NULL; ++time_idx)
            if (encase->times->elems[time_idx]->time_set_number == df->ts)
                time = encase->times->elems[time_idx];


    s32 asterisk_idx = Encas_MutStr_FindChar(&df->filename, '*');
    if (asterisk_idx == -1) {
        memcpy(filename + filename_length, df->filename.buffer, df->filename.len);
        filename[filename_length + df->filename.len] = '\0';
    } else {
        if (time == NULL) {
            Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Encas_LoadVariableData: Time with 'time set number = %d' not found!\n", df->ts);
            return NULL;
        }

        u32 asterisk_count = 1;
        for (u32 i = asterisk_idx + 1; i < df->filename.len && df->filename.buffer[i] == '*'; ++i)
            ++asterisk_count;

        u32 file_num = time->filename_start_number + time->filename_increment * time_value_idx;

        // Copy the first part of the filename (before the asterisks)
        memcpy(filename + filename_length, df->filename.buffer, asterisk_idx);

        // Format the number with leading zeros based on asterisk count
        char tmp[256];
        snprintf(tmp, 256, "%0*d", asterisk_count, file_num);
        u32 tmp_len = strlen(tmp);
        if (tmp_len != asterisk_count) {
            Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Pattern '*' is shorter than the generated number!\n");
            return NULL;
        }

        // Copy the formatted number
        memcpy(filename + filename_length + asterisk_idx, tmp, tmp_len);

        // Copy the rest of the filename (after the asterisks)
        u32 remaining_len = df->filename.len - (asterisk_idx + asterisk_count);
        if (remaining_len > 0) {
            memcpy(filename + filename_length + asterisk_idx + tmp_len,
                   df->filename.buffer + asterisk_idx + asterisk_count,
                   remaining_len);
        }

        // Null-terminate the filename
        filename[filename_length + asterisk_idx + tmp_len + remaining_len] = '\0';
    }

    //printf("filename: %s\n", filename);
    Encas_Log(ENCAS_LOG_LEVEL_INFO, "Filename: %s\n", filename);
    switch (df->type) {
        case ENCAS_VARIABLE_SCALAR_PER_NODE: {
            return Encas_ReadVariableDataPerNode(encase, mesh_info, filename, 1);
            break;
        }
        case ENCAS_VARIABLE_VECTOR_PER_NODE: {
            return Encas_ReadVariableDataPerNode(encase, mesh_info, filename, 3);
            break;
        }
        case ENCAS_VARIABLE_SCALAR_PER_ELEMENT: {
            return Encas_ReadVariableDataPerElement(encase, mesh_info, filename, 1);
            break;
        }
        case ENCAS_VARIABLE_VECTOR_PER_ELEMENT: {
            return Encas_ReadVariableDataPerElement(encase, mesh_info, filename, 3);
            break;
        }
        default: {
            break;
        }
    }
    return NULL;
}

// TODO: split every type to tetrahedrons
ENCAS_API void Encas_MeshArray_To_FlatMesh(Encas_Case *encas, Encas_MeshArray *mesh, Encas_FlatMesh *flat, u32 time_idx, u32 variable_idx) {
    u32 mesh_info_array_idx = time_idx;
    if (time_idx > encas->geometry->model->num_of_files - 1)
        mesh_info_array_idx = 0;

    Encas_MeshInfo *mesh_info = &encas->geometry->model->mesh_info_array.elems[mesh_info_array_idx];

    u64 vertices_size = 0, elem_vert_map_size = 0;
    for (u32 part_idx = 0; part_idx < mesh_info->len; ++part_idx) {
        Encas_MeshInfoPart *part = mesh_info->parts + part_idx;
        vertices_size += (u64)part->num_of_coords;
        elem_vert_map_size += part->elem_vert_map_array_size;
    }

    flat->vertices = (Encas_Vertex *)ENCAS_MALLOC(vertices_size * sizeof(Encas_Vertex));
    flat->elem_vert_map = (u64 *)ENCAS_MALLOC(elem_vert_map_size * sizeof(u64));

    flat->vertices_size = vertices_size;
    flat->elem_vert_map_size = elem_vert_map_size;

    u64 vert_offset = 0, elem_offset = 0;
    for (u32 part_idx = 0; part_idx < mesh->len; ++part_idx) {
        Encas_Mesh *mesh_part = mesh->elems[part_idx];
        u64 elem_size = mesh_info->parts[part_idx].elem_vert_map_array_size;

        for (u64 elem_idx = 0; elem_idx < elem_size; ++elem_idx) {
            flat->elem_vert_map[elem_offset + elem_idx] = (u64)mesh_part->elem_vert_map_array[elem_idx] + vert_offset;
        }
        elem_offset += elem_size;

        for (u64 vert_idx = 0; vert_idx < mesh_part->vert_array_size; ++vert_idx) {
            Encas_Vertex *vs = flat->vertices + vert_offset + vert_idx;

            vs->x = mesh_part->vert_array.x[vert_idx];
            vs->y = mesh_part->vert_array.y[vert_idx];
            vs->z = mesh_part->vert_array.z[vert_idx];
        }

        vert_offset += mesh_part->vert_array_size;
    }

    flat->num_variables = encas->variable->len;
    flat->data = (float **)ENCAS_MALLOC(flat->num_variables * sizeof(float *));
    flat->data_sizes = (u32 *)ENCAS_MALLOC(flat->num_variables * sizeof(u32));

    for (u32 var_idx = 0; var_idx < flat->num_variables; ++var_idx) {

        Encas_DescFile *variable = encas->variable->elems[var_idx];
        u64 data_size;

        switch (variable->type) {
            case ENCAS_VARIABLE_SCALAR_PER_ELEMENT:
                data_size = elem_vert_map_size / 4;
                break;
            case ENCAS_VARIABLE_VECTOR_PER_ELEMENT:
                data_size = elem_vert_map_size / 4 * 3;
                break;
            case ENCAS_VARIABLE_SCALAR_PER_NODE:
                data_size = vertices_size;
                break;
            case ENCAS_VARIABLE_VECTOR_PER_NODE:
                data_size = vertices_size * 3;
                break;
            default:
                data_size = 0;
                break;
        }

        flat->data_sizes[var_idx] = data_size;

        float **var_data = Encas_LoadVariableData(encas, time_idx, var_idx);
        if (var_data == NULL) {
            Encas_Log(ENCAS_LOG_LEVEL_ERROR, "Could't load ensight gold variable!\n");
            exit(EXIT_FAILURE);
        }

        u64 var_offset = 0;
        flat->data[var_idx] = (float *)ENCAS_MALLOC(data_size * sizeof(float));

        if (variable->type == ENCAS_VARIABLE_SCALAR_PER_NODE) {
            for (u32 part_idx = 0; part_idx < mesh->len; ++part_idx) {
                Encas_MeshInfoPart *part = mesh_info->parts + part_idx;
                u64 num_of_coords = part->num_of_coords;

                memcpy(flat->data[var_idx] + var_offset, var_data[part_idx], num_of_coords * sizeof(float));
                var_offset += num_of_coords;
            }
        } else if (variable->type == ENCAS_VARIABLE_VECTOR_PER_NODE) { /* ENCAS_VARIABLE_VECTOR_PER_NODE */
            for (u32 part_idx = 0; part_idx < mesh->len; ++part_idx) {
                Encas_MeshInfoPart *part = mesh_info->parts + part_idx;
                u64 num_of_coords = part->num_of_coords;

                for (u64 i = 0; i < num_of_coords; ++i) {
                    flat->data[var_idx][var_offset + 0] = var_data[part_idx][i + 0 * num_of_coords];
                    flat->data[var_idx][var_offset + 1] = var_data[part_idx][i + 1 * num_of_coords];
                    flat->data[var_idx][var_offset + 2] = var_data[part_idx][i + 2 * num_of_coords];

                    var_offset += 3;
                }
            }
        }

        // Need to interpolate
        else if (variable->type == ENCAS_VARIABLE_SCALAR_PER_ELEMENT) {
            for (u32 part_idx = 0; part_idx < mesh->len; ++part_idx) {
                Encas_MeshInfoPart *part = mesh_info->parts + part_idx;
                u64 num_of_tetras = part->elem_vert_map_array_size / 4;

                memcpy(flat->data[var_idx] + var_offset, var_data[part_idx], num_of_tetras * sizeof(float));
                var_offset += num_of_tetras;
            }
        } else if (variable->type == ENCAS_VARIABLE_VECTOR_PER_ELEMENT) {
            for (u32 part_idx = 0; part_idx < mesh->len; ++part_idx) {
                Encas_MeshInfoPart *part = mesh_info->parts + part_idx;
                u64 num_of_tetras = part->elem_vert_map_array_size / 4;

                for (u64 i = 0; i < num_of_tetras; ++i) {
                    flat->data[var_idx][var_offset + 0] = var_data[part_idx][i + 0 * num_of_tetras];
                    flat->data[var_idx][var_offset + 1] = var_data[part_idx][i + 1 * num_of_tetras];
                    flat->data[var_idx][var_offset + 2] = var_data[part_idx][i + 2 * num_of_tetras];

                    var_offset += 3;
                }
            }
        }

        for (u32 part_idx = 0; part_idx < mesh->len; ++part_idx)
            ENCAS_FREE(var_data[part_idx]);

        ENCAS_FREE(var_data);
    }
}

ENCAS_API void Encas_DeleteFlatMesh(Encas_FlatMesh *flat) {
    ENCAS_FREE(flat->vertices);
    ENCAS_FREE(flat->elem_vert_map);
    ENCAS_FREE(flat->data_sizes);

    for (u32 var_idx = 0; var_idx < flat->num_variables; ++var_idx)
        ENCAS_FREE(flat->data[var_idx]);
    ENCAS_FREE(flat->data);
}

// FNV-1a 64-bit hash
static inline u64 hash_facekey(const Encas_FaceKey *key) {
    const u8 *data = (const u8 *)key;
    u64 hash = 14695981039346656037ULL;
    for (u32 i = 0; i < sizeof(Encas_FaceKey); ++i) {
        hash ^= data[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

ENCAS_API bool Encas_EqualFaceKey(const Encas_FaceKey *a, const Encas_FaceKey *b) {
    return memcmp(a, b, sizeof(Encas_FaceKey)) == 0;
}

ENCAS_API void Encas_CreateFaceKeyMap(Encas_FaceKeyMap *map, u32 cap) {
    map->cap = cap;
    map->len = 0;
    map->keys = (Encas_FaceKey *)ENCAS_MALLOC(map->cap * sizeof(Encas_FaceKey));
    memset(map->keys, 0, map->cap * sizeof(Encas_FaceKey));
    map->values = (u8 *)ENCAS_MALLOC(map->cap * sizeof(u8));
    map->global_indices = (u32 *)ENCAS_MALLOC(map->cap * sizeof(u32));
    memset(map->values, 0, map->cap * sizeof(u8));
}

ENCAS_API void Encas_DeleteFaceKeyMap(Encas_FaceKeyMap *map) {
    ENCAS_FREE(map->keys);
    ENCAS_FREE(map->values);
    ENCAS_FREE(map->global_indices);
    map->keys = NULL;
    map->values = NULL;
    map->global_indices = NULL;
    map->cap = 0;
    map->len = 0;
}

ENCAS_API void Encas_SetFaceKeyMap(Encas_FaceKeyMap *map, Encas_FaceKey key, u8 value, u32 global_idx) {
    if ((float)map->len / map->cap >= LOAD_FACTOR) {
        Encas_RehashFaceKeyMap(map, map->cap * 2);
    }

    u64 hash = hash_facekey(&key);
    u32 index = hash % map->cap;

    while (map->values[index] != 0) {
        if (Encas_EqualFaceKey(&map->keys[index], &key)) {
            map->values[index] = value;
            map->global_indices[index] = global_idx;
            return;
        }
        index = (index + 1) % map->cap;
    }

    map->keys[index] = key;
    map->values[index] = value;
    map->global_indices[index] = global_idx;
    map->len++;
}

ENCAS_API bool Encas_GetFaceKeyMap(Encas_FaceKeyMap *map, Encas_FaceKey key, u8 *out_value) {
    uint64_t hash = hash_facekey(&key);
    uint32_t index = hash % map->cap;

    while (map->values[index] != 0) {
        if (Encas_EqualFaceKey(&map->keys[index], &key)) {
            *out_value = map->values[index];
            return true;
        }
        index = (index + 1) % map->cap;
    }
    return false;
}

ENCAS_API void Encas_RehashFaceKeyMap(Encas_FaceKeyMap *map, u32 new_cap) {
    Encas_FaceKey *old_keys = map->keys;
    u8 *old_values = map->values;
    u32 *old_global_indices = map->global_indices;
    u32 old_cap = map->cap;

    map->keys = (Encas_FaceKey *)ENCAS_MALLOC(new_cap * sizeof(Encas_FaceKey));
    memset(map->keys, 0, new_cap * sizeof(Encas_FaceKey));
    map->values = (u8 *)ENCAS_MALLOC(new_cap * sizeof(u8));
    memset(map->values, 0, new_cap * sizeof(u8));
    map->global_indices = (u32 *)ENCAS_MALLOC(new_cap * sizeof(u32));
    map->cap = new_cap;
    map->len = 0;

    for (u32 i = 0; i < old_cap; ++i) {
        if (old_values[i] != 0) {
            Encas_SetFaceKeyMap(map, old_keys[i], old_values[i], old_global_indices[i]);
        }
    }

    ENCAS_FREE(old_keys);
    ENCAS_FREE(old_values);
}

#endif /* ENCAS_IMPLEMENTATION */
#endif /* ENCAS_H */
