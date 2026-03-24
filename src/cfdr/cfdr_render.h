typedef U32 CFDR_Render_Pipeline;
enum {
  CFDR_Render_Pipeline_Grid,
  CFDR_Render_Pipeline_Surface,
  CFDR_Render_Pipeline_Volume,

  CFDR_Render_Pipeline_Count
};

typedef struct CFDR_Render {
  R_Pipeline      pipelines[CFDR_Render_Pipeline_Count];
  R_Buffer        quad_vertex_buffer;
  R_Buffer        quad_index_buffer;
  R_Bind_Group    white_bind_group;

  R_Buffer        cube_index_count;
  R_Buffer        cube_vertex_buffer;
  R_Buffer        cube_index_buffer;
} CFDR_Render;

fn_internal void cfdr_render_init(CFDR_Render *render) {
  render->pipelines[CFDR_Render_Pipeline_Grid] = r_pipeline_create(&(R_Pipeline_Layout) {
    .shader      = R_Shader_Grid_3D,
    .format      = &R_Vertex_Format_XNUC_3D,
    .depth_test  = 1,
    .depth_write = 0,
    .depth_bias  = 1,
  });

  render->pipelines[CFDR_Render_Pipeline_Surface] = r_pipeline_create(&(R_Pipeline_Layout) {
    .shader      = R_Shader_Edit_3D,
    .format      = &R_Vertex_Format_XNUC_3D,
    .depth_test  = 1,
    .depth_write = 1,
    .depth_bias  = 0,
  });
 
  render->pipelines[CFDR_Render_Pipeline_Volume] = r_pipeline_create(&(R_Pipeline_Layout) {
    .shader      = R_Shader_DVR_3D,
    .format      = &R_Vertex_Format_XNUC_3D,
    .depth_test  = 1,
    .depth_write = 1,
    .depth_bias  = 0,
  });

  R_Vertex_XNUC_3D quad_vertices[] = {
    { .X = v3f(-1, 0, -1), .N = v3f(0, 1, 0), .U = v2f(0, 0), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },
    { .X = v3f(+1, 0, -1), .N = v3f(0, 1, 0), .U = v2f(1, 0), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },
    { .X = v3f(+1, 0, +1), .N = v3f(0, 1, 0), .U = v2f(1, 1), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },
    { .X = v3f(-1, 0, +1), .N = v3f(0, 1, 0), .U = v2f(0, 1), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },
  };

  render->quad_vertex_buffer = r_buffer_allocate(sizeof(quad_vertices), R_Buffer_Mode_Static);
  r_buffer_download(render->quad_vertex_buffer, 0, sizeof(quad_vertices), quad_vertices);

  U32 quad_indices[] = { 0, 2, 1, 0, 3, 2 };
  render->quad_index_buffer = r_buffer_allocate(sizeof(quad_indices), R_Buffer_Mode_Static);
  r_buffer_download(render->quad_index_buffer, 0, sizeof(quad_indices), quad_indices);

  R_Vertex_XNUC_3D cube_vertices[] = {
    // Bottom
    { .X = v3f(-1, -1, -1), .N = v3f(0, -1, 0), .U = v2f(0, 0), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },
    { .X = v3f(+1, -1, -1), .N = v3f(0, -1, 0), .U = v2f(1, 0), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },
    { .X = v3f(+1, -1, +1), .N = v3f(0, -1, 0), .U = v2f(1, 1), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },
    { .X = v3f(-1, -1, +1), .N = v3f(0, -1, 0), .U = v2f(0, 1), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },

    // Top
    { .X = v3f(-1, +1, -1), .N = v3f(0, +1, 0), .U = v2f(0, 0), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },
    { .X = v3f(+1, +1, -1), .N = v3f(0, +1, 0), .U = v2f(1, 0), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },
    { .X = v3f(+1, +1, +1), .N = v3f(0, +1, 0), .U = v2f(1, 1), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },
    { .X = v3f(-1, +1, +1), .N = v3f(0, +1, 0), .U = v2f(0, 1), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },

    // Left
    { .X = v3f(-1, -1, -1), .N = v3f(-1, 0, 0), .U = v2f(0, 0), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },
    { .X = v3f(-1, -1, +1), .N = v3f(-1, 0, 0), .U = v2f(1, 0), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },
    { .X = v3f(-1, +1, +1), .N = v3f(-1, 0, 0), .U = v2f(1, 1), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },
    { .X = v3f(-1, +1, -1), .N = v3f(-1, 0, 0), .U = v2f(0, 1), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },

    // Right
    { .X = v3f(+1, -1, -1), .N = v3f(+1, 0, 0), .U = v2f(0, 0), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },
    { .X = v3f(+1, -1, +1), .N = v3f(+1, 0, 0), .U = v2f(1, 0), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },
    { .X = v3f(+1, +1, +1), .N = v3f(+1, 0, 0), .U = v2f(1, 1), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },
    { .X = v3f(+1, +1, -1), .N = v3f(+1, 0, 0), .U = v2f(0, 1), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },

    // Front
    { .X = v3f(-1, -1, -1), .N = v3f(0, 0, -1), .U = v2f(0, 0), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },
    { .X = v3f(+1, -1, -1), .N = v3f(0, 0, -1), .U = v2f(1, 0), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },
    { .X = v3f(+1, +1, -1), .N = v3f(0, 0, -1), .U = v2f(1, 1), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },
    { .X = v3f(-1, +1, -1), .N = v3f(0, 0, -1), .U = v2f(0, 1), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },

    // Back
    { .X = v3f(-1, -1, +1), .N = v3f(0, 0, +1), .U = v2f(0, 0), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },
    { .X = v3f(+1, -1, +1), .N = v3f(0, 0, +1), .U = v2f(1, 0), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },
    { .X = v3f(+1, +1, +1), .N = v3f(0, 0, +1), .U = v2f(1, 1), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },
    { .X = v3f(-1, +1, +1), .N = v3f(0, 0, +1), .U = v2f(0, 1), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },
  };
  
  U32 cube_indices[] = {
    0,   2,  1, 0,   3,  2,
    4,   5,  6, 4,   6,  7,
    8,  10,  9,  8, 11, 10,
    12, 13, 14, 12, 14, 15,
    16, 17, 18, 16, 18, 19,
    20, 22, 21, 20, 23, 22
  };

  render->cube_index_count = sarray_len(cube_indices);

  render->cube_vertex_buffer = r_buffer_allocate(sizeof(cube_vertices), R_Buffer_Mode_Static);
  r_buffer_download(render->cube_vertex_buffer, 0, sizeof(cube_vertices), cube_vertices);

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

fn_internal void cfdr_render_grid_init(CFDR_Render_Grid *grid) {
  grid->state_buffer = r_buffer_allocate(sizeof(R_Constant_Buffer_World_3D), R_Buffer_Mode_Dynamic);
  grid->bind_group   = r_bind_group_create(&Flat_2D_Layout, &(R_Bind_Group_Entry_List) {
    .count      = 4,
    .entry_list = {
      { .binding = 0, .type = R_Binding_Type_Texture_2D, .resource = R_Texture_2D_White            },
      { .binding = 1, .type = R_Binding_Type_Sampler,    .resource = R_Sampler_Nearest_Clamp       },
      { .binding = 2, .type = R_Binding_Type_Uniform,    .resource = grid->state_buffer            },
      { .binding = 3, .type = R_Binding_Type_Texture_3D, .resource = R_Texture_3D_White            },
    }
  });

  grid->scale   = 1000.f;
  grid->subdiv  = 1;
  grid->color   = v4f(0, 0, 1, 1);
}

fn_internal void cfdr_render_grid_draw(CFDR_Render *render, CFDR_Render_Grid *grid, M4F view_projection, R2F viewport) {
  if (grid->visible) {
    M4F world_view_projection = m4f_mul(m4f_diag(v4f(grid->scale, 1, grid->scale, 1)), view_projection);

    R_Constant_Buffer_World_3D world_data = {
      .World_View_Projection = world_view_projection,
      .Eye_Position          = v3f(0, 0, 0),
      .Volume_Density        = 0.0f,
      .Grid_Scale            = grid->subdiv * grid->scale,
      .Color                 = rgba_from_hsva(grid->color)
    };

    r_buffer_download(grid->state_buffer, 0, sizeof(world_data), &world_data);

    r_command_push_draw(&(R_Command_Draw) {
      .pipeline           = render->pipelines[CFDR_Render_Pipeline_Grid],
      .bind_group         = grid->bind_group,
      .vertex_buffer      = render->quad_vertex_buffer,
      .index_buffer       = render->quad_index_buffer,
      .draw_index_count   = 6,
      .draw_index_offset  = 0,

      .depth_test         = 1,
      .draw_region        = r2i_from_r2f(viewport),
      .clip_region        = r2i_from_r2f(viewport),
    });

  }
}

typedef struct CFDR_Render_Surface {
  CFDR_Resource_Surface *resource;
  R_Buffer               state_buffer;
  R_Bind_Group           bind_group;
  HSVA                   color;
  M4F                    transform;
} CFDR_Render_Surface;


fn_internal void cfdr_render_surface_draw(CFDR_Render *render, CFDR_Render_Surface *surface, V3F eye_position, M4F view_projection, R2F viewport) {
  M4F basis_change = {
    .e11 = +1,
    .e32 = -1,
    .e23 = +1,
    .e44 = +1,
  };

  M4F world                 = basis_change;
  world                     = m4f_mul(surface->transform, world);
  M4F world_view_projection = m4f_mul(world, view_projection);

  M4F world_inv = { };
  m4f_inv(world, &world_inv);

  V4F color = rgba_from_hsva(surface->color);
  color.rgb = v3f_mul_f32(color.rgb, color.a);

  R_Constant_Buffer_World_3D world_data = {
    .World_View_Projection   = world_view_projection,
    .World_Inverse_Transpose = m4f_trans(world_inv),
    .World                   = world,
    .Eye_Position            = eye_position,
    .Color                   = color
  };

  r_buffer_download(surface->state_buffer, 0, sizeof(world_data), &world_data);

  r_command_push_draw(&(R_Command_Draw) {
    .pipeline           = render->pipelines[CFDR_Render_Pipeline_Surface],
    .bind_group         = surface->bind_group,
    .vertex_buffer      = surface->resource->vertex_buffer,
    .index_buffer       = surface->resource->index_buffer,
    .draw_index_count   = surface->resource->index_count,
    .draw_index_offset  = 0,

    .depth_test         = 1,
    .draw_region        = r2i_from_r2f(viewport),
    .clip_region        = r2i_from_r2f(viewport),
  });
}

typedef struct CFDR_Render_Volume {
  CFDR_Resource_Volume  *resource;
  M4F                    transform;
  F32                    volume_density;
} CFDR_Render_Volume;

fn_internal void cfdr_render_volume_draw(CFDR_Render *render, CFDR_Render_Volume *volume, V3F eye_position, M4F view_projection, R2F viewport) {
  M4F basis_change = {
    .e11 = +1,
    .e32 = +1,
    .e23 = +1,
    .e44 = +1,
  };

  M4F world                 = basis_change;
  world                     = m4f_mul(volume->transform, world);
  M4F world_view_projection = m4f_mul(world, view_projection);

  M4F world_inv = { };
  m4f_inv(world, &world_inv);

  V3F volume_min = m4f_mul_v4f(v4f(-1, -1, -1, 1), world).xyz;
  V3F volume_max = m4f_mul_v4f(v4f(1, 1, 1, 1),    world).xyz;

  R_Constant_Buffer_World_3D world_data = {
    .World_View_Projection   = world_view_projection,
    .World                   = world,
    .Eye_Position            = eye_position,
    .Color                   = v4f(1, 1, 1, 1),
    .Grid_Scale              = 100.f,
    .Volume_Min              = volume_min,
    .Volume_Max              = volume_max,
    .Volume_Density          = volume->volume_density,
  };

  r_buffer_download(volume->resource->constant_buffer, 0, sizeof(world_data), &world_data);

  r_command_push_draw(&(R_Command_Draw) {
    .pipeline           = render->pipelines[CFDR_Render_Pipeline_Volume],
    .bind_group         = volume->resource->bind_group,
    .vertex_buffer      = render->cube_vertex_buffer,
    .index_buffer       = render->cube_index_buffer,
    .draw_index_count   = render->cube_index_count,
    .draw_index_offset  = 0,

    .depth_test         = 1,
    .draw_region        = r2i_from_r2f(viewport),
    .clip_region        = r2i_from_r2f(viewport),
  });
}
