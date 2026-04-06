// (C) Copyright 2025 Matyas Constans
// Licensed under the MIT License (https://opensource.org/license/mit/)

// TODO(cmat):
// -- ID stack implementation.

var_global struct {
  Arena         arena;
  Arena         arena_frame;
  U64           hash_count;
  UI_Node_List *hash_array;

  // NOTE(cmat): We use a stack for parents,
  // since it's possible to suddenly switch to a different overlay
  // node (instead of the root node), invalidating a strategy where
  // we refer to node->tree.parent.
  U32            parent_stack_cap;
  U32            parent_stack_len;
  UI_Node      **parent_stack;

  U32           font_stack_cap;
  U32           font_stack_len; 
  G2_Font     **font_stack;

  UI_Node_Location next_location;
  UI_Node         *root;
  
  B32              context_defining;
  B32              context_collapse;
  B32              context_spawned;
  V2F              context_position;
  UI_Node         *context_owner;
  UI_Node         *context;
  UI_Node         *context_last_frame;

  UI_Node_List     overlay_list;

} UI_State = { };

#define UI_Font_Scope(font_ptr_) Defer_Scope(ui_font_push(font_ptr_), ui_font_pop())

fn_internal void ui_font_push(G2_Font *font) {
  Assert(UI_State.font_stack_len <= UI_State.font_stack_cap, "ui_font_push() exceeding stack size");
  If_Likely (UI_State.font_stack_len <= UI_State.font_stack_cap) {
    UI_State.font_stack[UI_State.font_stack_len++] = font;
  }
}

fn_internal void ui_font_pop(void) {
  Assert(UI_State.font_stack_len >= 0, "ui_font_pop() no more to pop");
  If_Likely (UI_State.font_stack_len >= 0) {
    UI_State.font_stack_len -= 1;
  }
}

fn_internal G2_Font *ui_font_current(void) {
  G2_Font *font = 0;
  if (UI_State.font_stack_len > 0) {
    font = UI_State.font_stack[UI_State.font_stack_len - 1];
  }

  return font;
}

fn_internal UI_Node  *ui_node_push    (Str label, UI_Flags flags);
fn_internal void      ui_parent_push  (UI_Node *parent);

fn_internal void ui_init(void) {
  zero_fill(&UI_State);

  arena_init(&UI_State.arena);
  arena_init(&UI_State.arena_frame);
  UI_State.hash_count = 2048;
  UI_State.hash_array = arena_push_count(&UI_State.arena, UI_Node_List, UI_State.hash_count);

  UI_State.font_stack_cap = 32;
  UI_State.font_stack_len = 0;
  UI_State.font_stack     = arena_push_count(&UI_State.arena, G2_Font *, UI_State.font_stack_cap);

  UI_State.parent_stack_cap = 2048;
  UI_State.parent_stack_len = 0;
  UI_State.parent_stack     = arena_push_count(&UI_State.arena, UI_Node *, UI_State.parent_stack_cap);

  // NOTE(cmat): Setup root node for initial frame.
  // --
  UI_State.root = ui_node_push(UI_Root_Label, UI_Flag_None);
  ui_parent_push(UI_State.root);

  UI_State.root->layout.size[Axis2_X] = UI_Size_Fixed(pl_display()->resolution.x);
  UI_State.root->layout.size[Axis2_Y] = UI_Size_Fixed(pl_display()->resolution.y);

  UI_Theme_Active = UI_Theme_Dark;
}

fn_internal UI_Node *ui_cache(UI_ID id) {
  U64 bucket_index   = id % UI_State.hash_count;
  UI_Node_List *list = UI_State.hash_array + bucket_index;

  UI_Node *result = 0;
  if (!list->first) {
    list->first = arena_push_type(&UI_State.arena, UI_Node);
    list->last  = list->first;

    result      = list->last;
    result->id = id;

    // log_debug("Created UI element with id # %u", result->id);

  } else {
    UI_Node *entry = list->first;
    while (entry) {
      if (entry->id == id) {
        result = entry;
        break;
      }

      if (!entry->hash_next) {   
        list->last->hash_next  = arena_push_type(&UI_State.arena, UI_Node);
        list->last             = list->last->hash_next;
        result                 = list->last;
        result->id             = id;
        break;
      }

      entry = entry->hash_next;
    }
  }

  return result;
}

#define UI_Parent_Scope(node_) Defer_Scope(ui_parent_push(node_), ui_parent_pop())

fn_internal void ui_parent_push(UI_Node *parent) {
  Assert(UI_State.parent_stack_len < UI_State.parent_stack_cap, "parent_push limit exceeded");
  If_Likely (UI_State.parent_stack_len < UI_State.parent_stack_cap) {
    UI_State.parent_stack[UI_State.parent_stack_len++] = parent;
  }
}

fn_internal void ui_parent_pop(void) {
  Assert(UI_State.parent_stack_len > 0, "can't pop further");
  If_Likely (UI_State.parent_stack_len > 0) {
    UI_State.parent_stack_len--;
  }
}

fn_internal UI_Node *ui_parent_current(void) {
  UI_Node *result = 0;

  if (UI_State.parent_stack_len) {
    result = UI_State.parent_stack[UI_State.parent_stack_len - 1];
  }

  return result;
}

fn_internal void ui_node_update_tree(UI_Node *node, UI_Node *parent) {
  UI_Node_Tree *tree = &node->tree;

  tree->parent = parent;
  if (tree->parent) {
    if (parent->tree.first_child) {
      UI_Node *it = parent->tree.first_child;
      for (;;) {
        it->tree.last = node;
        if (!it->tree.next) {
          it->tree.next = node;
          break;
        }

        it = it->tree.next;
      }
    } else {
      parent->tree.first_child = node;
    }

    node->tree.first = parent->tree.first_child;
    node->tree.last  = node;
  } else {
    node->tree.first = node;
    node->tree.last  = node;
  }

  node->tree.next         = 0;
  node->tree.first_child  = 0;
}

fn_internal void ui_node_update_response(UI_Node *node) {
  UI_Response *response = &node->response;

  // NOTE(cmat): If the context node is overshadowing the node, we ignore some input.
  B32 ignore_input = 0;
  if (UI_State.context_last_frame && !UI_State.context_defining) {
    R2F context_region = UI_State.context_last_frame->solved.region_absolute;
    if (r2f_contains_v2f(context_region, pl_input()->mouse.position)) {
      ignore_input = 1;
    }
  }

  response->hover           = 0;
  response->down            = 0;
  response->press           = 0;
  response->release         = 0;
  response->in_region       = 0;
  response->press_secondary = 0;

  if (ignore_input) {
    response->drag          = 0;
    response->drag_origin   = v2f(0, 0);
    response->drag_position = v2f(0, 0);
    response->drag_change   = v2f(0, 0);
  } else {
    V2F mouse  = pl_input()->mouse.position;
    R2F region = node->solved.region_absolute;

    if (r2f_contains_v2f(region, mouse)) {
      response->in_region = 1;

      if (node->flags & UI_Flag_Response_Hover) {
        response->hover = 1;
      }

      if (pl_input()->mouse.right.down) {
        if (pl_input()->mouse.right.press) {
          if (node->flags & UI_Flag_Response_Press) {
            response->press_secondary = 1;
          }
        }
      }

      if (pl_input()->mouse.left.down) {
        if (pl_input()->mouse.left.press) {
          if (node->flags & UI_Flag_Response_Press) {
            response->press = 1;
          }

          if (node->flags & UI_Flag_Response_Drag) {
            response->drag = 1;
            response->drag_origin = pl_input()->mouse.position;
            response->drag_position = response->drag_origin;
            response->picked_position = r2f_clip(node->solved.region_absolute, response->drag_position);
            response->picked_position = v2f_sub(response->picked_position, node->solved.region_absolute.min);
          }
        }

        if (node->flags & UI_Flag_Response_Down) {
          response->down = 1;
        } 
      }
    }

    if (response->drag) {
      response->down = 1;

      response->drag_change   = v2f_sub(pl_input()->mouse.position, response->drag_position);
      response->drag_position = pl_input()->mouse.position;
      response->picked_position = r2f_clip(node->solved.region_absolute, response->drag_position);
      response->picked_position = v2f_sub(response->picked_position, node->solved.region_absolute.min);
      if (!pl_input()->mouse.left.down) {
        response->drag          = 0;
        response->drag_change   = v2f(0, 0);
        response->drag_position = v2f(0, 0);
      }
    }

    if (node->flags & UI_Flag_Trigger_Context_Collapse) {
      if (response->press) {
        UI_State.context_collapse = 1;
      }
    }
  }
}

fn_internal void ui_node_update_animation(UI_Node *node) {
  UI_Animation *anim = &node->animation;

  U32 frame_index = pl_display()->frame_index;

  // NOTE(cmat): If there were skipped frames, that is,
  // the UI component was hidden, we invalidate the animation cache.
  if (frame_index != node->frame_index + 1) {
    zero_fill(anim);
  }

  node->frame_index = frame_index;

  F32 refresh_rate_coeff = pl_display()->frame_delta;

  anim->spawn_t = f32_exp_smoothing(anim->spawn_t, 1.f,                  f32_min(1.f, refresh_rate_coeff * 15.f));
  anim->hover_t = f32_exp_smoothing(anim->hover_t, node->response.hover, f32_min(1.f, refresh_rate_coeff * 15.f));
  anim->down_t  = f32_exp_smoothing(anim->down_t, node->response.down,   f32_min(1.f, refresh_rate_coeff * 15.f));

#if 1
  anim->spawn_t = anim->spawn_t > .99f ? 1.f : anim->spawn_t;
  anim->spawn_t = anim->spawn_t < .01f ? 0.f : anim->spawn_t;

  anim->hover_t = anim->hover_t > .99f ? 1.f : anim->hover_t;
  anim->hover_t = anim->hover_t < .01f ? 0.f : anim->hover_t;

  anim->down_t = anim->down_t > .99f ? 1.f : anim->down_t;
  anim->down_t = anim->down_t < .01f ? 0.f : anim->down_t;
#endif
}

fn_internal UI_ID ui_node_id(Str label, UI_ID parent_id) {

  // NOTE(cmat): If the string contains ###, only hash the part after "###"
  // NOTE(cmat): If the string contains ##, we hash everything but ignore things after ## for the label.
  if (str_contains(label, str_lit("###"))) {
    U32 it = 0;
    while (label.txt[it] != '#') {
      it++;
    }

    it += 2;
    label = str_slice(label, it, label.len - it);
  }

  UI_ID hash = 0;
  Scratch scratch = { };
  Scratch_Scope(&scratch, 0) {
    U32  bytes = label.len + sizeof(UI_ID);
    U08 *data  = arena_push_size(scratch.arena, bytes);

    memory_copy(data, &parent_id, sizeof(UI_ID));
    memory_copy(data + sizeof(UI_ID), label.txt, label.len);

    hash = crc32(bytes, data);
  }

  return hash;
}

fn_internal void ui_next_location(UI_Node_Location location) {
  UI_State.next_location = location;
}

fn_internal UI_Node *ui_node_push(Str label, UI_Flags flags) {
  UI_Node *parent     = ui_parent_current();
  UI_ID    parent_id  = parent ? parent->id : 0;
  UI_ID    id         = ui_node_id(label, parent_id);
  UI_Node *node       = ui_cache(id);

  node->flags     = flags;
  node->draw.font = ui_font_current();
  node->label     = arena_push_str(&UI_State.arena_frame, label);

  ui_node_update_response   (node);

  // NOTE(cmat): Do not add parent to tree if it's a context / overlay node.
  // TODO(cmat): Doing this for ID generation context/overlay-wise, but this feels not right.
  // -           Maybe pusing ID-s of the parent on the stack is better?
  ui_node_update_tree       (node, UI_State.next_location == UI_Node_Location_None ? parent : 0);
  ui_node_update_animation  (node);

  // NOTE(cmat): If it's an overlay, push on the overlay stack.
  switch (UI_State.next_location) {
    case UI_Node_Location_Overlay: {
      UI_Node_List *list = &UI_State.overlay_list;
      queue_push_ext(list->first, list->last, node, overlay_next);
    } break;
    
    case UI_Node_Location_Context: {
      UI_State.context = node;
    } break;
  }

  UI_State.next_location = UI_Node_Location_None;
  return node;
}

// ------------------------------------------------------------
// #-- NOTE(cmat): Layout solver

fn_internal void ui_solve_label(UI_Node *node) {
  if (node) {
    for (UI_Node *it = node->tree.first_child; it; it = it->tree.next) {
      ui_solve_label(it);
    }

    if (str_contains(node->label, str_lit("##"))) {
      U32 it = 0;
      while (node->label.txt[it] != '#') {
        it++;
      }

      node->solved.text = str_slice(node->label, 0, it);
    } else {
      node->solved.text = node->label;
    }
  }
}

// TODO(cmat): I don't like how we have to call ui_size_animate inside of each ui_solve_* function.
// Seems clunky, but since there's a dependency between stages, I don't see a better way right now.
// Maybe we can make this explicit or something, right after the solve calls, but again, it's hard
// because some passes like Grow have also a fit pass beforehand.
fn_internal void ui_size_animate(UI_Node *node, Axis2 axis) {
  if ((axis == Axis2_X && (node->flags & UI_Flag_Animation_Grow_X)) ||
      (axis == Axis2_Y && (node->flags & UI_Flag_Animation_Grow_Y))) {
    node->solved.size.dat[axis] = f32_lerp(node->animation.spawn_t, 0.f, node->solved.size.dat[axis]);
  }

}

fn_internal void ui_solve_layout_size_known_for_axis(UI_Node *node, Axis2 axis) {
  if (node) {
    UI_Size size = node->layout.size[axis];
    switch (size.type) {
      case UI_Size_Type_Fixed: {
        node->solved.size.dat[axis] = size.value;
        node->solved.size.dat[axis] += 2.f * node->layout.gap_border[axis];
        ui_size_animate(node, axis);
      } break;

      case UI_Size_Type_Text: {
        if (axis == Axis2_X) {
          node->solved.size.dat[axis] = fo_text_width(&node->draw.font->font, node->solved.text);
          node->solved.size.dat[axis] += 2.f * node->layout.gap_border[Axis2_X];
        } else {
          node->solved.size.dat[axis] = node->draw.font->font.metric_height;
          node->solved.size.dat[axis] += 2.f * node->layout.gap_border[Axis2_Y];
        }

        ui_size_animate(node, axis);
      } break;
    }

    for (UI_Node *it = node->tree.first_child; it; it = it->tree.next) {
      ui_solve_layout_size_known_for_axis(it, axis);
    }
  }
}

fn_internal void ui_solve_layout_size_known(UI_Node *node) {
  ui_solve_layout_size_known_for_axis(node, Axis2_X);
  ui_solve_layout_size_known_for_axis(node, Axis2_Y);
}

fn_internal void ui_solve_layout_size_fit_for_axis(UI_Node *node, Axis2 axis) {
  if (node) {
    for (UI_Node *it = node->tree.first_child; it; it = it->tree.next) {
      ui_solve_layout_size_fit_for_axis(it, axis);
    }

    UI_Size size = node->layout.size[axis];
    switch (size.type) {
      case UI_Size_Type_Fill:
      case UI_Size_Type_Fit: {
         
          F32 used_space = 0;
          if (node) {
            if (axis == node->layout.direction) {
              F32 children_sum    = 0;
              F32 children_count  = 0;

              for (UI_Node *it = node->tree.first_child; it; it = it->tree.next) {
                children_count += 1;
                children_sum   += it->solved.size.dat[axis];
              }

              used_space += 2 * node->layout.gap_border[axis];
              used_space += children_sum;
              if (children_count) {
                used_space += (children_count - 1) * node->layout.gap_child;
              }

            } else {
              // NOTE(cmat): Max of children sizes.
              F32 children_max   = 0;
              F32 children_count = 0;

              for (UI_Node *it = node->tree.first_child; it; it = it->tree.next) {
                children_count += 1;
                children_max   = f32_max(children_max, it->solved.size.dat[axis]);
              }

              used_space += 2 * node->layout.gap_border[axis];
              used_space += children_max;
            }
          }

        node->solved.size.dat[axis] = used_space;
        ui_size_animate(node, axis);

      } break;
    }
  }
}

fn_internal void ui_solve_layout_size_fit(UI_Node *node) {
  ui_solve_layout_size_fit_for_axis(node, Axis2_X);
  ui_solve_layout_size_fit_for_axis(node, Axis2_Y);
}

fn_internal void ui_solve_layout_size_fill_for_axis(UI_Node *node, Axis2 axis, F32 free_space) {
  if (node) {
    UI_Size size = node->layout.size[axis];
    switch (size.type) {
      case UI_Size_Type_Fill: {
        node->solved.size.dat[axis] += free_space;
        ui_size_animate(node, axis);
      } break;
    }

    if (axis != node->layout.direction) {
      for (UI_Node *it = node->tree.first_child; it; it = it->tree.next) {  
        free_space = node->solved.size.dat[axis] - it->solved.size.dat[axis] - 2.f * node->layout.gap_border[axis];
        ui_solve_layout_size_fill_for_axis(it, axis, free_space);
      }
    } else {
      F32 free_space          = node->solved.size.dat[axis];
      F32 children_count      = 0;
      F32 fill_children_count = 0;

      free_space -= 2.f * node->layout.gap_border[axis];
      for (UI_Node *it = node->tree.first_child; it; it = it->tree.next) {
        UI_Size it_size = it->layout.size[axis];

        children_count += 1;
        fill_children_count += it_size.type == UI_Size_Type_Fill;
        free_space     -= it->solved.size.dat[axis];
      }

      if (children_count) {
        free_space -= (children_count - 1) * node->layout.gap_child;
      }

      free_space *= f32_div_safe(1.f, fill_children_count);
      free_space = f32_max(free_space, 0);
      for (UI_Node *it = node->tree.first_child; it; it = it->tree.next) {
        ui_solve_layout_size_fill_for_axis(it, axis, free_space);
      }
    }
  }
}

fn_internal void ui_solve_layout_size_fill(UI_Node *node, V2F free_space) {
  ui_solve_layout_size_fill_for_axis(node, Axis2_X, free_space.x);
  ui_solve_layout_size_fill_for_axis(node, Axis2_Y, free_space.y);
}

fn_internal F32 ui_solve_position_relative_for_axis(UI_Node *node, Axis2 axis, Axis2 layout_direction, F32 relative_position) {
  if (node) {
    if (axis == Axis2_X) {
      if (node->flags & UI_Flag_Layout_Float_X) {
        node->solved.position_relative.x = node->layout.float_position[Axis2_X];
      } else {
        node->solved.position_relative.x = relative_position;
      }
    }

    if (axis == Axis2_Y) {
      if (node->flags & UI_Flag_Layout_Float_Y) {
        node->solved.position_relative.y = node->layout.float_position[Axis2_Y];
      } else {
        node->solved.position_relative.y = relative_position;
      }
    }

    if (layout_direction == axis) {
      relative_position += node->solved.size.dat[axis];
    }

    F32 child_position = node->layout.gap_border[axis];
    for (UI_Node *it = node->tree.first_child; it; it = it->tree.next) {
      child_position = ui_solve_position_relative_for_axis(it, axis, node->layout.direction, child_position);
      if (node->layout.direction == axis)
        child_position += node->layout.gap_child;
    }
  }

  return relative_position;
}

fn_internal void ui_solve_position_relative(UI_Node *node, Axis2 layout_direction, V2F relative_position) {
  ui_solve_position_relative_for_axis(node, Axis2_X, layout_direction, relative_position.x);
  ui_solve_position_relative_for_axis(node, Axis2_Y, layout_direction, relative_position.y);
}

fn_internal void ui_solve_region(UI_Node *node, V2F position_at) {
  if (node) {
    position_at = v2f_add(position_at, node->solved.position_relative);

    V2F display_resolution = pl_display()->resolution;
    V2F size               = node->solved.size;
    V2F position_absolute  = v2f(position_at.x, display_resolution.y - position_at.y - node->solved.size.y);
    node->solved.region_absolute = r2f_v(position_absolute, v2f_add(position_absolute, size));

    for (UI_Node *it = node->tree.first_child; it; it = it->tree.next) {
      ui_solve_region(it, position_at);
    }
  }
}

fn_internal void ui_solve(UI_Node *node) {
  ui_solve_label(node);

  ui_solve_layout_size_known(node);
  ui_solve_layout_size_fit(node);
  ui_solve_layout_size_fill(node, v2f(0, 0));

  ui_solve_position_relative(node, Axis2_X, v2f(0, 0));
  ui_solve_region(node, v2f(0, 0));
}

// ------------------------------------------------------------
// #-- NOTE(cmat): Draw UI tree

typedef struct UI_Draw_Context {
  F32 opacity;
  R2I clip_region;
} UI_Draw_Context;

fn_internal void ui_draw(UI_Node *node, UI_Draw_Context *context) {
  if (node) {

    F32 rounding = 0.f;
    if (node->flags & UI_Flag_Draw_Rounded) {
      rounding = 0;
    }

    R2F region = node->solved.region_absolute;
    if (region.max.y - region.min.y <= 5) rounding = 0;

    F32 opacity = context->opacity;
    if (node->flags & UI_Flag_Animation_Fade_In) {
      opacity = f32_lerp(node->animation.spawn_t, 0.f, 1.f);
    }
    

#if 0
    if (node->flags & UI_Flag_Draw_Shadow) {
      V2F position = region.min;
      V2F size     = v2f_sub(region.max, region.min);
      position = v2f_sub(position, v2f(10.f, 10.f));
      size     = v2f_add(size,     v2f(20.f, 20.f));

      g2_draw_rect(position, size, .color = v4f(0, 0, 0, .05f));
    }
#endif

    // TODO(cmat): Force clipping to be hierarchical.
    R2I clip_region = context->clip_region;
    if (node->flags & UI_Flag_Draw_Clip_Content) {
      clip_region = r2i(region.x0, region.y0, region.x1, region.y1);
    }

    g2_clip_region(clip_region);


    if (node->flags & UI_Flag_Draw_Background) {
      HSV hsv_color = { };
      hsv_color = node->palette.idle;
      hsv_color = v3f_lerp(node->animation.hover_t, hsv_color, node->palette.hover);
      hsv_color = v3f_lerp(node->animation.down_t,  hsv_color, node->palette.down);

      V4F rgb_color = { 0 };
      rgb_color.rgb = rgb_from_hsv(hsv_color);
      rgb_color.a   = opacity;

      V2F position = region.min;
      V2F size     = v2f_sub(region.max, region.min);

      V4F border_rgba = { 0 };
      border_rgba.rgb = rgb_from_hsv(node->palette.border);
      border_rgba.a   = opacity;

      if (node->flags & UI_Flag_Draw_Border) {
        g2_draw_rect_rounded(position, size, rounding, .segments = 5, .inner_color = border_rgba, .outer_color = border_rgba);
        g2_draw_rect_rounded(v2f(position.x + 1, position.y + 1), v2f(size.x - 2, size.y - 2), rounding, .segments = 5, .inner_color = rgb_color, .outer_color = rgb_color);
      } else {
        g2_draw_rect_rounded(position, size, rounding, .segments = 5, .inner_color = rgb_color, .outer_color = rgb_color);
      }
    }

    if (node->flags & UI_Flag_Draw_Inner_Fill) {
      RGBA rgb_color = { };
      rgb_color.rgb = rgb_from_hsv(node->palette.inner_fill);
      rgb_color.a   = opacity;

      R2F region_inner_fill = r2f(region.x0 + node->draw.inner_fill_border,
                                  region.y0 + node->draw.inner_fill_border,
                                  region.x1 - node->draw.inner_fill_border,
                                  region.y1 - node->draw.inner_fill_border);

      V2F position = region_inner_fill.min;
      V2F size     = v2f_sub(region_inner_fill.max, region_inner_fill.min);
      if (size.x >= 0.1f) {
        g2_draw_rect_rounded(position, size, size.x/4, .segments=5, .inner_color = rgb_color, .outer_color = rgb_color);
      }
    }

    if (node->flags & UI_Flag_Draw_Content_Hook) {
      Assert(node->draw.content_hook, "draw content hook not set");
      If_Likely (node->draw.content_hook) {
        node->draw.content_hook(&node->response, region, node->draw.content_user_data);
      }
    }

    if (node->flags & UI_Flag_Draw_Label && node->draw.font) {
      V2F text_at = v2f(0, 0);

      if (node->flags & UI_Flag_Draw_Label_Centered) {
        V2F bounds  = v2f(fo_text_width(&node->draw.font->font, node->solved.text), node->draw.font->font.metric_height);
        text_at.x   = region.min.x + .5f * ((region.max.x - region.min.x) - bounds.x);
        text_at.y   = region.min.y + .5f * ((region.max.y - region.min.y) - bounds.y);
        text_at.y  -= node->draw.font->font.metric_descent;
      } else {
        text_at = v2f_add(region.min, v2f(0, -node->draw.font->font.metric_descent));
        text_at = v2f_add(text_at, v2f(node->layout.gap_border[Axis2_X], node->layout.gap_border[Axis2_Y]));
      }

      // TODO(cmat): Only floor if there is no active animation.
      text_at = v2f(f32_floor(text_at.x), f32_floor(text_at.y));
      g2_draw_text(node->solved.text, node->draw.font, v2f_add(text_at, v2f(+1, -1)), .color = v4f(0, 0, 0, opacity * .6f), .flip = node->draw.flip_label);
      g2_draw_text(node->solved.text, node->draw.font, text_at, .color = v4f(1, 1, 1, opacity), .flip = node->draw.flip_label);
    }

    UI_Draw_Context child_context = {
      .opacity     = opacity,
      .clip_region = clip_region,
    };

    for (UI_Node *it = node->tree.first_child; it; it = it->tree.next) {
      ui_draw(it, &child_context);
    }
  }
}

fn_internal void ui_frame_flush(void) {
  ui_parent_pop();
  Assert(UI_State.parent_stack_len == 0, "mismatched ui_parent_push(), pop missing");

  UI_Draw_Context draw_context = {
    .opacity = 1.f,
    .clip_region = G2_Clip_None,
  };

  // NOTE(cmat): Solve and draw root.
  ui_solve  (UI_State.root);
  ui_draw   (UI_State.root, &draw_context);

  // NOTE(cmat): Solve and draw overlays.
  for (UI_Node *it = UI_State.overlay_list.first; it; it = it->overlay_next) {
    ui_solve  (it);
    ui_draw   (it, &draw_context);
  }

  // NOTE(cmat): Solve and draw context menu.
  ui_solve  (UI_State.context);
  ui_draw   (UI_State.context, &draw_context);

  // NOTE(cmat): Reset frame arena.
  arena_clear(&UI_State.arena_frame);

  // --
  // NOTE(cmat): Setup next frame.

  UI_State.root = ui_node_push(UI_Root_Label, UI_Flag_None);
  ui_parent_push(UI_State.root);

  UI_State.root->layout.size[Axis2_X] = UI_Size_Fixed(pl_display()->resolution.x);
  UI_State.root->layout.size[Axis2_Y] = UI_Size_Fixed(pl_display()->resolution.y);

  // NOTE(cmat): Clear overlay list.
  UI_State.overlay_list.first = 0;
  UI_State.overlay_list.last  = 0;

  UI_State.context_last_frame = UI_State.context;
  UI_State.context            = 0;
  UI_State.context_spawned    = 0;

  UI_State.context_defining   = 0;
  UI_State.context_collapse   = 0;
}

// NOTE(cmat): Containers. These are for convenience.

typedef U32 UI_Container_Mode;
enum {
  UI_Container_None, // NOTE(cmat): Invisible
  UI_Container_Box,
  UI_Container_Float,
  UI_Container_Box_Choice,
};

fn_internal UI_Node *ui_container(Str label, UI_Container_Mode mode, Axis2 layout_direction, UI_Size size_x, UI_Size size_y) {
  UI_Flags flags = 0;

  switch (mode) {
    case UI_Container_Float: {
      flags = UI_Flag_Response_Hover  |
              UI_Flag_Response_Down   |
              UI_Flag_Draw_Background |
              UI_Flag_Layout_Float_X  |
              UI_Flag_Layout_Float_Y  |
              UI_Flag_Animation_Grow_X |
              UI_Flag_Animation_Grow_Y |
              UI_Flag_Animation_Fade_In |
              UI_Flag_Draw_Clip_Content |
              UI_Flag_Draw_Rounded      |
              UI_Flag_Draw_Shadow |
              UI_Flag_Draw_Border;
    } break;
    case UI_Container_Box: {
      flags = UI_Flag_Response_Hover  |
              UI_Flag_Response_Down   |
              UI_Flag_Response_Press  |
              UI_Flag_Response_Drag   |
              UI_Flag_Draw_Background |
              UI_Flag_Draw_Border;
    } break;
    case UI_Container_Box_Choice: {
      flags = UI_Flag_Response_Hover  |
              UI_Flag_Response_Down   |
              UI_Flag_Response_Press  |
              UI_Flag_Response_Drag   |
              UI_Flag_Draw_Background |
              UI_Flag_Draw_Border     |
              UI_Flag_Trigger_Context_Collapse;
    } break;
    case UI_Container_None: {
      flags = UI_Flag_Response_Hover  |
              UI_Flag_Response_Down   |
              UI_Flag_Response_Press  |
              UI_Flag_Response_Drag;
    } break;
  }

  UI_Node *node = ui_node_push(label, flags);
  node->layout.direction           = layout_direction;
  node->layout.size[Axis2_X]       = size_x;
  node->layout.size[Axis2_Y]       = size_y;

  if (mode == UI_Container_Box) {
    node->layout.gap_border[Axis2_X] = 4;
    node->layout.gap_border[Axis2_Y] = 4;
    node->layout.gap_child           = 2;
  }

  node->palette = UI_Theme_Active.box;
  return node;
}

// NOTE(cmat): Context menu 
// - The user can spawn and attach a context-menu node to a given node.
// - This is used for UI nodes that spawn other elements, such as 
// - a dropdown menu for instance, or right-clicking a component.

fn_internal void ui_context_spawn(UI_Node *owner, V2F float_position) { 
  UI_State.context_position = float_position;
  UI_State.context_owner    = owner;
  UI_State.context_spawned  = 1;
}

#define UI_Context_Scope(owner_, label_) \
  for (B32 it = ui_context_start_if_active(owner_, label_); it; it = (ui_context_end(), 0))

fn_internal B32 ui_context_start_if_active(UI_Node *owner, Str label) {
  B32 result = 0;
  if (UI_State.context_owner && UI_State.context_owner == owner) {
    result = 1;

    UI_State.context_defining = 1;

    ui_parent_push(owner);
    ui_next_location(UI_Node_Location_Context);
    UI_Node *context                        = ui_container(label, UI_Container_Float, Axis2_Y, UI_Size_Fit, UI_Size_Fit);
    context->layout.float_position[Axis2_X] = UI_State.context_position.x;
    context->layout.float_position[Axis2_Y] = UI_State.context_position.y;

    ui_parent_push(context);
  }

  return result;
}

fn_internal void ui_context_end(void) {
  ui_parent_pop(); // context
  ui_parent_pop(); // node

  UI_State.context_defining = 0;
  
  // NOTE(cmat): Close context if input detected.
  if (!UI_State.context_spawned && UI_State.context) {

    R2F region = UI_State.context->solved.region_absolute;
    if (UI_State.context_collapse || (!r2f_contains_v2f(region, pl_input()->mouse.position) && pl_input()->mouse.left.press)) {
      UI_State.context_owner = 0;
      UI_State.context       = 0;
    }
  }
}

// ------------------------------------------------------------
// #-- NOTE(cmat): UI component definitions

fn_internal UI_Response ui_label(Str label) {
  UI_Flags flags = UI_Flag_Draw_Label;

  UI_Node *node = ui_node_push(label, flags);
  node->layout.size[Axis2_X] = UI_Size_Text;
  node->layout.size[Axis2_Y] = UI_Size_Text;
  node->layout.gap_border[Axis2_X] = fo_em(&node->draw.font->font, .1f);
  node->layout.gap_border[Axis2_Y] = fo_em(&node->draw.font->font, .1f);

  return node->response;
}

fn_internal UI_Response ui_separator(Str label) {
  UI_Flags flags = UI_Flag_Draw_Background;

  UI_Node *node = ui_node_push(label, flags);
  node->layout.size[Axis2_X] = UI_Size_Fill;
  node->layout.size[Axis2_Y] = UI_Size_Fixed(1);

  node->palette = UI_Theme_Active.separator;

  return node->response;
}

fn_internal UI_Response ui_button(Str label) {
  UI_Flags flags =
    UI_Flag_Response_Hover   |
    UI_Flag_Response_Down    |
    UI_Flag_Response_Press   |
    UI_Flag_Draw_Background  |
    UI_Flag_Draw_Border      |
    UI_Flag_Draw_Label       |
    UI_Flag_Draw_Rounded     |
    UI_Flag_Trigger_Context_Collapse;

  UI_Node *node                    = ui_node_push(label, flags);
  node->layout.size[Axis2_X]       = UI_Size_Text;
  node->layout.size[Axis2_Y]       = UI_Size_Text;
  node->layout.gap_border[Axis2_X] = fo_em(&node->draw.font->font, .25f);
  node->layout.gap_border[Axis2_Y] = fo_em(&node->draw.font->font, .1f);

  node->palette = UI_Theme_Active.button;
  return node->response;
}

fn_internal UI_Response ui_button_flipped(Str label) {
  UI_Flags flags =
    UI_Flag_Response_Hover   |
    UI_Flag_Response_Down    |
    UI_Flag_Response_Press   |
    UI_Flag_Draw_Background  |
    UI_Flag_Draw_Border      |
    UI_Flag_Draw_Label       |
    UI_Flag_Draw_Rounded     |
    UI_Flag_Trigger_Context_Collapse;

  UI_Node *node                    = ui_node_push(label, flags);
  node->layout.size[Axis2_X]       = UI_Size_Text;
  node->layout.size[Axis2_Y]       = UI_Size_Text;
  node->layout.gap_border[Axis2_X] = fo_em(&node->draw.font->font, .25f);
  node->layout.gap_border[Axis2_Y] = fo_em(&node->draw.font->font, .1f);

  node->palette         = UI_Theme_Active.button;
  node->draw.flip_label = 1;
  return node->response;
}

fn_internal UI_Response ui_button_entry(Str label) {
  UI_Node *container = ui_container(label, UI_Container_Box, Axis2_X, UI_Size_Fill, UI_Size_Fit);
  container->palette = UI_Theme_Active.button;

  UI_Parent_Scope(container) {
    UI_Flags flags = UI_Flag_Response_Hover   |
                     UI_Flag_Response_Down    |
                     UI_Flag_Response_Press   |
                     UI_Flag_Draw_Label       |
                     UI_Flag_Trigger_Context_Collapse;

    UI_Node *node                    = ui_node_push(label, flags);
    node->layout.size[Axis2_X]       = UI_Size_Text;
    node->layout.size[Axis2_Y]       = UI_Size_Text;
    node->layout.gap_border[Axis2_X] = fo_em(&node->draw.font->font, .25f);
    node->layout.gap_border[Axis2_Y] = fo_em(&node->draw.font->font, .1f);

    ui_container(str_lit("##padding_right"), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fill);
  }

  return container->response;
}

fn_internal UI_Response ui_checkbox_fixed(Str label, B32 *value) {
  UI_Response response = { };

  Assert(value, "value is null");
  If_Likely (value) {
    UI_Node *container = ui_container(label, UI_Container_None, Axis2_X, UI_Size_Fit, UI_Size_Fit);
    container->layout.gap_child = 5.0f;

    UI_Parent_Scope(container) {
      ui_label(label);

      ui_container(str_lit("##padding"), UI_Container_None, Axis2_X, UI_Size_Fixed(5), UI_Size_Fill);

      UI_Flags flags =
        UI_Flag_Response_Hover   |
        UI_Flag_Response_Down    |
        UI_Flag_Response_Press   |
        UI_Flag_Draw_Background  |
        UI_Flag_Draw_Border      |
        UI_Flag_Draw_Inner_Fill  |
        UI_Flag_Draw_Rounded     |
        UI_Flag_Trigger_Context_Collapse;


      UI_Node *node = ui_node_push(str_lit("checkbox"), flags);

      F32 checkbox_size          = fo_em(&node->draw.font->font, 1.f);
      node->layout.size[Axis2_X] = UI_Size_Fixed(checkbox_size);
      node->layout.size[Axis2_Y] = UI_Size_Fixed(checkbox_size);

      node->palette = UI_Theme_Active.checkbox;

      if (node->response.press) {
        *value = !(*value);
      }

      F32 border_active   = .18f * checkbox_size;
      F32 border_inactive = .5f  * checkbox_size;

      node->draw.inner_fill_border = border_inactive;
      if (*value) {
        if (node->response.down) {
          node->draw.inner_fill_border = f32_lerp(node->animation.down_t, border_inactive, border_active);
        } else {
          node->draw.inner_fill_border = border_active;
        }

      } else {
        if (node->response.down) {
          node->draw.inner_fill_border = f32_lerp(node->animation.down_t, border_active, border_inactive);
        } else {
          node->draw.inner_fill_border = border_inactive;
        }
      }

      response = node->response;
    }
  }

  return response;
}

fn_internal UI_Response ui_checkbox(Str label, B32 *value) {
  UI_Response response = { };

  Assert(value, "value is null");
  If_Likely (value) {
    UI_Node *container = ui_container(label, UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fit);
    container->layout.gap_child = 5.0f;

    UI_Parent_Scope(container) {
      ui_label(label);

      ui_container(str_lit("##padding"), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fill);

      UI_Flags flags =
        UI_Flag_Response_Hover   |
        UI_Flag_Response_Down    |
        UI_Flag_Response_Press   |
        UI_Flag_Draw_Background  |
        UI_Flag_Draw_Border      |
        UI_Flag_Draw_Inner_Fill  |
        UI_Flag_Draw_Rounded     |
        UI_Flag_Trigger_Context_Collapse;


      UI_Node *node = ui_node_push(str_lit("checkbox"), flags);

      F32 checkbox_size          = fo_em(&node->draw.font->font, 1.f);
      node->layout.size[Axis2_X] = UI_Size_Fixed(checkbox_size);
      node->layout.size[Axis2_Y] = UI_Size_Fixed(checkbox_size);

      node->palette = UI_Theme_Active.checkbox;

      if (node->response.press) {
        *value = !(*value);
      }

      F32 border_active   = .18f * checkbox_size;
      F32 border_inactive = .5f  * checkbox_size;

      node->draw.inner_fill_border = border_inactive;
      if (*value) {
        if (node->response.down) {
          node->draw.inner_fill_border = f32_lerp(node->animation.down_t, border_inactive, border_active);
        } else {
          node->draw.inner_fill_border = border_active;
        }

      } else {
        if (node->response.down) {
          node->draw.inner_fill_border = f32_lerp(node->animation.down_t, border_active, border_inactive);
        } else {
          node->draw.inner_fill_border = border_inactive;
        }
      }

      response = node->response;
    }
  }

  return response;
}

fn_internal void ui_f32_edit_static(Str label, F32 *value, F32 value_min, F32 value_max, F32 step) {
  UI_Parent_Scope(ui_container(label, UI_Container_None, Axis2_X, UI_Size_Fit, UI_Size_Fit)) {
    ui_label(label);

    ui_container(str_lit("##padding_1"), UI_Container_None, Axis2_X, UI_Size_Fixed(5), UI_Size_Fit);

    UI_Flags flags = 
      UI_Flag_Response_Hover   |
      UI_Flag_Response_Down    |
      UI_Flag_Response_Drag    |
      UI_Flag_Response_Press   |
      UI_Flag_Draw_Background  |
      UI_Flag_Draw_Border      |
      UI_Flag_Draw_Label;

    char button_label_buffer[512] = { };
    Str  button_label = { .txt = (U08 *)button_label_buffer, .len = 0 };
    button_label.len  = stbsp_snprintf(button_label_buffer, 512, "%.2f###button", *value);

    UI_Node *node = ui_node_push(button_label, flags);

    char largest_label_size[512] = { };
    stbsp_snprintf(largest_label_size, 512, "%.2f", value_max);
    F32 width = fo_text_width(&node->draw.font->font, str_from_cstr(largest_label_size));

    stbsp_snprintf(largest_label_size, 512, "%.2f", value_min);
    width = f32_max(width, fo_text_width(&node->draw.font->font, str_from_cstr(largest_label_size)));

    node->layout.size[Axis2_X]       = UI_Size_Fixed(width);
    node->layout.size[Axis2_Y]       = UI_Size_Text;
    node->layout.gap_border[Axis2_X] = fo_em(&node->draw.font->font, .25f);
    node->layout.gap_border[Axis2_Y] = fo_em(&node->draw.font->font, .1f);

    node->palette = UI_Theme_Active.edit_value;

    if (node->response.drag) {
      *value = f32_clamp(*value + step * node->response.drag_change.x, value_min, value_max);
    }
  }
}

fn_internal void ui_f32_edit(Str label, F32 *value, F32 value_min, F32 value_max, F32 step) {
  UI_Parent_Scope(ui_container(label, UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fit)) {
    ui_label(label);

    ui_container(str_lit("##padding_1"), UI_Container_None, Axis2_X, UI_Size_Fixed(5), UI_Size_Fit);
    ui_container(str_lit("##padding_2"), UI_Container_None, Axis2_X, UI_Size_Fill,      UI_Size_Fill);

    UI_Flags flags = 
      UI_Flag_Response_Hover   |
      UI_Flag_Response_Down    |
      UI_Flag_Response_Drag    |
      UI_Flag_Response_Press   |
      UI_Flag_Draw_Background  |
      UI_Flag_Draw_Border      |
      UI_Flag_Draw_Label;

    char button_label_buffer[512] = { };
    Str  button_label = { .txt = (U08 *)button_label_buffer, .len = 0 };
    button_label.len  = stbsp_snprintf(button_label_buffer, 512, "%.2f###button", *value);

    UI_Node *node = ui_node_push(button_label, flags);

    char largest_label_size[512] = { };
    stbsp_snprintf(largest_label_size, 512, "%.2f", value_max);
    F32 width = fo_text_width(&node->draw.font->font, str_from_cstr(largest_label_size));

    stbsp_snprintf(largest_label_size, 512, "%.2f", value_min);
    width = f32_max(width, fo_text_width(&node->draw.font->font, str_from_cstr(largest_label_size)));

    node->layout.size[Axis2_X]       = UI_Size_Fixed(width);
    node->layout.size[Axis2_Y]       = UI_Size_Text;
    node->layout.gap_border[Axis2_X] = fo_em(&node->draw.font->font, .25f);
    node->layout.gap_border[Axis2_Y] = fo_em(&node->draw.font->font, .1f);
    node->palette = UI_Theme_Active.edit_value;

    if (node->response.drag) {
      *value = f32_clamp(*value + step * node->response.drag_change.x, value_min, value_max);
    }
  }
}

fn_internal void ui_list(Str label, I32 *entry_selection, I32 entry_count, Str *entry_list) {
  *entry_selection = i32_clamp(*entry_selection, 0, entry_count - 1);

  UI_Parent_Scope(ui_container(label, UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fit)) {

    char buffer[512] = {};
    stbsp_snprintf(buffer, 512, "%.*s##label", str_expand(label));
    ui_label(str_from_cstr(buffer));

    ui_container(str_lit("##padding_1"), UI_Container_None, Axis2_X, UI_Size_Fixed(5), UI_Size_Fit);
    ui_container(str_lit("##padding_2"), UI_Container_None, Axis2_X, UI_Size_Fill,      UI_Size_Fill);


    UI_Node *container = ui_container(label, UI_Container_Box, Axis2_X, UI_Size_Fit, UI_Size_Fit);
    container->flags  |= UI_Flag_Draw_Rounded;

    container->palette = UI_Theme_Active.list;

    container->layout.gap_border[Axis2_X] = 0;
    container->layout.gap_border[Axis2_Y] = 0;
    container->layout.gap_child           = 0;

    UI_Parent_Scope(container) {
      UI_Flags flags = UI_Flag_Response_Hover   |
                       UI_Flag_Response_Down    |
                       UI_Flag_Response_Press   |
                       UI_Flag_Draw_Label;

      ui_label(entry_list[*entry_selection]);
    }

    if (container->response.press) {
      V2F spawn_at = v2f(container->solved.region_absolute.x0, pl_display()->resolution.y - container->solved.region_absolute.y0);
      ui_context_spawn(container, spawn_at);
    }

    UI_Context_Scope(container, str_lit("##dropdown")) {
      For_U32(it, entry_count) {
        UI_Node *entry_container = ui_container(entry_list[it], UI_Container_Box_Choice, Axis2_X, UI_Size_Fill, UI_Size_Fit);

        entry_container->palette = UI_Theme_Active.list;

        UI_Parent_Scope(entry_container) {
          ui_label(entry_list[it]);
        }

        if (entry_container->response.press) {
          *entry_selection = it;
        }
      }
    }
  }
}

fn_internal B32 ui_list_fixed(Str label, I32 *entry_selection, I32 entry_count, Str *entry_list) {
  B32 result = 0;
  *entry_selection = i32_clamp(*entry_selection, 0, entry_count - 1);

  UI_Parent_Scope(ui_container(label, UI_Container_None, Axis2_X, UI_Size_Fit, UI_Size_Fit)) {

    char buffer[512] = {};
    stbsp_snprintf(buffer, 512, "%.*s##label", str_expand(label));
    ui_label(str_from_cstr(buffer));

    ui_container(str_lit("##padding_1"), UI_Container_None, Axis2_X, UI_Size_Fixed(5), UI_Size_Fit);
    // ui_container(str_lit("##padding_2"), UI_Container_None, Axis2_X, UI_Size_Fill,      UI_Size_Fill);


    UI_Node *container = ui_container(label, UI_Container_Box, Axis2_X, UI_Size_Fit, UI_Size_Fit);
    container->flags  |= UI_Flag_Draw_Rounded;

    container->palette = UI_Theme_Active.list;

    container->layout.gap_border[Axis2_X] = 0;
    container->layout.gap_border[Axis2_Y] = 0;
    container->layout.gap_child           = 0;

    UI_Parent_Scope(container) {
      UI_Flags flags = UI_Flag_Response_Hover   |
                       UI_Flag_Response_Down    |
                       UI_Flag_Response_Press   |
                       UI_Flag_Draw_Label;

      ui_label(entry_list[*entry_selection]);
    }

    if (container->response.press) {
      V2F spawn_at = v2f(container->solved.region_absolute.x0, pl_display()->resolution.y - container->solved.region_absolute.y0);
      ui_context_spawn(container, spawn_at);
    }

    UI_Context_Scope(container, str_lit("##dropdown")) {
      For_U32(it, entry_count) {
        UI_Node *entry_container = ui_container(entry_list[it], UI_Container_Box_Choice, Axis2_X, UI_Size_Fill, UI_Size_Fit);

        entry_container->palette = UI_Theme_Active.list;

        UI_Parent_Scope(entry_container) {
          ui_label(entry_list[it]);
        }

        if (entry_container->response.press) {
          *entry_selection = it;
          result = 1;
        }
      }
    }
  }

  return result;
}


typedef struct UI_Dropdown_Entry {
  G2_Font  *icon_font;
  Str       icon;
  Str       text;
  Str       keybind;
} UI_Dropdown_Entry;

fn_internal void ui_dropdown(Str label, I32 *entry_selection, I32 entry_count, UI_Dropdown_Entry *entry_list) {
  *entry_selection = i32_clamp(*entry_selection, 0, entry_count - 1);

  UI_Node *container       = ui_container(label, UI_Container_Box, Axis2_X, UI_Size_Fit, UI_Size_Fit);
  container->flags        |= UI_Flag_Draw_Rounded;
  container->palette       = UI_Theme_Active.list;

  container->layout.gap_border[Axis2_X] = 0;
  container->layout.gap_border[Axis2_Y] = 0;
  container->layout.gap_child           = 0;

  UI_Parent_Scope(container) {
    UI_Flags flags = UI_Flag_Response_Hover   |
                     UI_Flag_Response_Down    |
                     UI_Flag_Response_Press   |
                     UI_Flag_Draw_Label;

    UI_Dropdown_Entry *entry = entry_list + *entry_selection;
    UI_Font_Scope(entry->icon_font) { ui_label(entry->icon); };
    ui_label(entry->text);
  }

  if (container->response.press) {
    V2F spawn_at = v2f(container->solved.region_absolute.x0, pl_display()->resolution.y - container->solved.region_absolute.y0);
    ui_context_spawn(container, spawn_at);
  }

  UI_Context_Scope(container, str_lit("##dropdown")) {
    For_U32(it, entry_count) {
      UI_Dropdown_Entry *entry = entry_list + it;

      UI_Node *entry_container = ui_container(entry->text, UI_Container_Box_Choice, Axis2_X, UI_Size_Fill, UI_Size_Fit);
      entry_container->palette = UI_Theme_Active.list;

      UI_Parent_Scope(entry_container) {
        UI_Font_Scope(entry->icon_font) { ui_label(entry->icon); };
        ui_label(entry->text);
        ui_container(str_lit("##padding_center"), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fill);
        ui_container(str_lit("##gap"), UI_Container_None, Axis2_X, UI_Size_Fixed(40), UI_Size_Fill);
        ui_label(entry->keybind);
      }

      if (entry_container->response.press) {
        *entry_selection = it;
      }
    }
  }
}

fn_internal void ui_hook_draw_color_picker_hsv_sv(UI_Response *response, R2F draw_region, void *user_data) {
  U32 res_x = 50;
  U32 res_y = 50;
  V2F scale = v2f((draw_region.x1 - draw_region.x0) / (F32)(res_x + 1), (draw_region.y1 - draw_region.y0) / (F32)(res_y + 1));

  F32 h = 0;
  memory_copy(&h, &user_data, sizeof(F32));
  For_U32(v, res_y + 1) {
    For_U32(s, res_x + 1) {
      RGBA colors[] = {
        rgba_from_hsva(v4f(h, (F32)(s + 0) / (F32)(res_x + 1), (F32)(v + 0) / (F32)(res_y + 1), 1.f)),
        rgba_from_hsva(v4f(h, (F32)(s + 1) / (F32)(res_x + 1), (F32)(v + 0) / (F32)(res_y + 1), 1.f)),
        rgba_from_hsva(v4f(h, (F32)(s + 1) / (F32)(res_x + 1), (F32)(v + 1) / (F32)(res_y + 1), 1.f)),
        rgba_from_hsva(v4f(h, (F32)(s + 0) / (F32)(res_x + 1), (F32)(v + 1) / (F32)(res_y + 1), 1.f)),
      };

      V2F uvs[] = {
        v2f(0.f, 0.f),
        v2f(1.f, 0.f),
        v2f(1.f, 1.f),
        v2f(0.f, 1.f),
      };

      g2_draw_rect_grad(v2f(draw_region.x0 + scale.x * s, draw_region.y0 + scale.y * v), scale, colors, uvs);
    }
  }

  F32 picker_size = 6;

  V2F size        = r2f_size(draw_region);
  V2F sv          = v2f(response->picked_position.x / size.x, response->picked_position.y / size.y);
  V2F picker_at   = v2f_had(sv, v2f(draw_region.x1 - draw_region.x0, draw_region.y1 - draw_region.y0));
  picker_at       = v2f_add(draw_region.min, picker_at);

  RGB rgb = rgb_from_hsv(v3f(h, sv.x, sv.y));
  g2_draw_disk(picker_at, picker_size + 2, .resolution = 32, .color = v4f(0, 0, 0, 1));
  g2_draw_disk(picker_at, picker_size + 1, .resolution = 32, .color = v4f(1, 1, 1, 1));
  g2_draw_disk(picker_at, picker_size,     .resolution = 32, .color = v4f(rgb.r, rgb.g, rgb.b, 1));
}

fn_internal void ui_hook_draw_color_picker_hsv_h(UI_Response *response, R2F draw_region, void *) {
  U32 res   = 50;
  F32 scale = (draw_region.y1 - draw_region.y0) / (F32)(res + 1);
  For_U32(it, res + 1) {
    RGB  rgb  = rgb_from_hsv(v3f((F32)it / (F32)res, 1.f, 1.f));
    RGBA rgba = v4f(rgb.r, rgb.g, rgb.b, 1.f);
 
    RGBA colors[] = {
      rgba_from_hsva(v4f((F32)(it + 0) / (res + 1), 1.f, 1.f, 1.f)),
      rgba_from_hsva(v4f((F32)(it + 0) / (res + 1), 1.f, 1.f, 1.f)),
      rgba_from_hsva(v4f((F32)(it + 1) / (res + 1), 1.f, 1.f, 1.f)),
      rgba_from_hsva(v4f((F32)(it + 1) / (res + 1), 1.f, 1.f, 1.f)),
    };

    V2F uvs[] = {
      v2f(0.f, 0.f),
      v2f(1.f, 0.f),
      v2f(1.f, 1.f),
      v2f(0.f, 1.f),
    };

    g2_draw_rect_grad(v2f(draw_region.x0, draw_region.y0 + scale * it), v2f(draw_region.x1 - draw_region.x0, scale), colors, uvs);
  }

  V2F size = r2f_size(draw_region);
  F32 h    = response->picked_position.y / size.y;
  F32 y    = draw_region.y0 + (h * size.y);
  g2_draw_tri(v2f(draw_region.x0 - 8*1.25f, y - 5*1.25f), v2f(draw_region.x0 - 1*1.25f, y), v2f(draw_region.x0 - 8*1.25f, y + 5*1.25f));
}

fn_internal void ui_color_picker_hsv(Str label, HSV *color) {
  UI_Node *container = ui_container(label, UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fill);
  container->layout.gap_child = 15.f;
  container->layout.gap_border[Axis2_X] = 7.f;
  container->layout.gap_border[Axis2_Y] = 7.f;
  UI_Parent_Scope(container) {

    UI_Node *sv_picker = ui_container(str_lit("##sv_picker"), UI_Container_None, Axis2_X, UI_Size_Fixed(256), UI_Size_Fixed(256));
    sv_picker->flags |= UI_Flag_Draw_Content_Hook;
    sv_picker->draw.content_hook = ui_hook_draw_color_picker_hsv_sv;
    memory_copy(&sv_picker->draw.content_user_data, &color->h, sizeof(F32));

    sv_picker->layout.gap_border[Axis2_X] = 0;
    sv_picker->layout.gap_border[Axis2_Y] = 0;

    if (sv_picker->response.drag) {
      V2F size = r2f_size(sv_picker->solved.region_absolute);
      V2F sv   = v2f(sv_picker->response.picked_position.x / size.x, sv_picker->response.picked_position.y / size.y);
      color->sv = sv;
    } else {
      V2F size = r2f_size(sv_picker->solved.region_absolute);
      sv_picker->response.picked_position = v2f_had(color->sv, size);
    }

    UI_Node *h_picker = ui_container(str_lit("##h_picker"), UI_Container_None, Axis2_X, UI_Size_Fixed(25), UI_Size_Fixed(256));
    h_picker->flags |= UI_Flag_Draw_Content_Hook;
    h_picker->draw.content_hook = ui_hook_draw_color_picker_hsv_h;
    h_picker->layout.gap_border[Axis2_X] = 0;
    h_picker->layout.gap_border[Axis2_Y] = 0;
 
    if (h_picker->response.drag) {
      V2F size = r2f_size(h_picker->solved.region_absolute);
      F32 h    = h_picker->response.picked_position.y / size.y;
      color->h = h;
    } else {
      V2F size = r2f_size(h_picker->solved.region_absolute);
      h_picker->response.picked_position.y = color->h * size.y;
    }

    UI_Node *numeric_edit = ui_container(str_lit("##numeric"), UI_Container_None, Axis2_Y, UI_Size_Fit, UI_Size_Fill);
    numeric_edit->layout.gap_child = 5.f;
    UI_Parent_Scope(numeric_edit) {

      V3F color_hsv = v3f((I32)(color->h * 360), (I32)(color->s * 100), (I32)(color->v * 100));
      ui_container(str_lit("##padding_1"), UI_Container_None, Axis2_Y, UI_Size_Fit, UI_Size_Fill);
      ui_f32_edit(str_lit("H"), &color_hsv.h, 0.0f, 360.0f, 1.0f / 255.f);
      ui_f32_edit(str_lit("S"), &color_hsv.s, 0.0f, 100.0f, 1.0f / 255.f);
      ui_f32_edit(str_lit("V"), &color_hsv.v, 0.0f, 100.0f, 1.0f / 255.f);

      ui_container(str_lit("##padding_2"), UI_Container_None, Axis2_Y, UI_Size_Fit, UI_Size_Fill);

      RGB color_rgb = rgb_from_hsv(*color);
      color_rgb = v3f((I32)(color_rgb.r * 255), (I32)(color_rgb.g * 255), (I32)(color_rgb.b * 255));
      ui_f32_edit(str_lit("R"), &color_rgb.r, 0.0f, 255.0f, 1.0f / 255.f);
      ui_f32_edit(str_lit("G"), &color_rgb.g, 0.0f, 255.0f, 1.0f / 255.f);
      ui_f32_edit(str_lit("B"), &color_rgb.b, 0.0f, 255.0f, 1.0f / 255.f);

      ui_container(str_lit("##padding_3"), UI_Container_None, Axis2_Y, UI_Size_Fit, UI_Size_Fill);
    }
  }
}

fn_internal void ui_color_hsv(Str label, HSV *color_hsv) {
  UI_Parent_Scope(ui_container(label, UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fit)) {

    ui_label(label);
    ui_container(str_lit("##padding_1"), UI_Container_None, Axis2_X, UI_Size_Fixed(5), UI_Size_Fit);
    ui_container(str_lit("##padding_2"), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fit);

    UI_Flags flags =
      UI_Flag_Response_Hover   |
      UI_Flag_Response_Down    |
      UI_Flag_Response_Press   |
      UI_Flag_Draw_Background  |
      UI_Flag_Draw_Border      |
      UI_Flag_Draw_Rounded     |
      UI_Flag_Trigger_Context_Collapse;

    UI_Node *node                    = ui_node_push(str_lit("##color_button"), flags);
    node->layout.size[Axis2_X]       = UI_Size_Fixed(fo_em(&node->draw.font->font, 1.f));
    node->layout.size[Axis2_Y]       = UI_Size_Fixed(fo_em(&node->draw.font->font, 1.f));
    node->layout.gap_border[Axis2_X] = fo_em(&node->draw.font->font, .1f);
    node->layout.gap_border[Axis2_Y] = fo_em(&node->draw.font->font, .1f);

    node->palette.idle   = *color_hsv;
    node->palette.hover  = v3f(color_hsv->h, f32_min(color_hsv->s + .1f, 1.f), f32_min(color_hsv->v + .1f, 1.f));
    node->palette.down   = v3f(color_hsv->h, f32_max(color_hsv->s - .1f, 0.f), f32_min(color_hsv->v - .1f, 1.f));
    node->palette.border = hsv_u32(200, 50, 40);

    if (node->response.press) {
      V2F spawn_at = v2f(node->solved.region_absolute.x0, pl_display()->resolution.y - node->solved.region_absolute.y0 + 5);

      // TODO(cmat): Use actual size, or better yet, handle directly in spawn.
      if (spawn_at.x + 255 > pl_display()->resolution.x) {
        spawn_at.x -= 400;
      }

      ui_context_spawn(node, spawn_at);
    }

    UI_Context_Scope(node, str_lit("##color_picker")) {
      ui_color_picker_hsv(label, color_hsv);
    }
  }
}

fn_internal void ui_color_hsv_fixed(Str label, HSV *color_hsv) {
  UI_Parent_Scope(ui_container(label, UI_Container_None, Axis2_X, UI_Size_Fit, UI_Size_Fit)) {

    ui_label(label);
    ui_container(str_lit("##padding_1"), UI_Container_None, Axis2_X, UI_Size_Fixed(5), UI_Size_Fit);

    UI_Flags flags =
      UI_Flag_Response_Hover   |
      UI_Flag_Response_Down    |
      UI_Flag_Response_Press   |
      UI_Flag_Draw_Background  |
      UI_Flag_Draw_Border      |
      UI_Flag_Draw_Rounded     |
      UI_Flag_Trigger_Context_Collapse;

    UI_Node *node                    = ui_node_push(str_lit("##color_button"), flags);
    node->layout.size[Axis2_X]       = UI_Size_Fixed(fo_em(&node->draw.font->font, 1.f));
    node->layout.size[Axis2_Y]       = UI_Size_Fixed(fo_em(&node->draw.font->font, 1.f));
    node->layout.gap_border[Axis2_X] = fo_em(&node->draw.font->font, .1f);
    node->layout.gap_border[Axis2_Y] = fo_em(&node->draw.font->font, .1f);

    node->palette.idle   = *color_hsv;
    node->palette.hover  = v3f(color_hsv->h, f32_min(color_hsv->s + .1f, 1.f), f32_min(color_hsv->v + .1f, 1.f));
    node->palette.down   = v3f(color_hsv->h, f32_max(color_hsv->s - .1f, 0.f), f32_min(color_hsv->v - .1f, 1.f));
    node->palette.border = hsv_u32(200, 50, 40);

    if (node->response.press) {
      V2F spawn_at = v2f(node->solved.region_absolute.x0, pl_display()->resolution.y - node->solved.region_absolute.y0 + 5);
      ui_context_spawn(node, spawn_at);
    }

    UI_Context_Scope(node, str_lit("##color_picker")) {
      ui_color_picker_hsv(label, color_hsv);
    }
  }
}
