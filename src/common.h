#pragma once
#define VERSION "1.0"

#include <curl/curl.h>
#include <curl/urlapi.h>
#include <inttypes.h>
#include <limits.h>
// #include <math.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"

#define offsetof(type, member) __builtin_offsetof(type, member)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define NEW(T) (T*)malloc(sizeof(T))

#define MAX_PARALLEL 8

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)

#ifdef __GNUC__
#ifdef HAVE_FPRINTF_STDERR
#define ERROR(format, ...) fprintf(stderr, "massget: " format "\n" __VA_OPT__(, ) __VA_ARGS__)
#else
#define ERROR(format, ...) printf("massget: " format "\n" __VA_OPT__(, ) __VA_ARGS__)
#endif
#else
#error "ERROR macro can't be defined on non-GNUC"
#endif

#define MEMORY_FAILURE()                                                                                               \
    {                                                                                                                  \
        ERROR("Can't allocate memory");                                                                                \
        exit(1);                                                                                                       \
    }
#define ASSERT_ALLOC(V)                                                                                                \
    if (V == NULL) {                                                                                                   \
        ERROR("Can't allocate memory");                                                                                \
        exit(1);                                                                                                       \
    }

#define ASSUME(C) __attribute__((assume(C)))
#define ASSERT(C)                                                                                                      \
    {                                                                                                                  \
        if (!((_Bool)(C))) {                                                                                           \
            ERROR("FATAL assertion failed: " #C " in " __FILE__                                                        \
                  ":" STRINGIZE(__LINE__) "\nThis should **NEVER** occur at runtime");                                 \
            exit(1);                                                                                                   \
        }                                                                                                              \
    }

enum ARGTYPE { ARGTYPE_Single, ARGTYPE_Double };
typedef enum ARGTYPE ARGTYPE;

typedef struct {
    char* targetPath;
    char* url;
} URLTARGET;

typedef struct {
    int         num_urls;
    ARGTYPE     argType;
    int         timeout_ms;
    URLTARGET** url;
    int         max_parallel;
} GLOBAL;

typedef struct {
    char* url;
    char* file_path;
    int   n;
    FILE* file;
} INSTANCE;

typedef struct {
    char* prefix;
    char* suffix;
} SPLIT;

#define HELP                                                                                                           \
    "massget " VERSION "\n"                                                                                            \
    "Public domain software\n"                                                                                         \
    "\n"                                                                                                               \
    "Usage:\n"                                                                                                         \
    "  massget [options] (FILEPATH=URL)*\n"                                                                            \
    "  massget [options] [-s/--script-safe] (FILEPATH URL)*\n"                                                         \
    "\n"                                                                                                               \
    "Options:\n"                                                                                                       \
    "-t / --timeout=<ms>: Timeout in milliseconds until failure\n"                                                     \
    "-s / --script-safe : Enable to split by argv as opposed to splitting by\n"                                        \
    "                   : the \"=\" character\n"                                                                       \
    "-p / --parallel    : Maximum requests in parallel\n"                                                              \
    "\n"
