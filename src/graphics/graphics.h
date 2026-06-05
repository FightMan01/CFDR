// (C) Copyright 2025 Matyas Constans
// Licensed under the MIT License (https://opensource.org/license/mit/)

// TODO(cmat): Cleanup entire Font API-s
// -- Ideally, we want the following hierarchy.
// -- TEXTURE CACHE -> Manages all texture memory via atlases + packing.
// -- FONT          -> Manages font rasterization. Packs into TEXTURE CACHE region.
// -- GRAPHICS      -> Given the texture cache, can draw stuff on screen.

// ------------------------------------------------------------
// #-- 2D Immediate Mode API

#define G2_Clip_None r2i(0, 0, i32_limit_max, i32_limit_max)

fn_internal void g2_init        (void);
fn_internal void g2_frame_flush (void);
fn_internal void g2_clip_region (R2I region);

typedef struct G2_Material {
  U32          id;
  R_Bind_Group bind_group;
} G2_Material;

fn_internal G2_Material g2_material_create  (R_Texture_2D texture, R_Sampler sampler);
fn_internal void        g2_material_delete  (G2_Material *material);

var_external G2_Material G2_Material_White;

// NOTE(cmat): Rectangle
typedef struct G2_Rect {
  V2F          pos;
  V2F          size;
  V2F          uv_bl;
  V2F          uv_tr;
  RGBA         color;
  G2_Material  mat;
} G2_Rect;

fn_internal void g2_draw_rect_ext(G2_Rect *rect);
#define g2_draw_rect(pos_, size_, ...)        \
  g2_draw_rect_ext(&(G2_Rect) {               \
      .pos            = pos_,                 \
      .size           = size_,                \
      .uv_bl          = v2f(0, 0),            \
      .uv_tr          = v2f(1, 1),            \
      .color          = v4f(1, 1, 1, 1),      \
      .mat            = G2_Material_White,    \
      ##__VA_ARGS__,                          \
  })

// NOTE(cmat): Rounded-Rectangle

typedef struct G2_Rect_Rounded {
  V2F       pos;
  V2F       size;
  RGBA      color;
  F32       radius;
} G2_Rect_Rounded;

fn_internal void g2_draw_rounded_rect_ext(G2_Rect_Rounded *rect);
#define g2_draw_rect_rounded(pos_, size_, radius_, ...)       \
  g2_draw_rect_rounded_ext(&(G2_Rect_Rounded) {               \
      .pos            = pos_,                                 \
      .size           = size_,                                \
      .radius         = radius_,                              \
      .color          = v4f(1, 1, 1, 1),                      \
      ##__VA_ARGS__,                                          \
  })

// NOTE(cmat): Circle

typedef struct G2_Disk {
  V2F       pos;
  F32       radius;
  RGBA      color;
} G2_Disk;

fn_internal void g2_draw_disk_ext(G2_Disk *disk);
#define g2_draw_disk(pos_, radius_, ...)                      \
  g2_draw_disk_ext(&(G2_Disk) {                               \
      .pos            = pos_,                                 \
      .radius         = radius_,                              \
      .color          = v4f(1, 1, 1, 1),                      \
      ##__VA_ARGS__                                           \
  })

typedef struct G2_Font {
  B32         init;
  FO_Font     font;
  G2_Material mat;
} G2_Font;

fn_internal void g2_font_init(G2_Font *font, Arena *arena, Str font_data, I32 font_size, V2_U16 atlas_size, Array_Codepoint codepoints) {
  font->init = 1;
  fo_font_init(&font->font, arena, font_data, font_size, atlas_size, codepoints);
  font->mat = g2_material_create(font->font.glyph_atlas, R_Sampler_Nearest_Clamp);
}

fn_internal void g2_font_destroy(G2_Font *font) {
  font->init = 0;
  g2_material_delete(&font->mat);
  r_texture_2D_destroy(&font->font.glyph_atlas);
}

// NOTE(cmat): Text
typedef struct G2_Text {
  Str      text;
  G2_Font *font;
  V2F      pos;
  V4F      color;
  B32      flip;
} G2_Text;

fn_internal void g2_draw_text_ext(G2_Text *text);
#define g2_draw_text(text_, font_, pos_, ...)    \
  g2_draw_text_ext(&(G2_Text) {                           \
      .text    = text_,                                   \
      .font    = font_,                                   \
      .pos     = pos_,                                    \
      .color   = v4f(1.f, 1.f, 1.f, 1.f),                 \
      .flip    = 0,                                       \
      ##__VA_ARGS__                                       \
  });

