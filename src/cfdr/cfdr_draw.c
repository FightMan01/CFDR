#if 0

typedef struct Camera {
  V3F look_at;
  F32 radius_m;
  F32 theta_deg;
  F32 phi_deg;

  F32 radius_m_t;
  F32 theta_deg_t;
  F32 phi_deg_t;

  F32 near_m;
  F32 far_m;
  F32 fov_deg;

  B32 orthographic;
  F32 orthographic_t;

  F32 computed_aspect_ratio;
  V3F computed_position_m;
} Camera;

enum Draw_Mode {
  Draw_Mode_Grid,
  Draw_Mode_Wireframe,
  Draw_Mode_Surface,
  Draw_Mode_Volume,
  Draw_Mode_Position,

  Draw_Mode_Count, 
} Draw_Mode;

typedef struct Draw_Grid {
  R_Buffer      constant_buffer;
  R_Bind_Group  bind_group;
  F32           scale;
  F32           subdiv;
  HSVA          color;
} Draw_Grid;

typedef struct Draw_3D_Indicator {
  R_Buffer      constant_buffer;
  R_Bind_Group  bind_group;
} Draw_3D_Indicator;

typedef struct Viewport {
  Camera            camera;
  Draw_Grid         grid;
  Draw_3D_Indicator indicator;

  B32               light_mode;
  B32               show_grid;
  B32               show_axes;
  B32               show_indicator;

  HSV               background_color;
} Viewport;

var_global struct {
  Arena arena;

  R_Pipeline pipelines[Draw_Mode_Count];

  // NOTE(cmat): We can get rid of this, just bake it into the grid shader.
  R_Buffer quad_vertex_buffer;
  R_Buffer quad_index_buffer;

  R_Buffer cube_vertex_buffer;
  R_Buffer cube_index_buffer;
  R_Buffer cube_index_count;


  Viewport viewport_array[2];

  G2_Font  title_font;
} CFDR_Draw_State;

fn_internal void camera_update(Camera *camera, R2F draw_region) {
  F32 frame_delta = pl_display()->frame_delta;
  
  camera->radius_m = f32_clamp(camera->radius_m, 3.f, 50.f);
  camera->phi_deg  = f32_clamp(camera->phi_deg,  0.01f, 179.99f);

  camera->radius_m_t  = f32_exp_smoothing(camera->radius_m_t,     camera->radius_m,  frame_delta * 15.f);
  camera->theta_deg_t = f32_exp_smoothing(camera->theta_deg_t,    camera->theta_deg, frame_delta * 15.f);
  camera->phi_deg_t   = f32_exp_smoothing(camera->phi_deg_t,      camera->phi_deg,   frame_delta * 15.f);

  camera->orthographic_t = f32_exp_smoothing(camera->orthographic_t, camera->orthographic, frame_delta * 15.f);

  F32 theta_rad = f32_radians_from_degrees(camera->theta_deg_t);
  F32 phi_rad   = f32_radians_from_degrees(camera->phi_deg_t);

  camera->computed_aspect_ratio = (draw_region.x1 - draw_region.x0) / (draw_region.y1 - draw_region.y0);
  camera->computed_position_m = v3f_mul(camera->radius_m_t,
                                        v3f(f32_cos(theta_rad) * f32_sin(phi_rad),
                                        f32_cos(phi_rad),
                                        f32_sin(theta_rad) * f32_sin(phi_rad)));

  camera->computed_position_m = v3f_add(camera->computed_position_m, camera->look_at);
}

fn_internal M4F camera_view(Camera *camera) {
  M4F view = m4f_hom_look_at(v3f(0, 1, 0), camera->computed_position_m, camera->look_at);
  return view;
}

fn_internal F32 camera_world_units_per_pixel(Camera *camera, R2F draw_region) {
  F32 fov_rad = f32_radians_from_degrees(camera->fov_deg);
  F32 h = 2.f * camera->radius_m_t * f32_tan(.5f * fov_rad);
  F32 result = f32_div_safe(h, draw_region.max.y - draw_region.min.y);
  return result;
}

fn_internal M4F camera_projection(Camera *camera) {
  F32 fov_rad = f32_radians_from_degrees(camera->fov_deg);

  M4F projection = { };
  M4F perspective = m4f_hom_perspective(camera->computed_aspect_ratio, fov_rad, camera->near_m, camera->far_m);
 
  F32 h = 2.f * camera->radius_m_t * f32_tan(.5f * fov_rad);
  F32 w = h * camera->computed_aspect_ratio;

  V2F bottom_left = v2f(-.5f * w, -.5f * h);
  V2F top_right   = v2f(+.5f * w, +.5f * h);
  M4F orthographic = m4f_hom_orthographic(bottom_left, top_right, 0, 0);

  projection = m4f_lerp(camera->orthographic_t, perspective, orthographic);
  return projection;
}

fn_internal M4F camera_transform(Camera *camera) {
  M4F view        = camera_view(camera);
  M4F projection  = camera_projection(camera);
  M4F transform   = m4f_mul(view, projection);
  return transform;
}

fn_internal void cfdr_draw_grid_init(Draw_Grid *grid) {
  grid->constant_buffer = r_buffer_allocate(sizeof(R_Constant_Buffer_World_3D), R_Buffer_Mode_Dynamic);
  grid->bind_group      = r_bind_group_create(&Flat_2D_Layout, &(R_Bind_Group_Entry_List) {
    .count      = 4,
    .entry_list = {
      { .binding = 0, .type = R_Binding_Type_Texture_2D, .resource = R_Texture_2D_White            },
      { .binding = 1, .type = R_Binding_Type_Sampler,    .resource = R_Sampler_Nearest_Clamp       },
      { .binding = 2, .type = R_Binding_Type_Uniform,    .resource = grid->constant_buffer         },
      { .binding = 3, .type = R_Binding_Type_Texture_3D, .resource = R_Texture_3D_White            },
    }
  });

  grid->scale   = 1000;
  grid->subdiv  = 1;
  grid->color   = v4f(0, 0, 1, 1);
}

fn_internal void cfdr_draw_grid(Draw_Grid *grid, M4F view_projection, R2I viewport) {

  M4F world_view_projection = m4f_mul(m4f_diag(v4f(grid->scale, 1, grid->scale, 1)), view_projection);

 R_Constant_Buffer_World_3D world_data = {
    .World_View_Projection = world_view_projection,
    .Eye_Position          = v3f(0, 0, 0),
    .Volume_Density        = 0.0f,
    .Grid_Scale            = grid->subdiv * grid->scale,
    .Color                 = rgba_from_hsva(grid->color)
  };

  r_buffer_download(grid->constant_buffer, 0, sizeof(world_data), &world_data);

  r_command_push_draw(&(R_Command_Draw) {
    .pipeline           = CFDR_Draw_State.pipelines[Draw_Mode_Grid],
    .bind_group         = grid->bind_group,
    .vertex_buffer      = CFDR_Draw_State.quad_vertex_buffer,
    .index_buffer       = CFDR_Draw_State.quad_index_buffer,
    .draw_index_count   = 6,
    .draw_index_offset  = 0,

    .depth_test         = 1,
    .draw_region        = viewport,
    .clip_region        = viewport,
  });
}

fn_internal void cfdr_draw_viewport_init(Viewport *viewport) {
  viewport->camera = (Camera) {
    .look_at      = { 0, 0, 0 },
    .radius_m     = 10.f,
    .theta_deg    = 45.f,
    .phi_deg      = 45.f,
    .near_m       = 0.1f,
    .far_m        = 1000.f,
    .fov_deg      = 60.f,
    .orthographic = 0,
  };

  cfdr_draw_grid_init(&viewport->grid);

  viewport->show_grid = 1;
}

var_global U08 Title_Font_Raw[] = {
#embed "../cfdr_data/PTSerif-Bold.ttf"
};

var_global Str Title_Font_Baked = (Str) { .len = sizeof(Title_Font_Raw), .txt = Title_Font_Raw };

fn_internal void cfdr_draw_init(void) {
  arena_init(&CFDR_Draw_State.arena);

  CFDR_Draw_State.pipelines[Draw_Mode_Grid] = r_pipeline_create(&(R_Pipeline_Layout) {
    .shader       = R_Shader_Grid_3D,
    .format       = &R_Vertex_Format_XNUC_3D,
    .depth_test   = 1,
    .depth_write  = 0,
    .depth_bias   = 1,
  });

  CFDR_Draw_State.pipelines[Draw_Mode_Wireframe] = r_pipeline_create(&(R_Pipeline_Layout) {
    .shader       = R_Shader_Grid_3D,
    .format       = &R_Vertex_Format_XNUC_3D,
    .depth_test   = 1,
    .depth_write  = 0,
    .depth_bias   = 1,
  });

  CFDR_Draw_State.pipelines[Draw_Mode_Surface] = r_pipeline_create(&(R_Pipeline_Layout) {
    .shader       = R_Shader_Edit_3D,
    .format       = &R_Vertex_Format_XNUC_3D,
    .depth_test   = 1,
    .depth_write  = 1
  });

  CFDR_Draw_State.pipelines[Draw_Mode_Volume] = r_pipeline_create(&(R_Pipeline_Layout) {
    .shader       = R_Shader_DVR_3D,
    .format       = &R_Vertex_Format_XNUC_3D,
    .depth_test   = 1,
    .depth_write  = 1
  });

  CFDR_Draw_State.pipelines[Draw_Mode_Position] = r_pipeline_create(&(R_Pipeline_Layout) {
    .shader       = R_Shader_Position_3D,
    .format       = &R_Vertex_Format_XNUC_3D,
    .depth_test   = 1,
    .depth_write  = 1
  });

  R_Vertex_XNUC_3D quad_vertices[] = {
    { .X = v3f(-1, 0, -1), .N = v3f(0, 1, 0), .U = v2f(0, 0), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },
    { .X = v3f(+1, 0, -1), .N = v3f(0, 1, 0), .U = v2f(1, 0), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },
    { .X = v3f(+1, 0, +1), .N = v3f(0, 1, 0), .U = v2f(1, 1), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },
    { .X = v3f(-1, 0, +1), .N = v3f(0, 1, 0), .U = v2f(0, 1), .C = abgr_u32_from_rgba_premul(v4f(1.f, 1.f, 1.f, 1.f)), },
  };
  
  U32 quad_indices[] = { 0, 2, 1, 0, 3, 2 };
 
  CFDR_Draw_State.quad_vertex_buffer = r_buffer_allocate(sizeof(quad_vertices), R_Buffer_Mode_Static);
  r_buffer_download(CFDR_Draw_State.quad_vertex_buffer, 0, sizeof(quad_vertices), quad_vertices);

  CFDR_Draw_State.quad_index_buffer = r_buffer_allocate(sizeof(quad_indices), R_Buffer_Mode_Static);
  r_buffer_download(CFDR_Draw_State.quad_index_buffer, 0, sizeof(quad_indices), quad_indices);

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
  
  For_U32(it, sarray_len(cube_vertices)) {
    cube_vertices[it].X.x = -(cube_vertices[it].X.x + 1.f) * 1000.f  * 0.025f;
    cube_vertices[it].X.y = (cube_vertices[it].X.y + 1.f) * 100.f   * 0.025f;
    cube_vertices[it].X.z = (cube_vertices[it].X.z + 1.f) * 1000.f  * 0.025f;
  } 

  U32 cube_indices[] = {
    0,   2,  1, 0,   3,  2,
    4,   5,  6, 4,   6,  7,
    8,  10,  9,  8, 11, 10,
    12, 13, 14, 12, 14, 15,
    16, 17, 18, 16, 18, 19,
    20, 22, 21, 20, 23, 22
  };

  CFDR_Draw_State.cube_index_count = sarray_len(cube_indices);

  CFDR_Draw_State.cube_vertex_buffer = r_buffer_allocate(sizeof(cube_vertices), R_Buffer_Mode_Static);
  r_buffer_download(CFDR_Draw_State.cube_vertex_buffer, 0, sizeof(cube_vertices), cube_vertices);

  CFDR_Draw_State.cube_index_buffer = r_buffer_allocate(sizeof(cube_indices), R_Buffer_Mode_Static);
  r_buffer_download(CFDR_Draw_State.cube_index_buffer, 0, sizeof(cube_indices), cube_indices);

  cfdr_draw_viewport_init(&CFDR_Draw_State.viewport_array[0]);
  cfdr_draw_viewport_init(&CFDR_Draw_State.viewport_array[1]);

  g2_font_init(&CFDR_Draw_State.title_font, &CFDR_Draw_State.arena, Title_Font_Baked, 70.f, v2_u16(1024, 1024), Codepoints_ASCII);
}

// ------------------------------------------------------------
// #-- Draw viewport.

fn_internal void cfdr_draw_layer_text(CFDR_Layer *layer, R2F draw_region) {
  V2F text_at     = { };
  F32 text_width  = fo_text_width(&CFDR_Draw_State.title_font.font, layer->text);

  switch (layer->alignment_x) {
    case Align2_Left:    { text_at.x = draw_region.min.x + layer->border_x;                                                } break;
    case Align2_Right:   { text_at.x = draw_region.max.x - text_width - layer->border_x;                                   } break;
    case Align2_Center:  { text_at.x = draw_region.min.x + .5f * ((draw_region.max.x - draw_region.min.x) - text_width);   } break;
  }

  switch (layer->alignment_y) {
    case Align2_Bottom:    { text_at.y = draw_region.min.y - CFDR_Draw_State.title_font.font.metric_descent + layer->border_y;                                 } break;
    case Align2_Top:       { text_at.y = draw_region.max.y - CFDR_Draw_State.title_font.font.metric_ascent - layer->border_y;                                  } break;
    case Align2_Center:    { text_at.y = draw_region.min.y + .5f * ((draw_region.max.y - draw_region.min.y) - CFDR_Draw_State.title_font.font.metric_height);  } break;
  }

  if (layer->background_overlay) {
    V2F text_bounds = v2f(fo_text_width(&CFDR_Draw_State.title_font.font, layer->text), CFDR_Draw_State.title_font.font.metric_height);
    g2_draw_rect(v2f(text_at.x, text_at.y + CFDR_Draw_State.title_font.font.metric_descent), v2f_add(text_bounds, v2f(2, 2)), .color = v4f(0.0f, 0.0f, 0.0f, 1.f));
  }

  R2I draw_region_px  = r2i(draw_region.x0, draw_region.y0, draw_region.x1, draw_region.y1);
  text_at = v2f(f32_floor(text_at.x), f32_floor(text_at.y));
  if (layer->shadow_overlay) {
    g2_draw_text(layer->text, &CFDR_Draw_State.title_font, v2f_add(text_at, v2f(layer->shadow_overlay_offset, -layer->shadow_overlay_offset)), .color = v4f(0, 0, 0, layer->shadow_overlay_opacity));
  }

  g2_draw_text(layer->text, &CFDR_Draw_State.title_font, text_at, .color = rgba_from_hsva(layer->color));
  g2_submit_draw();
}

fn_internal void cfdr_draw_layer_surface(CFDR_Layer *layer, V3F eye_position, M4F view_projection, R2F draw_region) {
  cfdr_surface_update(&layer->surface);
  if (layer->surface.downloaded) {

   R_Constant_Buffer_World_3D world_data = {
      .World_View_Projection = view_projection,
      .Eye_Position          = eye_position,
      .Color                 = rgba_from_hsva(layer->color),
      .Grid_Scale            = 1.0f,
    };

    r_buffer_download(layer->surface.constant_buffer, 0, sizeof(world_data), &world_data);


    // NOTE(cmat): Draw solid geometry.
    R2I draw_region_px  = r2i(draw_region.x0, draw_region.y0, draw_region.x1, draw_region.y1);

    if (layer->material_type == CFDR_Material_Position) {
      r_command_push_draw(&(R_Command_Draw) {
        .pipeline           = CFDR_Draw_State.pipelines[Draw_Mode_Position],
        .bind_group         = layer->surface.bind_group,
        .vertex_buffer      = layer->surface.vertex_buffer,
        .index_buffer       = layer->surface.index_buffer,
        .draw_index_count   = layer->surface.index_count,
        .draw_index_offset  = 0,

        .depth_test         = 1,
        .draw_region        = draw_region_px,
        .clip_region        = draw_region_px,
      });
    } else if (layer->material_type == CFDR_Material_Flat) {
      r_command_push_draw(&(R_Command_Draw) {
        .pipeline           = CFDR_Draw_State.pipelines[Draw_Mode_Surface],
        .bind_group         = layer->surface.bind_group,
        .vertex_buffer      = layer->surface.vertex_buffer,
        .index_buffer       = layer->surface.index_buffer,
        .draw_index_count   = layer->surface.index_count,
        .draw_index_offset  = 0,

        .depth_test         = 1,
        .draw_region        = draw_region_px,
        .clip_region        = draw_region_px,
      });
    }

    // NOTE(cmat): Draw wireframe.
    if (layer->wireframe) {
      r_command_push_draw(&(R_Command_Draw) {
        .pipeline           = CFDR_Draw_State.pipelines[Draw_Mode_Wireframe],
        .bind_group         = layer->surface.bind_group,
        .vertex_buffer      = layer->surface.vertex_buffer,
        .index_buffer       = layer->surface.index_buffer,
        .draw_index_count   = layer->surface.index_count,
        .draw_index_offset  = 0,

        .depth_test         = 1,
        .draw_region        = draw_region_px,
        .clip_region        = draw_region_px,
      });
    }
  }
}

fn_internal void cfdr_draw_layer_volume(CFDR_Layer *layer, V3F eye_position, M4F view_projection, R2F draw_region) {
  cfdr_volume_update(&layer->volume);
  if (layer->volume.downloaded) {

   R_Constant_Buffer_World_3D world_data = {
      .World_View_Projection = view_projection,
      .Eye_Position          = eye_position,
      .Color                 = rgba_from_hsva(layer->color),
      .Grid_Scale            = 1.0f,
      .Volume_Density        = layer->volume_density
    };

    r_buffer_download(layer->volume.constant_buffer, 0, sizeof(world_data), &world_data);

    // NOTE(cmat): Draw solid geometry.
    R2I draw_region_px  = r2i(draw_region.x0, draw_region.y0, draw_region.x1, draw_region.y1);
    r_command_push_draw(&(R_Command_Draw) {
      .pipeline           = CFDR_Draw_State.pipelines[Draw_Mode_Volume],
      .bind_group         = layer->volume.bind_group,
      .vertex_buffer      = CFDR_Draw_State.cube_vertex_buffer,
      .index_buffer       = CFDR_Draw_State.cube_index_buffer,
      .draw_index_count   = CFDR_Draw_State.cube_index_count,
      .draw_index_offset  = 0,

      .depth_test         = 1,
      .draw_region        = draw_region_px,
      .clip_region        = draw_region_px,
    });
  }
}

fn_internal void cfdr_draw_viewport(UI_Response *response, R2F draw_region, void *user_data) {
  Viewport *vp = (Viewport *)user_data;

  V2F position  = draw_region.min;
  V2F size      = v2f_sub(draw_region.max, draw_region.min);

  g2_draw_rect(position, size, .color = rgba_from_hsva(v4f(vp->background_color.h, vp->background_color.s, vp->background_color.v, 1)));
  g2_submit_draw();

  camera_update(&vp->camera, draw_region);
  if (response->drag) {
    if (pl_input()->keyboard.state[PL_KB_Shift_Left].down) {
      V3F z_axis = v3f_noz(v3f_sub(vp->camera.computed_position_m, vp->camera.look_at));
      V3F x_axis = v3f_noz(v3f_cross(v3f(0, 1, 0), z_axis));
      V3F y_axis = v3f_noz(v3f_cross(z_axis, x_axis));

      F32 units_per_pixel = camera_world_units_per_pixel(&vp->camera, draw_region);
      F32 scale = units_per_pixel;
      vp->camera.look_at = v3f_add(vp->camera.look_at, v3f_mul(-scale * pl_input()->mouse.position_dt.x, x_axis));
      vp->camera.look_at = v3f_add(vp->camera.look_at, v3f_mul(-scale * pl_input()->mouse.position_dt.y, y_axis));
      camera_update(&vp->camera, draw_region);
    } else {
      vp->camera.theta_deg += 10.f * pl_display()->frame_delta * pl_input()->mouse.position_dt.x;
      vp->camera.phi_deg   += 10.f * pl_display()->frame_delta * pl_input()->mouse.position_dt.y;
    }
  }

  if (response->hover) {
    vp->camera.radius_m += pl_input()->mouse.scroll_dt.y * .025f;
  }

  R2I draw_region_px  = r2i(draw_region.x0, draw_region.y0, draw_region.x1, draw_region.y1);
  M4F view_projection = camera_transform(&vp->camera);

  For_U32(it, CFDR_Res_State.layer_array.len) {
    CFDR_Layer *layer = &CFDR_Res_State.layer_array.dat[it];
    if (layer->visible) {
      switch (layer->type) {
        case CFDR_Layer_Type_Text:    { cfdr_draw_layer_text    (layer, draw_region);                                                   } break;
        case CFDR_Layer_Type_Surface: { cfdr_draw_layer_surface (layer, vp->camera.computed_position_m, view_projection, draw_region);  } break;
        case CFDR_Layer_Type_Volume:  { cfdr_draw_layer_volume  (layer, vp->camera.computed_position_m, view_projection, draw_region);  } break;
      }
    }
  }
  if (vp->show_grid) {
    cfdr_draw_grid(&vp->grid, view_projection, draw_region_px);
  }
}

#endif
