// (C) Copyright 2025 Matyas Constans
// Licensed under the MIT License (https://opensource.org/license/mit/)

// ------------------------------------------------------------
// #-- 2D

// ------------------------------------------------------------
// #-- 3D

// typedef Array_V3F GEO3_Path;
typedef struct GEO3_Path {
  Array_V3F vertices;
  Array_U32 control_indices;
  Array_F32 control_times;
} GEO3_Path;

typedef struct GEO3_Surface {
  Array_V3F vertices;
  Array_U32 indices;
} GEO3_Surface;

fn_internal GEO3_Path     geo3_build_path(Arena *arena, Array_V3F control_nodes, U64 oversample, U64 subdiv_level);
fn_internal GEO3_Surface  geo3_build_tube(Arena *arena, GEO3_Path path, F32 radius, I32 resolution);
fn_internal GEO3_Surface  geo3_build_arrow(Arena *arena, V3F dir, F32 length, F32 cylinder_radius, F32 cone_length, F32 cone_radius, U32 resolution);

// NOTE(cmat): Implementation based on Inigo Quilez's article, see: https://iquilezles.org/articles/intersectors/
F32 geo3_intersect_ray_cylinder(V3F ro, V3F rd, V3F a, V3F b, F32 ra )
{
    V3F ba   = v3f_sub(b, a);
    V3F oc   = v3f_sub(ro, a);
    F32 baba = v3f_dot(ba, ba);
    F32 bard = v3f_dot(ba, rd);
    F32 baoc = v3f_dot(ba, oc);
    F32 k2   = baba - bard * bard;
    F32 k1   = baba * v3f_dot(oc,rd) - baoc * bard;
    F32 k0   = baba * v3f_dot(oc,oc) - baoc * baoc - ra * ra * baba;
    F32 h    = k1*k1 - k2*k0;

    if (h < 0.f) {
      return -1.f;
    }

    h     = f32_sqrt(h);
    F32 t = (-k1 - h) / k2;
    F32 y = baoc + t * bard;

    if(y > 0.f && y < baba) {
      return t;
    }

    t = ((( y < 0.f) ? 0.0 : baba) - baoc) / bard;
    if(f32_abs(k1 + k2 * t) < h) {
      return t;
    }

    return -1.f;
}

F32 geo3_intersect_ray_plane(V3F ro, V3F rd, V4F p)
{
  F32 distance = -(v3f_dot(ro, p.xyz) + p.w) / v3f_dot(rd, p.xyz);
  return distance;
}
