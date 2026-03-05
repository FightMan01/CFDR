fn_internal void cfdr_state_default(CFDR_State *state) {
  arena_init(&state->arena);

  state->version  = 0;
  // state->project  = str_lit("");
  state->project = (Str) { .len = 0 };

  state->workspace = (CFDR_Workspace) {
    .light_mode = 0,
  };

  state->viewport = (CFDR_Viewport) {
    .orthographic = 1,
    .background   = hsva_u32(10, 10, 20, 255),
    .grid_enabled = 1,
    .grid_color   = hsva_u32(10, 10, 20, 255),
    .grid_level   = 1.f,
    .view_2D      = 0,
  };

  cfdr_overlay_init(&state->overlay);
}
