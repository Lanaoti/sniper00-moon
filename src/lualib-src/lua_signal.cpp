#include "lua.hpp"
#ifndef _WIN32
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#endif

#define METANAME "lsignal"

static int lsigqueue(lua_State* L) {
#ifdef _WIN32
    return luaL_error(L, "signal sending not supported on Windows");
#else
    lua_Integer pid = luaL_checkinteger(L, 1);
    lua_Integer sigrt = luaL_checkinteger(L, 2);
    lua_Integer sival = luaL_checkinteger(L, 3);

    union sigval sigInfo;
    sigInfo.sival_int = sival;

    if (sigqueue(static_cast<pid_t>(pid), sigrt, sigInfo) == -1) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, strerror(errno));
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
#endif
}

extern "C"
{
int LUAMOD_API luaopen_signal(lua_State* L) {
    luaL_Reg l[] = {
        { "sigqueue", lsigqueue },
        { nullptr, nullptr }
    };
    luaL_checkversion(L);
    luaL_newlib(L, l);
    return 1;
}
}