// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_SCRIPTING_HPP_
#define _METADOT_SCRIPTING_HPP_

class LuaLayer;
namespace MuScript {
    class MuScriptInterpreter;
}

struct Scripts
{
    MuScript::MuScriptInterpreter *MuCore = nullptr;
    LuaLayer *LuaCore = nullptr;

    void Init();
    void End();
    void Update();
    void LoadMuFuncs();
};

#endif