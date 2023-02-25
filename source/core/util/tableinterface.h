#ifndef _METADOT_TABLEINTERFACE_H
#define _METADOT_TABLEINTERFACE_H

#include "lua_error.h"
#include "scripting/lua/lua_wrapper.hpp"

class TableInterface {
private:
    lua_State *state;
    int offset;
    int arg;

    std::string lastKey = "unknown";
    int lastArrayIndex = 0;

    void popToStack(std::string key);
    void popToStack(int key);

public:
    TableInterface(lua_State *state, int arg);

    void throwError(std::string desc = "");

    size_t getSize();

    double getNumber(std::string key);
    double getNumber(std::string key, double defaultValue);

    int getInteger(std::string key);
    int getInteger(std::string key, int defaultValue);

    bool getBoolean(std::string key);
    bool getBoolean(std::string key, bool defaultValue);

    std::string getString(std::string key);
    std::string getString(std::string key, std::string defaultValue);

    double getNextNumber();
    int getNextInteger();
    bool getNextBoolean();
    std::string getNextString();
};

#endif  // _METADOT_TABLEINTERFACE_H
