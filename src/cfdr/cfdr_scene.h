typedef U32 CFDR_Object_Flag;
enum {
  CFDR_Object_Flag_None         = 0,
  CFDR_Object_Flag_Draw_Surface = 0,
};

typedef struct CFDR_Object_Node {
  struct CFDR_Object_Node  *next;
  CFDR_Object_Flag          flags;
  B32                       visible;
  Str                       tag;
  
  HSVA                      color;
  CFDR_Surface              surface;
} CFDR_Object_Node;

typedef struct CFDR_Scene {
  Arena              arena;
  U32                count;
  CFDR_Object_Node  *active;
  CFDR_Object_Node  *first;
  CFDR_Object_Node  *last;
} CFDR_Scene;

fn_internal void              cfdr_scene_init(CFDR_Scene *object);
fn_internal CFDR_Object_Node *cfdr_scene_push(CFDR_Scene *object);
fn_internal void              cfdr_scene_draw(CFDR_Scene *object, R2F draw_region);

