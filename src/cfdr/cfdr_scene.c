fn_internal void cfdr_scene_view_init(CFDR_Render *render, CFDR_Scene_View *view) {
  view->background = v4f(1, 0, 0, 1);
  cfdr_camera_init(&view->camera);
  cfdr_render_grid_init(render, &view->grid);

  view->camera_xyz.look_at      = v3f(0, 0, 0);
  view->camera_xyz.radius_m     = 30.f;
  view->camera_xyz.theta_deg    = view->camera.theta_deg;
  view->camera_xyz.phi_deg      = view->camera.phi_deg;
  view->camera_xyz.near_m       = 0.1f;
  view->camera_xyz.far_m        = 100.f;
  view->camera_xyz.fov_deg      = view->camera.fov_deg;
  view->camera_xyz.orthographic = view->camera.orthographic;
  view->camera_xyz.radius_m_t   = view->camera_xyz.radius_m;
  view->camera_xyz.theta_deg_t  = view->camera_xyz.theta_deg;
  view->camera_xyz.phi_deg_t    = view->camera_xyz.phi_deg;
  view->camera_xyz.orthographic_t = view->camera_xyz.orthographic;
}

fn_internal void cfdr_scene_init(CFDR_Render *render, CFDR_Scene *scene) {
  zero_fill(scene);
  arena_init(&scene->arena);

  scene->count = 0;
  scene->first = 0;
  scene->last  = 0;
  scene->cmap  = str_lit("hsv-pastel");

  scene->picker_grabbed_axis = -1;

  cfdr_scene_view_init(render, &scene->view);

  For_U32(it, 3) {
    scene->widget_arrow_xyz[it]         = cfdr_render_arrow_init(render, .1f, 2.f, .75f);
    scene->picker_arrow_xyz[it]         = cfdr_render_arrow_init(render, .1f, 2.f, .75f);

    F32 shadow = 0.08f;
    scene->picker_arrow_xyz_outline[it] = cfdr_render_arrow_init(render, .1f + shadow, 2.f + shadow, .75f + shadow);
  }
}

fn_internal CFDR_Object_Node *cfdr_scene_push(CFDR_Scene *scene) {
  CFDR_Object_Node *object = arena_push_type(&scene->arena, CFDR_Object_Node);
  queue_push(scene->first, scene->last, object);

  zero_fill(object);
  object->flags          = 0;
  object->color          = v4f(1.f, 0.f, 1.f, 1.f);
  object->scale          = v3f(1, 1, 1);
  object->volume_density = 1.f;
  object->volume_saturate = 2.5f;
  object->world_state    = r_buffer_allocate(sizeof(R_Constant_Buffer_World_3D), R_Buffer_Mode_Static);
  object->vol_state      = r_buffer_allocate(sizeof(R_Constant_Buffer_Vol_3D),   R_Buffer_Mode_Static);
  object->volume_xyz     = 0;

  return object;
}

fn_internal M4F cfdr_object_node_transform(CFDR_Object_Node *object, R3F bounds) {
  M4F scale     = m4f_hom_scale     (object->scale);
  M4F translate = m4f_hom_translate (object->translate);
  M4F rotate    = m4f_hom_rotate_xyz (v3f(f32_radians_from_degrees(object->rotate.x),
                                          f32_radians_from_degrees(object->rotate.y),
                                          f32_radians_from_degrees(object->rotate.z)));
 
  V3F pivot       = v3f_mul(.5f, v3f_add(bounds.min, bounds.max));
  M4F pivot_T     = m4f_hom_translate(pivot);

  V3F pivot_inv   = v3f_mul(-1.f, pivot);
  M4F pivot_inv_T = m4f_hom_translate(pivot_inv);

  M4F transform = m4f_mul(m4f_mul(pivot_inv_T, rotate), pivot_T);

  transform = m4f_mul(m4f_mul(scale, transform), translate);

  return transform;
}

// TODO(cmat): Temporary test globals.
var_global CFDR_Resource_Volume *Last_Volume;
var_global I32                   Last_Volume_XYZ;
var_global M4F                   Last_Volume_Transform;

fn_internal void cfdr_scene_draw_surface(CFDR_Render *render, CFDR_CMap_Table *cmap_table, Str cmap_key, CFDR_Object_Node *object, V3F eye_position, M4F view_projection, M4F scene_transform, R2F viewport) {
  if (object->flags & CFDR_Object_Flag_Draw_Surface) {
    cfdr_resource_surface_update(&object->surface);
    if (object->surface.valid && object->visible) {

      V2F vis_range = v2f(0, 1);
      V3F contour_color = v3f(0, 0, 0);
      B32 contour_visible = 0;
      F32 contour_value = 0;
      F32 contour_thickness = 0;
      if (object->material == CFDR_Material_Matcap || object->material == CFDR_Material_Flat) {
        if (object->bind_group != R_Resource_None) {
          r_bind_group_destroy(&object->bind_group);
        }

        object->bind_group = r_bind_group_create(&Flat_3D_Layout, &(R_Bind_Group_Entry_List) {
          .count      = 6,
          .entry_list = {
            { .binding = 0, .type = R_Binding_Type_Storage,    .resource = object->surface.X_buffer      },
            { .binding = 1, .type = R_Binding_Type_Storage,    .resource = object->surface.U_buffer      },
            { .binding = 2, .type = R_Binding_Type_Storage,    .resource = object->surface.N_buffer      },
            { .binding = 3, .type = R_Binding_Type_Texture_2D, .resource = R_Texture_2D_White            },
            { .binding = 4, .type = R_Binding_Type_Sampler,    .resource = R_Sampler_Nearest_Clamp       },
            { .binding = 5, .type = R_Binding_Type_Uniform,    .resource = object->world_state           },
          }
        });

      } if (object->material == CFDR_Material_Sample && Last_Volume) {
        if (object->bind_group_vol != R_Resource_None) {
          r_bind_group_destroy(&object->bind_group_vol);
        }

        CFDR_CMap *cmap   = cfdr_cmap_table_get(cmap_table, cmap_key);
        vis_range         = cfdr_cmap_range(cmap);
        contour_color     = cmap->contour_color;
        contour_visible   = cmap->contour_visible;
        contour_value     = cmap->contour_value;
        contour_thickness = cmap->contour_thickness;

        object->bind_group_vol = r_bind_group_create(&Vol_3D_Layout, &(R_Bind_Group_Entry_List) {
          .count      = 5,
          .entry_list = {
            { .binding = 0, .type = R_Binding_Type_Storage,    .resource = object->surface.X_buffer      },
            { .binding = 1, .type = R_Binding_Type_Texture_2D, .resource = cmap->texture                 },
            { .binding = 2, .type = R_Binding_Type_Sampler,    .resource = R_Sampler_Linear_Clamp        },
            { .binding = 3, .type = R_Binding_Type_Uniform,    .resource = object->vol_state             },
            { .binding = 4, .type = R_Binding_Type_Texture_3D, .resource = Last_Volume->volume           },
          }
        });
      }

      CFDR_Render_Surface render_surface = {
        .resource                = &object->surface,
        .sample_volume           = Last_Volume,
        .sample_volume_transform = Last_Volume_Transform,
        .sample_vis_range        = vis_range,
        .sample_volume_xyz       = Last_Volume_XYZ,
        .state_buffer            = object->world_state,
        .vol_buffer              = object->vol_state,
        .bind_group              = object->bind_group,
        .bind_group_vol          = object->bind_group_vol,
        .color                   = object->color,
        .material                = object->material,
        .transform               = m4f_mul(cfdr_object_node_transform(object, object->surface.bounds), scene_transform),
        .contour_color           = rgb_from_hsv(contour_color),
        .contour_visible         = contour_visible,
        .contour_value           = contour_value,
        .contour_thickness       = contour_thickness,
      };

      if (object->visible) {
        cfdr_render_surface_draw(render, &render_surface, eye_position, view_projection, viewport);
      }
    }
  }
}

fn_internal void cfdr_scene_draw_volume(CFDR_Render *render, CFDR_CMap_Table *cmap_table, Str cmap_key, CFDR_Scene_Step *step, CFDR_Object_Node *object, V3F eye_position, M4F view_projection, M4F scene_transform, R2F viewport) {
  if (object->flags & CFDR_Object_Flag_Draw_Volume) {
    U64 vol_idx = (step->step_count * step->var_at) + step->step_at;

    cfdr_resource_volume_update(&object->volume.vol_array[vol_idx]);
    if (object->volume.vol_array[vol_idx].valid) {

      if (object->bind_group_vol != R_Resource_None) {
        r_bind_group_destroy(&object->bind_group_vol);
      }

      CFDR_CMap *cmap = cfdr_cmap_table_get(cmap_table, cmap_key);
      V2F vis_range = cfdr_cmap_range(cmap);

      object->bind_group_vol = r_bind_group_create(&Vol_3D_Layout, &(R_Bind_Group_Entry_List) {
        .count      = 5,
        .entry_list = {
          { .binding = 0, .type = R_Binding_Type_Storage,    .resource = render->cube_X_buffer                              },
          { .binding = 1, .type = R_Binding_Type_Texture_2D, .resource = cmap->texture                                      },
          { .binding = 2, .type = R_Binding_Type_Sampler,    .resource = R_Sampler_Linear_Clamp                             },
          { .binding = 3, .type = R_Binding_Type_Uniform,    .resource = object->volume.vol_array[vol_idx].constant_buffer  },
          { .binding = 4, .type = R_Binding_Type_Texture_3D, .resource = object->volume.vol_array[vol_idx].volume           },
        }
      });

      CFDR_Render_Volume render_volume = {
        .resource                        = &object->volume.vol_array[vol_idx],
        .transform                       = m4f_mul(cfdr_object_node_transform(object, r3f_v(object->translate, object->scale)), scene_transform),
        .volume_density                  = object->volume_density,
        .volume_saturate                 = object->volume_saturate,
        .volume_xyz                      = object->volume_xyz,
        .bind_group_vol                  = object->bind_group_vol,
        .vis_range                       = vis_range,
      };

      // TODO(cmat): Temporary.
      Last_Volume           = render_volume.resource;
      Last_Volume_XYZ       = object->volume_xyz;
      Last_Volume_Transform = render_volume.transform;

      if (object->visible) { 
        cfdr_render_volume_draw(render, &render_volume, eye_position, view_projection, viewport);
      }
    }
  }
}

fn_internal R3F cfdr_scene_object_bounds(CFDR_Object_Node *obj) {
  R3F result = { };
  if (obj->flags & CFDR_Object_Flag_Draw_Volume) {
    result = r3f_v(obj->translate, obj->scale);
  } else if (obj->flags * CFDR_Object_Flag_Draw_Surface) {
    if (obj->surface.valid) {
      result = obj->surface.bounds;
      result.min = v3f_add(result.min, obj->translate);
      result.max = v3f_add(result.max, obj->translate);
    }
  }

  return result;
}

fn_internal R3F cfdr_scene_bounds(CFDR_Scene *scene) {
  R3F bounds = r3f( f32_largest_positive, f32_largest_positive, f32_largest_positive,
                    f32_largest_negative, f32_largest_negative, f32_largest_negative);

  for (CFDR_Object_Node *it = scene->first; it; it = it->next) {
    if ((it->visible && scene->scene_bounds_from_obj.len == 0) || (str_equals(it->tag, scene->scene_bounds_from_obj))) {
      if (it->flags & CFDR_Object_Flag_Draw_Volume) {

          M4F T = cfdr_object_node_transform(it, r3f_v(it->translate, it->scale));
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

          M4F T = cfdr_object_node_transform(it, it->surface.bounds);
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

fn_internal void cfdr_scene_draw_widget_arrow(CFDR_Render *render, CFDR_Scene *scene, R2F draw_region) {
  V3F eye_position    = scene->view.camera.computed_position_m;

  // NOTE(cmat): Make a special camera for the XYZ widget.
  scene->view.camera_xyz.look_at      = v3f(0, 0, 0);
  scene->view.camera_xyz.radius_m     = 15.f;
  scene->view.camera_xyz.theta_deg    = scene->view.camera.theta_deg;
  scene->view.camera_xyz.phi_deg      = scene->view.camera.phi_deg;
  scene->view.camera_xyz.near_m       = 0.1f;
  scene->view.camera_xyz.far_m        = 100.f;
  scene->view.camera_xyz.fov_deg      = scene->view.camera.fov_deg;
  scene->view.camera_xyz.orthographic = scene->view.camera.orthographic;

  cfdr_camera_update(&scene->view.camera_xyz, draw_region);
  M4F xyz_view_projection = cfdr_camera_transform(&scene->view.camera_xyz);

  M4F x_world = (cfdr_arrow_transform(v3f(1, 0, 0), 1));
  M4F y_world = (cfdr_arrow_transform(v3f(0, 1, 0), 1));
  M4F z_world = (cfdr_arrow_transform(v3f(0, 0, 1), 1));

  M4F x_wvp = m4f_mul(cfdr_render_apply_basis_change(x_world), xyz_view_projection);
  M4F y_wvp = m4f_mul(cfdr_render_apply_basis_change(y_world), xyz_view_projection);
  M4F z_wvp = m4f_mul(cfdr_render_apply_basis_change(z_world), xyz_view_projection);

  U32 draw_order[] = { 0, 1, 2 };
  V4F e1           = m4f_mul_v4f(v4f(1, 0, 0, 1), x_wvp);
  V4F e2           = m4f_mul_v4f(v4f(1, 0, 0, 1), y_wvp);
  V4F e3           = m4f_mul_v4f(v4f(1, 0, 0, 1), z_wvp);

  if (e1.z < e2.z) { Swap(V4F, e1, e2); Swap(U32, draw_order[0], draw_order[1]); }
  if (e1.z < e3.z) { Swap(V4F, e1, e3); Swap(U32, draw_order[0], draw_order[2]); }
  if (e2.z < e3.z) { Swap(V4F, e2, e3); Swap(U32, draw_order[1], draw_order[2]); }

  HSVA Color_Arrow_X = hsva_u32(0,   70, 100, 255);
  HSVA Color_Arrow_Y = hsva_u32(120, 70, 100, 255);
  HSVA Color_Arrow_Z = hsva_u32(240, 70, 100, 255);

  For_U32(it, 3) {
    U32 idx = draw_order[it];
    switch (idx) {
      case 0: { cfdr_render_arrow_draw(render, &scene->widget_arrow_xyz[it], x_world, xyz_view_projection, eye_position, Color_Arrow_X, draw_region); } break;
      case 1: { cfdr_render_arrow_draw(render, &scene->widget_arrow_xyz[it], y_world, xyz_view_projection, eye_position, Color_Arrow_Y, draw_region); } break;
      case 2: { cfdr_render_arrow_draw(render, &scene->widget_arrow_xyz[it], z_world, xyz_view_projection, eye_position, Color_Arrow_Z, draw_region); } break;
    }
  }
}

fn_internal void cfdr_scene_draw_picker_arrow(CFDR_Render_Arrow *arrow_xyz, CFDR_Render *render, CFDR_Scene *scene, M4F scene_transform, M4F view_projection, V3F eye_position, V3F scene_size, R2F draw_region, V4F color_tint, F32 size_px) {
  if (scene->active) {
    R3F active_bounds  = cfdr_scene_object_bounds(scene->active);
    V3F active_center  = v3f_mul(.5f, v3f_add(active_bounds.min, active_bounds.max));
    M4F obj_transform  = m4f_hom_translate(active_center);
    M4F world          = m4f_mul(obj_transform, scene_transform);

    V4F look_at        = { };
    look_at.xyz        = scene->view.camera.look_at;
    look_at.w          = 1.f;

    F32 desired_pixels = size_px; // 50.0f * js_web_device_pixel_ratio();
    F32 distance       = v3f_len(v3f_sub(m4f_mul_v4f(look_at, world).xyz, eye_position));
    F32 world_height   = 2.0f * distance * f32_tan(f32_radians_from_degrees(scene->view.camera.fov_deg) * 0.5f) * (desired_pixels / r2f_size(draw_region).y);

    F32 arrow_scale    = 0.5f * world_height * v3f_largest(scene_size) / 40.f;

    M4F x_world        = m4f_mul(cfdr_arrow_transform(v3f(1, 0, 0), arrow_scale), world);
    M4F y_world        = m4f_mul(cfdr_arrow_transform(v3f(0, 1, 0), arrow_scale), world);
    M4F z_world        = m4f_mul(cfdr_arrow_transform(v3f(0, 0, 1), arrow_scale), world);

    M4F x_world_b      = cfdr_render_apply_basis_change(x_world);
    M4F y_world_b      = cfdr_render_apply_basis_change(y_world);
    M4F z_world_b      = cfdr_render_apply_basis_change(z_world);

    M4F x_wvp = m4f_mul(cfdr_render_apply_basis_change(x_world), view_projection);
    M4F y_wvp = m4f_mul(cfdr_render_apply_basis_change(y_world), view_projection);
    M4F z_wvp = m4f_mul(cfdr_render_apply_basis_change(z_world), view_projection);

    // NOTE(cmat): Mouse-Picking.
    M4F inv_view_projection                         = {};
    m4f_inv(view_projection, &inv_view_projection);

    V2F viewport_size                               = r2f_size(draw_region);
    V2F NDC                                         = v2f_sub(pl_input()->mouse.position, draw_region.min);
    NDC                                             = v2f_sub(v2f_had(v2f_mul(2, NDC), v2f_rcp(viewport_size)), v2f(1, 1));

    V4F clip_near     = v4f(NDC.x, NDC.y, -1.f, 1.f);
    V4F clip_far      = v4f(NDC.x, NDC.y, +1.f, 1.f);

    V4F near_hom      = m4f_mul_v4f(clip_near, inv_view_projection);
    V4F far_hom       = m4f_mul_v4f(clip_far,  inv_view_projection);

    V3F world_near    = v3f_div(near_hom.xyz, near_hom.w);
    V3F world_far     = v3f_div(far_hom.xyz,  far_hom.w);

    V3F ray_origin    = world_near;
    V3F ray_direction = v3f_noz(v3f_sub(world_far, world_near));
    
    I32 picked_axis = -1;
    if (scene->picker_grabbed_axis < 0) {
      F32 picked_dist = f32_largest_positive;
      {
        F32 cylinder_rad  = 0.10f;
        F32 cylinder_len  = 5.f + 2.f;

        For_U32(it, 3) {
          M4F i_world_b = { };
          switch (it) {
            case 0: { i_world_b = x_world_b; } break;
            case 1: { i_world_b = y_world_b; } break;
            case 2: { i_world_b = z_world_b; } break;
          }

          V3F a = m4f_mul_v4f(v4f(0.f, 0.f, 0.f, 1.f), i_world_b).xyz;
          V3F b = m4f_mul_v4f(v4f(cylinder_len, 0, 0, 1), i_world_b).xyz;
          F32 t = geo3_intersect_ray_cylinder(ray_origin, ray_direction, a, b, cylinder_rad);
          if (t >= 0 && t < picked_dist) {
            picked_axis = it;
            picked_dist = t;
          }
        }
      }

      if (picked_axis >= 0 && pl_input()->mouse.left.press) {
        scene->picker_grabbed_axis = picked_axis;

        V3F center = { }; 
        V4F plane  = { };

        if (scene->picker_grabbed_axis == 0) {
          center = m4f_mul_v4f(v4f(0.f, 0.f, 0.f, 1.f), x_world_b).xyz;
          plane  = v4f(0, 1, 0, center.y);
        } else if (scene->picker_grabbed_axis == 1) {
          center = m4f_mul_v4f(v4f(0.f, 0.f, 0.f, 1.f), y_world_b).xyz;
          plane  = v4f(0, 1, 0, center.y);
        } else {
          center = m4f_mul_v4f(v4f(0.f, 0.f, 0.f, 1.f), z_world_b).xyz;
          plane  = v4f(0, 0, 1, center.z);
        }

        F32 t      = geo3_intersect_ray_plane(ray_origin, ray_direction, plane);

        scene->picker_grabbed_base = v3f_add(ray_origin, v3f_mul(t, ray_direction));
        scene->picker_translate_base = scene->active->translate;

      }
    } else {
      V3F center = { }; 
      V4F plane  = { };

      if (scene->picker_grabbed_axis == 0) {
        center = m4f_mul_v4f(v4f(0.f, 0.f, 0.f, 1.f), x_world_b).xyz;
        plane  = v4f(0, 1, 0, center.y);
      } else if (scene->picker_grabbed_axis == 1) {
        center = m4f_mul_v4f(v4f(0.f, 0.f, 0.f, 1.f), y_world_b).xyz;
        plane  = v4f(0, 1, 0, center.y);
      } else {
        center = m4f_mul_v4f(v4f(0.f, 0.f, 0.f, 1.f), z_world_b).xyz;
        plane  = v4f(0, 0, 1, center.z);
      }

      F32 t       = geo3_intersect_ray_plane(ray_origin, ray_direction, plane);
      V3F move_at = v3f_add(ray_origin, v3f_mul(t, ray_direction));


      I32 ax = scene->picker_grabbed_axis;
      scene->active->translate = scene->picker_translate_base;
      if (ax == 0) {
        scene->active->translate.dat[0] += (v3f_largest(scene_size) / 40.f) * (move_at.dat[0] - scene->picker_grabbed_base.dat[0]);
      } else if (ax == 1) {
        scene->active->translate.dat[1] -= (v3f_largest(scene_size) / 40.f) * (move_at.dat[2] - scene->picker_grabbed_base.dat[2]);
      } else if (ax == 2) {
        scene->active->translate.dat[2] += (v3f_largest(scene_size) / 40.f) * (move_at.dat[1] - scene->picker_grabbed_base.dat[1]);
      }
    }

    U32 draw_order[] = { 0, 1, 2 };
    V4F e1           = m4f_mul_v4f(v4f(1, 0, 0, 1), x_wvp);
    V4F e2           = m4f_mul_v4f(v4f(1, 0, 0, 1), y_wvp);
    V4F e3           = m4f_mul_v4f(v4f(1, 0, 0, 1), z_wvp);

    if (e1.z < e2.z) { Swap(V4F, e1, e2); Swap(U32, draw_order[0], draw_order[1]); }
    if (e1.z < e3.z) { Swap(V4F, e1, e3); Swap(U32, draw_order[0], draw_order[2]); }
    if (e2.z < e3.z) { Swap(V4F, e2, e3); Swap(U32, draw_order[1], draw_order[2]); }

    HSVA Color_Arrow_X = hsva_u32(0,   picked_axis == 0 ? 100 : 60, picked_axis == 0 ? 100 : 80, 255);
    HSVA Color_Arrow_Y = hsva_u32(120, picked_axis == 1 ? 100 : 60, picked_axis == 1 ? 100 : 80, 255);
    HSVA Color_Arrow_Z = hsva_u32(240, picked_axis == 2 ? 100 : 60, picked_axis == 2 ? 100 : 80, 255);

    if (scene->picker_grabbed_axis >= 0) {
      Color_Arrow_X.a = 0.15f;
      Color_Arrow_Y.a = 0.15f;
      Color_Arrow_Z.a = 0.15f;
    }

    For_U32(it, 3) {
      U32 idx = draw_order[it];
      switch (idx) {
        case 0: { cfdr_render_arrow_draw(render, &arrow_xyz[it], x_world, view_projection, eye_position, v4f_had(color_tint, Color_Arrow_X), draw_region); } break;
        case 1: { cfdr_render_arrow_draw(render, &arrow_xyz[it], y_world, view_projection, eye_position, v4f_had(color_tint, Color_Arrow_Y), draw_region); } break;
        case 2: { cfdr_render_arrow_draw(render, &arrow_xyz[it], z_world, view_projection, eye_position, v4f_had(color_tint, Color_Arrow_Z), draw_region); } break;
      }
    }
  }
}

fn_internal void cfdr_scene_draw(CFDR_Render *render, CFDR_CMap_Table *cmap_table, UI_Response *response, CFDR_Scene *scene, R2F draw_region) {
  if (!cfdr_cmap_table_get(cmap_table, scene->cmap)) {
    // TODO(cmat): Better way to fallback to default.
    scene->cmap = str_lit("Rainbow");
  }

  if (!pl_input()->mouse.left.down) {
    scene->picker_grabbed_axis = -1;
  }

  if (response->drag) {
    B32 move_position = pl_input()->keyboard.state[PL_KB_Shift_Left].down;

    if (scene->picker_grabbed_axis < 0) {
      cfdr_camera_control(&scene->view.camera, draw_region, move_position, pl_input()->mouse.position_dt);
    }
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
  scene->transform    = scene_transform;
  scene->view_projection = view_projection;

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

      scene->step.var_count  = it->volume.var_count;
      scene->step.var_names  = it->volume.var_array;
    }

    cfdr_scene_draw_volume  (render, cmap_table, scene->cmap, &scene->step, it, eye_position, view_projection, scene_transform, draw_region);
  }

  F32 picker_size_px = 50.0f * js_web_device_pixel_ratio();
  cfdr_scene_draw_picker_arrow(scene->picker_arrow_xyz_outline, render, scene, scene_transform, view_projection, eye_position, scene_size, draw_region, v4f(0, 0, 0, 1), picker_size_px);
  cfdr_scene_draw_picker_arrow(scene->picker_arrow_xyz,         render, scene, scene_transform, view_projection, eye_position, scene_size, draw_region, v4f(1, 1, 1, 1), picker_size_px);

  F32 xyz_size = 175.f * js_web_device_pixel_ratio();
  R2F xyz_draw_region = r2f(draw_region.x1 - xyz_size - 5, draw_region.y1 - xyz_size - 5, draw_region.x1 - 5, draw_region.y1 - 5);
  cfdr_scene_draw_widget_arrow(render, scene, xyz_draw_region);
}
