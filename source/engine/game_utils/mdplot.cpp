// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "mdplot.h"

#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

void MarkdownPlot::process_output(const MD_CHAR *text, MD_SIZE size, void *userdata) {
    // see md2html.c (probably, the c++ version will be very different)
    // membuf_append((struct membuffer*) userdata, text, size);
}

int MarkdownPlot::enter_block_callback(MD_BLOCKTYPE type, void * /*detail*/, void * /*userdata*/) {
    plot pl = BLANK_PLOT;
    switch (type) {
        case MD_BLOCK_DOC: /* noop */
            break;
        case MD_BLOCK_QUOTE:
            MDPLOT_PARSER_TEST("<...");
            break;
        case MD_BLOCK_UL:
            MDPLOT_PARSER_TEST("<...");
            break;
        case MD_BLOCK_OL:
            MDPLOT_PARSER_TEST("<...");
            break;
        case MD_BLOCK_LI:
            MDPLOT_PARSER_TEST("<...");
            break;
        case MD_BLOCK_HR:
            MDPLOT_PARSER_TEST("<...");
            break;
        case MD_BLOCK_H:
            MDPLOT_PARSER_TEST("<title");
            break;
        case MD_BLOCK_CODE:
            MDPLOT_PARSER_TEST("<...");
            break;
        case MD_BLOCK_HTML:
            MDPLOT_PARSER_TEST("<...");
            break;
        case MD_BLOCK_P:
            MDPLOT_PARSER_TEST("<p");
            break;
        case MD_BLOCK_TABLE:
            MDPLOT_PARSER_TEST("<table");
            break;
        case MD_BLOCK_THEAD:
            MDPLOT_PARSER_TEST("<table head");
            break;
        case MD_BLOCK_TBODY:
            MDPLOT_PARSER_TEST("<table body");
            break;
        case MD_BLOCK_TR:
            MDPLOT_PARSER_TEST("<...tr");
            plotmap.push_back(pl);
            break;
        case MD_BLOCK_TH:
            MDPLOT_PARSER_TEST("<...th");
            break;
        case MD_BLOCK_TD:
            MDPLOT_PARSER_TEST("<d");
            td++;
            break;
    }
    return 0;
}

int MarkdownPlot::leave_block_callback(MD_BLOCKTYPE type, void * /*detail*/, void * /*userdata*/) {
    switch (type) {
        case MD_BLOCK_DOC: /* noop */
            break;
        case MD_BLOCK_QUOTE:
            MDPLOT_PARSER_TEST(">");
            break;
        case MD_BLOCK_UL:
            MDPLOT_PARSER_TEST(">");
            break;
        case MD_BLOCK_OL:
            MDPLOT_PARSER_TEST(">");
            break;
        case MD_BLOCK_LI:
            MDPLOT_PARSER_TEST(">");
            break;
        case MD_BLOCK_HR:
            MDPLOT_PARSER_TEST(">");
            break;
        case MD_BLOCK_H:
            MDPLOT_PARSER_TEST("title>");
            break;
        case MD_BLOCK_CODE:
            MDPLOT_PARSER_TEST(">");
            break;
        case MD_BLOCK_HTML:
            MDPLOT_PARSER_TEST(">");
            break;
        case MD_BLOCK_P:
            MDPLOT_PARSER_TEST("p>");
            break;
        case MD_BLOCK_TABLE:
            MDPLOT_PARSER_TEST("table>");
            break;
        case MD_BLOCK_THEAD:
            MDPLOT_PARSER_TEST("table head>");
            break;
        case MD_BLOCK_TBODY:
            MDPLOT_PARSER_TEST("table body>");
            break;
        case MD_BLOCK_TR:
            MDPLOT_PARSER_TEST("tr>");
            tr++;
            td = 0;
            break;
        case MD_BLOCK_TH:
            MDPLOT_PARSER_TEST("th>");
            break;
        case MD_BLOCK_TD:
            MDPLOT_PARSER_TEST("d>");
            break;
    }
    return 0;
}

int MarkdownPlot::enter_span_callback(MD_SPANTYPE type, void * /*detail*/, void * /*userdata*/) {
    switch (type) {
        case MD_SPAN_EM:
            MDPLOT_PARSER_TEST("<em...");
            break;
        case MD_SPAN_STRONG:
            MDPLOT_PARSER_TEST("<strong...");
            break;
        case MD_SPAN_A:
            MDPLOT_PARSER_TEST("<...");
            break;
        case MD_SPAN_IMG:
            MDPLOT_PARSER_TEST("<...");
            break;
        case MD_SPAN_CODE:
            MDPLOT_PARSER_TEST("<...");
            break;
        case MD_SPAN_DEL:
        case MD_SPAN_LATEXMATH:
        case MD_SPAN_LATEXMATH_DISPLAY:
        case MD_SPAN_WIKILINK:
        case MD_SPAN_U:
            break;
    }

    return 0;
}

int MarkdownPlot::leave_span_callback(MD_SPANTYPE /*type*/, void * /*detail*/, void * /*userdata*/) { return 0; }

int MarkdownPlot::text_callback(MD_TEXTTYPE type, const MD_CHAR *wt, MD_SIZE size, void * /*userdata*/) {
    std::string text = std::string(wt, wt + size);
    switch (type) {
        case MD_TEXT_NORMAL:
            MDPLOT_PARSER_TEST(text.c_str());
            if (td) {
                plot &pl = plotmap[tr];
                pl.contents[td - 1] = text;
                pl.id = tr;
            }
            break;
        case MD_TEXT_NULLCHAR:
            MDPLOT_PARSER_TEST("...");
            break;
        case MD_TEXT_BR:
            MDPLOT_PARSER_TEST("...");
            break;
        case MD_TEXT_SOFTBR:
            MDPLOT_PARSER_TEST("...");
            break;
        case MD_TEXT_HTML:
            MDPLOT_PARSER_TEST("html %s", text.c_str());
            break;
        case MD_TEXT_ENTITY:
            MDPLOT_PARSER_TEST("...");
            break;
        default:
            td = 0;
            break;
    }
    return 0;
}

void MarkdownPlot::debug_log_callback(const char * /*msg*/, void * /* userdata */) {}

void MarkdownPlot::parser(std::string test) {
    unsigned parser_flags = MD_DIALECT_GITHUB;
    unsigned renderer_flags = 0;

    MD_RENDERER renderer = {.flags = parser_flags,
                            .enter_block = enter_block_callback,
                            .leave_block = leave_block_callback,
                            .enter_span = enter_span_callback,
                            .leave_span = leave_span_callback,
                            .text = text_callback,
                            .debug_log = debug_log_callback};

    UserdataPayload userdata{"userdata payload"};
    MD_RENDER_HTML render = {process_output, (void *)&userdata, renderer_flags};
    int status = md_parse(test.c_str(), test.size(), &renderer, (void *)&render);
}

plot *MarkdownPlot::check(std::string_view title) {
    for (auto &v : plotmap) {
        if (v.contents[0] == title) return &v;
    }
    return nullptr;
}

void MarkdownPlot::reset() {
    tr = 0;
    td = 0;
    plotmap.clear();
}

int MarkdownPlot::tr = 0;
int MarkdownPlot::td = 0;

std::vector<plot> MarkdownPlot::plotmap = {};

int test_mdplot() {

    std::string test = R"(
# Table
t1|t2|t3|t4
---|:---|:---|:---
c1_1|如果你发现这个世界的真相|yes|no
c1_2|你选了yes|yes|no
c1_3|你选了no|yes|no
)";

    int line_id = 0;
    int line_text = 1;

    MarkdownPlot::parser(test);

    MDPLOT_PARSER_TEST("------------------------------------");

    for (auto v : MarkdownPlot::plotmap) {
        if (v.id <= 0) continue;
        MDPLOT_PARSER_TEST("%d ", v.id);
        for (auto p : v.contents) {
            MDPLOT_PARSER_TEST("%s ", p.c_str());
        }
        MDPLOT_PARSER_TEST("");
    }
    MDPLOT_PARSER_TEST("------------------------------------");

    MDPLOT_PARSER_TEST(MarkdownPlot::check("c1_1")->contents[1].c_str());

    return 0;
}
