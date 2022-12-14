// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_ENGINE_H_
#define _METADOT_ENGINE_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

#include "utils.h"


//Engine functions called from main
int InitEngine();
void EngineUpdate();
void EngineUpdateEnd();
void EndEngine(int errorOcurred);

#endif