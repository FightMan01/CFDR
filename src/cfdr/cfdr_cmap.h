#define CFDR_CMap_Resolution 512

typedef I32 CFDR_CMap_Interpolate_Mode;
enum {
  CFDR_CMap_Interpolate_HSV,
  CFDR_CMap_Interpolate_RGB,
  CFDR_CMap_Interpolate_Step
};

var_global Str CFDR_CMap_Interpolate_String[] = {
  str_lit("HSV"),
  str_lit("RGB"),
  str_lit("Step"),
};

typedef I32 CFDR_CMap_Opacity_Mode;
enum {
  CFDR_CMap_Opacity_Solid,
  CFDR_CMap_Opacity_Smoothstep,
};

var_global Str CFDR_CMap_Opacity_String[] = {
  str_lit("Solid"),
  str_lit("Smoothstep"),
};

typedef I32 CFDR_CMap_Map_Mode;
enum {
  CFDR_CMap_Map_Min_Max,
  CFDR_CMap_Map_Clamp_To_Unit,
  CFDR_CMap_Map_Custom,
};

var_global Str CFDR_CMap_Map_String[] = {
  str_lit("Min-Max"),
  str_lit("Unit"),
  str_lit("Custom"),
};

typedef struct CFDR_CMap_Node {
  F32  t;
  HSVA color;
} CFDR_CMap_Node;

typedef struct CFDR_CMap {
  Str                         key;
  struct CFDR_CMap           *hash_next;

  R_Texture_2D                texture;
  G2_Material                 material;

  CFDR_CMap_Opacity_Mode      opacity;
  CFDR_CMap_Interpolate_Mode  interpolate;
  HSVA                        c0;
  HSVA                        c1;

  CFDR_CMap_Map_Mode          map_mode;
  V2F                         map_custom;
} CFDR_CMap;

fn_internal void cfdr_cmap_init(CFDR_CMap *cm, CFDR_CMap_Interpolate_Mode mode, CFDR_CMap_Opacity_Mode opacity, HSVA c0, HSVA c1) {
  cm->interpolate = mode;
  cm->c0          = c0;
  cm->c1          = c1;
  cm->map_mode    = CFDR_CMap_Map_Min_Max;
  cm->map_custom  = v2f(0, 1);
  cm->opacity    = opacity;

  cm->texture = r_texture_2D_allocate(R_Texture_Format_RGBA_U08_Normalized, CFDR_CMap_Resolution, 1);

  Scratch scratch = { };
  Scratch_Scope(&scratch, 0) {
    U08 *texture_data = arena_push_size(scratch.arena, CFDR_CMap_Resolution * sizeof(U32));

    For_U32 (it, CFDR_CMap_Resolution) {
      F32 t  = (F32)it / (CFDR_CMap_Resolution - 1);
      RGBA c = { };
      if (mode == CFDR_CMap_Interpolate_HSV) {
        c = rgba_from_hsva(v4f_lerp(t, c0, c1));
      } else if (mode == CFDR_CMap_Interpolate_RGB) {
        c = v4f_lerp(t, rgba_from_hsva(c0), rgba_from_hsva(c1));
      } else {
        c = t <= 0.5f ? c0 : c1;
      }

      if (opacity == CFDR_CMap_Opacity_Solid) {
        c.a = 1;
      } else if (opacity == CFDR_CMap_Opacity_Smoothstep) {
        c.a = f32_smoothstep(t, 1.0f, 0.0f);
      } else {
        c.a = 1;
      }

      U32 base = 4 * it;
      texture_data[base + 0] = (U08)(255 * (c.r * c.a));
      texture_data[base + 1] = (U08)(255 * (c.g * c.a));
      texture_data[base + 2] = (U08)(255 * (c.b * c.a));
      texture_data[base + 3] = (U08)(255 * c.a);
    }

    r_texture_2D_download(cm->texture, R_Texture_Format_RGBA_U08_Normalized, r2i(0, 0, CFDR_CMap_Resolution, 1), texture_data);
    cm->material = g2_material_create(cm->texture, R_Sampler_Linear_Clamp);
  }
}

fn_internal void cfdr_cmap_edit_colors(CFDR_CMap *cm, CFDR_CMap_Interpolate_Mode mode, CFDR_CMap_Opacity_Mode opacity, HSVA c0, HSVA c1) {
  Scratch scratch = { };
  Scratch_Scope(&scratch, 0) {
    U08 *texture_data = arena_push_size(scratch.arena, CFDR_CMap_Resolution * sizeof(U32));
    For_U32 (it, CFDR_CMap_Resolution) {
      F32 t  = (F32)it / (CFDR_CMap_Resolution - 1);
      RGBA c = { };
      if (mode == CFDR_CMap_Interpolate_HSV) {
        c = rgba_from_hsva(v4f_lerp(t, c0, c1));
      } else if (mode == CFDR_CMap_Interpolate_RGB) {
        c = v4f_lerp(t, rgba_from_hsva(c0), rgba_from_hsva(c1));
      } else {
        c = t <= 0.5f ? c0 : c1;
      }
      if (opacity == CFDR_CMap_Opacity_Solid) {
        c.a = 1;
      } else if (opacity == CFDR_CMap_Opacity_Smoothstep) {
        c.a = f32_smoothstep(t, 1.0f, 0.0f);
      } else {
        c.a = 1;
      }

      U32 base = 4 * it;
      texture_data[base + 0] = (U08)(255 * (c.r * c.a));
      texture_data[base + 1] = (U08)(255 * (c.g * c.a));
      texture_data[base + 2] = (U08)(255 * (c.b * c.a));
      texture_data[base + 3] = (U08)(255 * c.a);
    }

    r_texture_2D_download(cm->texture, R_Texture_Format_RGBA_U08_Normalized, r2i(0, 0, CFDR_CMap_Resolution, 1), texture_data);
  }
}

typedef struct CFDR_CMap_List {
  CFDR_CMap *first;
  CFDR_CMap *last;
} CFDR_CMap_List;

typedef struct CFDR_CMap_Table {
  Arena            arena;
  CFDR_CMap        fallback;
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

  // NOTE(cmat): Create default color-maps.
  cfdr_cmap_init(&cm->fallback, CFDR_CMap_Interpolate_HSV, CFDR_CMap_Opacity_Solid, v4f(0, 1.f, 1.f, 1.f), v4f(1.f, 1.f, 1.f, 1.f));

  {
    CFDR_CMap *cmap = cfdr_cmap_table_push(cm, str_lit("Rainbow"));
    if (cmap) {
      cfdr_cmap_init(cmap, CFDR_CMap_Interpolate_HSV, CFDR_CMap_Opacity_Solid, v4f(0, 1.f, 1.f, 1.f), v4f(1.f, .8f, 1.f, 1.f));
    }
  }

  {
    CFDR_CMap *cmap = cfdr_cmap_table_push(cm, str_lit("Pastel Rainbow"));
    if (cmap) {
      cfdr_cmap_init(cmap, CFDR_CMap_Interpolate_HSV, CFDR_CMap_Opacity_Solid, v4f(0, .6f, 1.f, 1.f), v4f(1.f, .8f, 1.f, 1.f));
    }
  }

  {
    CFDR_CMap *cmap = cfdr_cmap_table_push(cm, str_lit("X-Ray"));
    if (cmap) {
      cfdr_cmap_init(cmap, CFDR_CMap_Interpolate_HSV, CFDR_CMap_Opacity_Solid, v4f(0, .0f, 0.f, 1.f), v4f(0.f, .0f, 1.f, 1.f));
    }
  }
}
