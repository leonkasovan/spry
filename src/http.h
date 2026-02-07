#pragma once

struct lua_State;

void open_http_api(lua_State *L);

// call once at shutdown to free TLS library handles
void http_shutdown(void);
