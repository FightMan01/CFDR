fn_internal void cfdr_state_init(CFDR_State *state) {
  arena_init(&state->arena);

  state->version  = 0;
  // state->project  = str_lit("");
  state->project = (Str) { .len = 0 };

  state->workspace = (CFDR_Workspace) {
    .light_mode = 0,
  };


  cfdr_render_init(&state->render);
  cfdr_cmap_table_init(&state->cmap_table);
  cfdr_overlay_init(&state->overlay);
  cfdr_scene_init(&state->scene);
}
