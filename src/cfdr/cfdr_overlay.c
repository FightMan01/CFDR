var_global U08 Overlay_Font_Raw[] = {
#embed "../cfdr_data/PTSerif-Bold.ttf"
};

var_global Str Overlay_Font_Baked = (Str) { .len = sizeof(Overlay_Font_Raw), .txt = Overlay_Font_Raw };

fn_internal void cfdr_overlay_init(CFDR_Overlay *overlay) {
  zero_fill(overlay);
  arena_init(&overlay->arena);

  overlay->count = 0;
  overlay->first = 0;
  overlay->last  = 0;

  arena_init(&overlay->font_arena);
  overlay->font_scale = 25.f;
  F32 font_size = overlay->font_scale * js_web_device_pixel_ratio();
  overlay->font_last_size = font_size;
  g2_font_init(&overlay->font, &overlay->font_arena, Overlay_Font_Baked, font_size, v2_u16(1024, 1024), Codepoints_EU);
}

fn_internal CFDR_Overlay_Node *cfdr_overlay_push(CFDR_Overlay *overlay) {
  CFDR_Overlay_Node *node = arena_push_type(&overlay->arena, CFDR_Overlay_Node);
  queue_push(overlay->first, overlay->last, node);

  node->tag           = str_lit("No-Name");
  node->visible       = 1;
  node->content       = (Str) { };
  node->scale         = 100.f;
  node->color         = v4f(1, 0, 1, 1);
  node->color_shadow  = v4f(1, 0, 1, 1);
  node->position_x    = Align2_Center;
  node->position_y    = Align2_Center;
  node->border        = v2f(5, 5);
  node->shadow_offset = 1;

  arena_init(&node->font_arena);
  node->font            = (G2_Font) { };
  node->font_last_scale = 0;

  return node;
}

fn_internal void cfdr_overlay_draw_text(CFDR_Overlay *overlay, CFDR_Scene *scene, CFDR_Overlay_Node *node, R2F draw_region) {
  if (node->content.len != 0) { 
    Scratch scratch = { };
    Scratch_Scope(&scratch, 0) {

      char buffer[512] = { };
      stbsp_snprintf(buffer, 512, "%.3g", scene->step.step_value);
      Str step = str_from_cstr(buffer);

      // TODO(cmat): Implement this properly, as a seperate parse step.
      Str text = str_replace(scratch.arena, node->content, str_lit("$(step)"), step);

      // TODO(cmat): This is very much temporary. We want to refer to a global font cache.
      F32 real_scale = js_web_device_pixel_ratio();
      if (node->font_last_scale != real_scale * node->scale) {
        if (node->font.init) {
          g2_font_destroy(&node->font);
          arena_clear(&node->font_arena);
        }

        g2_font_init(&node->font, &node->font_arena, Overlay_Font_Baked, real_scale * node->scale, v2_u16(2048, 2048), Codepoints_ASCII_Extended);
        node->font_last_scale = real_scale * node->scale;
      }

      F32 text_width = fo_text_width(&node->font.font, text);
      V2F text_at    = v2f(0, 0);

      switch (node->position_x) {
        case Align2_Left:    { text_at.x = draw_region.min.x + node->border.x;                                                  } break;
        case Align2_Right:   { text_at.x = draw_region.max.x - text_width - node->border.x;                                     } break;
        case Align2_Center:  { text_at.x = draw_region.min.x + .5f * ((draw_region.max.x - draw_region.min.x) - text_width);    } break;
      }

      switch (node->position_y) {
        case Align2_Bottom:    { text_at.y = draw_region.min.y - node->font.font.metric_descent + node->border.y;                                   } break;
        case Align2_Top:       { text_at.y = draw_region.max.y - node->font.font.metric_ascent - node->border.y;                                    } break;
        case Align2_Center:    { text_at.y = draw_region.min.y + .5f * ((draw_region.max.y - draw_region.min.y) - node->font.font.metric_height);   } break;
      }
     
      node->color_shadow.a = node->color.a;
      if (node->flags & CFDR_Overlay_Flag_Shadow) {
        g2_draw_text(text, &node->font, v2f_add(text_at, v2f(node->shadow_offset, -node->shadow_offset)), .color = rgba_from_hsva(node->color_shadow));
      }

      if (node->flags & CFDR_Overlay_Flag_Background) {
        V2F from = v2f_sub(v2f(text_at.x, text_at.y + node->font.font.metric_descent), v2f(10, 0));
        V2F size = v2f_add(v2f(text_width, node->font.font.metric_height), v2f(20, 0));
        g2_draw_rect_rounded(from, size, 8, .color = v4f(0, 0, 0, .4f));
      }

      g2_draw_text(text, &node->font, text_at, .color = rgba_from_hsva(node->color));
    }
  }
}

fn_internal void cfdr_overlay_draw_tags(CFDR_Overlay *overlay, CFDR_Scene *scene, CFDR_Overlay_Node *node, R2F draw_region) {
  if (node->flags & CFDR_Overlay_Flag_Table) {
    cfdr_resource_table_update(&node->table);
    if (node->table.valid) {

      For_U32(it, node->table.row_count) {
        V3F world_space = node->table.position_array[it];
        Str tag         = node->table.tag_array[it];

        M4F basis_change = {
          .e11 = +1,
          .e32 = -1,
          .e23 = +1,
          .e44 = +1,
        };

        M4F world                 = basis_change;
        world                     = m4f_mul(scene->transform, world);
        M4F world_view_projection = m4f_mul(world, scene->view_projection);

        V3F transformed = m4f_mul_v4f(v4f(world_space.x, world_space.y, world_space.z, 1), world).xyz;
        V4F clip_space  = m4f_mul_v4f(v4f(world_space.x, world_space.y, world_space.z, 1), world_view_projection);
        if (clip_space.w > 0.0f) {
          V4F ndc_space   = v4f_div(clip_space, clip_space.w);

          V2F screen = v2f(draw_region.x0 + (ndc_space.x + 1.0f) * .5f * (draw_region.x1 - draw_region.x0),
                           draw_region.y0 + (ndc_space.y + 1.0f) * .5f * (draw_region.y1 - draw_region.y0));

          F32 f      = 1.0f / f32_tan(f32_radians_from_degrees(scene->view.camera.fov_deg) * .5f);
          F32 z      = v3f_dist(scene->view.camera.computed_position_m, transformed);
          F32 radius = 0.025f * f / z * ((draw_region.y1 - draw_region.y0) * 0.5f);
          radius = f32_clamp(radius, 0, 10);

          F32 blend_distance_min = 10;
          F32 blend_distance_max = 15;
          F32 t              = 1.f - ((f32_clamp(z, blend_distance_min, blend_distance_max) - blend_distance_min) / (blend_distance_max - blend_distance_min));
          F32 blend          = f32_smoothstep(t, 0, 1.f);

          if (t > 0) {
            g2_draw_disk(screen, radius + 1,  .color = v4f(0, 0, 0, t));
            g2_draw_disk(screen, radius,      .color = v4f(t, t, t, t));

            F32 text_shift = -.5f * fo_text_width(&overlay->font.font, tag);

            g2_draw_text(tag, &overlay->font, v2f_add(screen, v2f(text_shift + 3.f, radius + 1.f + 5.f)), .color = v4f(0, 0, 0, t));
            g2_draw_text(tag, &overlay->font, v2f_add(screen, v2f(text_shift + 2.f, radius + 2.f + 5.f)), .color = v4f(t, t, t, t));
          }

        }
      }
    }
  }

  g2_submit_draw();
}

fn_internal void cfdr_overlay_draw_cmap(CFDR_Overlay *overlay, CFDR_CMap_Table *cmap_table, CFDR_Scene *scene, R2F draw_region) {
  V2F tick_size = v2f(1, 10);

  F32 scale = js_web_device_pixel_ratio();

  R2F bar_region = r2f(draw_region.x0, draw_region.y0, draw_region.x0 + scale * 400, draw_region.y0 + scale * 40);
  bar_region.x0 += 20.f;
  bar_region.x1 += 20.f;
  bar_region.y0 += 20.f + tick_size.y / 2 + overlay->font.font.metric_height;
  bar_region.y1 += 20.f + tick_size.y / 2 + overlay->font.font.metric_height;

  I32 subdiv_count  = 4;

  CFDR_CMap *cmap = cfdr_cmap_table_get(cmap_table, scene->cmap);
  g2_draw_rect(bar_region.min, r2f_size(bar_region), .mat = cmap->material);
  
  g2_draw_text(scene->cmap, &overlay->font, v2f(bar_region.min.x - 1, bar_region.max.y - overlay->font.font.metric_descent - 1), .color = v4f(0, 0, 0, 1));
  g2_draw_text(scene->cmap, &overlay->font, v2f(bar_region.min.x, bar_region.max.y - overlay->font.font.metric_descent));

  For_I32 (it, subdiv_count + 1) {
    V2F at = v2f(bar_region.min.x + r2f_size(bar_region).x * ((F32)it / subdiv_count), bar_region.min.y - 3 * tick_size.y / 4);
    g2_draw_rect(v2f(at.x - 1, at.y - 1), v2f(tick_size.x + 2, tick_size.y + 2), .color = v4f(0, 0, 0, 1));
    g2_draw_rect(at, tick_size);
  }

  V2F range = cfdr_cmap_range(cmap);
  range.x = (range.x - cmap->data_shift) / cmap->data_scale;
  range.y = (range.y - cmap->data_shift) / cmap->data_scale;

  For_I32 (it, subdiv_count + 1) {
    V2F at = v2f(bar_region.min.x + r2f_size(bar_region).x * ((F32)it / subdiv_count), bar_region.min.y - 3 * tick_size.y / 4);
    char buffer[512] = { };
    stbsp_snprintf(buffer, 512, "%.4g", range.x + (range.y - range.x) * ((F32)it / subdiv_count));
    at = v2f(f32_floor(at.x), f32_floor(at.y));
    g2_draw_text(str_from_cstr(buffer), &overlay->font, v2f(at.x - 1, at.y - 1.25f * overlay->font.font.metric_ascent - 1), .color = v4f(0, 0, 0, 1));
    g2_draw_text(str_from_cstr(buffer), &overlay->font, v2f(at.x, at.y - 1.25f * overlay->font.font.metric_ascent));
  }
}

fn_internal void cfdr_overlay_draw(CFDR_Overlay *overlay, CFDR_CMap_Table *cmap_table, CFDR_Scene *scene, R2F draw_region) {
  g2_clip_region(r2i_from_r2f(draw_region));


  F32 font_scale = overlay->font_scale * js_web_device_pixel_ratio();
  if (overlay->font_last_size != font_scale) {
    if (overlay->font.init) {
      g2_font_destroy(&overlay->font);
      arena_clear(&overlay->font_arena);
    }

    g2_font_init(&overlay->font, &overlay->font_arena, Overlay_Font_Baked, font_scale, v2_u16(2048, 2048), Codepoints_ASCII_Extended);
    overlay->font_last_size = font_scale;
  }

  for (CFDR_Overlay_Node *it = overlay->first; it; it = it->next) {
    if (it->visible) {
      cfdr_overlay_draw_text(overlay, scene, it, draw_region);
      cfdr_overlay_draw_tags(overlay, scene, it, draw_region);
    }
  }

  // TODO(cmat): Temporary.
  cfdr_overlay_draw_cmap(overlay, cmap_table, scene, draw_region);
 
  g2_submit_draw();
}
