#ifndef _METADOT_SEQUENCE_H
#define _METADOT_SEQUENCE_H

#include <cstdlib>

#include "core/util/TableInterface.h"
#include "scripting/lua/lua_wrapper.hpp"

class Sequence {
private:
    size_t size;
    int *data;

    bool loop = false;
    size_t loopPoint = 0;

public:
    explicit Sequence(lua_State *L, int arg = 1);

    virtual ~Sequence();

    size_t getSize() const { return size; }

    int *getData() const { return data; }

    bool doesLoop() const { return loop; }

    size_t getLoopPoint() const { return loopPoint; }
};

#endif  // _METADOT_SEQUENCE_H
