typedef U32 CFDR_Object_Flag;
enum {
  CFDR_Object_Flag_None         = 0,
  CFDR_Object_Flag_Draw_Surface = 1 << 0,
  CFDR_Object_Flag_Draw_Volume  = 1 << 1,
};

typedef struct CFDR_Object_Node {
  struct CFDR_Object_Node  *next;
  CFDR_Object_Flag          flags;
  B32                       visible;
  Str                       tag;
  HSVA                      color;

  V3F                       scale;
  V3F                       translate;
  F32                       volume_density;

  CFDR_Resource_Surface     surface;
  CFDR_Resource_Volume      volume;

  R_Buffer                  world_state;
  R_Bind_Group              bind_group;
} CFDR_Object_Node;

typedef struct CFDR_Scene_View {
  HSVA              background;
  CFDR_Camera       camera;
  CFDR_Render_Grid  grid;
} CFDR_Scene_View;

typedef struct CFDR_Scene {
  Arena              arena;
  U32                count;
  CFDR_Object_Node  *active;
  CFDR_Object_Node  *first;
  CFDR_Object_Node  *last;
  CFDR_Scene_View    view;
} CFDR_Scene;

fn_internal void              cfdr_scene_init(CFDR_Scene *object);
fn_internal CFDR_Object_Node *cfdr_scene_push(CFDR_Scene *object);
fn_internal void              cfdr_scene_draw(CFDR_Render *render, UI_Response *response, CFDR_Scene *object, R2F draw_region);

