#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/**
The data in SPLIT is allocated
If the seperator is not within str, returns NULL
 */
SPLIT* splitstr(char sep, const char* str, unsigned int len) {
    unsigned int i;

    for (i = 0; i < len; i++) {
        if (sep == str[i]) {
            break;
        };
    }
    if (i == len) {
        return NULL;
    }

    SPLIT* s = NEW(SPLIT);
    ASSERT_ALLOC(s);

    s->prefix = (char*)malloc(i + 2);
    ASSERT_ALLOC(s->prefix);
    memcpy(s->prefix, str, i);
    s->prefix[i] = '\0';

    s->suffix = (char*)malloc((len - i) + 1);
    ASSERT_ALLOC(s->suffix);
    memcpy(s->suffix, &str[i + 1], (len - i));
    s->suffix[(len - i)] = '\0';

    return s;
}

void freesplit(SPLIT* s) {
    free(s->prefix);
    free(s->suffix);
    free(s);
}

static void add_transfer(GLOBAL* gl, CURLM* cm, unsigned int i, int* left) {
    CURL*     eh = curl_easy_init();
    INSTANCE* inst = NEW(INSTANCE);
    ASSERT_ALLOC(inst);

    inst->url = gl->url[i]->url;
    inst->n = i;
    inst->file_path = gl->url[i]->targetPath;

    FILE* fi = fopen(gl->url[i]->targetPath, "wb");
    inst->file = fi;

    curl_easy_setopt(eh, CURLOPT_URL, gl->url[i]->url);
    curl_easy_setopt(eh, CURLOPT_PRIVATE, inst);
    curl_easy_setopt(eh, CURLOPT_WRITEDATA, fi);
    curl_easy_setopt(eh, CURLOPT_TIMEOUT_MS, gl->timeout_ms);

    curl_multi_add_handle(cm, eh);

    (*left)++;
}

inline static void set_timeout_from_arg(GLOBAL* gi, const char* rest) {
    char* endPtr;
#ifdef HAVE_STRTOIMAX
    const intmax_t timeout = strtoimax(rest, &endPtr, 10);
#else
#ifdef HAVE_STRTOL
    const long int timeout = strtol(next, &endPtr, 10);
#else
    const long long timeout = strtoq(next, &endPtr, 10);
#endif
#endif
    if (timeout > INT_MAX) {
        ERROR("Timeout overflow");
        exit(1);
    }
    if (timeout <= 0) {
        ERROR("Timeout underflow");
        exit(1);
    }
    if (endPtr != &rest[strlen(rest)]) {
        ERROR("Unknown characters after timeout specifier");
        exit(1);
    }
    gi->timeout_ms = (int)timeout;
}
inline static void set_parallel_from_arg(GLOBAL* gi, const char* rest) {
    char* endPtr;
#ifdef HAVE_STRTOIMAX
    const intmax_t parallel = strtoimax(rest, &endPtr, 10);
#else
#ifdef HAVE_STRTOL
    const long int timeout = strtol(next, &endPtr, 10);
#else
    const long long timeout = strtoq(next, &endPtr, 10);
#endif
#endif
    if (parallel > 500) {
        ERROR("Parallel overflow");
        exit(1);
    }
    if (parallel <= 0) {
        ERROR("Parallel underflow");
        exit(1);
    }
    if (endPtr != &rest[strlen(rest)]) {
        ERROR("Unknown characters after parallel specifier");
        exit(1);
    }
    gi->max_parallel = (int)parallel;
}

int main(int argc, const char** argv) {
    GLOBAL gm = {0, ARGTYPE_Single, 1000, NULL, 8};

    if (argc == 1) {
        fprintf(stdout, HELP);
        return 1;
    }

    int argSkip;
    for (argSkip = 1; argSkip < argc; argSkip++) {
        int arglen = strlen(argv[argSkip]);
        if (arglen == 0) {
            continue;
        }
        if (argv[argSkip][0] != '-') {
            break;
        }
        const char* rest = &argv[argSkip][1];
        if (argv[argSkip][1] == '-') {
            rest = &argv[argSkip][2];
            if (0 == strcmp(rest, "script-safe")) {
                gm.argType = ARGTYPE_Double;
            } else if (0 == strncmp(rest, "timeout=", 8)) {
                set_timeout_from_arg(&gm, &rest[2]);
            } else if (0 == strncmp(rest, "parallel=", 9)) {
                set_parallel_from_arg(&gm, &rest[2]);
            }
        } else if (0 == strcmp(rest, "s")) {
            gm.argType = ARGTYPE_Double;
        } else if (0 == strncmp(rest, "t=", 2)) {
            set_timeout_from_arg(&gm, &rest[2]);
        } else if (0 == strncmp(rest, "p=", 2)) {
            set_parallel_from_arg(&gm, &rest[2]);
        }
    }

    CURLM*   cm;
    CURLMsg* msg;

    int transfers = 0;
    int msgs_left = -1;
    int left = 0;

    curl_global_init(CURL_GLOBAL_ALL);
    cm = curl_multi_init();

    int amount = MAX(0, argc - argSkip);
    if (gm.argType == ARGTYPE_Double) {
        if (amount % 2 != 0) {
            ERROR("In script-safe mode, number of arguments must be "
                  "an even amount\n");
            return 1;
        }
        amount = amount / 2;
    }

    URLTARGET** urls = (URLTARGET**)malloc(amount * sizeof(void*));
    ASSERT_ALLOC(urls);
    memset(urls, 0, amount * sizeof(void*));

    for (int curr = 0; curr < amount; curr++) {
        if (gm.argType == ARGTYPE_Single) {
            const char* t = argv[curr + argSkip];
            SPLIT*      tar = splitstr('=', t, (unsigned int)strlen(t));
            if (tar == NULL) {
                ERROR("Can't find = in argument");
                for (int i = 0; i < curr; i += 1) {
                    free(urls[i]->targetPath);
                    free(urls[i]->url);
                    free(urls[i]);
                }
                free(urls);
                curl_multi_cleanup(cm);
                curl_global_cleanup();
                return 1;
            }
            ASSERT(tar->prefix != NULL);
            ASSERT(tar->suffix != NULL);
            // printf("%s = %s\n", tar->prefix, tar->suffix);
            URLTARGET* targ = NEW(URLTARGET);
            ASSERT_ALLOC(targ);

            targ->targetPath = tar->prefix;
            targ->url = tar->suffix;
            free(tar);
            urls[curr] = targ;
        } else {
            const char* path = argv[curr * 2 + argSkip];
            const char* url = argv[curr * 2 + argSkip + 1];
            const int   path_len = strlen(path);
            const int   url_len = strlen(url);
            char*       m_path = malloc(path_len + 1);
            ASSERT_ALLOC(m_path);
            memcpy(m_path, path, path_len + 1);
            char* m_url = malloc(url_len + 1);
            ASSERT_ALLOC(m_url);
            memcpy(m_url, url, url_len + 1);

            URLTARGET* targ = NEW(URLTARGET);
            ASSERT_ALLOC(targ);
            targ->targetPath = m_path;
            targ->url = m_url;
            urls[curr] = targ;
        }
    }

    gm.url = urls;
    gm.num_urls = amount;

    curl_multi_setopt(cm, CURLMOPT_MAXCONNECTS, (long)32);

    for (transfers = 0; transfers < gm.max_parallel && transfers < amount; transfers++) {
        add_transfer(&gm, cm, transfers, &left);
    }

    do {
        int still_alive = 1;
        curl_multi_perform(cm, &still_alive);

        while ((msg = curl_multi_info_read(cm, &msgs_left)) != NULL) {
            if (msg->msg == CURLMSG_DONE) {
                INSTANCE* instance;
                CURL*     e = msg->easy_handle;
                curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &instance);
                ERROR("R: %d - %s <%s>", msg->data.result, curl_easy_strerror(msg->data.result), instance->url);
                curl_multi_remove_handle(cm, e);
                curl_easy_cleanup(e);
                fclose(instance->file);
                free(instance);
                left--;
            } else {
                ERROR("E: CURLMsg (%d)", msg->msg);
            }
            if (transfers < amount)
                add_transfer(&gm, cm, transfers++, &left);
        }
        if (left)
            curl_multi_wait(cm, NULL, 0, 1000, NULL);

    } while (left);

    for (int i = 0; i < amount; i += 1) {
        free(urls[i]->targetPath);
        free(urls[i]->url);
        free(urls[i]);
    }
    free(urls);

    curl_multi_cleanup(cm);
    curl_global_cleanup();

    return 0;
}
