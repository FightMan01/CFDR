#define CFDR_CMap_Resolution  512
#define CFDR_CMap_Node_Cap    32

typedef struct CFDR_CMap_Node {
  F32  step_t;
  HSVA color;
} CFDR_CMap_Node;

typedef struct CFDR_CMap {
  Str                         key;
  struct CFDR_CMap           *hash_next;

  R_Texture_2D                texture;
  G2_Material                 material;


  B32                         contour_visible;
  F32                         contour_value;
  HSV                         contour_color;
  F32                         contour_thickness;

  U32                         node_cap;
  U32                         node_len;
  CFDR_CMap_Node              node_dat[CFDR_CMap_Node_Cap];

  B32                         interpolate;
  B32                         map_opacity;
  F32                         data_scale;
  F32                         data_shift;
} CFDR_CMap;

fn_internal void cfdr_cmap_init(CFDR_CMap *cm) {
  cm->texture     = r_texture_2D_allocate(R_Texture_Format_RGBA_U08_Normalized, CFDR_CMap_Resolution, 1, 1);
  cm->material    = g2_material_create(cm->texture, R_Sampler_Linear_Clamp);
  cm->node_cap    = CFDR_CMap_Node_Cap;
  cm->node_len    = 0;
  cm->interpolate = 1;
  cm->map_opacity = 0;
  cm->data_scale  = 1;
  cm->data_shift  = 0;
  cm->contour_thickness = 1;
  sarray_zero(cm->node_dat);
}

fn_internal F32 cfdr_cmap_step_transform(F32 step, CFDR_CMap *cm) {
  return cm->data_shift + (cm->data_scale * step);
}

fn_internal V2F cfdr_cmap_range(CFDR_CMap *cm) {
  return v2f(cfdr_cmap_step_transform(cm->node_dat[0].step_t, cm),
             cfdr_cmap_step_transform(cm->node_dat[cm->node_len - 1].step_t, cm));
}

fn_internal void cfdr_cmap_update(CFDR_CMap *cm) {
  if (cm->node_len > 0) {
    F32 step_min = cfdr_cmap_step_transform(cm->node_dat[0].step_t, cm); 
    F32 step_max = cfdr_cmap_step_transform(cm->node_dat[cm->node_len - 1].step_t, cm);

    Scratch scratch = { };
    Scratch_Scope(&scratch, 0) {
      U08 *texture_data = arena_push_size(scratch.arena, CFDR_CMap_Resolution * sizeof(U32));

      F32 fill_t = 0;
      For_U32(it, cm->node_len - 1) {
        F32 step_t1 = f32_div_safe(cfdr_cmap_step_transform(cm->node_dat[it + 0].step_t, cm) - step_min, step_max - step_min);
        F32 step_t2 = f32_div_safe(cfdr_cmap_step_transform(cm->node_dat[it + 1].step_t, cm) - step_min, step_max - step_min);
        V4F c1      = cm->node_dat[it + 0].color;
        V4F c2      = cm->node_dat[it + 1].color;

        // NOTE(cmat): Fill color-map until next step.
        while (fill_t <= step_t2) {
          U32 fill_it = u32_clamp(fill_t * CFDR_CMap_Resolution, 0, CFDR_CMap_Resolution - 1);

          F32 t = (fill_t - step_t1) / (step_t2 - step_t1);
          V4F c = { };
          if (cm->interpolate) {
            c = v4f_lerp(t, c1, c2);
          } else {
            c = c1;
          }

          c = rgba_from_hsva(c);

          if (cm->map_opacity) {
            c.a = f32_smoothstep(fill_t, 1, 0);
          }

          U32 base = 4 * fill_it;
          texture_data[base + 0] = (U08)(255 * (c.r * c.a));
          texture_data[base + 1] = (U08)(255 * (c.g * c.a));
          texture_data[base + 2] = (U08)(255 * (c.b * c.a));
          texture_data[base + 3] = (U08)(255 *        c.a);

          fill_t += 1.f / CFDR_CMap_Resolution;
        }
      }

      r_texture_2D_download(cm->texture, R_Texture_Format_RGBA_U08_Normalized, r2i(0, 0, CFDR_CMap_Resolution, 1), texture_data);
    }
  }
}

typedef struct CFDR_CMap_List {
  CFDR_CMap *first;
  CFDR_CMap *last;
} CFDR_CMap_List;

typedef struct CFDR_CMap_Table {
  Arena            arena;
  U64              bucket_count;
  CFDR_CMap_List  *bucket_array;

  // TODO(cmat): Linked list.
  U64              key_count;
  Str              key_list[512];
} CFDR_CMap_Table;

fn_internal CFDR_CMap *cfdr_cmap_table_push(CFDR_CMap_Table *cm, Str key) {
  U64              bucket_index = crc32(key.len, key.txt) % cm->bucket_count;
  CFDR_CMap_List  *list         = &cm->bucket_array[bucket_index];
  CFDR_CMap       *result       = 0;

  if (!list->first) {
    result          = arena_push_type(&cm->arena, CFDR_CMap);
    result->key  = key;
    queue_push_ext(list->first, list->last, result, hash_next);
  } else {
    CFDR_CMap *entry = list->first;
    while (entry) {
      if (str_equals(entry->key, key)) {
        break;
      }

      if (!entry->hash_next) {
        result          = arena_push_type(&cm->arena, CFDR_CMap);
        result->key  = key;
        queue_push_ext(list->first, list->last, result, hash_next);
        break;
      }

      entry = entry->hash_next;
    }
  }

  if (result) {
    if (cm->key_count <= sarray_len(cm->key_list)) {
      cm->key_list[cm->key_count++] = arena_push_str(&cm->arena, key);
    }
  }

  return result;
}

fn_internal CFDR_CMap *cfdr_cmap_table_get(CFDR_CMap_Table *cm, Str key) {
  U64              bucket_index = crc32(key.len, key.txt) % cm->bucket_count;
  CFDR_CMap_List  *list         = &cm->bucket_array[bucket_index];
  CFDR_CMap       *result       = 0;

  if (list->first) {
    CFDR_CMap *entry = list->first;
    while (entry) {
      if (str_equals(entry->key, key)) {
        result = entry;
        break;
      }

      entry = entry->hash_next;
    }
  } 

  return result;
}

fn_internal void cfdr_cmap_table_init(CFDR_CMap_Table *cm) {
  zero_fill(cm);
  arena_init(&cm->arena);
  cm->bucket_count = 256;
  cm->bucket_array = arena_push_count(&cm->arena, CFDR_CMap_List, cm->bucket_count);

  {
    CFDR_CMap *cmap = cfdr_cmap_table_push(cm, str_lit("Rainbow"));
    if (cmap) {
      cfdr_cmap_init(cmap);
      cmap->node_len = 2;
      cmap->node_dat[0] = (CFDR_CMap_Node) { .step_t = 0,   .color = v4f(0.7, 1, 1, 1) };
      cmap->node_dat[1] = (CFDR_CMap_Node) { .step_t = 1.0, .color = v4f(0.0, 1, 1, 1) };
      cfdr_cmap_update(cmap);
    }
  }


#if 0

  // NOTE(cmat): Create default color-maps.
  cfdr_cmap_init(&cm->fallback, CFDR_CMap_Interpolate_HSV, CFDR_CMap_Opacity_Solid, v4f(0.7f, 1.f, 1.f, 1.f), v4f(0.0f, 1.f, 1.f, 1.f));

  {
    CFDR_CMap *cmap = cfdr_cmap_table_push(cm, str_lit("Rainbow"));
    if (cmap) {
      cfdr_cmap_init(cmap, CFDR_CMap_Interpolate_HSV, CFDR_CMap_Opacity_Solid, v4f(0.7f, 1.f, 1.f, 1.f), v4f(0.0f, 1.f, 1.f, 1.f));
    }
  }

  {
    CFDR_CMap *cmap = cfdr_cmap_table_push(cm, str_lit("Rainbow-Pastel"));
    if (cmap) {
      cfdr_cmap_init(cmap, CFDR_CMap_Interpolate_HSV, CFDR_CMap_Opacity_Solid, v4f(0.7f, .6f, 1.f, 1.f), v4f(0.0f, 0.8f, 1.f, 1.f));
    }
  }

  {
    CFDR_CMap *cmap = cfdr_cmap_table_push(cm, str_lit("X-Ray"));
    if (cmap) {
      cfdr_cmap_init(cmap, CFDR_CMap_Interpolate_HSV, CFDR_CMap_Opacity_Solid, v4f(0, .0f, 0.f, 1.f), v4f(0.f, .0f, 1.f, 1.f));
    }
  }
#endif

}
