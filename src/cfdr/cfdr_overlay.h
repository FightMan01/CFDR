typedef U32 CFDR_Overlay_Flag;
enum {
  CFDR_Overlay_Flag_None        = 0,
  CFDR_Overlay_Flag_Background  = 1 << 0,
  CFDR_Overlay_Flag_Shadow      = 1 << 1,
};

typedef struct CFDR_Overlay_Node {
  struct CFDR_Overlay_Node *next;
  CFDR_Overlay_Flag         flags;
  B32                       visible;
  Str                       tag;
  Str                       content;
  F32                       scale;
  HSVA                      color;
  HSVA                      color_shadow;
  Align2                    position_x;
  Align2                    position_y;
  V2F                       border;
  F32                       shadow_offset;

  
  // TODO(cmat): Just cache fonts instead, don't want an arena per overlay.
  Arena                     font_arena;
  B32                       font_last_scale;
  G2_Font                   font;
} CFDR_Overlay_Node;

typedef struct CFDR_Overlay {
  Arena               arena;
  U32                 count;
  CFDR_Overlay_Node  *active;
  CFDR_Overlay_Node  *first;
  CFDR_Overlay_Node  *last;
  G2_Font             font;
} CFDR_Overlay;

fn_internal void                cfdr_overlay_init(CFDR_Overlay *overlay);
fn_internal CFDR_Overlay_Node  *cfdr_overlay_push(CFDR_Overlay *overlay);
fn_internal void                cfdr_overlay_draw(CFDR_Overlay *overlay, CFDR_Scene *scene, R2F draw_region);
