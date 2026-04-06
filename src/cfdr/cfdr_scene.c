fn_internal void cfdr_scene_view_init(CFDR_Scene_View *view) {
  view->background = v4f(1, 0, 0, 1);
  cfdr_camera_init(&view->camera);
  cfdr_render_grid_init(&view->grid);
}

fn_internal void cfdr_scene_init(CFDR_Scene *scene) {
  zero_fill(scene);
  arena_init(&scene->arena);

  scene->count = 0;
  scene->first = 0;
  scene->last  = 0;
  scene->cmap  = str_lit("hsv-pastel");

  cfdr_scene_view_init(&scene->view);
}

fn_internal CFDR_Object_Node *cfdr_scene_push(CFDR_Scene *scene) {
  CFDR_Object_Node *object = arena_push_type(&scene->arena, CFDR_Object_Node);
  queue_push(scene->first, scene->last, object);

  zero_fill(object);
  object->flags          = 0;
  object->color          = v4f(1.f, 0.f, 1.f, 1.f);
  object->scale          = v3f(1, 1, 1);
  object->volume_density = 1.f;
  object->world_state    = r_buffer_allocate(sizeof(R_Constant_Buffer_World_3D), R_Buffer_Mode_Static);

  object->bind_group  = r_bind_group_create(&Flat_2D_Layout, &(R_Bind_Group_Entry_List) {
    .count      = 4,
    .entry_list = {
      { .binding = 0, .type = R_Binding_Type_Texture_2D, .resource = R_Texture_2D_White            },
      { .binding = 1, .type = R_Binding_Type_Sampler,    .resource = R_Sampler_Nearest_Clamp       },
      { .binding = 2, .type = R_Binding_Type_Uniform,    .resource = object->world_state           },
      { .binding = 3, .type = R_Binding_Type_Texture_3D, .resource = R_Texture_3D_White            },
    }
  });

  object->bind_group_sample = R_Resource_None;
  return object;
}

fn_internal M4F cfdr_object_node_transform(CFDR_Object_Node *object) {
  M4F scale     = m4f_hom_scale     (object->scale);
  M4F translate = m4f_hom_translate (object->translate);
  M4F transform = m4f_mul(scale, translate);

  return transform;
}

// TODO(cmat): Temporary test globals.
var_global CFDR_Resource_Volume *Last_Volume;
var_global M4F                   Last_Volume_Transform;

fn_internal void cfdr_scene_draw_surface(CFDR_Render *render, CFDR_CMap_Table *cmap_table, Str cmap_key, CFDR_Object_Node *object, V3F eye_position, M4F view_projection, M4F scene_transform, R2F viewport) {
  if (object->flags & CFDR_Object_Flag_Draw_Surface) {
    cfdr_resource_surface_update(&object->surface);
    if (object->surface.valid && object->visible) {

      // if (object->material == CFDR_Material_Sample && Last_Volume && object->bind_group_sample == R_Resource_None) {

      V2F vis_range = v2f(0, 1);
      if (object->material == CFDR_Material_Sample && Last_Volume) {
        if (object->bind_group_sample != R_Resource_None) {
          r_bind_group_destroy(&object->bind_group_sample);
        }

        CFDR_CMap *cmap = cfdr_cmap_table_get(cmap_table, cmap_key);
        if (cmap->map_mode == CFDR_CMap_Map_Min_Max) {
          vis_range = Last_Volume->data_range;
        } else if (cmap->map_mode == CFDR_CMap_Map_Clamp_To_Unit) {
          vis_range = v2f(0, 1);
        } else {
          vis_range = cmap->map_custom;
        }

        object->bind_group_sample = r_bind_group_create(&Flat_2D_Layout, &(R_Bind_Group_Entry_List) {
          .count      = 4,
          .entry_list = {
            { .binding = 0, .type = R_Binding_Type_Texture_2D, .resource = cmap->texture                 },
            { .binding = 1, .type = R_Binding_Type_Sampler,    .resource = R_Sampler_Nearest_Clamp       },
            { .binding = 2, .type = R_Binding_Type_Uniform,    .resource = object->world_state           },
            { .binding = 3, .type = R_Binding_Type_Texture_3D, .resource = Last_Volume->volume           },
          }
        });
      }

      // }

      CFDR_Render_Surface render_surface = {
        .resource                = &object->surface,
        .sample_volume           = Last_Volume,
        .sample_volume_transform = Last_Volume_Transform,
        .sample_vis_range        = vis_range,
        .state_buffer            = object->world_state,
        .bind_group              = object->bind_group,
        .bind_group_sample       = object->bind_group_sample,
        .color                   = object->color,
        .material                = object->material,
        .transform               = m4f_mul(cfdr_object_node_transform(object), scene_transform),
      };

      if (object->visible) {
        cfdr_render_surface_draw(render, &render_surface, eye_position, view_projection, viewport);
      }
    }
  }
}

fn_internal void cfdr_scene_draw_volume(CFDR_Render *render, U32 step_at, CFDR_Object_Node *object, V3F eye_position, M4F view_projection, M4F scene_transform, R2F viewport) {
  if (object->flags & CFDR_Object_Flag_Draw_Volume) {
    cfdr_resource_volume_update(&object->volume.vol_array[step_at]);
    if (object->volume.vol_array[step_at].valid) {
      
      CFDR_Render_Volume render_volume = {
        .resource     = &object->volume.vol_array[step_at],
        .transform    = m4f_mul(cfdr_object_node_transform(object), scene_transform),
        .volume_density = object->volume_density
      };

      // TODO(cmat): Temporary.
      Last_Volume           = render_volume.resource;
      Last_Volume_Transform = render_volume.transform;

      if (object->visible) { 
        cfdr_render_volume_draw(render, &render_volume, eye_position, view_projection, viewport);
      }
    }
  }
}

fn_internal R3F cfdr_scene_bounds(CFDR_Scene *scene) {
  R3F bounds = r3f( f32_largest_positive, f32_largest_positive, f32_largest_positive,
                    f32_largest_negative, f32_largest_negative, f32_largest_negative);

  for (CFDR_Object_Node *it = scene->first; it; it = it->next) {
    if (it->visible) {
      if (it->flags & CFDR_Object_Flag_Draw_Volume) {

          M4F T = cfdr_object_node_transform(it);
          V4F x = { };
          x          = v4f(0, 0, 0, 1);
          x          = m4f_mul_v4f(x, T);
          bounds.min = v3f(f32_min(x.x, bounds.min.x), f32_min(x.y, bounds.min.y), f32_min(x.z, bounds.min.z));
          bounds.max = v3f(f32_max(x.x, bounds.max.x), f32_max(x.y, bounds.max.y), f32_max(x.z, bounds.max.z));

          x          = v4f(1, 1, 1, 1);
          x          = m4f_mul_v4f(x, T);
          bounds.min = v3f(f32_min(x.x, bounds.min.x), f32_min(x.y, bounds.min.y), f32_min(x.z, bounds.min.z));
          bounds.max = v3f(f32_max(x.x, bounds.max.x), f32_max(x.y, bounds.max.y), f32_max(x.z, bounds.max.z));
      }

      if (it->flags & CFDR_Object_Flag_Draw_Surface) {
        cfdr_resource_surface_update(&it->surface);
        if (it->surface.valid) {

          M4F T = cfdr_object_node_transform(it);
          V4F x = { };

          x          = v4f(it->surface.bounds.min.x, it->surface.bounds.min.y, it->surface.bounds.min.z, 1);
          x          = m4f_mul_v4f(x, T);
          bounds.min = v3f(f32_min(x.x, bounds.min.x), f32_min(x.y, bounds.min.y), f32_min(x.z, bounds.min.z));
          bounds.max = v3f(f32_max(x.x, bounds.max.x), f32_max(x.y, bounds.max.y), f32_max(x.z, bounds.max.z));

          x          = v4f(it->surface.bounds.max.x, it->surface.bounds.max.y, it->surface.bounds.max.z, 1);
          x          = m4f_mul_v4f(x, T);
          bounds.min = v3f(f32_min(x.x, bounds.min.x), f32_min(x.y, bounds.min.y), f32_min(x.z, bounds.min.z));
          bounds.max = v3f(f32_max(x.x, bounds.max.x), f32_max(x.y, bounds.max.y), f32_max(x.z, bounds.max.z));
        }
      }
    }
  }

  if (bounds.min.x == f32_largest_positive) {
    bounds = r3f(-1, -1, -1, 1, 1, 1);
  }

  return bounds;
}

fn_internal void cfdr_scene_draw(CFDR_Render *render, CFDR_CMap_Table *cmap_table, UI_Response *response, CFDR_Scene *scene, R2F draw_region) {

  if (!cfdr_cmap_table_get(cmap_table, scene->cmap)) {
    // TODO(cmat): Better way to fallback to default.
    scene->cmap = str_lit("Rainbow");
  }

  if (response->drag) {
    B32 move_position = pl_input()->keyboard.state[PL_KB_Shift_Left].down;
    cfdr_camera_control(&scene->view.camera, draw_region, move_position, pl_input()->mouse.position_dt);
  }

  if (response->hover) {
    scene->view.camera.radius_m += pl_input()->mouse.scroll_dt.y * .025f;
  }

  cfdr_camera_update(&scene->view.camera, draw_region);
  V3F eye_position    = scene->view.camera.computed_position_m;
  M4F view_projection = cfdr_camera_transform(&scene->view.camera);

  g2_draw_rect(draw_region.min, r2f_size(draw_region), .color = rgba_from_hsva(scene->view.background));
  g2_submit_draw();

  R3F scene_region    = cfdr_scene_bounds(scene);
  V3F scene_size      = r3f_size(scene_region);
  F32 largest_axis    = v3f_largest(scene_size);
  M4F scene_transform = m4f_id();

  scene_transform     = m4f_mul(m4f_hom_scale(v3f_f32(40.f / largest_axis)), scene_transform);
  scene_transform     = m4f_mul(m4f_hom_translate(v3f(-scene_region.min.x, -scene_region.min.y, -scene_region.min.z)), scene_transform);
  scene_transform     = m4f_mul(m4f_hom_translate(v3f(-scene_size.x / 2, -scene_size.y / 2, 0)), scene_transform);

  cfdr_render_grid_draw(render, &scene->view.grid, view_projection, draw_region);

  // NOTE(cmat): Draw opaque objects first.
  for (CFDR_Object_Node *it = scene->first; it; it = it->next) {
    cfdr_scene_draw_surface (render, cmap_table, scene->cmap, it, eye_position, view_projection, scene_transform, draw_region);
  }

  // NOTE(cmat): Draw opaque transparent objects last.
  for (CFDR_Object_Node *it = scene->first; it; it = it->next) {
    if (it->flags & CFDR_Object_Flag_Draw_Volume) {
      scene->step.step_count = i32_max(scene->step.step_count, it->volume.step_count);
      scene->step.step_value = it->volume.step_array[scene->step.step_at];
    }

    cfdr_scene_draw_volume  (render, scene->step.step_at, it, eye_position, view_projection, scene_transform, draw_region);
  }
}
