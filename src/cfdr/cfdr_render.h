typedef U32 CFDR_Render_Pipeline;
enum {
  CFDR_Render_Pipeline_Grid,
  CFDR_Render_Pipeline_Flat,
  CFDR_Render_Pipeline_Flat_Overlay,
  CFDR_Render_Pipeline_Matcap,
  CFDR_Render_Pipeline_Volume,
  CFDR_Render_Pipeline_Sample,

  CFDR_Render_Pipeline_Count
};


// TODO(cmat): Proper materials. We want to store bindgroups here essentially.
typedef I32 CFDR_Material;
enum {
  CFDR_Material_Flat,
  CFDR_Material_Matcap,
  CFDR_Material_Sample,
};

var_global Str CFDR_Material_String_List[] = {
  str_lit("Flat"),
  str_lit("Matcap"),
  str_lit("Sample"),
};

typedef struct CFDR_Render {
  R_Pipeline      pipelines[CFDR_Render_Pipeline_Count];
  R_Buffer        quad_X_buffer;
  R_Buffer        quad_U_buffer;
  R_Buffer        quad_index_buffer;
  R_Bind_Group    white_bind_group;

  R_Buffer        cube_index_count;
  R_Buffer        cube_X_buffer;
  R_Buffer        cube_index_buffer;
} CFDR_Render;

fn_internal void cfdr_render_init(CFDR_Render *render) {
  render->pipelines[CFDR_Render_Pipeline_Grid] = r_pipeline_create(&(R_Pipeline_Layout) {
    .shader      = R_Shader_Grid_3D,
    .depth_test  = 1,
    .depth_write = 0,
    .depth_bias  = 1,
    .cull_face   = 0,
  });

  render->pipelines[CFDR_Render_Pipeline_Flat] = r_pipeline_create(&(R_Pipeline_Layout) {
    .shader      = R_Shader_Flat_3D,
    .depth_test  = 1,
    .depth_write = 1,
    .depth_bias  = 0,
    .cull_face   = 1,
  });

  render->pipelines[CFDR_Render_Pipeline_Flat_Overlay] = r_pipeline_create(&(R_Pipeline_Layout) {
    .shader      = R_Shader_Flat_3D,
    .depth_test  = 0,
    .depth_write = 0,
    .depth_bias  = 0,
    .cull_face   = 0,
  });

  render->pipelines[CFDR_Render_Pipeline_Matcap] = r_pipeline_create(&(R_Pipeline_Layout) {
    .shader      = R_Shader_Edit_3D,
    .depth_test  = 1,
    .depth_write = 1,
    .depth_bias  = 0,
    .cull_face   = 1,
  });
 
  render->pipelines[CFDR_Render_Pipeline_Volume] = r_pipeline_create(&(R_Pipeline_Layout) {
    .shader      = R_Shader_DVR_3D,
    .depth_test  = 1,
    .depth_write = 1,
    .depth_bias  = 0,
    .cull_face   = 1,
  });

  render->pipelines[CFDR_Render_Pipeline_Sample] = r_pipeline_create(&(R_Pipeline_Layout) {
    .shader      = R_Shader_SLI_3D,
    .depth_test  = 1,
    .depth_write = 1,
    .depth_bias  = 0,
    .cull_face   = 0,
  });

  V4F quad_X[] = {
    v4f(-1, 0, -1, 1.f),
    v4f(+1, 0, -1, 1.f),
    v4f(+1, 0, +1, 1.f),
    v4f(-1, 0, +1, 1.f),
  };

  V2F quad_U[] = {
    v2f(0, 0),
    v2f(1, 0),
    v2f(1, 1),
    v2f(0, 1),
  };

  render->quad_X_buffer = r_buffer_allocate(sizeof(quad_X), R_Buffer_Mode_Static);
  r_buffer_download(render->quad_X_buffer, 0, sizeof(quad_X), quad_X);

  render->quad_U_buffer = r_buffer_allocate(sizeof(quad_U), R_Buffer_Mode_Static);
  r_buffer_download(render->quad_U_buffer, 0, sizeof(quad_U), quad_U);

  U32 quad_indices[] = { 0, 2, 1, 0, 3, 2 };
  render->quad_index_buffer = r_buffer_allocate(sizeof(quad_indices), R_Buffer_Mode_Static);
  r_buffer_download(render->quad_index_buffer, 0, sizeof(quad_indices), quad_indices);

  V4F cube_vertices[] = {
    // Bottom
    { -1, -1, -1 },
    { +1, -1, -1 },
    { +1, -1, +1 },
    { -1, -1, +1 },

    // Top
    { -1, +1, -1 },
    { +1, +1, -1 },
    { +1, +1, +1 },
    { -1, +1, +1 },

    // Left
    { -1, -1, -1 },
    { -1, -1, +1 },
    { -1, +1, +1 },
    { -1, +1, -1 },

    // Right
    { +1, -1, -1 },
    { +1, -1, +1 },
    { +1, +1, +1 },
    { +1, +1, -1 },

    // Front
    { -1, -1, -1 },
    { +1, -1, -1 },
    { +1, +1, -1 },
    { -1, +1, -1 },

    // Back
    { -1, -1, +1 },
    { +1, -1, +1 },
    { +1, +1, +1 },
    { -1, +1, +1 },
  };

  For_U64 (it, sarray_len(cube_vertices)) {
    V4F *X = &cube_vertices[it];
    X->x = (X->x + 1.f) / 2.f;
    X->y = (X->y + 1.f) / 2.f;
    X->z = (X->z + 1.f) / 2.f;
    X->w = 1.f;
  }
  
  U32 cube_indices[] = {
    0,   1,  2, 0,   2,  3,
    4,   6,  5, 4,   7,  6,
    8,   9,  10,  8, 10, 11,
    12, 14, 13, 12, 15, 14,
    16, 18, 17, 16, 19, 18,
    20, 21, 22, 20, 22, 23
  };

  render->cube_index_count = sarray_len(cube_indices);

  render->cube_X_buffer = r_buffer_allocate(sizeof(cube_vertices), R_Buffer_Mode_Static);
  r_buffer_download(render->cube_X_buffer, 0, sizeof(cube_vertices), cube_vertices);

  render->cube_index_buffer = r_buffer_allocate(sizeof(cube_indices), R_Buffer_Mode_Static);
  r_buffer_download(render->cube_index_buffer, 0, sizeof(cube_indices), cube_indices);
}

typedef struct CFDR_Render_Grid {
  B32            visible;
  R_Buffer       state_buffer;
  R_Bind_Group   bind_group;
  F32            scale;
  F32            subdiv;
  HSVA           color;
} CFDR_Render_Grid;

fn_internal void cfdr_render_grid_init(CFDR_Render *render, CFDR_Render_Grid *grid) {
grid->state_buffer = r_buffer_allocate(sizeof(R_Constant_Buffer_Grid_3D), R_Buffer_Mode_Dynamic);
  grid->bind_group   = r_bind_group_create(&Grid_3D_Layout, &(R_Bind_Group_Entry_List) {
    .count      = 3,
    .entry_list = {
      { .binding = 0, .type = R_Binding_Type_Storage, .resource = render->quad_X_buffer },
      { .binding = 1, .type = R_Binding_Type_Storage, .resource = render->quad_U_buffer },
      { .binding = 2, .type = R_Binding_Type_Uniform, .resource = grid->state_buffer    },
    }
  });

  grid->scale   = 1000.f;
  grid->subdiv  = 1;
  grid->color   = v4f(0, 0, 1, 1);
  grid->visible = 1;
}

fn_internal void cfdr_render_grid_draw(CFDR_Render *render, CFDR_Render_Grid *grid, M4F view_projection, R2F viewport) {
  if (grid->visible) {
    M4F world_view_projection = m4f_mul(m4f_diag(v4f(grid->scale, 1, grid->scale, 1)), view_projection);

    R_Constant_Buffer_Grid_3D grid_data = {
      .World_View_Projection = world_view_projection,
      .Grid_Scale            = grid->subdiv * grid->scale,
      .Color                 = rgba_from_hsva(grid->color),
    };

    r_buffer_download(grid->state_buffer, 0, sizeof(grid_data), &grid_data);

    r_command_push_draw(&(R_Command_Draw) {
      .pipeline           = render->pipelines[CFDR_Render_Pipeline_Grid],
      .bind_group         = grid->bind_group,
      .index_buffer       = render->quad_index_buffer,
      .draw_index_count   = 6,
      .draw_index_offset  = 0,
      .draw_instance_count = 1,

      .depth_test         = 1,
      .draw_region        = r2i_from_r2f(viewport),
      .clip_region        = r2i_from_r2f(viewport),
    });

  }
}

fn_internal M4F cfdr_render_apply_basis_change(M4F transform) {
  M4F basis_change = {
    .e11 = +1,
    .e32 = -1,
    .e23 = +1,
    .e44 = +1,
  };

  return m4f_mul(transform, basis_change);
}

typedef struct CFDR_Render_Surface {
  CFDR_Resource_Surface *resource;
  CFDR_Resource_Volume  *sample_volume;
  M4F                    sample_volume_transform;
  V2F                    sample_vis_range;
  I32                    sample_volume_xyz;
  V3F                    contour_color;
  B32                    contour_visible;
  F32                    contour_value;
  F32                    contour_thickness;

  R_Buffer               state_buffer;
  R_Buffer               vol_buffer;
  R_Bind_Group           bind_group;
  R_Bind_Group           bind_group_vol;
  CFDR_Material          material;
  HSVA                   color;
  M4F                    transform;
} CFDR_Render_Surface;

fn_internal void cfdr_render_surface_draw(CFDR_Render *render, CFDR_Render_Surface *surface, V3F eye_position, M4F view_projection, R2F viewport) {
  M4F world                 = cfdr_render_apply_basis_change(surface->transform);
  M4F world_view_projection = m4f_mul(world, view_projection);

  M4F world_inv = { };
  m4f_inv(world, &world_inv);

  V4F color = rgba_from_hsva(surface->color);
  color.rgb = v3f_mul_f32(color.rgb, color.a);

  if (surface->material == CFDR_Material_Sample) {

    if (surface->sample_volume) {
      M4F sample_world     = cfdr_render_apply_basis_change(surface->sample_volume_transform);

      V3F volume_min = m4f_mul_v4f(v4f(0, 0, 0, 1), sample_world).xyz;
      V3F volume_max = m4f_mul_v4f(v4f(1, 1, 1, 1), sample_world).xyz;

      R_Constant_Buffer_Vol_3D vol_data = {
        .World_View_Projection   = world_view_projection,
        .World_Inverse_Transpose = m4f_trans(world_inv),
        .World                   = world,
        .Eye_Position            = eye_position,
        .Volume_Density          = 1,
        .Volume_Min              = volume_min,
        .Volume_Max              = volume_max,
        .Volume_Data_Bounds      = surface->sample_volume->data_range,
        .Visualize_Range         = surface->sample_vis_range,
        .Volume_Saturate         = 1,
        .Volume_XYZ              = surface->sample_volume_xyz,
        .Contour_Color           = surface->contour_color,
        .Contour_Visible         = surface->contour_visible,
        .Contour_Value           = surface->contour_value,
        .Contour_Thickness       = surface->contour_thickness,
      };

      r_buffer_download(surface->vol_buffer, 0, sizeof(vol_data), &vol_data);
      r_command_push_draw(&(R_Command_Draw) {
        .pipeline           = render->pipelines[CFDR_Render_Pipeline_Sample],
        .bind_group         = surface->bind_group_vol,
        .index_buffer       = surface->resource->index_buffer,
        .draw_index_count   = surface->resource->index_count,
        .draw_index_offset  = 0,
        .draw_instance_count = 1,

        .depth_test         = 1,
        .draw_region        = r2i_from_r2f(viewport),
        .clip_region        = r2i_from_r2f(viewport),
      });
    }
  } else {
    R_Constant_Buffer_World_3D world_data = {
      .World_View_Projection   = world_view_projection,
      .World_Inverse_Transpose = m4f_trans(world_inv),
      .World                   = world,
      .Eye_Position            = eye_position,
      .Color                   = color,
    };

    r_buffer_download(surface->state_buffer, 0, sizeof(world_data), &world_data);

    R_Pipeline pipeline = R_Resource_None;
    switch (surface->material) {
      case CFDR_Material_Flat:    { pipeline = render->pipelines[CFDR_Render_Pipeline_Flat];   } break;
      case CFDR_Material_Matcap:  { pipeline = render->pipelines[CFDR_Render_Pipeline_Matcap]; } break;
    }

    r_command_push_draw(&(R_Command_Draw) {
      .pipeline           = pipeline,
      .bind_group         = surface->bind_group,
      .index_buffer       = surface->resource->index_buffer,
      .draw_index_count   = surface->resource->index_count,
      .draw_index_offset  = 0,
      .draw_instance_count = 1,

      .depth_test         = 1,
      .draw_region        = r2i_from_r2f(viewport),
      .clip_region        = r2i_from_r2f(viewport),
    });
  }
}

typedef struct CFDR_Render_Volume {
  CFDR_Resource_Volume  *resource;
  M4F                    transform;
  R_Bind_Group           bind_group_vol;
  F32                    volume_density;
  F32                    volume_saturate;
  I32                    volume_xyz;
  V2F                    vis_range;
} CFDR_Render_Volume;

fn_internal void cfdr_render_volume_draw(CFDR_Render *render, CFDR_Render_Volume *volume, V3F eye_position, M4F view_projection, R2F viewport) {
  M4F world                 = cfdr_render_apply_basis_change(volume->transform);
  M4F world_view_projection = m4f_mul(world, view_projection);

  M4F world_inv = { };
  m4f_inv(world, &world_inv);

  V3F volume_min = m4f_mul_v4f(v4f(0, 0, 0, 1), world).xyz;
  V3F volume_max = m4f_mul_v4f(v4f(1, 1, 1, 1), world).xyz;

  R_Constant_Buffer_Vol_3D vol_data = {
    // TODO
    .World_View_Projection   = world_view_projection,
    .World                   = world,
    .Eye_Position            = eye_position,
    .Volume_Min              = volume_min,
    .Volume_Max              = volume_max,
    .Volume_Density          = volume->volume_density,
    .Volume_Data_Bounds      = volume->resource->data_range,
    .Visualize_Range         = volume->vis_range,
    .Volume_Saturate         = volume->volume_saturate,
    .Volume_XYZ              = volume->volume_xyz,
  };

  r_buffer_download(volume->resource->constant_buffer, 0, sizeof(vol_data), &vol_data);

  r_command_push_draw(&(R_Command_Draw) {
    .pipeline           = render->pipelines[CFDR_Render_Pipeline_Volume],
    .bind_group         = volume->bind_group_vol,
    .index_buffer       = render->cube_index_buffer,
    .draw_index_count   = render->cube_index_count,
    .draw_index_offset  = 0,
    .draw_instance_count = 1,

    .depth_test         = 1,
    .draw_region        = r2i_from_r2f(viewport),
    .clip_region        = r2i_from_r2f(viewport),
  });
}

typedef struct CFDR_Render_Arrow {
  R_Bind_Group    arrow_bind_group;
  R_Buffer        arrow_state_buffer;

  U32             arrow_index_count;
  R_Buffer        arrow_X_buffer;
  R_Buffer        arrow_U_buffer;
  R_Buffer        arrow_N_buffer;
  R_Buffer        arrow_index_buffer;
} CFDR_Render_Arrow;

fn_internal CFDR_Render_Arrow cfdr_render_arrow_init(CFDR_Render *render, F32 cylinder_r, F32 cone_len, F32 cone_r) {
  CFDR_Render_Arrow arrow  = { };

  Scratch scratch = { };
  Scratch_Scope(&scratch, 0) {
    GEO3_Surface arrow_geo = geo3_build_arrow(scratch.arena, v3f(1, 0, 0),
        5.0f,  // cylinder length
        cylinder_r, // 0.1f,  // cylidner radius
        cone_len, // 2.f,   // cone length
        cone_r, // 0.75f, // cone radius
        32);

    V4F *X_data = arena_push_count(scratch.arena, V4F, arrow_geo.vertices.len);
    V4F *N_data = arena_push_count(scratch.arena, V4F, arrow_geo.vertices.len);
    V2F *U_data = arena_push_count(scratch.arena, V2F, arrow_geo.vertices.len);
    For_U64(it, arrow_geo.vertices.len) {
      V3F x = arrow_geo.vertices.dat[it];
      X_data[it] = v4f(x.x, x.y, x.z, 1);
      N_data[it] = v4f(1, 0, 0, 0);
      U_data[it] = v2f(0, 0);
    }

    arrow.arrow_X_buffer = r_buffer_allocate(arrow_geo.vertices.len * sizeof(V4F), R_Buffer_Mode_Static);
    r_buffer_download(arrow.arrow_X_buffer, 0, arrow_geo.vertices.len * sizeof(V4F), X_data);

    arrow.arrow_N_buffer = r_buffer_allocate(arrow_geo.vertices.len * sizeof(V4F), R_Buffer_Mode_Static);
    r_buffer_download(arrow.arrow_N_buffer, 0, arrow_geo.vertices.len * sizeof(V4F), N_data);

    arrow.arrow_U_buffer = r_buffer_allocate(arrow_geo.vertices.len * sizeof(V2F), R_Buffer_Mode_Static);
    r_buffer_download(arrow.arrow_U_buffer, 0, arrow_geo.vertices.len * sizeof(V2F), U_data);

    arrow.arrow_index_count  = arrow_geo.indices.len;
    arrow.arrow_index_buffer = r_buffer_allocate(arrow_geo.indices.len * sizeof(U32), R_Buffer_Mode_Static);
    r_buffer_download(arrow.arrow_index_buffer, 0, arrow_geo.indices.len * sizeof(U32), arrow_geo.indices.dat);
  }

  arrow.arrow_state_buffer = r_buffer_allocate(sizeof(R_Constant_Buffer_World_3D), R_Buffer_Mode_Static);
  arrow.arrow_bind_group   = r_bind_group_create(&Flat_3D_Layout, &(R_Bind_Group_Entry_List) {
    .count = 6,
    .entry_list = {
      { .binding = 0, .type = R_Binding_Type_Storage,    .resource = arrow.arrow_X_buffer      },
      { .binding = 1, .type = R_Binding_Type_Storage,    .resource = arrow.arrow_U_buffer      },
      { .binding = 2, .type = R_Binding_Type_Storage,    .resource = arrow.arrow_N_buffer      },
      { .binding = 3, .type = R_Binding_Type_Texture_2D, .resource = R_Texture_2D_White          },
      { .binding = 4, .type = R_Binding_Type_Sampler,    .resource = R_Sampler_Nearest_Clamp     },
      { .binding = 5, .type = R_Binding_Type_Uniform,    .resource = arrow.arrow_state_buffer    },
    }
  });

  return arrow;
}

fn_internal M4F cfdr_arrow_transform(V3F direction, F32 scale) {
  V3F up = v3f(0, 1, 0);
  V3F e1 = direction;
  V3F e2 = v3f_noz(v3f_cross(up, e1));
  if (v3f_len(e2) < NOZ_Epsilon) { e2 = v3f(1.f, 0.f, 0.f); }
  V3F e3 = v3f_noz(v3f_cross(e1, e2));

  M4F rotation = {
    .row_1 = v4f_mul(scale, v4f(e1.x, e1.y, e1.z, 0.f)),
    .row_2 = v4f_mul(scale, v4f(e2.x, e2.y, e2.z, 0.f)),
    .row_3 = v4f_mul(scale, v4f(e3.x, e3.y, e3.z, 0.f)),
    .row_4 = v4f(0.f,  0.f,  0.f,  1.f),
  };

  M4F result = m4f_trans(rotation);
  return result;
}

fn_internal void cfdr_render_arrow_draw(CFDR_Render *render, CFDR_Render_Arrow *arrow, M4F world, M4F view_projection, V3F eye_position, HSVA color, R2F viewport) {
  world                     = cfdr_render_apply_basis_change(world);
  M4F world_view_projection = m4f_mul(world, view_projection);

  M4F world_inv = { };
  m4f_inv(world, &world_inv);

  RGBA color_rgba = rgba_from_hsva(color);
  color_rgba.rgb = v3f_mul_f32(color_rgba.rgb, color_rgba.a);

  R_Constant_Buffer_World_3D world_data = {
    .World_View_Projection   = world_view_projection,
    .World_Inverse_Transpose = m4f_trans(world_inv),
    .World                   = world,
    .Eye_Position            = eye_position,
    .Color                   = color_rgba,
  };

  r_buffer_download(arrow->arrow_state_buffer, 0, sizeof(world_data), &world_data);

  r_command_push_draw(&(R_Command_Draw) {
    .pipeline           = render->pipelines[CFDR_Render_Pipeline_Flat_Overlay],
    .bind_group         = arrow->arrow_bind_group,
    .index_buffer       = arrow->arrow_index_buffer,
    .draw_index_count   = arrow->arrow_index_count,
    .draw_index_offset  = 0,
    .draw_instance_count = 1,

    .depth_test         = 1,
    .draw_region        = r2i_from_r2f(viewport),
    .clip_region        = r2i_from_r2f(viewport),
  });
}
