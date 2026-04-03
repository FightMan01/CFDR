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
  Scratch scratch = { };
  Scratch_Scope(&scratch, 0) {

    char buffer[512] = { };
    stbsp_snprintf(buffer, 512, "%d", scene->step.step_value);
    Str step = str_from_cstr(buffer);

    // TODO(cmat): Implement this properly, as a seperate parse step.
    Str text = str_replace(scratch.arena, node->content, str_lit("$/step"), step);

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
      case Align2_Center:    { text_at.y = draw_region.min.y + .5f * ((draw_region.max.y - draw_region.min.y) - overlay->font.font.metric_height);   } break;
    }
   
    node->color_shadow.a = node->color.a;
    if (node->flags & CFDR_Overlay_Flag_Shadow) {
      g2_draw_text(text, &node->font, v2f_add(text_at, v2f(node->shadow_offset, -node->shadow_offset)), .color = rgba_from_hsva(node->color_shadow));
    }

    if (node->flags & CFDR_Overlay_Flag_Background) {
      V2F from = v2f_sub(v2f(text_at.x, text_at.y + node->font.font.metric_descent), v2f(10, 0));
      V2F size = v2f_add(v2f(text_width, node->font.font.metric_height), v2f(20, 0));
      g2_draw_rect_rounded(from, size, 8, .inner_color = v4f(0, 0, 0, .3f), .outer_color = v4f(0, 0, 0, .3f));
    }

    g2_draw_text(text, &node->font, text_at, .color = rgba_from_hsva(node->color));
  }
}

fn_internal void cfdr_overlay_draw(CFDR_Overlay *overlay, CFDR_Scene *scene, R2F draw_region) {
  g2_clip_region(r2i_from_r2f(draw_region));
  for (CFDR_Overlay_Node *it = overlay->first; it; it = it->next) {
    if (it->visible) {
      cfdr_overlay_draw_text(overlay, scene, it, draw_region);
    }
  }
 
  g2_submit_draw();
}
