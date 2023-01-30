

/*****************************************************************************
clocks.c:
   Module to register the time (seconds) between two events

Design:
   'lprofC_start_timer()' marks the first event
   'lprofC_get_seconds()' gives you the seconds elapsed since the timer
                          was started
*****************************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "profiler_lua.h"

/*
   Here you can choose what time function you are going to use.
   These two defines ('TIMES' and 'CLOCK') correspond to the usage of
   functions times() and clock() respectively.
        Which one is better? It depends on your needs:
                TIMES - returns the clock ticks since the system was up
              (you may use it if you want to measure a system
              delay for a task, like time spent to get input from keyboard)
                CLOCK - returns the clock ticks dedicated to the program
                        (this should be prefered in a multithread system and is
              the default choice)

   note: I guess TIMES don't work for win32
*/

#ifdef TIMES

#include <sys/times.h>

static struct tms t;

#define times(t) times(t)

#else /* ifdef CLOCK */

#define times(t) clock()

#endif

void lprofC_start_timer(clock_t* time_marker) { *time_marker = times(&t); }

static clock_t get_clocks(clock_t time_marker) { return times(&t) - time_marker; }

float lprofC_get_seconds(clock_t time_marker) {
    clock_t clocks;
    clocks = get_clocks(time_marker);
    return (float)clocks / (float)CLOCKS_PER_SEC;
}

/*****************************************************************************
stack.c:
   Simple stack manipulation
*****************************************************************************/

void lprofS_push(lprofS_STACK* p, lprofS_STACK_RECORD r) {
    lprofS_STACK q;
    q = (lprofS_STACK)malloc(sizeof(lprofS_STACK_RECORD));
    *q = r;
    q->next = *p;
    *p = q;
}

lprofS_STACK_RECORD lprofS_pop(lprofS_STACK* p) {
    lprofS_STACK_RECORD r;
    lprofS_STACK q;

    r = **p;
    q = *p;
    *p = (*p)->next;
    free(q);
    return r;
}

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

#ifdef DEBUG
#include <stdlib.h>
#define ASSERT(e, msg)                                                    \
    if (!e) {                                                             \
        fprintf(stdout, "function_meter.c: assertion failed: %s\n", msg); \
        exit(1);                                                          \
    }
#else
#define ASSERT(e, msg)
#endif

/* structures to receive stack elements, declared globals */
/* in the hope they will perform faster                   */
static lprofS_STACK_RECORD newf;      /* used in 'enter_function' */
static lprofS_STACK_RECORD leave_ret; /* used in 'leave_function' */

/* sum the seconds based on the time marker */
static void compute_local_time(lprofS_STACK_RECORD* e) {
    ASSERT(e, "local time null");
    e->local_time += lprofC_get_seconds(e->time_marker_function_local_time);
}

/* sum the seconds based on the time marker */
static void compute_total_time(lprofS_STACK_RECORD* e) {
    ASSERT(e, "total time null");
    e->total_time += lprofC_get_seconds(e->time_marker_function_total_time);
}

/* compute the local time for the current function */
void lprofM_pause_local_time(lprofP_STATE* S) { compute_local_time(S->stack_top); }

/* pause the total timer for all the functions that are in the stack */
void lprofM_pause_total_time(lprofP_STATE* S) {
    lprofS_STACK aux;

    ASSERT(S->stack_top, "pause_total_time: stack_top null");

    /* auxiliary stack */
    aux = S->stack_top;

    /* pause */
    while (aux) {
        compute_total_time(aux);
        aux = aux->next;
    }
}

/* pause the local and total timers for all functions in the stack */
void lprofM_pause_function(lprofP_STATE* S) {

    ASSERT(S->stack_top, "pause_function: stack_top null");

    lprofM_pause_local_time(S);
    lprofM_pause_total_time(S);
}

/* resume the local timer for the current function */
void lprofM_resume_local_time(lprofP_STATE* S) {

    ASSERT(S->stack_top, "resume_local_time: stack_top null");

    /* the function is in the top of the stack */
    lprofC_start_timer(&(S->stack_top->time_marker_function_local_time));
}

/* resume the total timer for all the functions in the stack */
void lprofM_resume_total_time(lprofP_STATE* S) {
    lprofS_STACK aux;

    ASSERT(S->stack_top, "resume_total_time: stack_top null");

    /* auxiliary stack */
    aux = S->stack_top;

    /* resume */
    while (aux) {
        lprofC_start_timer(&(aux->time_marker_function_total_time));
        aux = aux->next;
    }
}

/* resume the local and total timers for all functions in the stack */
void lprofM_resume_function(lprofP_STATE* S) {

    ASSERT(S->stack_top, "resume_function: stack_top null");

    lprofM_resume_local_time(S);
    lprofM_resume_total_time(S);
}

/* the local time for the parent function is paused  */
/* and the local and total time markers are started */
void lprofM_enter_function(lprofP_STATE* S, char* file_defined, char* fcn_name, long linedefined, long currentline) {
    char* prev_name;
    char* cur_name;
    /* the flow has changed to another function: */
    /* pause the parent's function timer timer   */
    if (S->stack_top) {
        lprofM_pause_local_time(S);
        prev_name = S->stack_top->function_name;
    } else
        prev_name = "top level";
    /* measure new function */
    lprofC_start_timer(&(newf.time_marker_function_local_time));
    lprofC_start_timer(&(newf.time_marker_function_total_time));
    newf.file_defined = file_defined;
    if (fcn_name != NULL) {
        newf.function_name = fcn_name;
    } else if (strcmp(file_defined, "=[C]") == 0) {
        cur_name = (char*)malloc(sizeof(char) * (strlen("called from ") + strlen(prev_name) + 1));
        sprintf(cur_name, "called from %s", prev_name);
        newf.function_name = cur_name;
    } else {
        cur_name = (char*)malloc(sizeof(char) * (strlen(file_defined) + 12));
        sprintf(cur_name, "%s:%li", file_defined, linedefined);
        newf.function_name = cur_name;
    }
    newf.line_defined = linedefined;
    newf.current_line = currentline;
    newf.local_time = 0.0;
    newf.total_time = 0.0;
    lprofS_push(&(S->stack_top), newf);
}

/* computes times and remove the top of the stack         */
/* 'isto_resume' specifies if the parent function's timer */
/* should be restarted automatically. If it's false,      */
/* 'resume_local_time()' must be called when the resume   */
/* should be done                                         */
/* returns the funcinfo structure                         */
/* warning: use it before another call to this function,  */
/* because the funcinfo will be overwritten               */
lprofS_STACK_RECORD* lprofM_leave_function(lprofP_STATE* S, int isto_resume) {

    ASSERT(S->stack_top, "leave_function: stack_top null");

    leave_ret = lprofS_pop(&(S->stack_top));
    compute_local_time(&leave_ret);
    compute_total_time(&leave_ret);
    /* resume the timer for the parent function ? */
    if (isto_resume) lprofM_resume_local_time(S);
    return &leave_ret;
}

/* init stack */
lprofP_STATE* lprofM_init() {
    lprofP_STATE* S;
    S = (lprofP_STATE*)malloc(sizeof(lprofP_STATE));
    if (S) {
        S->stack_level = 0;
        S->stack_top = NULL;
        return S;
    } else
        return NULL;
}

/*****************************************************************************
core_profiler.c:
   Lua version independent profiler interface.
   Responsible for handling the "enter function" and "leave function" events
   and for writing the log file.

Design (using the Lua callhook mechanism) :
   'lprofP_init_core_profiler' set up the profile service
   'lprofP_callhookIN'         called whenever Lua enters a function
   'lprofP_callhookOUT'        called whenever Lua leaves a function
*****************************************************************************/

/*****************************************************************************
   The profiled program can be viewed as a graph with the following properties:
directed, multigraph, cyclic and connected. The log file generated by a
profiler section corresponds to a path on this graph.
   There are several graphs for which this path fits on. Some times it is
easier to consider this path as being generated by a simpler graph without
properties like cyclic and multigraph.
   The profiler log file can be viewed as a "reversed" depth-first search
(with the depth-first search number for each vertex) vertex listing of a graph
with the following properties: simple, acyclic, directed and connected, for
which each vertex appears as many times as needed to strip the cycles and
each vertex has an indegree of 1.
   "reversed" depth-first search means that instead of being "printed" before
visiting the vertex's descendents (as done in a normal depth-first search),
the vertex is "printed" only after all his descendents have been processed (in
a depth-first search recursive algorithm).
*****************************************************************************/

/* default log name (%s is used to place a random string) */
#define OUT_FILENAME "lprof_%s.out"

#define MAX_FUNCTION_NAME_LENGTH 20

/* for faster execution (??) */
static FILE* outf;
static lprofS_STACK_RECORD* info;
static float function_call_time;

/* output a line to the log file, using 'printf()' syntax */
/* assume the timer is off */
static void output(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    vfprintf(outf, format, ap);
    va_end(ap);

    /* write now to avoid delays when the timer is on */
    fflush(outf);
}

/* do not allow a string with '\n' and '|' (log file format reserved chars) */
/* - replace them by ' '                                                    */
static void formats(char* s) {
    int i;
    if (!s) return;
    for (i = strlen(s); i >= 0; i--) {
        if ((s[i] == '|') || (s[i] == '\n')) s[i] = ' ';
    }
}

/* computes new stack and new timer */
void lprofP_callhookIN(lprofP_STATE* S, char* func_name, char* file, int linedefined, int currentline) {
    S->stack_level++;
    lprofM_enter_function(S, file, func_name, linedefined, currentline);
}

/* pauses all timers to write a log line and computes the new stack */
/* returns if there is another function in the stack */
int lprofP_callhookOUT(lprofP_STATE* S) {

    if (S->stack_level == 0) {
        return 0;
    }

    S->stack_level--;

    /* 0: do not resume the parent function's timer yet... */
    info = lprofM_leave_function(S, 0);
    /* writing a log may take too long to be computed with the function's time ...*/
    lprofM_pause_total_time(S);
    info->local_time += function_call_time;
    info->total_time += function_call_time;

    char* source = info->file_defined;
    if (source[0] != '@') {
        source = "(string)";
    } else {
        formats(source);
    }
    char* name = info->function_name;

    if (strlen(name) > MAX_FUNCTION_NAME_LENGTH) {
        name = malloc(MAX_FUNCTION_NAME_LENGTH + 10);
        name[0] = '\"';
        strncpy(name + 1, info->function_name, MAX_FUNCTION_NAME_LENGTH);
        name[MAX_FUNCTION_NAME_LENGTH] = '"';
        name[MAX_FUNCTION_NAME_LENGTH + 1] = '\0';
    }
    formats(name);
    output("%d\t%s\t%s\t%d\t%d\t%f\t%f\n", S->stack_level, source, name, info->line_defined, info->current_line, info->local_time, info->total_time);
    /* ... now it's ok to resume the timer */
    if (S->stack_level != 0) {
        lprofM_resume_function(S);
    }

    return 1;
}

/* opens the log file */
/* returns true if the file could be opened */
lprofP_STATE* lprofP_init_core_profiler(const char* _out_filename, int isto_printheader, float _function_call_time) {
    lprofP_STATE* S;
    char auxs[256];
    char* s;
    char* randstr;
    const char* out_filename;

    function_call_time = _function_call_time;
    out_filename = (_out_filename) ? (_out_filename) : (OUT_FILENAME);

    /* the random string to build the logname is extracted */
    /* from 'tmpnam()' (the '/tmp/' part is deleted)     */
    randstr = tmpnam(NULL);
    for (s = strtok(randstr, "/\\"); s; s = strtok(NULL, "/\\")) {
        randstr = s;
    }

    if (randstr[strlen(randstr) - 1] == '.') randstr[strlen(randstr) - 1] = '\0';

    sprintf(auxs, out_filename, randstr);
    outf = fopen(auxs, "a");
    if (!outf) {
        return 0;
    }

    if (isto_printheader) {
        output("stack_level\tfile_defined\tfunction_name\tline_defined\tcurrent_line\tlocal_time\ttotal_time\n");
    }

    /* initialize the 'function_meter' */
    S = lprofM_init();
    if (!S) {
        fclose(outf);
        return 0;
    }

    return S;
}

void lprofP_close_core_profiler(lprofP_STATE* S) {
    if (outf) fclose(outf);
    if (S) free(S);
}

lprofP_STATE* lprofP_create_profiler(float _function_call_time) {
    lprofP_STATE* S;

    function_call_time = _function_call_time;

    /* initialize the 'function_meter' */
    S = lprofM_init();
    if (!S) {
        return 0;
    }

    return S;
}
