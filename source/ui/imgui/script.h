

#ifndef _METADOT_IMGUICSS_LUA_SCRIPT_H__
#define _METADOT_IMGUICSS_LUA_SCRIPT_H__

#include <string>
#include <vector>

#include "ui/imgui/imgui_script.h"

struct lua_State;
struct luaL_Reg;

namespace ImGuiCSS {
class ImGuiCSS;
class Element;
class RefMapper;

class LuaScriptState : public ScriptState {
public:
    LuaScriptState(lua_State* L);
    virtual ~LuaScriptState();

    /**
     * Runs lua script. Script must return Lua table
     *
     * @param scriptData script string
     */
    void initialize(const char* scriptData);

    /**
     * Creates new script state object from Lua table
     *
     * @param state Lua table
     */
    void initialize(Object state);

    /**
     * Clones script state
     *
     * Creates a new script non-initialized script state which is using the same lua_State
     */
    ScriptState* clone() const;

    /**
     * Get or create object from state object
     */
    MutableObject operator[](const char* key);

    /**
     * Evaluate expression
     *
     * @param str script data
     * @param retval evaluated script converted to string
     */
    void eval(const char* str, std::string* retval = 0, Fields* fields = 0, ScriptState::Context* ctx = 0);

    Object getObject(const char* str, Fields* fields = 0, ScriptState::Context* ctx = 0);

    /**
     * Removes all field listeners
     */
    void removeListeners();

    /**
     * Handle lifecycle callbacks
     */
    void lifecycleCallback(LifecycleCallbackType cb);

    void requested(ScriptState::FieldHash h);

    void requested(const char* field);

private:
    friend class ImGuiCSS;

    void setupState(int ref);

    bool runScript(const char* script, ScriptState::Context* ctx);

    typedef ImVector<ScriptState::FieldHash> FieldAccessLog;

    void setObject(char* key, Object& value, int tableIndex = -1);

    void activateContext(ScriptState::Context* ctx);

    lua_State* mLuaState;
    FieldAccessLog mAccessLog;
    RefMapper* mRefMapper;
    ImGuiCSS* mImVue;
    int mRef;
    int mFuncUpvalue;

    bool mLogAccess;
};

void registerBindings(lua_State* L);
}  // namespace ImGuiCSS

#endif
