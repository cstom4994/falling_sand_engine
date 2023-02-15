// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_GAMESCRIPTWRAP_HPP_
#define _METADOT_GAMESCRIPTWRAP_HPP_

#include <cstddef>
#include <string>
#include <utility>

#include "engine/engine_scripting.hpp"

struct Biome;

class IGameSystem {
private:
    U32 priority;

public:
    IGameSystem(U32 p = -1) {
        if (p != -1) {
            priority = p;
        }
    };
    ~IGameSystem(){};

    virtual void Create();
    virtual void Destory();

    // Register Lua always been called before Create()
    virtual void RegisterLua(LuaWrapper::State &s_lua);
};

class IGameObject {
public:
    IGameObject(){};
    ~IGameObject(){};

    virtual void Create();
    virtual void Destory();
    virtual void RegisterReflection();
};

class GameplayScriptSystem : public IGameSystem {
public:
    GameplayScriptSystem(U32 p) : IGameSystem(p) {}

    void Create() override;
    void Destory() override;
    void RegisterLua(LuaWrapper::State &s_lua) override;
};

Biome *BiomeGet(std::string name);

#endif