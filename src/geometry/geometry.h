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

