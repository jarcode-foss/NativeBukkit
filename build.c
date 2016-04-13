#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <unistd.h>

#include <errno.h>
#include <assert.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#define LSET(S, K, V)                           \
    do {                                        \
        lua_pushstring(S, K);                   \
        lua_pushcfunction(S, V);                \
        lua_settable(S, -3);                    \
    } while (0)

static int c_list(lua_State* state) {
    const char* arg = luaL_checkstring(state, 1);
    DIR* directory = opendir(arg);
    struct dirent *entry;
    if (directory == NULL || errno == ENOENT) {
        return 0;
    }
    int idx = 1;
    lua_newtable(state);
    while ((entry = readdir(directory)) != NULL) {
        if (strcmp("..", entry->d_name) == 0 || strcmp(".", entry->d_name) == 0) {
            continue;
        }
        lua_pushstring(state, entry->d_name);
        lua_rawseti(state, -2, idx);
        ++idx;
    }
    closedir(directory);
    return 1;
}

static const char* dirs[6] = {
    "/usr/bin", "/bin", "/usr/local/bin",   /* always check */
    "/sbin", "/usr/sbin", "/usr/local/sbin" /* if superuser */
};

static int c_findprog(lua_State* state) {
    const char* name = luaL_checkstring(state, 1);
    char a[PATH_MAX];
    bool ret = false;
    size_t t, len = strlen(name), pre;
    struct stat buf;
    
    for (t = 0; getuid() == 0 ? t < 6 : t < 3; ++t) {
        pre = strlen(dirs[t]);
        assert(len + pre <= PATH_MAX - 2);
        memcpy(a, dirs[t], pre);
        a[pre] = '/';
        memcpy(a + pre + 1, name, len);
        a[pre + len + 1] = '\0';
        if (!stat(a, &buf)) {
        st: if (S_ISREG(buf.st_mode)) {
                ret = true;
                break;
            }
            else if (S_ISLNK(buf.st_mode)) { /* resolve symlink */
                realpath(a, a);
                stat(a, &buf);
                goto st;
            }
        }
        else {
            fprintf(stderr, "failed to stat '%s': %s", a, strerror(errno));
            errno = 0;
        }
    }
    
    lua_pushboolean(state, ret);
    return 1;
}

static int c_isfile(lua_State* state) {
    const char* arg = luaL_checkstring(state, 1);
    char buf[PATH_MAX];
    realpath(arg, buf);
    struct stat path_stat;
    if (stat(buf, &path_stat)) {
        lua_pushboolean(state, false);
        return 1;
    }
    lua_pushboolean(state, S_ISREG(path_stat.st_mode));
    return 1;
}

/* because some implemetnations of Lua's os.system do this differently */
static int c_system(lua_State* state) {
    const char* arg = luaL_checkstring(state, 1);
    int status = system(arg);
    lua_pushinteger(state, WEXITSTATUS(status));
    return 1;
}

int entry(lua_State* state) {
    printf("\x1b[32m" "defining native functions..." "\x1b[0m\n");
    lua_newtable(state);
    LSET(state, "list", &c_list);
    LSET(state, "isfile", &c_isfile);
    LSET(state, "findprog", &c_findprog);
    LSET(state, "system", &c_system);
    lua_setglobal(state, "C");
    return 0;
}

