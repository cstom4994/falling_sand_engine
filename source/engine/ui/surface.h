

#ifndef ME_SURFACE_H
#define ME_SURFACE_H

#define ME_SURFACE_PI 3.14159265358979323846264338327f

namespace ME {

typedef struct MEsurface_context MEsurface_context;

struct MEsurface_color {
    union {
        float rgba[4];
        struct {
            float r, g, b, a;
        };
    };
};
typedef struct MEsurface_color MEsurface_color;

struct MEsurface_paint {
    float xform[6];
    float extent[2];
    float radius;
    float feather;
    MEsurface_color innerColor;
    MEsurface_color outerColor;
    int image;
};
typedef struct MEsurface_paint MEsurface_paint;

enum MEsurface_winding {
    ME_SURFACE_CCW = 1,  // Winding for solid shapes
    ME_SURFACE_CW = 2,   // Winding for holes
};

enum MEsurface_solidity {
    ME_SURFACE_SOLID = 1,  // CCW
    ME_SURFACE_HOLE = 2,   // CW
};

enum MEsurface_lineCap {
    ME_SURFACE_BUTT,
    ME_SURFACE_ROUND,
    ME_SURFACE_SQUARE,
    ME_SURFACE_BEVEL,
    ME_SURFACE_MITER,
};

enum MEsurface_align {
    // Horizontal align
    ME_SURFACE_ALIGN_LEFT = 1 << 0,    // Default, align text horizontally to left.
    ME_SURFACE_ALIGN_CENTER = 1 << 1,  // Align text horizontally to center.
    ME_SURFACE_ALIGN_RIGHT = 1 << 2,   // Align text horizontally to right.
    // Vertical align
    ME_SURFACE_ALIGN_TOP = 1 << 3,       // Align text vertically to top.
    ME_SURFACE_ALIGN_MIDDLE = 1 << 4,    // Align text vertically to middle.
    ME_SURFACE_ALIGN_BOTTOM = 1 << 5,    // Align text vertically to bottom.
    ME_SURFACE_ALIGN_BASELINE = 1 << 6,  // Default, align text vertically to baseline.
};

enum MEsurface_blendFactor {
    ME_SURFACE_ZERO = 1 << 0,
    ME_SURFACE_ONE = 1 << 1,
    ME_SURFACE_SRC_COLOR = 1 << 2,
    ME_SURFACE_ONE_MINUS_SRC_COLOR = 1 << 3,
    ME_SURFACE_DST_COLOR = 1 << 4,
    ME_SURFACE_ONE_MINUS_DST_COLOR = 1 << 5,
    ME_SURFACE_SRC_ALPHA = 1 << 6,
    ME_SURFACE_ONE_MINUS_SRC_ALPHA = 1 << 7,
    ME_SURFACE_DST_ALPHA = 1 << 8,
    ME_SURFACE_ONE_MINUS_DST_ALPHA = 1 << 9,
    ME_SURFACE_SRC_ALPHA_SATURATE = 1 << 10,
};

enum MEsurface_compositeOperation {
    ME_SURFACE_SOURCE_OVER,
    ME_SURFACE_SOURCE_IN,
    ME_SURFACE_SOURCE_OUT,
    ME_SURFACE_ATOP,
    ME_SURFACE_DESTINATION_OVER,
    ME_SURFACE_DESTINATION_IN,
    ME_SURFACE_DESTINATION_OUT,
    ME_SURFACE_DESTINATION_ATOP,
    ME_SURFACE_LIGHTER,
    ME_SURFACE_COPY,
    ME_SURFACE_XOR,
};

struct MEsurface_compositeOperationState {
    int srcRGB;
    int dstRGB;
    int srcAlpha;
    int dstAlpha;
};
typedef struct MEsurface_compositeOperationState MEsurface_compositeOperationState;

struct MEsurface_glyphPosition {
    const char* str;   // Position of the glyph in the input string.
    float x;           // The x-coordinate of the logical glyph position.
    float minx, maxx;  // The bounds of the glyph shape.
};
typedef struct MEsurface_glyphPosition MEsurface_glyphPosition;

struct MEsurface_textRow {
    const char* start;  // Pointer to the input text where the row starts.
    const char* end;    // Pointer to the input text where the row ends (one past the last character).
    const char* next;   // Pointer to the beginning of the next row.
    float width;        // Logical width of the row.
    float minx, maxx;   // Actual bounds of the row. Logical with and bounds can differ because of kerning and some parts over extending.
};
typedef struct MEsurface_textRow MEsurface_textRow;

enum MEsurface_imageFlags {
    ME_SURFACE_IMAGE_GENERATE_MIPMAPS = 1 << 0,  // Generate mipmaps during creation of the image.
    ME_SURFACE_IMAGE_REPEATX = 1 << 1,           // Repeat image in X direction.
    ME_SURFACE_IMAGE_REPEATY = 1 << 2,           // Repeat image in Y direction.
    ME_SURFACE_IMAGE_FLIPY = 1 << 3,             // Flips (inverses) image in Y direction when rendered.
    ME_SURFACE_IMAGE_PREMULTIPLIED = 1 << 4,     // Image data has premultiplied alpha.
    ME_SURFACE_IMAGE_NEAREST = 1 << 5,           // Image interpolation is Nearest instead Linear
};

// Begin drawing a new frame
// Calls to engine drawing API should be wrapped in ME_surface_BeginFrame() & ME_surface_EndFrame()
// ME_surface_BeginFrame() defines the size of the window to render to in relation currently
// set viewport (i.e. glViewport on GL backends). Device pixel ration allows to
// control the rendering on Hi-DPI devices.
// For example, GLFW returns two dimension for an opened window: window size and
// frame buffer size. In that case you would set windowWidth/Height to the window size
// devicePixelRatio to: frameBufferWidth / windowWidth.
void ME_surface_BeginFrame(MEsurface_context* ctx, float windowWidth, float windowHeight, float devicePixelRatio);

// Cancels drawing the current frame.
void ME_surface_CancelFrame(MEsurface_context* ctx);

// Ends drawing flushing remaining render state.
void ME_surface_EndFrame(MEsurface_context* ctx);

//
// Composite operation
//
// The composite operations in here are modeled after HTML Canvas API, and
// the blend func is based on OpenGL (see corresponding manuals for more info).
// The colors in the blending state have premultiplied alpha.

// Sets the composite operation. The op parameter should be one of MEsurface_compositeOperation.
void ME_surface_GlobalCompositeOperation(MEsurface_context* ctx, int op);

// Sets the composite operation with custom pixel arithmetic. The parameters should be one of MEsurface_blendFactor.
void ME_surface_GlobalCompositeBlendFunc(MEsurface_context* ctx, int sfactor, int dfactor);

// Sets the composite operation with custom pixel arithmetic for RGB and alpha components separately. The parameters should be one of MEsurface_blendFactor.
void ME_surface_GlobalCompositeBlendFuncSeparate(MEsurface_context* ctx, int srcRGB, int dstRGB, int srcAlpha, int dstAlpha);

//
// Color utils
//

// Returns a color value from red, green, blue values. Alpha will be set to 255 (1.0f).
MEsurface_color ME_surface_RGB(unsigned char r, unsigned char g, unsigned char b);

// Returns a color value from red, green, blue values. Alpha will be set to 1.0f.
MEsurface_color ME_surface_RGBf(float r, float g, float b);

// Returns a color value from red, green, blue and alpha values.
MEsurface_color ME_surface_RGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a);

// Returns a color value from red, green, blue and alpha values.
MEsurface_color ME_surface_RGBAf(float r, float g, float b, float a);

// Linearly interpolates from color c0 to c1, and returns resulting color value.
MEsurface_color ME_surface_LerpRGBA(MEsurface_color c0, MEsurface_color c1, float u);

// Sets transparency of a color value.
MEsurface_color ME_surface_TransRGBA(MEsurface_color c0, unsigned char a);

// Sets transparency of a color value.
MEsurface_color ME_surface_TransRGBAf(MEsurface_color c0, float a);

// Returns color value specified by hue, saturation and lightness.
// HSL values are all in range [0..1], alpha will be set to 255.
MEsurface_color ME_surface_HSL(float h, float s, float l);

// Returns color value specified by hue, saturation and lightness asnd alpha.
// HSL values are all in range [0..1], alpha in range [0..255]
MEsurface_color ME_surface_HSLA(float h, float s, float l, unsigned char a);

//
// State Handling
//
// here contains state which represents how paths will be rendered.
// The state contains transform, fill and stroke styles, text and font styles,
// and scissor clipping.

// Pushes and saves the current render state into a state stack.
// A matching ME_surface_Restore() must be used to restore the state.
void ME_surface_Save(MEsurface_context* ctx);

// Pops and restores current render state.
void ME_surface_Restore(MEsurface_context* ctx);

// Resets current render state to default values. Does not affect the render state stack.
void ME_surface_Reset(MEsurface_context* ctx);

//
// the<engine>().eng()-> styles
//
// Fill and stroke render style can be either a solid color or a paint which is a gradient or a pattern.
// Solid color is simply defined as a color value, different kinds of paints can be created
// using ME_surface_LinearGradient(), ME_surface_BoxGradient(), ME_surface_RadialGradient() and ME_surface_ImagePattern().
//
// Current render style can be saved and restored using ME_surface_Save() and ME_surface_Restore().

// Sets whether to draw antialias for ME_surface_Stroke() and ME_surface_Fill(). It's enabled by default.
void ME_surface_ShapeAntiAlias(MEsurface_context* ctx, int enabled);

// Sets current stroke style to a solid color.
void ME_surface_StrokeColor(MEsurface_context* ctx, MEsurface_color color);

// Sets current stroke style to a paint, which can be a one of the gradients or a pattern.
void ME_surface_StrokePaint(MEsurface_context* ctx, MEsurface_paint paint);

// Sets current fill style to a solid color.
void ME_surface_FillColor(MEsurface_context* ctx, MEsurface_color color);

// Sets current fill style to a paint, which can be a one of the gradients or a pattern.
void ME_surface_FillPaint(MEsurface_context* ctx, MEsurface_paint paint);

// Sets the miter limit of the stroke style.
// Miter limit controls when a sharp corner is beveled.
void ME_surface_MiterLimit(MEsurface_context* ctx, float limit);

// Sets the stroke width of the stroke style.
void ME_surface_StrokeWidth(MEsurface_context* ctx, float size);

// Sets how the end of the line (cap) is drawn,
// Can be one of: ME_SURFACE_BUTT (default), ME_SURFACE_ROUND, ME_SURFACE_SQUARE.
void ME_surface_LineCap(MEsurface_context* ctx, int cap);

// Sets how sharp path corners are drawn.
// Can be one of ME_SURFACE_MITER (default), ME_SURFACE_ROUND, ME_SURFACE_BEVEL.
void ME_surface_LineJoin(MEsurface_context* ctx, int join);

// Sets the transparency applied to all rendered shapes.
// Already transparent paths will get proportionally more transparent as well.
void ME_surface_GlobalAlpha(MEsurface_context* ctx, float alpha);

//
// Transforms
//
// The paths, gradients, patterns and scissor region are transformed by an transformation
// matrix at the time when they are passed to the API.
// The current transformation matrix is a affine matrix:
//   [sx kx tx]
//   [ky sy ty]
//   [ 0  0  1]
// Where: sx,sy define scaling, kx,ky skewing, and tx,ty translation.
// The last row is assumed to be 0,0,1 and is not stored.
//
// Apart from ME_surface_ResetTransform(), each transformation function first creates
// specific transformation matrix and pre-multiplies the current transformation by it.
//
// Current coordinate system (transformation) can be saved and restored using ME_surface_Save() and ME_surface_Restore().

// Resets current transform to a identity matrix.
void ME_surface_ResetTransform(MEsurface_context* ctx);

// Premultiplies current coordinate system by specified matrix.
// The parameters are interpreted as matrix as follows:
//   [a c e]
//   [b d f]
//   [0 0 1]
void ME_surface_Transform(MEsurface_context* ctx, float a, float b, float c, float d, float e, float f);

// Translates current coordinate system.
void ME_surface_Translate(MEsurface_context* ctx, float x, float y);

// Rotates current coordinate system. Angle is specified in radians.
void ME_surface_Rotate(MEsurface_context* ctx, float angle);

// Skews the current coordinate system along X axis. Angle is specified in radians.
void ME_surface_SkewX(MEsurface_context* ctx, float angle);

// Skews the current coordinate system along Y axis. Angle is specified in radians.
void ME_surface_SkewY(MEsurface_context* ctx, float angle);

// Scales the current coordinate system.
void ME_surface_Scale(MEsurface_context* ctx, float x, float y);

// Stores the top part (a-f) of the current transformation matrix in to the specified buffer.
//   [a c e]
//   [b d f]
//   [0 0 1]
// There should be space for 6 floats in the return buffer for the values a-f.
void ME_surface_CurrentTransform(MEsurface_context* ctx, float* xform);

// The following functions can be used to make calculations on 2x3 transformation matrices.
// A 2x3 matrix is represented as float[6].

// Sets the transform to identity matrix.
void ME_surface_TransformIdentity(float* dst);

// Sets the transform to translation matrix matrix.
void ME_surface_TransformTranslate(float* dst, float tx, float ty);

// Sets the transform to scale matrix.
void ME_surface_TransformScale(float* dst, float sx, float sy);

// Sets the transform to rotate matrix. Angle is specified in radians.
void ME_surface_TransformRotate(float* dst, float a);

// Sets the transform to skew-x matrix. Angle is specified in radians.
void ME_surface_TransformSkewX(float* dst, float a);

// Sets the transform to skew-y matrix. Angle is specified in radians.
void ME_surface_TransformSkewY(float* dst, float a);

// Sets the transform to the result of multiplication of two transforms, of A = A*B.
void ME_surface_TransformMultiply(float* dst, const float* src);

// Sets the transform to the result of multiplication of two transforms, of A = B*A.
void ME_surface_TransformPremultiply(float* dst, const float* src);

// Sets the destination to inverse of specified transform.
// Returns 1 if the inverse could be calculated, else 0.
int ME_surface_TransformInverse(float* dst, const float* src);

// Transform a point by given transform.
void ME_surface_TransformPoint(float* dstx, float* dsty, const float* xform, float srcx, float srcy);

// Converts degrees to radians and vice versa.
float ME_surface_DegToRad(float deg);
float ME_surface_RadToDeg(float rad);

//
// Images
//
// here allows you to load jpg, png, psd, tga, pic and gif files to be used for rendering.
// In addition you can upload your own image. The image loading is provided by stb_image.
// The parameter imageFlags is combination of flags defined in MEsurface_imageFlags.

// Creates image by loading it from the disk from specified file name.
// Returns handle to the image.
int ME_surface_CreateImage(MEsurface_context* ctx, const char* filename, int imageFlags);

// Creates image by loading it from the specified chunk of memory.
// Returns handle to the image.
int ME_surface_CreateImageMem(MEsurface_context* ctx, int imageFlags, unsigned char* data, int ndata);

// Creates image from specified image data.
// Returns handle to the image.
int ME_surface_CreateImageRGBA(MEsurface_context* ctx, int w, int h, int imageFlags, const unsigned char* data);

// Updates image data specified by image handle.
void ME_surface_UpdateImage(MEsurface_context* ctx, int image, const unsigned char* data);

// Returns the dimensions of a created image.
void ME_surface_ImageSize(MEsurface_context* ctx, int image, int* w, int* h);

// Deletes created image.
void ME_surface_DeleteImage(MEsurface_context* ctx, int image);

//
// Paints
//
// here supports four types of paints: linear gradient, box gradient, radial gradient and image pattern.
// These can be used as paints for strokes and fills.

// Creates and returns a linear gradient. Parameters (sx,sy)-(ex,ey) specify the start and end coordinates
// of the linear gradient, icol specifies the start color and ocol the end color.
// The gradient is transformed by the current transform when it is passed to ME_surface_FillPaint() or ME_surface_StrokePaint().
MEsurface_paint ME_surface_LinearGradient(MEsurface_context* ctx, float sx, float sy, float ex, float ey, MEsurface_color icol, MEsurface_color ocol);

// Creates and returns a box gradient. Box gradient is a feathered rounded rectangle, it is useful for rendering
// drop shadows or highlights for boxes. Parameters (x,y) define the top-left corner of the rectangle,
// (w,h) define the size of the rectangle, r defines the corner radius, and f feather. Feather defines how blurry
// the border of the rectangle is. Parameter icol specifies the inner color and ocol the outer color of the gradient.
// The gradient is transformed by the current transform when it is passed to ME_surface_FillPaint() or ME_surface_StrokePaint().
MEsurface_paint ME_surface_BoxGradient(MEsurface_context* ctx, float x, float y, float w, float h, float r, float f, MEsurface_color icol, MEsurface_color ocol);

// Creates and returns a radial gradient. Parameters (cx,cy) specify the center, inr and outr specify
// the inner and outer radius of the gradient, icol specifies the start color and ocol the end color.
// The gradient is transformed by the current transform when it is passed to ME_surface_FillPaint() or ME_surface_StrokePaint().
MEsurface_paint ME_surface_RadialGradient(MEsurface_context* ctx, float cx, float cy, float inr, float outr, MEsurface_color icol, MEsurface_color ocol);

// Creates and returns an image pattern. Parameters (ox,oy) specify the left-top location of the image pattern,
// (ex,ey) the size of one image, angle rotation around the top-left corner, image is handle to the image to render.
// The gradient is transformed by the current transform when it is passed to ME_surface_FillPaint() or ME_surface_StrokePaint().
MEsurface_paint ME_surface_ImagePattern(MEsurface_context* ctx, float ox, float oy, float ex, float ey, float angle, int image, float alpha);

//
// Scissoring
//
// Scissoring allows you to clip the rendering into a rectangle. This is useful for various
// user interface cases like rendering a text edit or a timeline.

// Sets the current scissor rectangle.
// The scissor rectangle is transformed by the current transform.
void ME_surface_Scissor(MEsurface_context* ctx, float x, float y, float w, float h);

// Intersects current scissor rectangle with the specified rectangle.
// The scissor rectangle is transformed by the current transform.
// Note: in case the rotation of previous scissor rect differs from
// the current one, the intersection will be done between the specified
// rectangle and the previous scissor rectangle transformed in the current
// transform space. The resulting shape is always rectangle.
void ME_surface_IntersectScissor(MEsurface_context* ctx, float x, float y, float w, float h);

// Reset and disables scissoring.
void ME_surface_ResetScissor(MEsurface_context* ctx);

//
// Paths
//
// Drawing a new shape starts with ME_surface_BeginPath(), it clears all the currently defined paths.
// Then you define one or more paths and sub-paths which describe the shape. The are functions
// to draw common shapes like rectangles and circles, and lower level step-by-step functions,
// which allow to define a path curve by curve.
//
// here uses even-odd fill rule to draw the shapes. Solid shapes should have counter clockwise
// winding and holes should have counter clockwise order. To specify winding of a path you can
// call ME_surface_PathWinding(). This is useful especially for the common shapes, which are drawn CCW.
//
// Finally you can fill the path using current fill style by calling ME_surface_Fill(), and stroke it
// with current stroke style by calling ME_surface_Stroke().
//
// The curve segments and sub-paths are transformed by the current transform.

// Clears the current path and sub-paths.
void ME_surface_BeginPath(MEsurface_context* ctx);

// Starts new sub-path with specified point as first point.
void ME_surface_MoveTo(MEsurface_context* ctx, float x, float y);

// Adds line segment from the last point in the path to the specified point.
void ME_surface_LineTo(MEsurface_context* ctx, float x, float y);

// Adds cubic bezier segment from last point in the path via two control points to the specified point.
void ME_surface_BezierTo(MEsurface_context* ctx, float c1x, float c1y, float c2x, float c2y, float x, float y);

// Adds quadratic bezier segment from last point in the path via a control point to the specified point.
void ME_surface_QuadTo(MEsurface_context* ctx, float cx, float cy, float x, float y);

// Adds an arc segment at the corner defined by the last path point, and two specified points.
void ME_surface_ArcTo(MEsurface_context* ctx, float x1, float y1, float x2, float y2, float radius);

// Closes current sub-path with a line segment.
void ME_surface_ClosePath(MEsurface_context* ctx);

// Sets the current sub-path winding, see MEsurface_winding and MEsurface_solidity.
void ME_surface_PathWinding(MEsurface_context* ctx, int dir);

// Creates new circle arc shaped sub-path. The arc center is at cx,cy, the arc radius is r,
// and the arc is drawn from angle a0 to a1, and swept in direction dir (ME_SURFACE_CCW, or ME_SURFACE_CW).
// Angles are specified in radians.
void ME_surface_Arc(MEsurface_context* ctx, float cx, float cy, float r, float a0, float a1, int dir);

// Creates new rectangle shaped sub-path.
void ME_surface_Rect(MEsurface_context* ctx, float x, float y, float w, float h);

// Creates new rounded rectangle shaped sub-path.
void ME_surface_RoundedRect(MEsurface_context* ctx, float x, float y, float w, float h, float r);

// Creates new rounded rectangle shaped sub-path with varying radii for each corner.
void ME_surface_RoundedRectVarying(MEsurface_context* ctx, float x, float y, float w, float h, float radTopLeft, float radTopRight, float radBottomRight, float radBottomLeft);

// Creates new ellipse shaped sub-path.
void ME_surface_Ellipse(MEsurface_context* ctx, float cx, float cy, float rx, float ry);

// Creates new circle shaped sub-path.
void ME_surface_Circle(MEsurface_context* ctx, float cx, float cy, float r);

// Fills the current path with current fill style.
void ME_surface_Fill(MEsurface_context* ctx);

// Fills the current path with current stroke style.
void ME_surface_Stroke(MEsurface_context* ctx);

//
// Text
//
// here allows you to load .ttf files and use the font to render text.
//
// The appearance of the text can be defined by setting the current text style
// and by specifying the fill color. Common text and font settings such as
// font size, letter spacing and text align are supported. Font blur allows you
// to create simple text effects such as drop shadows.
//
// At render time the font face can be set based on the font handles or name.
//
// Font measure functions return values in local space, the calculations are
// carried in the same resolution as the final rendering. This is done because
// the text glyph positions are snapped to the nearest pixels sharp rendering.
//
// The local space means that values are not rotated or scale as per the current
// transformation. For example if you set font size to 12, which would mean that
// line height is 16, then regardless of the current scaling and rotation, the
// returned line height is always 16. Some measures may vary because of the scaling
// since aforementioned pixel snapping.
//
// While this may sound a little odd, the setup allows you to always render the
// same way regardless of scaling. I.e. following works regardless of scaling:
//
//      const char* txt = "Text me up.";
//      ME_surface_TextBounds(surface, x,y, txt, NULL, bounds);
//      ME_surface_BeginPath(surface);
//      ME_surface_RoundedRect(surface, bounds[0],bounds[1], bounds[2]-bounds[0], bounds[3]-bounds[1]);
//      ME_surface_Fill(surface);
//
// Note: currently only solid color fill is supported for text.

// Creates font by loading it from the disk from specified file name.
// Returns handle to the font.
int ME_surface_CreateFont(MEsurface_context* ctx, const char* name, const char* filename);

// fontIndex specifies which font face to load from a .ttf/.ttc file.
int ME_surface_CreateFontAtIndex(MEsurface_context* ctx, const char* name, const char* filename, const int fontIndex);

// Creates font by loading it from the specified memory chunk.
// Returns handle to the font.
int ME_surface_CreateFontMem(MEsurface_context* ctx, const char* name, unsigned char* data, int ndata, int freeData);

// fontIndex specifies which font face to load from a .ttf/.ttc file.
int ME_surface_CreateFontMemAtIndex(MEsurface_context* ctx, const char* name, unsigned char* data, int ndata, int freeData, const int fontIndex);

// Finds a loaded font of specified name, and returns handle to it, or -1 if the font is not found.
int ME_surface_FindFont(MEsurface_context* ctx, const char* name);

// Adds a fallback font by handle.
int ME_surface_AddFallbackFontId(MEsurface_context* ctx, int baseFont, int fallbackFont);

// Adds a fallback font by name.
int ME_surface_AddFallbackFont(MEsurface_context* ctx, const char* baseFont, const char* fallbackFont);

// Resets fallback fonts by handle.
void ME_surface_ResetFallbackFontsId(MEsurface_context* ctx, int baseFont);

// Resets fallback fonts by name.
void ME_surface_ResetFallbackFonts(MEsurface_context* ctx, const char* baseFont);

// Sets the font size of current text style.
void ME_surface_FontSize(MEsurface_context* ctx, float size);

// Sets the blur of current text style.
void ME_surface_FontBlur(MEsurface_context* ctx, float blur);

// Sets the letter spacing of current text style.
void ME_surface_TextLetterSpacing(MEsurface_context* ctx, float spacing);

// Sets the proportional line height of current text style. The line height is specified as multiple of font size.
void ME_surface_TextLineHeight(MEsurface_context* ctx, float lineHeight);

// Sets the text align of current text style, see MEsurface_align for options.
void ME_surface_TextAlign(MEsurface_context* ctx, int align);

// Sets the font face based on specified id of current text style.
void ME_surface_FontFaceId(MEsurface_context* ctx, int font);

// Sets the font face based on specified name of current text style.
void ME_surface_FontFace(MEsurface_context* ctx, const char* font);

// Draws text string at specified location. If end is specified only the sub-string up to the end is drawn.
float ME_surface_Text(MEsurface_context* ctx, float x, float y, const char* string, const char* end);

// Draws multi-line text string at specified location wrapped at the specified width. If end is specified only the sub-string up to the end is drawn.
// White space is stripped at the beginning of the rows, the text is split at word boundaries or when new-line characters are encountered.
// Words longer than the max width are slit at nearest character (i.e. no hyphenation).
void ME_surface_TextBox(MEsurface_context* ctx, float x, float y, float breakRowWidth, const char* string, const char* end);

// Measures the specified text string. Parameter bounds should be a pointer to float[4],
// if the bounding box of the text should be returned. The bounds value are [xmin,ymin, xmax,ymax]
// Returns the horizontal advance of the measured text (i.e. where the next character should drawn).
// Measured values are returned in local coordinate space.
float ME_surface_TextBounds(MEsurface_context* ctx, float x, float y, const char* string, const char* end, float* bounds);

// Measures the specified multi-text string. Parameter bounds should be a pointer to float[4],
// if the bounding box of the text should be returned. The bounds value are [xmin,ymin, xmax,ymax]
// Measured values are returned in local coordinate space.
void ME_surface_TextBoxBounds(MEsurface_context* ctx, float x, float y, float breakRowWidth, const char* string, const char* end, float* bounds);

// Calculates the glyph x positions of the specified text. If end is specified only the sub-string will be used.
// Measured values are returned in local coordinate space.
int ME_surface_TextGlyphPositions(MEsurface_context* ctx, float x, float y, const char* string, const char* end, MEsurface_glyphPosition* positions, int maxPositions);

// Returns the vertical metrics based on the current text style.
// Measured values are returned in local coordinate space.
void ME_surface_TextMetrics(MEsurface_context* ctx, float* ascender, float* descender, float* lineh);

// Breaks the specified text into lines. If end is specified only the sub-string will be used.
// White space is stripped at the beginning of the rows, the text is split at word boundaries or when new-line characters are encountered.
// Words longer than the max width are slit at nearest character (i.e. no hyphenation).
int ME_surface_TextBreakLines(MEsurface_context* ctx, const char* string, const char* end, float breakRowWidth, MEsurface_textRow* rows, int maxRows);

//
// Internal the<engine>().eng()-> API
//
enum MEsurface_texture {
    ME_SURFACE_TEXTURE_ALPHA = 0x01,
    ME_SURFACE_TEXTURE_RGBA = 0x02,
};

struct MEsurface_scissor {
    float xform[6];
    float extent[2];
};
typedef struct MEsurface_scissor MEsurface_scissor;

struct MEsurface_vertex {
    float x, y, u, v;
};
typedef struct MEsurface_vertex MEsurface_vertex;

struct MEsurface_path {
    int first;
    int count;
    unsigned char closed;
    int nbevel;
    MEsurface_vertex* fill;
    int nfill;
    MEsurface_vertex* stroke;
    int nstroke;
    int winding;
    int convex;
};
typedef struct MEsurface_path MEsurface_path;

struct MEsurface_funcs {
    void* userPtr;
    int edgeAntiAlias;
    int (*renderCreate)(void* uptr);
    int (*renderCreateTexture)(void* uptr, int type, int w, int h, int imageFlags, const unsigned char* data);
    int (*renderDeleteTexture)(void* uptr, int image);
    int (*renderUpdateTexture)(void* uptr, int image, int x, int y, int w, int h, const unsigned char* data);
    int (*renderGetTextureSize)(void* uptr, int image, int* w, int* h);
    void (*renderViewport)(void* uptr, float width, float height, float devicePixelRatio);
    void (*renderCancel)(void* uptr);
    void (*renderFlush)(void* uptr);
    void (*renderFill)(void* uptr, MEsurface_paint* paint, MEsurface_compositeOperationState compositeOperation, MEsurface_scissor* scissor, float fringe, const float* bounds,
                       const MEsurface_path* paths, int npaths);
    void (*renderStroke)(void* uptr, MEsurface_paint* paint, MEsurface_compositeOperationState compositeOperation, MEsurface_scissor* scissor, float fringe, float strokeWidth,
                         const MEsurface_path* paths, int npaths);
    void (*renderTriangles)(void* uptr, MEsurface_paint* paint, MEsurface_compositeOperationState compositeOperation, MEsurface_scissor* scissor, const MEsurface_vertex* verts, int nverts,
                            float fringe);
    void (*renderDelete)(void* uptr);
};
typedef struct MEsurface_funcs MEsurface_funcs;

// Constructor and destructor, called by the render back-end.
MEsurface_context* ME_surface_CreateInternal(MEsurface_funcs* params);
void ME_surface_DeleteInternal(MEsurface_context* ctx);

MEsurface_funcs* ME_surface_InternalParams(MEsurface_context* ctx);

// Debug function to dump cached path data.
void ME_surface_DebugDumpPathCache(MEsurface_context* ctx);

}  // namespace ME

#endif
