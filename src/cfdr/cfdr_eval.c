fn_internal CFDR_Value cfdr_eval_list(Arena *arena, TK_Scan *scan) {
  CFDR_Value value = { };
  value.type       = CFDR_Value_Type_List;
  value.list       = arena_push_type(arena, CFDR_List);

  B32 separated = 1;
  for (;;) {
    TK_Token token = tk_scan_look(scan);
    if (token.type == TK_Type_Block_Close_Bracket) {
      tk_scan_next(scan);
      break;
    } else if (!separated) {
      token = tk_scan_look(scan);
      if (token.type != TK_Type_Op_Comma) {
        log_fatal("expected command after element in list");
        break;
      } else {
        tk_scan_next(scan);
        token = tk_scan_look(scan);
        if (token.type == TK_Type_Block_Close_Bracket) {
          tk_scan_next(scan);
          break;
        }
      }
    }

    CFDR_List_Node *node  = arena_push_type(arena, CFDR_List_Node);
    node->value           = cfdr_eval_expr(arena, scan);

    value.list->count    += 1;
    queue_push(value.list->first, value.list->last, node);
    separated = 0;
  }

  return value;
}


fn_internal CFDR_Value cfdr_eval_table(Arena *arena, TK_Scan *scan) {
  CFDR_Value value = { };
  value.type       = CFDR_Value_Type_Table;
  value.table      = arena_push_type(arena, CFDR_Table);

  TK_Token token = tk_scan_next(scan);
  B32 separated = 1;
  for (;;) {
    if (token.type == TK_Type_Identifier) {
      if (separated) {
        if (tk_scan_require(scan, TK_Type_Op_Colon, 0)) {
          CFDR_Table_Node *node = arena_push_type(arena, CFDR_Table_Node);
          node->label = token.text;
          node->value = cfdr_eval_expr(arena, scan);
          queue_push(value.table->first, value.table->last, node);
        } else {
          log_fatal("missing : between identifier and value");
        }

      } else {
        log_fatal("invalid syntax for table, missing ','");
        break;
      }
    } else if (token.type == TK_Type_Block_Close_Brace) {
      break;
    } else {
      log_fatal("Invalid syntax for table, expecting an identifier");
      break;
    }

    token = tk_scan_next(scan);
    if (token.type == TK_Type_Op_Comma) {
      token     = tk_scan_next(scan);
      separated = 1;
    }
  }

  return value;
}

fn_internal V3F cfdr_eval_get_color(CFDR_Value value) {
  V3F result = v3f(0, 0, 0);
  if (value.type == CFDR_Value_Type_Color) {
    result = value.color;
  } else {
    log_fatal("type mismatch, expected a color value");
  }

  return result;
}

fn_internal B32 cfdr_eval_get_bool(CFDR_Value value) {
  B32 result = 0;
  if (value.type == CFDR_Value_Type_Bool) {
    result = value.b32;
  } else {
    log_fatal("type mismatch, expected a bool value (true or false)");
  }

  return result;
}

fn_internal Str cfdr_eval_get_str(CFDR_Value value) {
  Str result = { };
  if (value.type == CFDR_Value_Type_String) {
    result = value.str;
  } else {
    log_fatal("type mismatch, expected a string value");
  }

  return result;
}

fn_internal F32 cfdr_eval_get_f32(CFDR_Value value) {
  F32 result = 0;
  if (value.type == CFDR_Value_Type_Float) {
    result = value.f32;
  } else {
    log_fatal("type mismatch, expected an float value");
  }

  return result;
}

fn_internal I32 cfdr_eval_get_i32(CFDR_Value value) {
  return (I32)cfdr_eval_get_f32(value);
}

fn_internal Align2 cfdr_eval_get_align2(CFDR_Value value) {
  I32 result = 0;
  if (value.type == CFDR_Value_Type_Align) {
    result = value.align;
  } else {
    log_fatal("type mismatch, expected an #align value");
  }

  return result;
}
fn_internal V2I cfdr_eval_get_v2i(CFDR_Value value) {
  V2I result = { };
  if (value.type == CFDR_Value_Type_List) {
    if (value.list->count == 2) {
      I32 list_values[2] = { };
      
      I64 at = 0;
      for (CFDR_List_Node *it = value.list->first; it; it = it->next) {
        if (at >= value.list->count) { break; }
        list_values[at++] = (I32)it->value.f32;
      }

      result = v2i(list_values[0], list_values[1]);
    } else {
      log_fatal("expected a list of 2 integers");
    }
  } else {
    log_fatal("type mismatch, expected a list value");
  }

  return result;
}

fn_internal V3I cfdr_eval_get_v3i(CFDR_Value value) {
  V3I result = { };
  if (value.type == CFDR_Value_Type_List) {
    if (value.list->count == 3) {
      I32 list_values[3] = { };
      
      I64 at = 0;
      for (CFDR_List_Node *it = value.list->first; it; it = it->next) {
        if (at >= value.list->count) { break; }
        list_values[at++] = it->value.f32;
      }

      result = v3i(list_values[0], list_values[1], list_values[2]);
    } else {
      log_fatal("expected a list of 3 integers");
    }
  } else {
    log_fatal("type mismatch, expected a list value");
  }

  return result;
}

fn_internal V2F cfdr_eval_get_v2f(CFDR_Value value) {
  V2I result_v2i = cfdr_eval_get_v2i(value);
  V2F result     = v2f(result_v2i.x, result_v2i.y);
  return result;
}

fn_internal V3F cfdr_eval_get_v3f(CFDR_Value value) {
  V3I result_v3i = cfdr_eval_get_v3i(value);
  V3F result     = v3f(result_v3i.x, result_v3i.y, result_v3i.z);
  return result;
}

fn_internal CFDR_Value cfdr_eval_literal_def(Arena *arena, TK_Scan *scan) {
  CFDR_Value value = { };

  TK_Token token = tk_scan_next(scan);
  if (str_equals(token.text, str_lit("rgb"))) {
    value.type = CFDR_Value_Type_Color;
    CFDR_Value color_value = cfdr_eval_expr(arena, scan);
    if (color_value.type == CFDR_Value_Type_List) {
      if (color_value.list->count == 3) {
        V3I rgb = cfdr_eval_get_v3i(color_value);
        value.color = rgb_u32(rgb.x, rgb.y, rgb.z);
      } else {
        log_fatal("@rgb directive requires a list with 3 elements");
      }
    } else {
      log_fatal("expected list after @rgb directive");
    }
  } else if (str_equals(token.text, str_lit("hsv"))) {
    value.type = CFDR_Value_Type_Color;
    CFDR_Value color_value = cfdr_eval_expr(arena, scan);
    if (color_value.type == CFDR_Value_Type_List) {
      if (color_value.list->count == 3) {
        V3I hsv     = cfdr_eval_get_v3i(color_value);
        value.color = hsv_u32(hsv.x, hsv.y, hsv.z);
      } else {
        log_fatal("@hsv directive requires a list with 3 elements");
      }
    } else {
      log_fatal("expected list after @hsv directive");
    }
  } else if (str_equals(token.text, str_lit("align"))) {
    if (tk_scan_require(scan, TK_Type_Literal_String, &token)) {
      value.type = CFDR_Value_Type_Align;
      if        (str_equals(token.value.str, str_lit("center")))   { value.align = Align2_Center; }
      else if   (str_equals(token.value.str, str_lit("left")))     { value.align = Align2_Left;   }
      else if   (str_equals(token.value.str, str_lit("right")))    { value.align = Align2_Right;  }
      else if   (str_equals(token.value.str, str_lit("bottom")))   { value.align = Align2_Bottom; }
      else if   (str_equals(token.value.str, str_lit("top")))      { value.align = Align2_Top;    }
      else {
        log_fatal("string for @align directive must be one of \"center\", \"left\", \"right\", \"bottom\" or \"top\"");
      }
    } else {
      log_fatal("expected string after @align");
    }
  }

  return value;
}


fn_internal CFDR_Value cfdr_eval_expr(Arena *arena, TK_Scan *scan) {
  CFDR_Value value = { };

  TK_Token token = tk_scan_next(scan);
  if      (token.type == TK_Type_Block_Open_Bracket)  { value = cfdr_eval_list        (arena, scan);                                            }
  else if (token.type == TK_Type_Block_Open_Brace)    { value = cfdr_eval_table       (arena, scan);                                            }
  else if (token.type == TK_Type_Op_At)               { value = cfdr_eval_literal_def (arena, scan);                                            }
  else if (token.type == TK_Type_Literal_String)      { value = (CFDR_Value) { .type = CFDR_Value_Type_String,  .str = token.value.str };       }
  else if (token.type == TK_Type_Literal_Integer)     { value = (CFDR_Value) { .type = CFDR_Value_Type_Float,   .f32 = (I32)token.value.i64 };  }
  else if (token.type == TK_Type_Literal_Float)       { value = (CFDR_Value) { .type = CFDR_Value_Type_Float,   .f32 = (F32)token.value.f64 };  }
  else if (token.type == TK_Type_Op_Plus) {
    value = cfdr_eval_expr(arena, scan);
    if (value.type != CFDR_Value_Type_Float) {
      log_fatal("Expected float after '+'");
    } 
  } 
  else if (token.type == TK_Type_Op_Minus) {
    value = cfdr_eval_expr(arena, scan);
    if (value.type == CFDR_Value_Type_Float) {
      value.f32 *= -1;
    } else {
      log_fatal("Expected integer or float after '-'");
    }
  }
  else if (token.type == TK_Type_Identifier) {
    if      (str_equals(token.text, str_lit("true")))  { value = (CFDR_Value) { .type = CFDR_Value_Type_Bool, .b32 = 1 }; }
    else if (str_equals(token.text, str_lit("false"))) { value = (CFDR_Value) { .type = CFDR_Value_Type_Bool, .b32 = 0 }; }
  }

  return value;
}

fn_internal void cfdr_eval_directive_viewport(CFDR_State *state, Arena *arena, TK_Scan *scan) {
  CFDR_Value table = cfdr_eval_expr(arena, scan);
  if (table.type == CFDR_Value_Type_Table) {
    CFDR_Scene_View *vp = &state->scene.view;

    for (CFDR_Table_Node *it = table.table->first; it; it = it->next) {
      if      (str_equals(it->label, str_lit("background")))   { vp->background.hsv                 = cfdr_eval_get_color  (it->value); }
      else if (str_equals(it->label, str_lit("orthographic"))) { vp->camera.orthographic            = cfdr_eval_get_bool   (it->value); }
      else if (str_equals(it->label, str_lit("grid_enabled"))) { vp->grid.visible                   = cfdr_eval_get_bool   (it->value); }
      else if (str_equals(it->label, str_lit("grid_color")))   { vp->grid.color.hsv                 = cfdr_eval_get_color  (it->value); }
      else if (str_equals(it->label, str_lit("grid_level")))   { vp->grid.subdiv                    = cfdr_eval_get_i32    (it->value); }
      else if (str_equals(it->label, str_lit("colormap")))     { state->scene.cmap                  = cfdr_eval_get_str    (it->value); }
      else if (str_equals(it->label, str_lit("scene_bounds"))) { state->scene.scene_bounds_from_obj = cfdr_eval_get_str    (it->value); }
    }

  } else {
    log_fatal("expected table after #viewport directive");
  }
}

fn_internal void cfdr_eval_directive_colormap(CFDR_State *state, Arena *arena, TK_Scan *scan) {
  CFDR_Value key_value = cfdr_eval_expr(arena, scan);
  if (key_value.type == CFDR_Value_Type_String) {
    Str  key = cfdr_eval_get_str(key_value);

    CFDR_Value table = cfdr_eval_expr(arena, scan);
    if (table.type == CFDR_Value_Type_Table) {

      U32            node_len = 0;
      CFDR_CMap_Node node_dat[CFDR_CMap_Node_Cap] = { };
      B32            interpolate = 1;
      B32            map_opacity;
      F32            data_scale = 1;
      F32            data_shift = 0;

      for (CFDR_Table_Node *it = table.table->first; it; it = it->next) {
        if (str_equals(it->label, str_lit("interpolate"))) {
            interpolate = cfdr_eval_get_bool(it->value);
        } else if (str_equals(it->label, str_lit("map_opacity"))) {
            map_opacity = cfdr_eval_get_bool(it->value);
        } else if (str_equals(it->label, str_lit("data_scale"))) {
            data_scale = cfdr_eval_get_f32(it->value);
        } else if (str_equals(it->label, str_lit("data_shift"))) {
            data_shift = cfdr_eval_get_f32(it->value);
        } else if (str_equals(it->label, str_lit("nodes"))) {

          if (it->value.type == CFDR_Value_Type_List) {
            CFDR_List *list = it->value.list;
            for (CFDR_List_Node *it = list->first; it; it = it->next) {
              if (it->value.type == CFDR_Value_Type_List) {
                CFDR_List *pair = it->value.list;
                F32 step  = cfdr_eval_get_f32   (it->value.list->first->value);
                HSV color = cfdr_eval_get_color (it->value.list->last->value);

                if (node_len < CFDR_CMap_Node_Cap) {
                  node_dat[node_len++] = (CFDR_CMap_Node) { .step_t = step, .color = v4f(color.h, color.s, color.v, 1) };
                } else {
                  log_fatal("too many nodes given for colormap (max is %d)", CFDR_CMap_Node_Cap);
                }
              } else {
                log_fatal("Expected float-color pair for list entry in nodes, in #colormap");
              }
            }
          } else {
            log_fatal("Expected list after nodes in #colormap");
          }
        }
      }

      CFDR_CMap *cmap = cfdr_cmap_table_push(&state->cmap_table, key);
      if (cmap) {
        cfdr_cmap_init(cmap);
        cmap->node_len = node_len;
        memory_copy(cmap->node_dat, node_dat, cmap->node_len * sizeof(CFDR_CMap_Node));
        cmap->interpolate = interpolate;
        cmap->map_opacity = map_opacity;
        cmap->data_scale  = data_scale;
        cmap->data_shift  = data_shift;
        cfdr_cmap_update(cmap);
      } else {
        log_fatal("colormap \"%.*s\" already exists! Ignoring #colormap directive");
      }
    } else {
      log_fatal("expected table after #colormap [tag] directive");
    }
  } else {
      log_fatal("expected string after #colormap directive");
  }
}

fn_internal void cfdr_eval_directive_overlay(CFDR_State *state, Arena *arena, TK_Scan *scan) {
  CFDR_Value key_value = cfdr_eval_expr(arena, scan);
  if (key_value.type == CFDR_Value_Type_String) {
    Str  key = cfdr_eval_get_str(key_value);
    CFDR_Value table = cfdr_eval_expr(arena, scan);
    if (table.type == CFDR_Value_Type_Table) {
      CFDR_Overlay_Node *ov = cfdr_overlay_push(&state->overlay);
      ov->tag = key;
      for (CFDR_Table_Node *it = table.table->first; it; it = it->next) {
             if (str_equals(it->label, str_lit("visible")))       { ov->visible           = cfdr_eval_get_bool    (it->value);                       }
        else if (str_equals(it->label, str_lit("content")))       { ov->content           = cfdr_eval_get_str     (it->value);                       }
        else if (str_equals(it->label, str_lit("scale")))         { ov->scale             = cfdr_eval_get_i32     (it->value);                       }
        else if (str_equals(it->label, str_lit("color")))         { ov->color.hsv         = cfdr_eval_get_color   (it->value);                       }
        else if (str_equals(it->label, str_lit("opacity")))       { ov->color.a           = cfdr_eval_get_i32     (it->value) / 100.f;               }
        else if (str_equals(it->label, str_lit("shadow_color")))  { ov->color_shadow.hsv  = cfdr_eval_get_color   (it->value);                       }
        else if (str_equals(it->label, str_lit("position_x")))    { ov->position_x        = cfdr_eval_get_align2  (it->value);                       }
        else if (str_equals(it->label, str_lit("position_y")))    { ov->position_y        = cfdr_eval_get_align2  (it->value);                       }
        else if (str_equals(it->label, str_lit("border")))        { ov->border            = cfdr_eval_get_v2f     (it->value);                       }
        else if (str_equals(it->label, str_lit("shadow_offset"))) { ov->shadow_offset     = cfdr_eval_get_i32     (it->value);                       }

        else if (str_equals(it->label, str_lit("background")))    { if  (cfdr_eval_get_bool(it->value)) { ov->flags |= CFDR_Overlay_Flag_Background; }  }
        else if (str_equals(it->label, str_lit("shadow")))        { if  (cfdr_eval_get_bool(it->value)) { ov->flags |= CFDR_Overlay_Flag_Shadow;     }  }

        else if (str_equals(it->label, str_lit("table"))) {
          cfdr_resource_table_init(&ov->table, cfdr_eval_get_str(it->value));
          ov->flags |= CFDR_Overlay_Flag_Table;
        }
      }

    } else {
      log_fatal("expected table after #overlay [tag] directive");
    }
  } else {
      log_fatal("expected string after #overlay directive");
  }
}

fn_internal void cfdr_eval_directive_object(CFDR_State *state, Arena *arena, TK_Scan *scan) {
  CFDR_Value key_value = cfdr_eval_expr(arena, scan);
  if (key_value.type == CFDR_Value_Type_String) {
    Str  key = cfdr_eval_get_str(key_value);
    CFDR_Value table = cfdr_eval_expr(arena, scan);
    if (table.type == CFDR_Value_Type_Table) {
      CFDR_Object_Node *obj = cfdr_scene_push(&state->scene);
      obj->tag = key;
      for (CFDR_Table_Node *it = table.table->first; it; it = it->next) {
        if (str_equals(it->label, str_lit("visible")))                  { obj->visible           = cfdr_eval_get_bool    (it->value);                       }
        else if (str_equals(it->label, str_lit("color")))               { obj->color.hsv         = cfdr_eval_get_color   (it->value);                       }
        else if (str_equals(it->label, str_lit("opacity")))             { obj->color.a           = cfdr_eval_get_i32     (it->value) / 100.f;               }
        else if (str_equals(it->label, str_lit("scale")))               { obj->scale             = cfdr_eval_get_v3f     (it->value);                       }
        else if (str_equals(it->label, str_lit("translate")))           { obj->translate         = cfdr_eval_get_v3f     (it->value);                       }
        else if (str_equals(it->label, str_lit("volume_density")))      { obj->volume_density    = cfdr_eval_get_f32     (it->value) / 100.f;               }
        else if (str_equals(it->label, str_lit("volume_saturation")))   { obj->volume_saturate   = cfdr_eval_get_f32     (it->value) / 100.f;               }
        else if (str_equals(it->label, str_lit("volume_xyz")))          { obj->volume_xyz        = cfdr_eval_get_i32     (it->value);                       }
        else if (str_equals(it->label, str_lit("material"))) {
          // obj->translate         = cfdr_eval_get_v3f     (it->value);
          Str material = cfdr_eval_get_str(it->value);
          if (str_equals_any_case(material, str_lit("flat"))) {
            obj->material = CFDR_Material_Flat;
          } else if (str_equals_any_case(material, str_lit("matcap"))) {
            obj->material = CFDR_Material_Matcap;
          } else if (str_equals_any_case(material, str_lit("sample"))) {
            obj->material = CFDR_Material_Sample;
          } else {
            log_warning("unrecognized material %.*s", str_expand(material));
          }
        }

        else if (str_equals(it->label, str_lit("surface")))       {
          obj->flags |= CFDR_Object_Flag_Draw_Surface;
          Str surface_path = cfdr_eval_get_str(it->value);
          cfdr_resource_surface_init(&obj->surface, surface_path);
          log_info("Added surface resource [%.*s]", str_expand(surface_path));
        }

#if 0
        else if (str_equals(it->label, str_lit("volume")))        {
          obj->flags |= CFDR_Object_Flag_Draw_Volume;
          if (it->value.type == CFDR_Value_Type_String) {
            obj->volume.step_count       = 1;
            obj->volume.step_array       = arena_push_type(&state->scene.arena, I32);
            obj->volume.vol_array        = arena_push_type(&state->scene.arena, CFDR_Resource_Volume);
            obj->volume.step_array[0]    = 0;
            Str volume_path              = cfdr_eval_get_str(it->value);

            cfdr_resource_volume_init(&obj->volume.vol_array[0], volume_path);
            log_info("Added volume resource [%.*s]", str_expand(volume_path));

          } else if (it->value.type == CFDR_Value_Type_List) {
            CFDR_List *list        = it->value.list;
            obj->volume.step_count = list->count;
            obj->volume.step_array = arena_push_count(&state->scene.arena, I32, list->count);
            obj->volume.vol_array  = arena_push_count(&state->scene.arena, CFDR_Resource_Volume, list->count);
            U64 step_at            = 0;

            for (CFDR_List_Node *it = list->first; it; it = it->next) {
              if (it->value.type != CFDR_Value_Type_List || it->value.list->count != 2) {
                log_fatal("invalid value type in list for volume entry (expected list of form: [step, path])");
              } else {
                I32 step        = cfdr_eval_get_i32(it->value.list->first->value);
                Str volume_path = cfdr_eval_get_str(it->value.list->last->value);
                obj->volume.step_array[step_at] = step;
                cfdr_resource_volume_init(&obj->volume.vol_array[step_at], volume_path);
                ++step_at;
                log_info("Added volume resource (step = %d) [%.*s]", step, str_expand(volume_path));
              }
            }
          } else {
            log_fatal("invalid value type for volume entry");
          }
#else
      else if (str_equals(it->label, str_lit("volume"))) {
        obj->flags |= CFDR_Object_Flag_Draw_Volume;

        // obj->volume.step_count = list->count;
        // obj->volume.step_array = arena_push_count(&state->scene.arena, I32, list->count);
        // obj->volume.vol_array  = arena_push_count(&state->scene.arena, CFDR_Resource_Volume, list->count);
        // U64 step_at            = 0;
        
        U64  step_count = 0;
        F32 *step_array = 0;

        U64  var_count = 0;
        Str *var_array = 0;

        CFDR_Value table = it->value;
        if (table.type == CFDR_Value_Type_Table) {

          Str path = str_lit("");
          for (CFDR_Table_Node *table_it = table.table->first; table_it; table_it = table_it->next) {
            if (str_equals(table_it->label, str_lit("step"))) {
              CFDR_List *list = table_it->value.list;
              step_count      = list->count;
              step_array      = arena_push_count(&state->scene.arena, F32, list->count);

              U64 step_at = 0;
              for (CFDR_List_Node *it = list->first; it; it = it->next) {
                F32 step = cfdr_eval_get_f32(it->value);
                step_array[step_at++] = step;
              }

            } else if (str_equals(table_it->label, str_lit("variable"))) {
              CFDR_List *list = table_it->value.list;
              var_count      = list->count;
              var_array      = arena_push_count(&state->scene.arena, Str, list->count);

              U64 var_at = 0;
              for (CFDR_List_Node *it = list->first; it; it = it->next) {
                Str var = cfdr_eval_get_str(it->value);
                var_array[var_at++] = arena_push_str(&state->scene.arena, var);
              }

            } else if (str_equals(table_it->label, str_lit("path"))) {
              path = cfdr_eval_get_str(table_it->value);
            }
          }

          obj->volume.step_count = step_count;
          obj->volume.step_array = step_array;
          obj->volume.var_count  = var_count;
          obj->volume.var_array  = var_array;
          obj->volume.vol_array  = arena_push_count(&state->scene.arena, CFDR_Resource_Volume, var_count * step_count);

          U64 vol_at = 0;
          Scratch scratch = { };
          Scratch_Scope(&scratch, arena) {
            For_U64 (it_var, var_count) {
              For_U64 (it_step, step_count) {
                  char buffer[1024] = { };
                  stbsp_snprintf(buffer, 512, "%d", (I32)obj->volume.step_array[it_step]);

                  Str sub_1 = str_replace(scratch.arena, path,  str_lit("$(var)"),  obj->volume.var_array[it_var]);
                  Str sub_2 = str_replace(scratch.arena, sub_1, str_lit("$(step)"), str_from_cstr(buffer));

                  log_info("STEP :: %.*s", str_expand(sub_2));
                  cfdr_resource_volume_init(&obj->volume.vol_array[vol_at++], sub_2);
              }
            }
          }

          log_info("Added volume resources. %llu steps, %llu variables", step_count, var_count);

        } else {
          log_fatal("expected table after volume key in #object table");
        }
      }
#endif
      }
    } else {
      log_fatal("expected table after #object [tag] directive");
    }
  } else {
    log_fatal("expected string after #object directive");
  }
}

fn_internal void cfdr_eval_directive(CFDR_State *state, Arena *arena, TK_Scan *scan) {
  TK_Token token = { };
  if (tk_scan_require(scan, TK_Type_Identifier, &token)) {
    if (str_equals(token.text, str_lit("version"))) {
      if (state->version) {
        log_fatal("version number already set! (duplicate #version directive)");
      } else {
        if (tk_scan_require(scan, TK_Type_Literal_Integer, &token)) {
          state->version = (I32)token.value.i64;
          if (state->version == 0) {
            log_fatal("#version directive, 0 is not a valid version");
          }
        } else {
          log_fatal("expected an integer after #version directive");
        }
      }
    } else if (str_equals(token.text, str_lit("project"))) {
      if (tk_scan_require(scan, TK_Type_Literal_String, &token)) {
        if (state->project.len == 0) {
          state->project = arena_push_str(&state->arena, token.value.str);
        } else {
          log_fatal("project name already set! (duplicate #project directive)");
        }
      } else {
        log_fatal("expected a string after #project directive");
      }
    } else if (str_equals(token.text, str_lit("viewport"))) {
      cfdr_eval_directive_viewport(state, arena, scan);
    } else if (str_equals(token.text, str_lit("colormap"))) {
      cfdr_eval_directive_colormap(state, arena, scan);
    } else if (str_equals(token.text, str_lit("overlay"))) {
      cfdr_eval_directive_overlay(state, arena, scan);
    } else if (str_equals(token.text, str_lit("object"))) {
      cfdr_eval_directive_object(state, arena, scan);
    } else {
      log_fatal("invalid directive: %.*s", str_expand(token.text));
    }
  }
}

fn_internal void cfdr_eval(CFDR_State *state, Str expr) {
  Log_Zone_Scope("parsing project file") {
    Scratch scratch = { };
    Scratch_Scope(&scratch, 0) {
      TK_Tokenizer tk = { };
      tk_init(&tk, scratch.arena, expr);
      TK_Token_List token_list = tk_tokenize(&tk, expr);
      for (TK_Error *it = tk_error_list(&tk); it; it = it->next) {
        log_fatal("%.*s:%u:%u %.*s", str_expand(Project_Path), it->line, it->column, str_expand(it->message));
      }

      TK_Scan scan    = tk_scan_init(token_list);
      TK_Token token  = { };
      for (;;) {
        token = tk_scan_look(&scan);
        if (token.type == TK_Type_End_Of_Stream) {
          break;
        } else if (token.type == TK_Type_Invalid) {
          break;
        } else if (token.type == TK_Type_Op_Hash) {
          tk_scan_next(&scan);
          cfdr_eval_directive(state, scratch.arena, &scan);
        } else {
          cfdr_eval_expr(scratch.arena, &scan);
        }
      }
    }
  }
}
