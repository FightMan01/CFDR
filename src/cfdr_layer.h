// $File: cfdr_layer.h
// $Last-Modified: "2025-08-07 15:40:32"
// $Author: Matyas Constans.
// $Notice: (C) Matyas Constans, Horvath Zoltan 2025 - All Rights Reserved.
// $License: You may use, distribute and modify this code under the terms of the MIT license.
// $Note: Layer representation.

typedef U08 CFDR_Layer_Type;
enum {
  CFDR_Layer_Type_Text,
  CFDR_Layer_Type_Image,
  CFDR_Layer_Type_Graph,
};

typedef U08 CFDR_Layer_Layout;
enum {
  CFDR_Layer_Layout_Left   = 0,
  CFDR_Layer_Layout_Bottom = 0,
  
  CFDR_Layer_Layout_Right  = 1,
  CFDR_Layer_Layout_Top    = 1,
  
  CFDR_Layer_Layout_Center = 2,
};

typedef U16 CFDR_Layer_Scale_Metric;
enum {
  CFDR_Layer_Scale_Metric_Display_Ratio,
};

typedef struct {
  CFDR_Layer_Scale_Metric metric;
  F32 value;
} CFDR_Layer_Scale;

typedef struct {
  GFX_Font *font;
  Str08 string;
  B32 underline;
} CFDR_Layer_Text;

typedef struct {
  GPU_Texture texture;
  U32         width;
  U32         height;
} CFDR_Layer_Image;

typedef struct {
  GFX_Font *title_font;
  GFX_Font *label_font;
  
  Str08 title;
  Str08 x_axis_label;
  F32   aspect_ratio;
} CFDR_Layer_Graph;

typedef struct {
  CFDR_Layer_Type   type;
  CFDR_Layer_Layout layout_x;
  CFDR_Layer_Layout layout_y;
  CFDR_Layer_Scale  margin_x;
  CFDR_Layer_Scale  margin_y;
  CFDR_Layer_Scale  scale;
  
  B32 visible;
  V4  color;
  
  union {
    CFDR_Layer_Text  *layer_text;
    CFDR_Layer_Image *layer_image;
    CFDR_Layer_Graph *layer_graph;
  };
  
} CFDR_Layer;

F32   cfdr_layer_scale_px (CFDR_Layer_Scale *scale, R2 parent_bounds);
V2    cfdr_layer_size_px  (CFDR_Layer *layer,       R2 parent_bounds);
Rect2 cfdr_layer_bounds   (CFDR_Layer *layer,       R2 parent_bounds);
void  cfdr_layer_render   (CFDR_Layer *layer,       R2 parent_bounds);
