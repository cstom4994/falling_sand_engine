// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_MDPLOT_H
#define ME_MDPLOT_H

#include "engine/core/core.hpp"
#include "engine/utils/utility.hpp"
#include "libs/md4c.h"

#define MDPLOT_PARSER_TEST(...) METADOT_BUG(__VA_ARGS__)

typedef struct {
    int id;
    std::vector<std::string> contents;
} plot;

#define BLANK_PLOT                                                                                          \
    {                                                                                                       \
        .id = -1, .contents = { "_placehold1", "_placehold2", "_placehold3", "_placehold4", "_placehold5" } \
    }

struct MarkdownPlot {
private:
    static void process_output(const MD_CHAR *text, MD_SIZE size, void *userdata);
    static int enter_block_callback(MD_BLOCKTYPE type, void * /*detail*/, void * /*userdata*/);
    static int leave_block_callback(MD_BLOCKTYPE type, void * /*detail*/, void * /*userdata*/);
    static int enter_span_callback(MD_SPANTYPE type, void * /*detail*/, void * /*userdata*/);
    static int leave_span_callback(MD_SPANTYPE /*type*/, void * /*detail*/, void * /*userdata*/);
    static int text_callback(MD_TEXTTYPE type, const MD_CHAR *wt, MD_SIZE size, void * /*userdata*/);
    static void debug_log_callback(const char * /*msg*/, void * /* userdata */);

    struct MD_RENDER_HTML {
        void (*process_output)(const MD_CHAR *, MD_SIZE, void *);
        void *userdata;
        unsigned flags;
    };

    struct UserdataPayload {
        std::string content;
    };

public:
    static void parser(std::string test);
    static plot *check(std::string_view title);
    static void reset();

public:
    static int tr;
    static int td;

    static std::vector<plot> plotmap;
};

int test_mdplot();

#endif
