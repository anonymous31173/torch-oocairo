#include <cairo.h>
#include <lua.h>
#include <lauxlib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#if CAIRO_VERSION < CAIRO_VERSION_ENCODE(1, 6, 0)
#error "This Lua binding requires Cairo version 1.6 or better."
#endif

int luaopen_oocairo (lua_State *L);

#define MT_NAME_CONTEXT  ("6404c570-6711-11dd-b66f-00e081225ce5")
#define MT_NAME_FONTFACE ("ee272774-6a1e-11dd-86de-00e081225ce5")
#define MT_NAME_MATRIX   ("6e2f4c64-6711-11dd-acfc-00e081225ce5")
#define MT_NAME_PATH     ("6d83bf34-6711-11dd-b4c2-00e081225ce5")
#define MT_NAME_PATTERN  ("6dd49a26-6711-11dd-88fd-00e081225ce5")
#define MT_NAME_SURFACE  ("6d31a064-6711-11dd-bdd8-00e081225ce5")

static const char * const format_option_names[] = {
    "argb32", "rgb24", "a8", "a1", 0
};
static const cairo_format_t format_option_values[] = {
    CAIRO_FORMAT_ARGB32, CAIRO_FORMAT_RGB24, CAIRO_FORMAT_A8, CAIRO_FORMAT_A1
};

static const char * const antialias_names[] = {
    "default", "none", "gray", "subpixel", 0
};
static const cairo_antialias_t antialias_values[] = {
    CAIRO_ANTIALIAS_DEFAULT, CAIRO_ANTIALIAS_NONE, CAIRO_ANTIALIAS_GRAY,
    CAIRO_ANTIALIAS_SUBPIXEL
};

static const char * const linecap_names[] = {
    "butt", "round", "square", 0
};
static const cairo_line_cap_t linecap_values[] = {
    CAIRO_LINE_CAP_BUTT, CAIRO_LINE_CAP_ROUND, CAIRO_LINE_CAP_SQUARE
};

static const char * const linejoin_names[] = {
    "miter", "round", "bevel", 0
};
static const cairo_line_cap_t linejoin_values[] = {
    CAIRO_LINE_JOIN_MITER, CAIRO_LINE_JOIN_ROUND, CAIRO_LINE_JOIN_BEVEL
};

static const char * const fillrule_names[] = {
    "winding", "even-odd", 0
};
static const cairo_fill_rule_t fillrule_values[] = {
    CAIRO_FILL_RULE_WINDING, CAIRO_FILL_RULE_EVEN_ODD
};

static const char * const operator_names[] = {
    "clear",
    "source", "over", "in", "out", "atop",
    "dest", "dest-over", "dest-in", "dest-out", "dest-atop",
    "xor", "add", "saturate",
    0
};
static const cairo_operator_t operator_values[] = {
    CAIRO_OPERATOR_CLEAR,
    CAIRO_OPERATOR_SOURCE, CAIRO_OPERATOR_OVER, CAIRO_OPERATOR_IN,
    CAIRO_OPERATOR_OUT, CAIRO_OPERATOR_ATOP,
    CAIRO_OPERATOR_DEST, CAIRO_OPERATOR_DEST_OVER, CAIRO_OPERATOR_DEST_IN,
    CAIRO_OPERATOR_DEST_OUT, CAIRO_OPERATOR_DEST_ATOP,
    CAIRO_OPERATOR_XOR, CAIRO_OPERATOR_ADD, CAIRO_OPERATOR_SATURATE
};

static const char * const content_names[] = {
    "color", "alpha", "color-alpha", 0
};
static const cairo_content_t content_values[] = {
    CAIRO_CONTENT_COLOR, CAIRO_CONTENT_ALPHA, CAIRO_CONTENT_COLOR_ALPHA
};

static const char * const extend_names[] = {
    "none", "repeat", "reflect", "pad", 0
};
static const cairo_extend_t extend_values[] = {
    CAIRO_EXTEND_NONE, CAIRO_EXTEND_REPEAT, CAIRO_EXTEND_REFLECT,
    CAIRO_EXTEND_PAD
};

static const char * const filter_names[] = {
    "fast", "good", "best", "nearest", "bilinear", "gaussian", 0
};
static const cairo_filter_t filter_values[] = {
    CAIRO_FILTER_FAST, CAIRO_FILTER_GOOD, CAIRO_FILTER_BEST,
    CAIRO_FILTER_NEAREST, CAIRO_FILTER_BILINEAR, CAIRO_FILTER_GAUSSIAN
};

static const char * const font_slant_names[] = {
    "normal", "italic", "oblique", 0
};
static const cairo_font_slant_t font_slant_values[] = {
    CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_SLANT_ITALIC, CAIRO_FONT_SLANT_OBLIQUE
};

static const char * const font_weight_names[] = {
    "normal", "bold", 0
};
static const cairo_font_weight_t font_weight_values[] = {
  CAIRO_FONT_WEIGHT_NORMAL, CAIRO_FONT_WEIGHT_BOLD
};

static const char * const font_type_names[] = {
    "toy", "ft", "win32", "quartz",
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 8, 0)
    "user",
#endif
    0
};
static const cairo_font_type_t font_type_values[] = {
    CAIRO_FONT_TYPE_TOY, CAIRO_FONT_TYPE_FT,
    CAIRO_FONT_TYPE_WIN32, CAIRO_FONT_TYPE_QUARTZ,
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 8, 0)
    CAIRO_FONT_TYPE_USER,
#endif
};

static void
to_lua_matrix (lua_State *L, cairo_matrix_t *mat, int pos) {
    double *matnums;
    int i;
    matnums = (double *) mat;
    for (i = 0; i < 6; ++i) {
        lua_pushnumber(L, matnums[i]);
        lua_rawseti(L, pos, i + 1);
    }
}

static void
create_lua_matrix (lua_State *L, cairo_matrix_t *mat) {
    lua_createtable(L, 6, 0);
    to_lua_matrix(L, mat, lua_gettop(L));
    luaL_getmetatable(L, MT_NAME_MATRIX);
    lua_setmetatable(L, -2);
}

static void
from_lua_matrix (lua_State *L, cairo_matrix_t *mat, int pos) {
    double *matnums;
    int i;
    luaL_checktype(L, pos, LUA_TTABLE);
    matnums = (double *) mat;
    for (i = 0; i < 6; ++i) {
        lua_rawgeti(L, pos, i + 1);
        if (!lua_isnumber(L, -1))
            luaL_error(L, "value %d in matrix isn't a number", i + 1);
        matnums[i] = lua_tonumber(L, -1);
        lua_pop(L, 1);
    }
}

static void
create_lua_font_extents (lua_State *L, cairo_font_extents_t *extents) {
    lua_createtable(L, 0, 5);
    lua_pushliteral(L, "ascent");
    lua_pushnumber(L, extents->ascent);
    lua_rawset(L, -3);
    lua_pushliteral(L, "descent");
    lua_pushnumber(L, extents->descent);
    lua_rawset(L, -3);
    lua_pushliteral(L, "height");
    lua_pushnumber(L, extents->height);
    lua_rawset(L, -3);
    lua_pushliteral(L, "max_x_advance");
    lua_pushnumber(L, extents->max_x_advance);
    lua_rawset(L, -3);
    lua_pushliteral(L, "max_y_advance");
    lua_pushnumber(L, extents->max_y_advance);
    lua_rawset(L, -3);
}

static void
create_lua_text_extents (lua_State *L, cairo_text_extents_t *extents) {
    lua_createtable(L, 0, 6);
    lua_pushliteral(L, "x_bearing");
    lua_pushnumber(L, extents->x_bearing);
    lua_rawset(L, -3);
    lua_pushliteral(L, "y_bearing");
    lua_pushnumber(L, extents->y_bearing);
    lua_rawset(L, -3);
    lua_pushliteral(L, "width");
    lua_pushnumber(L, extents->width);
    lua_rawset(L, -3);
    lua_pushliteral(L, "height");
    lua_pushnumber(L, extents->height);
    lua_rawset(L, -3);
    lua_pushliteral(L, "x_advance");
    lua_pushnumber(L, extents->x_advance);
    lua_rawset(L, -3);
    lua_pushliteral(L, "y_advance");
    lua_pushnumber(L, extents->y_advance);
    lua_rawset(L, -3);
}

static void
from_lua_glyph_array (lua_State *L, cairo_glyph_t **glyphs, int *num_glyphs,
                      int pos)
{
    int i;
    double n;

    luaL_checktype(L, pos, LUA_TTABLE);
    *num_glyphs = lua_objlen(L, pos);
    if (*num_glyphs == 0) {
        *glyphs = 0;
        return;
    }
    *glyphs = malloc(sizeof(cairo_glyph_t) * *num_glyphs);
    assert(*glyphs);

    for (i = 0; i < *num_glyphs; ++i) {
        lua_rawgeti(L, pos, i + 1);
        if (!lua_istable(L, -1)) {
            free(*glyphs);
            luaL_error(L, "glyph %d is not a table", i + 1);
        }
        else if (lua_objlen(L, -1) != 3) {
            free(*glyphs);
            luaL_error(L, "glyph %d should contain exactly 3 numbers", i + 1);
        }
        lua_rawgeti(L, -1, 1);
        if (!lua_isnumber(L, -1)) {
            free(*glyphs);
            luaL_error(L, "index of glyph %d should be a number", i + 1);
        }
        n = lua_tonumber(L, -1);
        if (n < 0) {
            free(*glyphs);
            luaL_error(L, "index number of glyph %d is negative", i + 1);
        }
        (*glyphs)[i].index = (unsigned long) n;
        lua_pop(L, 1);
        lua_rawgeti(L, -1, 2);
        if (!lua_isnumber(L, -1)) {
            free(*glyphs);
            luaL_error(L, "x position for glyph %d should be a number", i + 1);
        }
        (*glyphs)[i].x = lua_tonumber(L, -1);
        lua_pop(L, 1);
        lua_rawgeti(L, -1, 3);
        if (!lua_isnumber(L, -1)) {
            free(*glyphs);
            luaL_error(L, "y position for glyph %d should be a number", i + 1);
        }
        (*glyphs)[i].y = lua_tonumber(L, -1);
        lua_pop(L, 2);
    }
}

#include "obj_context.c"
#include "obj_font_face.c"
#include "obj_matrix.c"
#include "obj_path.c"
#include "obj_pattern.c"
#include "obj_surface.c"

static const luaL_Reg
constructor_funcs[] = {
    { "context_create", context_create },
    { "image_surface_create", image_surface_create },
    { "image_surface_create_from_png", image_surface_create_from_png },
    { "image_surface_create_from_png_stream",
      image_surface_create_from_png_stream },
    { "image_surface_create_from_png_string",
      image_surface_create_from_png_string },
    { "matrix_create", cairmat_create },
    { "pattern_create_for_surface", pattern_create_for_surface },
    { "pattern_create_linear", pattern_create_linear },
    { "pattern_create_radial", pattern_create_radial },
    { "pattern_create_rgb", pattern_create_rgb },
    { "pattern_create_rgba", pattern_create_rgba },
    { "surface_create_similar", surface_create_similar },
    { 0, 0 }
};

static void
add_funcs_to_table (lua_State *L, const luaL_Reg *funcs) {
    const luaL_Reg *l;
    for (l = funcs; l->name; ++l) {
        lua_pushstring(L, l->name);
        lua_pushcfunction(L, l->func);
        lua_rawset(L, -3);
    }
}

static void
create_object_metatable (lua_State *L, const char *mt_name,
                         const char *debug_name, const luaL_Reg *methods)
{
    if (luaL_newmetatable(L, mt_name)) {
        lua_pushliteral(L, "_NAME");
        lua_pushstring(L, debug_name);
        lua_rawset(L, -3);
        add_funcs_to_table(L, methods);
        lua_pushliteral(L, "__index");
        lua_pushvalue(L, -2);
        lua_rawset(L, -3);
    }
    lua_pop(L, 1);
}

int
luaopen_oocairo (lua_State *L) {
#ifdef VALGRIND_LUA_MODULE_HACK
    /* Hack to allow Valgrind to access debugging info for the module. */
    luaL_getmetatable(L, "_LOADLIB");
    lua_pushnil(L);
    lua_setfield(L, -2, "__gc");
    lua_pop(L, 1);
#endif

    /* Create the table to return from 'require' */
    lua_newtable(L);
    lua_pushliteral(L, "_NAME");
    lua_pushliteral(L, "cairo");
    lua_rawset(L, -3);
    lua_pushliteral(L, "_VERSION");
    lua_pushliteral(L, VERSION);
    lua_rawset(L, -3);
    lua_pushliteral(L, "_CAIRO_VERSION");
    lua_pushstring(L, cairo_version_string());
    lua_rawset(L, -3);
    add_funcs_to_table(L, constructor_funcs);

    /* Create the metatables for objects of different types. */
    create_object_metatable(L, MT_NAME_CONTEXT, "cairo context object",
                            context_methods);
    create_object_metatable(L, MT_NAME_FONTFACE, "cairo font face object",
                            fontface_methods);
    create_object_metatable(L, MT_NAME_MATRIX, "cairo matrix object",
                            cairmat_methods);
    create_object_metatable(L, MT_NAME_PATH, "cairo path object",
                            path_methods);
    create_object_metatable(L, MT_NAME_PATTERN, "cairo pattern object",
                            pattern_methods);
    create_object_metatable(L, MT_NAME_SURFACE, "cairo surface object",
                            surface_methods);

    return 1;
}

/* vi:set ts=4 sw=4 expandtab: */
