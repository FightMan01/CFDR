#define CFDR_Version 100

typedef struct CFDR_Workspace {
  B32 light_mode;
} CFDR_Workspace;

typedef struct CFDR_Viewport {
  HSVA    background;
  B32     orthographic;
  B32     grid_enabled;
  HSVA    grid_color;
  F32     grid_level;
  B32     view_2D;
} CFDR_Viewport;

typedef struct CFDR_State {
  Arena             arena;
  U32               version;
  Str               project;

  CFDR_Workspace    workspace;
  CFDR_Viewport     viewport;
  CFDR_Overlay      overlay;
  CFDR_Scene        scene;

} CFDR_State;

fn_internal void cfdr_state_default(CFDR_State *state);
