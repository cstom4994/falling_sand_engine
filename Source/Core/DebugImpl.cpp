// Copyright(c) 2022, KaoruXun All rights reserved.

#include "DebugImpl.hpp"
#include "Core/Core.hpp"

static const char *date = __DATE__;
static const char *mon[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                              "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static const char mond[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

int MetaDot_buildnum(void) {
    int m = 0, d = 0, y = 0;
    static int b = 0;

    if (b != 0) return b;

    for (m = 0; m < 11; m++) {
        if (!strncmp(&date[0], mon[m], 3)) break;
        d += mond[m];
    }

    d += atoi(&date[4]) - 1;
    y = atoi(&date[7]) - 2000;
    b = d + (int) ((y - 1) * 365.25f);

    if (((y % 4) == 0) && m > 1) { b += 1; }
    b -= 7950;

    return b;
}

const std::string metadata() {
    std::string result;

    result += "Copyright(c) 2022, KaoruXun All rights reserved.\n";
    result += "MetaDot\n";

#ifdef _WIN32
    result += "platform win32\n";
#elif defined __linux__
    result += "platform linux\n";
#elif defined __APPLE__
    result += "platform apple\n";
#elif defined __unix__
    result += "platform unix\n";
#else
    result += "platform unknown\n";
#endif

#if defined(__clang__)
    result += "compiler.family = clang\n";
    result += "compiler.version = ";
    result += __clang_version__;
    result += "\n";
#elif defined(__GNUC__) || defined(__GNUG__)
    result += "compiler.family = gcc\n";
    result += "compiler.version = ";
    result += std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__) + "." +
              std::to_string(__GNUC_PATCHLEVEL__);
    result += "\n";
#elif defined(_MSC_VER)
    result += "compiler.family = msvc\n";
    result += "compiler.version = ";
    result += _MSC_VER;
    result += "\n";
#else
    result += "compiler.family = unknown\n";
    result += "compiler.version = unknown";
    result += "\n";
#endif

#ifdef __cplusplus
    result += "compiler.c++ = ";
    result += std::to_string(__cplusplus);
    result += "\n";
#else
    result += "compiler.c++ = unknown\n";
#endif
    return result;
}

#if 1
DebugDraw::DebugDraw(METAENGINE_Render_Target *target) {
    this->target = target;
    m_drawFlags = 0;
}

DebugDraw::~DebugDraw() {}

void DebugDraw::Create() {}

void DebugDraw::Destroy() {}

b2Vec2 DebugDraw::transform(const b2Vec2 &pt) {
    float x = ((pt.x) * scale + xOfs);
    float y = ((pt.y) * scale + yOfs);
    return b2Vec2(x, y);
}

METAENGINE_Color DebugDraw::convertColor(const b2Color &color) {
    return {(UInt8) (color.r * 255), (UInt8) (color.g * 255), (UInt8) (color.b * 255),
            (UInt8) (color.a * 255)};
}

void DebugDraw::DrawPolygon(const b2Vec2 *vertices, int32 vertexCount, const b2Color &color) {
    b2Vec2 *verts = new b2Vec2[vertexCount];

    for (int i = 0; i < vertexCount; i++) { verts[i] = transform(vertices[i]); }

    // the "(float*)verts" assumes a b2Vec2 is equal to two floats (which it is)
    METAENGINE_Render_Polygon(target, vertexCount, (float *) verts, convertColor(color));

    delete[] verts;
}

void DebugDraw::DrawSolidPolygon(const b2Vec2 *vertices, int32 vertexCount, const b2Color &color) {
    b2Vec2 *verts = new b2Vec2[vertexCount];

    for (int i = 0; i < vertexCount; i++) { verts[i] = transform(vertices[i]); }

    // the "(float*)verts" assumes a b2Vec2 is equal to two floats (which it is)
    METAENGINE_Color c2 = convertColor(color);
    c2.a *= 0.25;
    METAENGINE_Render_PolygonFilled(target, vertexCount, (float *) verts, c2);
    METAENGINE_Render_Polygon(target, vertexCount, (float *) verts, convertColor(color));

    delete[] verts;
}

void DebugDraw::DrawCircle(const b2Vec2 &center, float radius, const b2Color &color) {
    b2Vec2 tr = transform(center);
    METAENGINE_Render_Circle(target, tr.x, tr.y, radius * scale, convertColor(color));
}

void DebugDraw::DrawSolidCircle(const b2Vec2 &center, float radius, const b2Vec2 &axis,
                                const b2Color &color) {
    b2Vec2 tr = transform(center);
    METAENGINE_Render_CircleFilled(target, tr.x, tr.y, radius * scale, convertColor(color));
}

void DebugDraw::DrawSegment(const b2Vec2 &p1, const b2Vec2 &p2, const b2Color &color) {
    b2Vec2 tr1 = transform(p1);
    b2Vec2 tr2 = transform(p2);
    METAENGINE_Render_Line(target, tr1.x, tr1.y, tr2.x, tr2.y, convertColor(color));
}

void DebugDraw::DrawTransform(const b2Transform &xf) {
    const float k_axisScale = 8.0f;
    b2Vec2 p1 = xf.p, p2;
    b2Vec2 tr1 = transform(p1), tr2;

    p2 = p1 + k_axisScale * xf.q.GetXAxis();
    tr2 = transform(p2);
    METAENGINE_Render_Line(target, tr1.x, tr1.y, tr2.x, tr2.y, {0xff, 0x00, 0x00, 0xcc});

    p2 = p1 + k_axisScale * xf.q.GetYAxis();
    tr2 = transform(p2);
    METAENGINE_Render_Line(target, tr1.x, tr1.y, tr2.x, tr2.y, {0x00, 0xff, 0x00, 0xcc});
}

void DebugDraw::DrawPoint(const b2Vec2 &p, float size, const b2Color &color) {
    b2Vec2 tr = transform(p);
    METAENGINE_Render_CircleFilled(target, tr.x, tr.y, 2, convertColor(color));
}

void DebugDraw::DrawString(int x, int y, const char *string, ...) {}

void DebugDraw::DrawString(const b2Vec2 &p, const char *string, ...) {}

void DebugDraw::DrawAABB(b2AABB *aabb, const b2Color &color) {
    b2Vec2 tr1 = transform(aabb->lowerBound);
    b2Vec2 tr2 = transform(aabb->upperBound);
    METAENGINE_Render_Line(target, tr1.x, tr1.y, tr2.x, tr1.y, convertColor(color));
    METAENGINE_Render_Line(target, tr2.x, tr1.y, tr2.x, tr2.y, convertColor(color));
    METAENGINE_Render_Line(target, tr2.x, tr2.y, tr1.x, tr2.y, convertColor(color));
    METAENGINE_Render_Line(target, tr1.x, tr2.y, tr1.x, tr1.y, convertColor(color));
}
#endif