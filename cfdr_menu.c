// $File: cfdr_menu.c
// $Last-Modified: "2025-08-14 12:01:16"
// $Author: Matyas Constans.
// $Notice: (C) Matyas Constans, Horvath Zoltan 2025 - All Rights Reserved.
:q
// $License: You may use, distribute and modify this code under the terms of the MIT license.
// $Note: CFDR menu bar rendering and update logic.

void cfdr_menu_init(CFDR_Menu *menu, GFX_Font *font) {
  menu->font = font;
}

typedef struct {
  GFX_Font *font;
  U32 active_idx;
  B32 expanded;
} CFDR_Dropdown;

F32 cfdr_dropdown(CFDR_Dropdown *dropdown, V2 at, F32 margin, F32 height, Str08 tag, Str08 *label_list, u32 label_count, u32 *selected_idx) {
  // dropdown->active_idx = *selected_idx;
  
  F32 font_height = .75f * height;
  F32 font_at_y = at.y + .5f * (height - font_height);
  V2 shadow_offset = v2(margin/3, -margin/3);
  
  Str08 active_label = label_list[dropdown->active_idx];

  F32 tag_width = gfx_font_text_width(dropdown->font, tag, font_height);
  GFX_IM_2D_Push_Text(tag, dropdown->font, v2(at.x + shadow_offset.x, font_at_y + shadow_offset.y), font_height, .color = v4(0, 0, 0, .9f));
  GFX_IM_2D_Push_Text(tag, dropdown->font, v2(at.x, font_at_y), font_height);
  at.x += tag_width + margin;
  
  F32 label_width = 0;
  Iter_U32(it, 0, label_count) {
    label_width = Max(label_width, gfx_font_text_width(dropdown->font, label_list[it], font_height));
  }
  
  F32 dropdown_triangle_size  = .5f * height;
  F32 button_width = margin + label_width + margin + dropdown_triangle_size + margin;

  V2 button_region_min = at;
  V2 button_region_max = v2_add(at, v2(button_width, height));

  V4 button_color = v4(.15f, .15f, .15f, 1.f);
  V2 mouse = platform_input()->mouse.position;
  if (mouse.x >= button_region_min.x &&
      mouse.y >= button_region_min.y &&
      mouse.x <= button_region_max.x &&
      mouse.y <= button_region_max.y) {

    button_color = v4(.20f, .20f, .20f, 1.f);

    if (platform_input()->mouse.left.down) {
      button_color = v4(94.f / 255.f, 103.f / 255.f, 153.f / 255.f, 1.0f);
    }

    if (platform_input()->mouse.left.down_first) {
      dropdown->expanded = 1;
    }
  }

  GFX_IM_2D_Push_Rect(at, v2(button_width, height), .color = button_color);
  GFX_IM_2D_Push_Text(active_label, dropdown->font, v2(margin + at.x + shadow_offset.x, font_at_y + shadow_offset.y), font_height, .color = v4(0, 0, 0, .9f));
  GFX_IM_2D_Push_Text(active_label, dropdown->font, v2(margin + at.x, font_at_y), font_height);

  V2 dropdown_triangle_at = v2(at.x + margin + label_width + margin, at.y + .25f * (height - dropdown_triangle_size));

  V2 tri_X1 = v2(dropdown_triangle_at.x,                                dropdown_triangle_at.y + 0.8660f * dropdown_triangle_size);
  V2 tri_X2 = v2(dropdown_triangle_at.x + dropdown_triangle_size,       dropdown_triangle_at.y + 0.8660f * dropdown_triangle_size);
  V2 tri_X3 = v2(dropdown_triangle_at.x + .5f * dropdown_triangle_size, dropdown_triangle_at.y);
  
  GFX_IM_2D_Push_Triangle(v2_add(tri_X1, shadow_offset), v2_add(tri_X2, shadow_offset), v2_add(tri_X3, shadow_offset), .color = v4(0, 0, 0, .9f));
  GFX_IM_2D_Push_Triangle(tri_X1, tri_X2, tri_X3);

  if (dropdown->expanded) {
    Iter_U32(it, 0, label_count) {
      at.y      -= height + margin;
      font_at_y -= height + margin;

      V2 button_region_min = at;
      V2 button_region_max = v2_add(at, v2(button_width, height));
    
      V4 button_color = v4(.15f, .15f, .15f, 1.f);
      if (it == dropdown->active_idx) {
        button_color = v4(94.f / 255.f, 103.f / 255.f, 153.f / 255.f, 1.0f);
      } else {
        button_color = v4(.15f, .15f, .15f, 1.f);

        if (mouse.x >= button_region_min.x &&
            mouse.y >= button_region_min.y &&
            mouse.x <= button_region_max.x &&
            mouse.y <= button_region_max.y) {

          button_color = v4(.20f, .20f, .20f, 1.f);

          if (platform_input()->mouse.left.down_first) {
            dropdown->expanded = 0;
            dropdown->active_idx = it;
          }
        }
      }
    
      GFX_IM_2D_Push_Rect(at, v2(button_width, height), .color = button_color);
      GFX_IM_2D_Push_Text(label_list[it], dropdown->font, v2(margin + at.x + shadow_offset.x, font_at_y + shadow_offset.y), font_height, .color = v4(0, 0, 0, .9f));
      GFX_IM_2D_Push_Text(label_list[it], dropdown->font, v2(margin + at.x, font_at_y), font_height);
    }
  }

  *selected_idx = dropdown->active_idx;
  return at.x + button_width + margin;
}

F32 cfdr_checkbox(GFX_Font *font, B32 *state, Str08 tag, V2 at, F32 height, F32 margin) {

  F32 font_height = .75f * height;
  F32 font_at_y = at.y + .5f * (height - font_height); 

  V2 shadow_offset = v2(margin/3, -margin/3);
  F32 tag_width = gfx_font_text_width(font, tag, font_height);

  GFX_IM_2D_Push_Text(tag, font, v2(at.x + shadow_offset.x, font_at_y + shadow_offset.y), font_height, .color = v4(0, 0, 0, .9f));
  GFX_IM_2D_Push_Text(tag, font, v2(at.x, font_at_y), font_height);

  at.x += tag_width + margin;
 
  F32 size = height - 2.f * margin;
  
  V2 box_min = at;
  V2 box_max = v2_add(at, v2(size, size));

  V4 button_color =  v4(.15f, .15f, .15f, 1);
  V2 mouse = platform_input()->mouse.position;
  if (mouse.x >= box_min.x && mouse.y >= box_min.y && mouse.x <= box_max.x && mouse.y <= box_max.y) {
    // NOTE(cmat): Hovering.
    button_color = v4(.175f, .175f, .175f, 1);
    if (platform_input()->mouse.left.down) {
      // NOTE(cmat): Pressing.
      button_color = v4(.05f, .05f, .05f, 1);
    }

    if (platform_input()->mouse.left.down_first) *state = !*state;
  }
  
  GFX_IM_2D_Push_Rect(at, v2(size, size), .color = button_color);
  if (*state) {
    GFX_IM_2D_Push_Rect(v2_add(at, v2(margin, margin)), v2(size - 2.f * margin, size - 2.f * margin), .color = v4(.3f, .8f, .3f, 1));
  }

  at.x += size + margin;
  return at.x;
}

Global_Variable U32   Variable_Name_Count = 0;
Global_Variable Str08 Variable_Name_Array[100] = { };
Global_Variable U32   Variable_Dim_Array[100] = { };

Global_Variable U32   Surface_Count = 4;
Global_Variable Str08 Surface_Label_Array[4] = {
  Str08_Literal("Shell"),
  Str08_Literal("Slice X"),
  Str08_Literal("Slice Y"),
  Str08_Literal("Slice Z"),
};

Global_Variable CFDR_Dropdown Surface_Dropdown_Var_Array[4] = { };
Global_Variable U32   Surface_Var_IDX_Array[4] = { 0, 0, 0, 0 };
Global_Variable B32   Surface_Visible_Array[4] = { 0, 1, 1, 1 };

void cfdr_menu_render(CFDR_Menu *menu, R2 menu_region) {
  GFX_IM_2D_Push_Rect(menu_region.min, v2_sub(menu_region.max, menu_region.min), .color = v4(.25f, .25f, .25f, 1.f));

  F32 button_height = .75f * (menu_region.max.y - menu_region.min.y);
  F32 margin        = .5f * ((menu_region.max.y - menu_region.min.y) - button_height);
  F32 button_at_y   = menu_region.min.y + margin;

  Local_Persist CFDR_Dropdown surface_dropdown = {};
  if (!surface_dropdown.font) { surface_dropdown.font = menu->font; }
 
  U32 surface_index = 0;
  V2 at = v2(margin, button_at_y);
  at.x = cfdr_dropdown(&surface_dropdown, at, margin, button_height, Str08_Literal("Object"), Surface_Label_Array, Surface_Count, &surface_index);
  
  if (!Surface_Dropdown_Var_Array[surface_index].font) { Surface_Dropdown_Var_Array[surface_index].font = menu->font; }
  at.x = cfdr_dropdown(Surface_Dropdown_Var_Array + surface_index, at, margin, button_height, Str08_Literal("Variable"), Variable_Name_Array, Variable_Name_Count, &Surface_Var_IDX_Array[surface_index]);
  at.x = cfdr_checkbox(menu->font, Surface_Visible_Array + surface_index, Str08_Literal("Dynamic Map"), at, button_height, margin); 
}

Global_Variable U32 Timestamp_At    = 0;
Global_Variable U32 Timestamp_Count = 0;

void cfdr_timeline_render(GFX_Font *font, F32 *time, V2 time_range, R2 region) {

#if 0
  
  GFX_IM_2D_Push_Rect(region.min, v2_sub(region.max, region.min), .color = v4(.075f, .075f, .075f, 1.f));

  F32 content_height = .75f * (region.max.y - region.min.y);
  F32 margin         = .5f * ((region.max.y - region.min.y) - content_height);
  F32 font_height    = .60f * content_height;

  F32 slot_width = (region.max.x - region.min.x - ((F32)Timestamp_Count + 1) * margin) / (F32)Timestamp_Count;

  F32 at_x = region.min.x + margin;
  Iter_U32(it, 0, Timestamp_Count) {

    R2 button_region = { };
    button_region.min = v2(at_x, region.min.y + margin);
    button_region.max = v2_add(button_region.min, v2(slot_width, content_height));
    
    V4 button_color = v4(.15f, .15f, 0.15f, 1.f);
   
    V2 mouse = platform_input()->mouse.position;
    if (mouse.x >= button_region.min.x &&
        mouse.y >= button_region.min.y &&
        mouse.x <= button_region.max.x &&
        mouse.y <= button_region.max.y) {

      button_color = v4(.20f, .20f, 0.20f, 1.f);
      if (platform_input()->mouse.left.down) Timestamp_At = it;
    }

    if (it == Timestamp_At) button_color = v4(.2f, .2f, .6f, 1.f);

    GFX_IM_2D_Push_Rect(button_region.min, v2_sub(button_region.max, button_region.min), .color = v4(.20f, .20f, .20f, 1.f));
    if (it != Timestamp_At) {
      GFX_IM_2D_Push_Rect(v2(at_x + 2, region.min.y + margin + 2), v2(slot_width - 4, content_height - 4), .color = button_color);
    } else {
      GFX_IM_2D_Push_Rect(v2(at_x + 2, region.min.y + margin + 2), v2(slot_width - 4, content_height - 4), .color = button_color);
    }

    char buffer[16] = {};
    Str08 buffer_str = {};
    buffer_str.txt = (U08 *)buffer;
    buffer_str.len = stbsp_snprintf(buffer, 16, "%u", it);

    F32 txt_w = gfx_font_text_width(font, buffer_str, font_height);
    GFX_IM_2D_Push_Text(buffer_str, font, v2(button_region.min.x + .5f * (button_region.max.x - button_region.min.x - txt_w),
                                             button_region.min.y + .5f * (content_height - font_height)), font_height);
    
    at_x += slot_width + margin;
  }
#endif
}
