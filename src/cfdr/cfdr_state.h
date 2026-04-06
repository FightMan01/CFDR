#define CFDR_Version 100

typedef struct CFDR_Workspace {
  B32 light_mode;
} CFDR_Workspace;

typedef struct CFDR_State {
  Arena             arena;
  U32               version;
  Str               project;

  CFDR_Workspace    workspace;
  CFDR_Render       render;
  CFDR_CMap_Table   cmap_table;
  CFDR_Overlay      overlay;
  CFDR_Scene        scene;
} CFDR_State;

fn_internal void cfdr_state_init(CFDR_State *state);
