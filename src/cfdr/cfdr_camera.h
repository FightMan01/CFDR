typedef struct CFDR_Camera {
  // NOTE(cmat): Parameters.
  V3F look_at;
  F32 radius_m;
  F32 theta_deg;
  F32 phi_deg;
  F32 near_m;
  F32 far_m;
  F32 fov_deg;
  B32 orthographic;

  // NOTE(cmat): Animation values.
  F32 radius_m_t;
  F32 theta_deg_t;
  F32 phi_deg_t;
  F32 orthographic_t;

  // NOTE(cmat): Computed values.
  F32 computed_aspect_ratio;
  V3F computed_position_m;
} CFDR_Camera;

fn_internal void cfdr_camera_init(CFDR_Camera *camera) {
  zero_fill(camera);

  camera->look_at        = v3f(0, 0, 0);
  camera->radius_m       = 40.f;
  camera->theta_deg      = 45.f;
  camera->phi_deg        = 45.f;
  camera->near_m         = 0.1f;
  camera->far_m          = 1000.f;
  camera->fov_deg        = 60.f;
  camera->orthographic   = 0;

  camera->radius_m_t     = camera->radius_m;
  camera->theta_deg_t    = camera->theta_deg;
  camera->phi_deg_t      = camera->phi_deg;
  camera->orthographic_t = camera->orthographic;

  camera->computed_aspect_ratio = 0;
  camera->computed_position_m   = v3f(0, 0, 0);
}

fn_internal void cfdr_camera_update(CFDR_Camera *camera, R2F draw_region) {
  F32 frame_delta               = pl_display()->frame_delta;

  camera->radius_m              = f32_clamp(camera->radius_m, 3.f, 50.f);
  camera->phi_deg               = f32_clamp(camera->phi_deg,  0.01f, 179.99f);

#if 1
  camera->radius_m_t            = f32_exp_smoothing(camera->radius_m_t,     camera->radius_m,     frame_delta * 15.f);
  camera->theta_deg_t           = f32_exp_smoothing(camera->theta_deg_t,    camera->theta_deg,    frame_delta * 15.f);
  camera->phi_deg_t             = f32_exp_smoothing(camera->phi_deg_t,      camera->phi_deg,      frame_delta * 15.f);
  camera->orthographic_t        = f32_exp_smoothing(camera->orthographic_t, camera->orthographic, frame_delta * 15.f);
#else
  camera->radius_m_t            = camera->raidus_m;
  camera->theta_deg_t           = camera->theta_deg;
  camera->phi_deg_t             = camera->phi_deg;
  camera->orthographic_t        = camera->orthographic;
#endif

  F32 theta_rad                 = f32_radians_from_degrees(camera->theta_deg_t);
  F32 phi_rad                   = f32_radians_from_degrees(camera->phi_deg_t);

  camera->computed_aspect_ratio = f32_div_safe(draw_region.x1 - draw_region.x0, draw_region.y1 - draw_region.y0);
  camera->computed_position_m   = v3f(f32_cos(theta_rad) * f32_sin(phi_rad), f32_cos(phi_rad), f32_sin(theta_rad) * f32_sin(phi_rad));
  camera->computed_position_m   = v3f_mul(camera->radius_m_t, camera->computed_position_m);
  camera->computed_position_m   = v3f_add(camera->computed_position_m, camera->look_at);
}

fn_internal M4F cfdr_camera_view(CFDR_Camera *camera) {
  M4F view = m4f_hom_look_at(v3f(0, 1, 0), camera->computed_position_m, camera->look_at);
  return view;
}

fn_internal F32 cfdr_camera_world_units_per_pixel(CFDR_Camera *camera, R2F draw_region) {
  F32 fov_rad = f32_radians_from_degrees(camera->fov_deg);
  F32 h = 2.f * camera->radius_m_t * f32_tan(.5f * fov_rad);
  F32 result = f32_div_safe(h, draw_region.max.y - draw_region.min.y);
  return result;
}

fn_internal M4F cfdr_camera_projection(CFDR_Camera *camera) {
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

fn_internal M4F cfdr_camera_transform(CFDR_Camera *camera) {
  M4F view        = cfdr_camera_view(camera);
  M4F projection  = cfdr_camera_projection(camera);
  M4F transform   = m4f_mul(view, projection);
  return transform;
}

fn_internal void cfdr_camera_control(CFDR_Camera *camera, R2F draw_region, B32 move_position, V2F mouse_dt) {
  if (move_position) {
    V3F z_axis = v3f_noz(v3f_sub(camera->computed_position_m, camera->look_at));
    V3F x_axis = v3f_noz(v3f_cross(v3f(0, 1, 0), z_axis));
    V3F y_axis = v3f_noz(v3f_cross(z_axis, x_axis));

    F32 units_per_pixel = cfdr_camera_world_units_per_pixel(camera, draw_region);
    F32 scale = units_per_pixel;
    camera->look_at = v3f_add(camera->look_at, v3f_mul(-scale * pl_input()->mouse.position_dt.x, x_axis));
    camera->look_at = v3f_add(camera->look_at, v3f_mul(-scale * pl_input()->mouse.position_dt.y, y_axis));
  } else {
    camera->theta_deg += 10.f * pl_display()->frame_delta * mouse_dt.x;
    camera->phi_deg   += 10.f * pl_display()->frame_delta * mouse_dt.y;
  }
}
