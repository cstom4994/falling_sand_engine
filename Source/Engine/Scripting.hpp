// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_SCRIPTING_HPP_
#define _METADOT_SCRIPTING_HPP_

#include <map>
#include <string>

class LuaCore;
namespace MuDSL {
    class MuDSLInterpreter;
}

struct Scripts
{
    MuDSL::MuDSLInterpreter *MuDSL = nullptr;
    std::map<std::string, LuaCore*> LuaMap;

    void Init();
    void End();
    void Update();
    void LoadMuFuncs();
};

#endif