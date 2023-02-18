

/*****************************************************************************
clocks.h:
   Module to register the time (seconds) between two events

Design:
   'lprofC_start_timer()' marks the first event
   'lprofC_get_seconds()' gives you the seconds elapsed since the timer
                          was started
*****************************************************************************/

#ifndef _METADOT_PROFILER_LUA_H_
#define _METADOT_PROFILER_LUA_H_

#include <time.h>

#include "scripting/lua/lua_wrapper.h"

void lprofC_start_timer(clock_t* time_marker);
float lprofC_get_seconds(clock_t time_marker);

typedef struct lprofS_sSTACK_RECORD lprofS_STACK_RECORD;

struct lprofS_sSTACK_RECORD {
    clock_t time_marker_function_local_time;
    clock_t time_marker_function_total_time;
    char* file_defined;
    char* function_name;
    char* source_code;
    long line_defined;
    long current_line;
    float local_time;
    float total_time;
    lprofS_STACK_RECORD* next;
};

typedef lprofS_STACK_RECORD* lprofS_STACK;

typedef struct lprofP_sSTATE lprofP_STATE;

struct lprofP_sSTATE {
    int stack_level;
    lprofS_STACK stack_top;
};

void lprofS_push(lprofS_STACK* p, lprofS_STACK_RECORD r);
lprofS_STACK_RECORD lprofS_pop(lprofS_STACK* p);

/*****************************************************************************
function_meter.c:
   Module to compute the times for functions (local times and total times)

Design:
   'lprofM_init'            set up the function times meter service
   'lprofM_enter_function'  called when the function stack increases one level
   'lprofM_leave_function'  called when the function stack decreases one level

   'lprofM_resume_function'   called when the profiler is returning from a time
                              consuming task
   'lprofM_resume_total_time' idem
   'lprofM_resume_local_time' called when a child function returns the execution
                              to it's caller (current function)
   'lprofM_pause_function'    called when the profiler need to do things that
                              may take too long (writing a log, for example)
   'lprofM_pause_total_time'  idem
   'lprofM_pause_local_time'  called when the current function has called
                              another one or when the function terminates
*****************************************************************************/

/* compute the local time for the current function */
void lprofM_pause_local_time(lprofP_STATE* S);

/* pause the total timer for all the functions that are in the stack */
void lprofM_pause_total_time(lprofP_STATE* S);

/* pause the local and total timers for all functions in the stack */
void lprofM_pause_function(lprofP_STATE* S);

/* resume the local timer for the current function */
void lprofM_resume_local_time(lprofP_STATE* S);

/* resume the total timer for all the functions in the stack */
void lprofM_resume_total_time(lprofP_STATE* S);

/* resume the local and total timers for all functions in the stack */
void lprofM_resume_function(lprofP_STATE* S);

/* the local time for the parent function is paused */
/* and the local and total time markers are started */
void lprofM_enter_function(lprofP_STATE* S, char* file_defined, char* fcn_name, long linedefined, long currentline);

/* computes times and remove the top of the stack         */
/* 'isto_resume' specifies if the parent function's timer */
/* should be restarted automatically. If it's false,      */
/* 'resume_local_time()' must be called when the resume   */
/* should be done                                         */
/* returns the funcinfo structure                         */
/* warning: use it before another call to this function,  */
/* because the funcinfo will be overwritten               */
lprofS_STACK_RECORD* lprofM_leave_function(lprofP_STATE* S, int isto_resume);

/* init stack */
lprofP_STATE* lprofM_init();

/*****************************************************************************
core_profiler.h:
   Lua version independent profiler interface.
   Responsible for handling the "enter function" and "leave function" events
   and for writing the log file.

Design (using the Lua callhook mechanism) :
   'lprofP_init_core_profiler' set up the profile service
   'lprofP_callhookIN'         called whenever Lua enters a function
   'lprofP_callhookOUT'        called whenever Lua leaves a function
*****************************************************************************/

/* computes new stack and new timer */
void lprofP_callhookIN(lprofP_STATE* S, char* func_name, char* file, int linedefined, int currentline);

/* pauses all timers to write a log line and computes the new stack */
/* returns if there is another function in the stack */
int lprofP_callhookOUT(lprofP_STATE* S);

/* opens the log file */
/* returns true if the file could be opened */
lprofP_STATE* lprofP_init_core_profiler(const char* _out_filename, int isto_printheader, float _function_call_time);

/*****************************************************************************
luaprofiler.h:
    Must be included by your main module, in order to profile Lua programs
*****************************************************************************/

void init_profiler(void*);

#ifdef __cplusplus
extern "C" {
#endif

int metadot_bind_profiler(lua_State* L);

#ifdef __cplusplus
}
#endif

#endif
