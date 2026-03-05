typedef U32 CFDR_Layer_Type;
enum {
  CFDR_Layer_Type_Text,
  CFDR_Layer_Type_Image,
  CFDR_Layer_Type_Surface,
  CFDR_Layer_Type_Volume,
  CFDR_Layer_Type_Annotation,

  CFDR_Layer_Type_Count
};

typedef I32 CFDR_Material_Type;
enum {
  CFDR_Material_Flat,
  CFDR_Material_Position,
};

typedef struct CFDR_Layer {
  CFDR_Layer_Type type;
  Str             label;
  B32             visible;
  HSVA            color;

  Str             text;
  Align2          alignment_x;
  Align2          alignment_y;
  F32             border_x;
  F32             border_y;

  B32             background_overlay;
  B32             shadow_overlay;
  F32             shadow_overlay_opacity;
  F32             shadow_overlay_offset;
 
  B32             wireframe;
  V3F             scale;

  CFDR_Surface    surface;
  CFDR_Volume     volume;
  F32             volume_density;

  CFDR_Material_Type material_type;

} CFDR_Layer;

typedef Array_Type(CFDR_Layer) CFDR_Array_Layer;

var_global struct {
  Arena             arena;
  CFDR_Array_Layer  layer_array;
  U32               layer_active;
} CFDR_Res_State = { };

fn_internal void cfdr_state_init() {
  zero_fill     (&CFDR_Res_State);
  arena_init    (&CFDR_Res_State.arena);
  array_reserve (&CFDR_Res_State.arena, &CFDR_Res_State.layer_array, 2048);

  CFDR_Layer layers[] = {
    { .type = CFDR_Layer_Type_Surface,  .label = str_lit("Terrain"),    .visible = 1, .color = v4f(.4f, .4f, .4f, 1.f) },
    { .type = CFDR_Layer_Type_Surface,  .label = str_lit("Buildings"),  .visible = 1, .color = v4f(.8f, .8f, .8f, 1.f) },
    { .type = CFDR_Layer_Type_Surface,  .label = str_lit("Slice"),      .visible = 0, .color = v4f(.8f, .8f, .8f, 1.f) },
    { .type = CFDR_Layer_Type_Volume,   .label = str_lit("Simulation"), .visible = 0, .color = v4f(1.f, 1.f, 1.f, 1.f) },
    { .type = CFDR_Layer_Type_Text,     .label = str_lit("Title"),      .visible = 1, .text = str_lit("CFDR-UAP - BETA v0.1"), .shadow_overlay = 1, .alignment_x = Align2_Left, .alignment_y = Align2_Top, .color = v4f(1.f, 0.f, 1.f, 1.f) },

    // { .type = CFDR_Layer_Type_Image,        .label = str_lit("HidALGO-2"),      .visible = 1 },
    // { .type = CFDR_Layer_Type_Annotation,   .label = str_lit("Notes"),          .visible = 0 },
    // { .type = CFDR_Layer_Type_Volume,       .label = str_lit("LBM Simulation"), .visible = 1 },
  };

  cfdr_surface_init (&layers[0].surface,  str_lit("terrain.stl"));
  cfdr_surface_init (&layers[1].surface,  str_lit("buildings.stl"));
  cfdr_surface_init (&layers[2].surface,  str_lit("uap/roi_surface.stl"));
  cfdr_volume_init  (&layers[3].volume,   str_lit("uap/velocity_32.vol32"));

  For_U32(it, sarray_len(layers)) {
    layers[it].border_x       = 5;
    layers[it].border_y       = 5;
    layers[it].shadow_overlay = 1;
    layers[it].shadow_overlay_opacity = .9f;
    layers[it].shadow_overlay_offset   = 4.f;
    layers[it].volume_density = 1.0;
    array_push(&CFDR_Res_State.layer_array, layers[it]);
  }
}
