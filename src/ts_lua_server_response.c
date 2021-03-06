
#include "ts_lua_util.h"

static void ts_lua_inject_server_response_header_api(lua_State *L);
static void ts_lua_inject_server_response_header_misc_api(lua_State *L);

static int ts_lua_server_response_header_get(lua_State *L);
static int ts_lua_server_response_header_set(lua_State *L);

static int ts_lua_server_response_header_get_status(lua_State *L);


void
ts_lua_inject_server_response_api(lua_State *L)
{
    lua_newtable(L);

    ts_lua_inject_server_response_header_api(L);

    lua_setfield(L, -2, "server_response");
}


static void
ts_lua_inject_server_response_header_api(lua_State *L)
{
    lua_newtable(L);         /* .header */

    ts_lua_inject_server_response_header_misc_api(L);

    lua_createtable(L, 0, 2);       /* metatable for .header */

    lua_pushcfunction(L, ts_lua_server_response_header_get);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, ts_lua_server_response_header_set);
    lua_setfield(L, -2, "__newindex");

    lua_setmetatable(L, -2);

    lua_setfield(L, -2, "header");
}

static void
ts_lua_inject_server_response_header_misc_api(lua_State *L)
{
    lua_pushcfunction(L, ts_lua_server_response_header_get_status);
    lua_setfield(L, -2, "get_status");
}

static int
ts_lua_server_response_header_get(lua_State *L)
{
    const char  *key;
    const char  *val;
    int         val_len;
    size_t      key_len;

    TSMLoc      field_loc;
    ts_lua_http_ctx  *http_ctx;

    http_ctx = ts_lua_get_http_ctx(L);

    /*  we skip the first argument that is the table */
    key = luaL_checklstring(L, 2, &key_len);

    if (!http_ctx->server_response_hdrp) {
        if (TSHttpTxnServerRespGet(http_ctx->txnp,
                    &http_ctx->server_response_bufp, &http_ctx->server_response_hdrp) != TS_SUCCESS) {

            lua_pushnil(L);
            return 1;
        }
    }

    if (key && key_len) {

        field_loc = TSMimeHdrFieldFind(http_ctx->server_response_bufp, http_ctx->server_response_hdrp, key, key_len);
        if (field_loc) {
            val = TSMimeHdrFieldValueStringGet(http_ctx->server_response_bufp, http_ctx->server_response_hdrp, field_loc, -1, &val_len);
            lua_pushlstring(L, val, val_len);
            TSHandleMLocRelease(http_ctx->server_response_bufp, http_ctx->server_response_hdrp, field_loc);

        } else {
            lua_pushnil(L);
        }

    } else {
        lua_pushnil(L);
    }

    return 1;
}

static int
ts_lua_server_response_header_set(lua_State *L)
{
    const char  *key;
    const char  *val;
    size_t      val_len;
    size_t      key_len;
    int         remove;

    TSMLoc      field_loc;

    ts_lua_http_ctx  *http_ctx;

    http_ctx = ts_lua_get_http_ctx(L);

    remove = 0;
    val = NULL;

    /*   we skip the first argument that is the table */
    key = luaL_checklstring(L, 2, &key_len);
    if (lua_isnil(L, 3)) {
        remove = 1;
    } else {
        val = luaL_checklstring(L, 3, &val_len);
    }

    if (!http_ctx->server_response_hdrp) {
        if (TSHttpTxnServerRespGet(http_ctx->txnp,
                    &http_ctx->server_response_bufp, &http_ctx->server_response_hdrp) != TS_SUCCESS) {
            return 0;
        }
    }

    field_loc = TSMimeHdrFieldFind(http_ctx->server_response_bufp, http_ctx->server_response_hdrp, key, key_len);

    if (remove) {
        if (field_loc) {
            TSMimeHdrFieldDestroy(http_ctx->server_response_bufp, http_ctx->server_response_hdrp, field_loc);
        }

    } else if (field_loc) {
        TSMimeHdrFieldValueStringSet(http_ctx->server_response_bufp, http_ctx->server_response_hdrp, field_loc, 0, val, val_len);

    } else if (TSMimeHdrFieldCreateNamed(http_ctx->server_response_bufp, http_ctx->server_response_hdrp,
                key, key_len, &field_loc) != TS_SUCCESS) {
        TSError("[%s] TSMimeHdrFieldCreateNamed error", __FUNCTION__);
        return 0;

    } else {
        TSMimeHdrFieldValueStringSet(http_ctx->server_response_bufp, http_ctx->server_response_hdrp, field_loc, -1, val, val_len);
        TSMimeHdrFieldAppend(http_ctx->server_response_bufp, http_ctx->server_response_hdrp, field_loc);
    }

    if (field_loc)
        TSHandleMLocRelease(http_ctx->server_response_bufp, http_ctx->server_response_hdrp, field_loc);

    return 0;
}

static int
ts_lua_server_response_header_get_status(lua_State *L)
{
    int              status;
    ts_lua_http_ctx  *http_ctx;

    http_ctx = ts_lua_get_http_ctx(L);

    if (!http_ctx->server_response_hdrp) {
        if (TSHttpTxnServerRespGet(http_ctx->txnp,
                    &http_ctx->server_response_bufp, &http_ctx->server_response_hdrp) != TS_SUCCESS) {

            lua_pushnil(L);
            return 1;
        }
    }

    status = TSHttpHdrStatusGet(http_ctx->server_response_bufp, http_ctx->server_response_hdrp);

    lua_pushinteger(L, status);

    return 1;
}

