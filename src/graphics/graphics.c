// (C) Copyright 2025 Matyas Constans
// Licensed under the MIT License (https://opensource.org/license/mit/)

// ------------------------------------------------------------
// #-- Graphics 2D Immediate Mode

typedef struct {
  V2F position;
  V2F size;
  V2F uv_position;
  V2F uv_size;
  U32 color[4];
  F32 rounding[4];
} G2_Quad;

#define G2_Quad_Array_Capacity   u64_thousands(200)

typedef struct G2_Buffer {
  U64               quad_at;
  U64               quad_capacity;
  G2_Quad          *quad_array;

  U32               draw_quad_count;
} G2_Buffer;

typedef U32 G2_Draw_Mode;
enum {
  G2_Draw_Mode_Flat,
  G2_Draw_Mode_Count,
};

var_global struct {
  Arena               arena;
  G2_Buffer           buffer;
  R_Buffer            quad_buffer;
  R_Buffer            index_buffer;
  R_Buffer            constant_viewport_2D;
  G2_Material         material;
  R_Pipeline          pipelines[G2_Draw_Mode_Count];
  G2_Draw_Mode        draw_mode;
  R2I                 last_clip_region;
  R2I                 active_clip_region;
  U64                 material_next_id;
} G2_State;

fn_internal void g2_init(void) {
  arena_init(&G2_State.arena);
  
  G2_State.buffer.quad_capacity  = G2_Quad_Array_Capacity;
  U64             index_capacity = 6 * G2_Quad_Array_Capacity;

  G2_State.buffer.quad_array   = arena_push_count(&G2_State.arena, G2_Quad, G2_State.buffer.quad_capacity);

  G2_State.quad_buffer  = r_buffer_allocate(G2_State.buffer.quad_capacity  * sizeof(G2_Quad), R_Buffer_Mode_Dynamic);
  G2_State.index_buffer = r_buffer_allocate(                index_capacity * sizeof(U32),     R_Buffer_Mode_Dynamic);

  // NOTE(cmat): Fill index-buffer.
  Scratch scratch = { };
  Scratch_Scope(&scratch, 0) {
    U32 *index_data = arena_push_count(scratch.arena, U32, index_capacity);
    For_U64(it, index_capacity / 6) {
      U32 offset = 6 * it;
      U32 base   = 4 * it;
      index_data[offset++] = base + 0;
      index_data[offset++] = base + 1;
      index_data[offset++] = base + 2;
      index_data[offset++] = base + 0;
      index_data[offset++] = base + 2;
      index_data[offset++] = base + 3;
    }

    r_buffer_download(G2_State.index_buffer, 0, index_capacity * sizeof(U32), index_data);
  }

  G2_State.pipelines[G2_Draw_Mode_Flat] = r_pipeline_create(&(R_Pipeline_Layout) {
    .shader = R_Shader_Flat_2D,
    .depth_test  = 0,
    .depth_write = 0,
  });

  G2_State.constant_viewport_2D = r_buffer_allocate(sizeof(R_Constant_Buffer_Viewport_2D), R_Buffer_Mode_Dynamic);
  G2_State.last_clip_region     = r2i(0, 0, i32_limit_max, i32_limit_max);
  G2_State.active_clip_region   = G2_State.last_clip_region;

  G2_Material_White = g2_material_create(R_Texture_2D_White, R_Sampler_Nearest_Clamp);
  G2_State.material = G2_Material_White;
}

G2_Material G2_Material_White = { };

fn_internal G2_Material g2_material_create(R_Texture_2D texture, R_Sampler sampler) {
  G2_Material material = { };
 
  material.id         = ++G2_State.material_next_id;
  material.bind_group = r_bind_group_create(&Flat_2D_Layout, &(R_Bind_Group_Entry_List) {
    .count      = 4,
    .entry_list = {
      { .binding = 0, .type = R_Binding_Type_Storage,    .resource = G2_State.quad_buffer          },
      { .binding = 1, .type = R_Binding_Type_Texture_2D, .resource = texture                       },
      { .binding = 2, .type = R_Binding_Type_Sampler,    .resource = sampler                       },
      { .binding = 3, .type = R_Binding_Type_Uniform,    .resource = G2_State.constant_viewport_2D },
    }
  });

  return material;
}

fn_internal void g2_material_delete(G2_Material *material) {
  r_bind_group_destroy(&material->bind_group);
  zero_fill(material);
}

fn_internal void g2_submit_draw(void) {
  if (G2_State.buffer.draw_quad_count) {

    // TODO(cmat): Move out some parts.
    V2F display_size = pl_display()->resolution;
    r_buffer_download(G2_State.constant_viewport_2D,
                        0, sizeof(R_Constant_Buffer_Viewport_2D),
                        &(R_Constant_Buffer_Viewport_2D) {
                          .NDC_From_Screen = {
                            .e11 = 2.f / display_size.width,
                            .e22 = 2.f / display_size.height,
                            .e33 = 0,
                            .e44 = 1,
                            .e14 = -1,
                            .e24 = -1,
                            .e34 =  0
                          },
                          .Viewport_Size = pl_display()->resolution
                        });

    V2F resolution = pl_display()->resolution;

    R2I clip_region = G2_State.last_clip_region;

    clip_region.x0 = i32_max(0, clip_region.x0);
    clip_region.y0 = i32_max(0, clip_region.y0);
    clip_region.x1 = i32_min((I32)pl_display()->resolution.x, clip_region.x1);
    clip_region.y1 = i32_min((I32)pl_display()->resolution.y, clip_region.y1);

    R_Command_Draw draw = {
        .pipeline              = G2_State.pipelines[G2_State.draw_mode],
        .bind_group            = G2_State.material.bind_group,

        .index_buffer          = G2_State.index_buffer,
        
        .draw_index_count      = 6 * G2_State.buffer.draw_quad_count,
        .draw_index_offset     = 6 * (G2_State.buffer.quad_at - G2_State.buffer.draw_quad_count),
        .draw_instance_count   = 1,

        .depth_test            = 0,

        .draw_region           = pl_display_region(),
        .clip_region           = clip_region,
    };

    r_command_push_draw(&draw);
    G2_State.buffer.draw_quad_count   = 0;
    G2_State.last_clip_region         = G2_State.active_clip_region;
  }
}

fn_internal void g2_frame_flush(void) {
  g2_submit_draw();
  if (G2_State.buffer.quad_at) {
    r_buffer_download(G2_State.quad_buffer,    0, G2_State.buffer.quad_at   * sizeof(G2_Quad),  (U08 *)G2_State.buffer.quad_array);
  }
  
  G2_State.buffer.quad_at           = 0;
  G2_State.buffer.draw_quad_count   = 0;
  G2_State.draw_mode                = 0;

  G2_State.last_clip_region   = r2i(0, 0, i32_limit_max, i32_limit_max);
  G2_State.active_clip_region = G2_State.last_clip_region;
}

fn_internal G2_Quad *g2_push_quad(G2_Material material) {
  Assert(G2_State.buffer.quad_at  + 1 < G2_State.buffer.quad_capacity,  "2D immediate quad buffer overflow");

  // NOTE(cmat): If we need to change the draw mode, or we exceed the number of textures
  // - we can bind, we flush with a draw call.
  G2_Draw_Mode mode = G2_Draw_Mode_Flat;
  B32 submit_draw = 0;
  if (G2_State.buffer.draw_quad_count && G2_State.draw_mode != mode) {
    submit_draw = 1;
  }

  if (G2_State.last_clip_region.x0 != G2_State.active_clip_region.x0 ||
      G2_State.last_clip_region.y0 != G2_State.active_clip_region.y0 ||
      G2_State.last_clip_region.x1 != G2_State.active_clip_region.x1 ||
      G2_State.last_clip_region.y1 != G2_State.active_clip_region.y1) {

    submit_draw = 1;
  }

  if (material.bind_group != G2_State.material.bind_group) {
    submit_draw = 1;
  }

  if (submit_draw) {
    g2_submit_draw();
  }

  G2_Quad *result  = G2_State.buffer.quad_array + G2_State.buffer.quad_at;
  zero_fill(result);

  G2_State.buffer.draw_quad_count   += 1;
  G2_State.buffer.quad_at           += 1;
  G2_State.draw_mode                 = mode;
  G2_State.material                  = material;

  return result;
}

fn_internal void g2_clip_region(R2I region) {
  G2_State.active_clip_region = region;
}

fn_internal void g2_draw_rect_ext(G2_Rect *rect) {
  G2_Quad *q      = g2_push_quad(rect->mat);
  q->position     = rect->pos;
  q->size         = rect->size;
  q->uv_position  = rect->uv_bl;
  q->uv_size      = v2f_sub(rect->uv_tr, rect->uv_bl);

  U32 color       = abgr_u32_from_rgba_premul(rect->color);
  q->color[0]     = color;
  q->color[1]     = color;
  q->color[2]     = color;
  q->color[3]     = color;
}

fn_internal void g2_draw_rect_rounded_ext(G2_Rect_Rounded *rect) {
  G2_Quad *q      = g2_push_quad(G2_Material_White);
  q->position     = rect->pos;
  q->size         = rect->size;
  q->uv_position  = v2f(0, 0);
  q->uv_size      = v2f(1, 1);

  U32 color       = abgr_u32_from_rgba_premul(rect->color);
  q->color[0]     = color;
  q->color[1]     = color;
  q->color[2]     = color;
  q->color[3]     = color;

  q->rounding[0]  = rect->radius;
  q->rounding[1]  = rect->radius;
  q->rounding[2]  = rect->radius;
  q->rounding[3]  = rect->radius;
}

fn_internal void g2_draw_disk_ext(G2_Disk *disk) {
  g2_draw_rect_rounded(v2f_sub_f32(disk->pos, disk->radius), v2f_f32(2.f * disk->radius), disk->radius, .color = disk->color);
}

fn_internal void g2_draw_text_ext(G2_Text *text) {
  U32 C         = abgr_u32_from_rgba_premul(text->color);
  V2F draw_at   = text->pos;
  U32 decode_at = 0;
  for (;;) {
    if (decode_at >= text->text.len) break;
    Str decode_str = str_slice(text->text, decode_at, text->text.len - decode_at);

    I32 advance = 0;
    U32 codepoint = codepoint_from_utf8(decode_str, &advance);
    if (!advance) {
      advance = 1;
    }

    decode_at += advance;

    FO_Glyph *g = fo_glyph_get(&text->font->font, codepoint);
    if (!g->no_texture) {
      V2F offset = v2f(g->pen_offset.x, g->pen_offset.y);

      G2_Quad *q      = g2_push_quad(text->font->mat);
      q->position     = v2f_add(draw_at, offset);
      q->size         = v2f(g->bounds.x, g->bounds.y);
      q->color[0]     = C;
      q->color[1]     = C;
      q->color[2]     = C;
      q->color[3]     = C;

      if (!text->flip) {
        q->uv_position  = g->atlas_uv.min;
        q->uv_size      = v2f_sub(g->atlas_uv.max, g->atlas_uv.min);
      } else {
        q->uv_position  = v2f(g->atlas_uv.max.x, g->atlas_uv.min.y);
        q->uv_size      = v2f(g->atlas_uv.min.x - g->atlas_uv.max.x, g->atlas_uv.max.y - g->atlas_uv.min.y);
      }
    }

    draw_at.x += g->pen_advance;
  }
}
