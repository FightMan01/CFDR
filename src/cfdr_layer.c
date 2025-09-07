// $File: cfdr_layer.c
// $Last-Modified: "2025-08-07 15:46:38"
// $Author: Matyas Constans.
// $Notice: (C) Matyas Constans, Horvath Zoltan 2025 - All Rights Reserved.
// $License: You may use, distribute and modify this code under the terms of the MIT license.
// $Note: Layer updates, rendering.

F32 cfdr_layer_scale_px(CFDR_Layer_Scale *scale, R2 parent_bounds) {
  F32 px = 0;
  switch (scale->metric) {
    case CFDR_Layer_Scale_Metric_Display_Ratio: { px = (parent_bounds.max.y - parent_bounds.min.y) * scale->value / 100.f; } break;
    Invalid_Default;
  }

  return px;
}

V2 cfdr_layer_size(CFDR_Layer *layer, R2 parent_bounds) {
  F32 scale_px = cfdr_layer_scale_px(&layer->scale, parent_bounds);
  V2 size_px   = v2(scale_px, scale_px);

  switch (layer->type) {
    case CFDR_Layer_Type_Text: {
      size_px.y = scale_px;
      size_px.x = gfx_font_text_width(layer->layer_text->font, layer->layer_text->string, scale_px);
    } break;

    case CFDR_Layer_Type_Image: {
      F32 aspect_ratio = (F32)layer->layer_image->width / (F32)layer->layer_image->height;
      size_px = v2_mul(scale_px, v2(aspect_ratio, 1.0f));
    } break;

    case CFDR_Layer_Type_Graph: {
      size_px = v2_mul(scale_px, v2(layer->layer_graph->aspect_ratio, 1.0f));
    } break;
  }

  return size_px;
}

Rect2 cfdr_layer_bounds(CFDR_Layer *layer, R2 parent_bounds) {
  V2  size         = cfdr_layer_size(layer, parent_bounds);
  V2  display_size = v2(parent_bounds.max.x - parent_bounds.min.x, parent_bounds.max.y - parent_bounds.min.y); 
  F32 margin_x_px  = cfdr_layer_scale_px(&layer->margin_x, parent_bounds);
  F32 margin_y_px  = cfdr_layer_scale_px(&layer->margin_y, parent_bounds);
  
  V2 origin = v2(0, 0);
  switch (layer->layout_x) {
    case CFDR_Layer_Layout_Left:   { origin.x = margin_x_px;                               } break;
    case CFDR_Layer_Layout_Right:  { origin.x = display_size.x - margin_x_px - size.x;     } break; 
    case CFDR_Layer_Layout_Center: { origin.x = (display_size.x - size.x) / 2.0f;          } break;
    Invalid_Default;
  }
  
  switch (layer->layout_y) {
    case CFDR_Layer_Layout_Bottom: { origin.y = margin_y_px;                               } break;
    case CFDR_Layer_Layout_Top:    { origin.y = display_size.y - margin_y_px - size.y;     } break; 
    case CFDR_Layer_Layout_Center: { origin.y = (display_size.y - size.y) / 2.0f;          } break;
    Invalid_Default;
  }

  return (Rect2) { .min = origin, .max = v2_add(origin, size) };
}

void cfdr_layer_text_render(CFDR_Layer *layer, Rect2 bounds) {
  F32 shadow_offset = Max(1, .03f * (bounds.max.y - bounds.min.y));

  GFX_IM_2D_Push_Text(layer->layer_text->string,
                      layer->layer_text->font,
                      v2_add(bounds.min, v2(shadow_offset, -shadow_offset)),
                      (bounds.max.y - bounds.min.y),
                      .color = v4(0, 0, 0, .5f));
  
  GFX_IM_2D_Push_Text(layer->layer_text->string,
                      layer->layer_text->font,
                      bounds.min,
                      (bounds.max.y - bounds.min.y),
                      .color = layer->color);

  if (layer->layer_text->underline) {
    F32 line_thickness = Max(2.0f, .04f * (bounds.max.y - bounds.min.y));
    GFX_IM_2D_Push_Rect(v2_sub(bounds.min, v2(0, .1f * (bounds.max.y - bounds.min.y))), v2(bounds.max.x - bounds.min.x, line_thickness), .color = layer->color);
  }
}

void cfdr_layer_image_render(CFDR_Layer *layer, Rect2 bounds) {
  F32 shadow_offset = Max(1, .03f * (bounds.max.y - bounds.min.y));
  GFX_IM_2D_Push_Rect(v2_add(bounds.min, v2(shadow_offset, -shadow_offset)), v2_sub(bounds.max, bounds.min), .color = v4(0, 0, 0, .5f), .texture = layer->layer_image->texture);
  GFX_IM_2D_Push_Rect(bounds.min, v2_sub(bounds.max, bounds.min), .color = layer->color, .texture = layer->layer_image->texture);
}

void cfdr_layer_graph_render(CFDR_Layer *layer, Rect2 bounds) {
  F32 axis_label_height   = Max(2.0f, .05f * (bounds.max.y - bounds.min.y));
  F32 axis_label_margin_x = .5f * axis_label_height;
  F32 axis_label_margin_y = .1f * axis_label_height;
  
  F32 axis_thickness = Max(2.0f, .0025f * (bounds.max.y - bounds.min.y));
  F32 axis_width  = bounds.max.x - bounds.min.x - 2 * (axis_label_height + axis_label_margin_x);
  F32 axis_height = bounds.max.y - bounds.min.y - 2 * (axis_label_height + axis_label_margin_y);
  V2  axis_origin = v2_add(bounds.min, v2(axis_label_height + axis_label_margin_x, axis_label_height + axis_label_margin_y));
  
  GFX_IM_2D_Push_Rect(axis_origin, v2(axis_width, axis_thickness),  .color = layer->color);
  GFX_IM_2D_Push_Rect(axis_origin, v2(axis_thickness, axis_height), .color = layer->color);

  F32 label_x_width = gfx_font_text_width(layer->layer_graph->label_font, layer->layer_graph->x_axis_label, axis_label_height);
  F32 label_y_width = gfx_font_text_width(layer->layer_graph->label_font, Str08_Literal("Scaling Efficiency"), axis_label_height);
 
  V2 label_x_origin = v2(axis_origin.x + .5f * (axis_width - label_x_width), bounds.min.y);
  GFX_IM_2D_Push_Text(layer->layer_graph->x_axis_label,
                      layer->layer_graph->label_font,
                      label_x_origin,
                      axis_label_height,
                      .color = layer->color);

  V2 label_y_origin = v2(axis_origin.x - axis_label_margin_x, axis_origin.y + .5f * (axis_height - label_y_width));
  GFX_IM_2D_Push_Text(Str08_Literal("Scaling Efficiency"),
                      layer->layer_graph->label_font,
                      label_y_origin,
                      axis_label_height,
                      .color = layer->color,
                      .angle_deg = 90);
  
  Rect2 graph_bounds = { .min = axis_origin, .max = v2_add(axis_origin, v2(axis_width, axis_height)) };
}

void cfdr_layer_render(CFDR_Layer *layer, R2 parent_bounds) {
  Rect2 bounds = cfdr_layer_bounds(layer, parent_bounds);
  
  switch (layer->type) {
    case CFDR_Layer_Type_Text:  { cfdr_layer_text_render  (layer, bounds); } break;
    case CFDR_Layer_Type_Image: { cfdr_layer_image_render (layer, bounds); } break;
    case CFDR_Layer_Type_Graph: { cfdr_layer_graph_render (layer, bounds); } break;
    Invalid_Default;
  }
}
