/***************************************************************************
 *
 * Copyright (c) 2003 - 2011 Petri Purho
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ***************************************************************************/

#ifndef INC_TIMER_IMPL_H
#define INC_TIMER_IMPL_H

// CTimer requires METADOT_ASSERT_E assert, that can be found in here
#include "Core/DebugImpl.hpp"

namespace ceng {
    namespace impl {
        namespace types {

            // platform dependent stuff
            typedef unsigned int uint32;

#if defined(_MSC_VER)
            typedef signed __int64 int64;
#else
            typedef signed long long int64;
#endif

            typedef uint32 ticks;

        }// namespace types

        types::ticks GetTime();

    }// end of namespace impl
}// end of namespace ceng

#endif