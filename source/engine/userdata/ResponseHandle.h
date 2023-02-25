#pragma once

#include <sstream>

struct lua_State;

namespace MetaEngine::net {
    class ResponseHandle {
    public:
        explicit ResponseHandle(std::stringstream *stream);

        static void initMetatable(lua_State *L);

        ResponseHandle **constructUserdata(lua_State *L);
        std::string readAll();

        std::stringstream *stream;
    };
}
