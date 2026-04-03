#define CFDR_VERSION_STRING "BETA v0.1"

#define Icon_X(name_, value_)
#define Icon_List_All_X                                          \
Icon_X(FA_PLAY,                                  "\xef\x81\x8b") \
Icon_X(FA_PAUSE,                                 "\xef\x81\x8c") \
Icon_X(FA_FORWARD,                               "\xef\x81\x8e") \
Icon_X(FA_FORWARD_FAST,                          "\xef\x81\x90") \
Icon_X(FA_FORWARD_STEP,                          "\xef\x81\x91") \
Icon_X(FA_FONT,                                  "\xef\x80\xb1") \
Icon_X(FA_EYE,                                   "\xef\x81\xae") \
Icon_X(FA_EYE_SLASH,                             "\xef\x81\xb0") \
Icon_X(FA_FILE,                                  "\xef\x85\x9b") \
Icon_X(FA_CUBE,                                  "\xef\x86\xb2") \
Icon_X(FA_CUBES,                                 "\xef\x86\xb3") \
Icon_X(FA_CHART_AREA,                            "\xef\x87\xbe") \
Icon_X(FA_TABLE,                                 "\xef\x83\x8e") \
Icon_X(FA_CAMERA_RETRO,                          "\xef\x82\x83") \
Icon_X(FA_ANGLE_DOWN,                            "\xef\x84\x87") \
Icon_X(FA_ANGLES_DOWN,                           "\xef\x84\x83") \
Icon_X(FA_STICKY_NOTE,                           "\xef\x89\x89") \
Icon_X(FA_IMAGE,                                 "\xef\x80\xbe") \
Icon_X(FA_ANGLES_RIGHT,                          "\xef\x84\x81") \
Icon_X(FA_WRENCH,                                "\xef\x82\xad") \
Icon_X(FA_ALIGN_JUSTIFY,                         "\xef\x80\xb9") \
Icon_X(FA_LIST_UL,                               "\xef\x83\x8a") \
Icon_X(FA_BOX_ARCHIVE,                           "\xef\x86\x87") \
Icon_X(FA_EXCLAMATION_TRIANGLE,                  "\xef\x81\xb1") \
Icon_X(FA_SQUARE_XMARK,                          "\xef\x8b\x93") \
Icon_X(FA_FLOPPY_DISK,                           "\xef\x83\x87") \
Icon_X(FA_TABLE_CELLS_LARGE,                     "\xef\x80\x89") \
Icon_X(FA_SUN,                                   "\xef\x86\x85") \
Icon_X(FA_MOON,                                  "\xef\x86\x86") \
Icon_X(FA_OBJECT_GROUP,                          "\xef\x89\x87")

#undef  Icon_X
#define Icon_X(name_, value_) var_global Str Icon_##name_ = str_lit(value_);
Icon_List_All_X

var_global U08 Text_Font_Raw[] = {
#embed"../cfdr_data/Geist-Regular.ttf"
};

var_global U08 Mono_Font_Raw[] = {
#embed"../cfdr_data/RobotoMono-Regular.ttf"
};

var_global U08 Icon_Font_Raw[] = {
#embed"../cfdr_data/FontAwesome.otf"
};

var_global Str Text_Font_Baked = (Str) { .len = sizeof(Text_Font_Raw), .txt = Text_Font_Raw };
var_global Str Mono_Font_Baked = (Str) { .len = sizeof(Text_Font_Raw), .txt = Mono_Font_Raw };
var_global Str Icon_Font_Baked = (Str) { .len = sizeof(Icon_Font_Raw), .txt = Icon_Font_Raw };

typedef I32 CFDR_UI_Split_Mode;
enum {
  CFDR_UI_Split_None,
  CFDR_UI_Split_Dual,
};

typedef struct CFDR_UI_State {
  CFDR_State *state;

  Arena   arena;
  Arena   arena_font;

  B32     font_init;
  F32     font_size;
  G2_Font font_text;
  G2_Font font_mono;
  G2_Font font_icon;

  CFDR_UI_Split_Mode split_mode;
  B32                fullscreen;
  B32                dark_mode;

  B32                overlay_tab;
} CFDR_UI_State;

fn_internal void cfdr_ui_viewport_draw_hook(UI_Response *response, R2F draw_region, void *user_data) {
  CFDR_State *state = (CFDR_State *)user_data;

  cfdr_scene_draw(&state->render, response, &state->scene, draw_region);
  cfdr_overlay_draw(&state->overlay, &state->scene, draw_region);
}

fn_internal void cfdr_ui_update_fonts(CFDR_UI_State *ui) {
  F32 font_size = 22.f * js_web_device_pixel_ratio();
  if (ui->font_size != font_size) {
    if (ui->font_init) {
      g2_font_destroy (&ui->font_text);
      g2_font_destroy (&ui->font_mono);
      g2_font_destroy (&ui->font_icon);
      arena_clear     (&ui->arena_font);
    }

    Codepoint icon_codepoints[] = {
#undef  Icon_X
#define Icon_X(name_, value_) codepoint_from_utf8(Icon_##name_, 0),
    Icon_List_All_X
    };

    g2_font_init(&ui->font_text, &ui->arena_font, Text_Font_Baked, font_size, v2_u16(1024, 1024), Codepoints_ASCII);
    g2_font_init(&ui->font_mono, &ui->arena_font, Mono_Font_Baked, font_size, v2_u16(1024, 1024), Codepoints_ASCII);
    g2_font_init(&ui->font_icon, &ui->arena_font, Icon_Font_Baked, font_size, v2_u16(1024, 1024), array_from_sarray(Array_Codepoint, icon_codepoints));


    ui->font_init = 1;
    ui->font_size = font_size;
  }
}

fn_internal void cfdr_ui_init(CFDR_UI_State *ui, CFDR_State *state) {
  arena_init(&ui->arena);
  arena_init(&ui->arena_font);

  ui->state     = state;
  ui->dark_mode = 1;

  ui->font_size = 0;
  ui->font_init = 0;
  cfdr_ui_update_fonts(ui);
}

fn_internal void cfdr_ui_menu_bar(CFDR_UI_State *ui) {
  UI_Node *menu_bar = ui_container(str_lit("##menu_bar"), UI_Container_Box, Axis2_X, UI_Size_Fill, UI_Size_Fit);

  UI_Parent_Scope(menu_bar) {

    var_local_persist Str demo_names[] = {
      str_lit("Urbain Airflow"),
      str_lit("MRI Scan"),
    };

    var_local_persist Str demo_paths[] = {
      str_lit("uap.cfdr"),
      str_lit("mri.cfdr"),
    };

    var_local_persist Str demo_list[] = { str_lit("Urban Air Pollution"), str_lit("Karman 2D") };
    var_local_persist I32 demo_index = 0;
    if (ui_list_fixed(str_lit("Examples"), &demo_index, sarray_len(demo_list), demo_names)) {
      U08 url_buffer[512] = { };
      js_web_current_url_base(511, url_buffer);
      Str url = str_from_cstr((char *)url_buffer);

      Scratch scratch = { };
      Scratch_Scope(&scratch, 0) {
        Str load_url = str_cat(scratch.arena, url, str_cat(scratch.arena, str_lit("/?project=examples/"), demo_paths[demo_index]));
        js_web_load_page(load_url.len, load_url.txt);
      }

    }

    ui_container(str_lit("##padding"), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fit);


    UI_Font_Scope(&ui->font_icon) {
      F32 icon_1_width = fo_text_width(&ui->font_icon.font, Icon_FA_MOON);
      F32 icon_2_width = fo_text_width(&ui->font_icon.font, Icon_FA_SUN);
      F32 icon_width   = f32_max(icon_1_width, icon_2_width);

      UI_Node *theme_button = ui_container(ui->dark_mode ? Icon_FA_MOON : Icon_FA_SUN, UI_Container_Box, Axis2_X, UI_Size_Fixed(icon_width), UI_Size_Fill);
      theme_button->flags  |= UI_Flag_Draw_Label;
      theme_button->flags  |= UI_Flag_Draw_Label_Centered;
      theme_button->flags  |= UI_Flag_Draw_Rounded;

      theme_button->palette.idle  = hsv_u32(235, 27, 25);
      theme_button->palette.hover = hsv_u32(235, 24, 44);
      theme_button->palette.down  = hsv_u32(235, 10, 15);

      if (theme_button->response.press) {
        ui->dark_mode = !ui->dark_mode;
        if (ui->dark_mode) {
          UI_Theme_Active = UI_Theme_Dark;
        } else {
          UI_Theme_Active = UI_Theme_Light;
        }
      }
    }
  }
}

fn_internal void cfdr_ui_status_bar(CFDR_UI_State *ui) {
  UI_Parent_Scope(ui_container(str_lit("##status_bar"), UI_Container_Box, Axis2_X, UI_Size_Fill, UI_Size_Fit)) {
    ui_label(ui->state->project);

    UI_Font_Scope(&ui->font_icon) { ui_label(Icon_FA_SQUARE_XMARK); }
    ui_label(str_lit("0 Errors"));
    UI_Font_Scope(&ui->font_icon) { ui_label(Icon_FA_EXCLAMATION_TRIANGLE); }
    ui_label(str_lit("0 Warnings"));
    ui_container(str_lit("##pad_left"), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fit);

    UI_Font_Scope(&ui->font_mono) {
      ui_label(str_lit(CFDR_VERSION_STRING));
    }
  }
}

fn_internal void cfdr_ui_viewport_menu_bar(CFDR_UI_State *ui, I32 viewport_index) {
  UI_Parent_Scope(ui_container(str_lit("##menu_bar"), UI_Container_Box, Axis2_X, UI_Size_Fill, UI_Size_Fit)) {

    UI_Parent_Scope(ui_container(str_lit("##viewport_tab"), UI_Container_None, Axis2_X, UI_Size_Fit, UI_Size_Fit)) {
      UI_Font_Scope(&ui->font_icon) {
        ui_label(Icon_FA_CAMERA_RETRO);
      }

      ui_label(str_lit("Viewport"));
    }

    UI_Parent_Scope(ui_container(str_lit("##left"), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fit)) {
      ui_container(str_lit("##padding_1"), UI_Container_None, Axis2_X, UI_Size_Fixed(25), UI_Size_Fit);
      UI_Parent_Scope(ui_container(str_lit("##toolbar"), UI_Container_None, Axis2_X, UI_Size_Fit, UI_Size_Fit)) {
        ui_f32_edit_static(str_lit("Grid Level"), &ui->state->scene.view.grid.subdiv, 0.1f, 10.f, 0.01f);

        ui_container(str_lit("##padding_1"), UI_Container_None, Axis2_X, UI_Size_Fixed(10), UI_Size_Fit);
        ui_color_hsv(str_lit("Grid Color"), &ui->state->scene.view.grid.color.hsv);

        ui_container(str_lit("##padding_2"), UI_Container_None, Axis2_X, UI_Size_Fixed(10), UI_Size_Fit);
        UI_Parent_Scope(ui_container(str_lit("##background_color"), UI_Container_None, Axis2_X, UI_Size_Fit, UI_Size_Fit)) {
          ui_color_hsv(str_lit("Background"), &ui->state->scene.view.background.hsv);
        }
      }
    }

    UI_Node *center = ui_container(str_lit("##center"), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fit);
    center->layout.gap_child = 2.f;
    UI_Parent_Scope(center) {
      UI_Font_Scope(&ui->font_icon) {

        Scratch scratch = { };
        Scratch_Scope(&scratch, 0) {
          
          Str forward_fast_reverse = str_cat(scratch.arena, Icon_FA_FORWARD_FAST, str_lit("##reverse"));
          Str forward_step_reverse = str_cat(scratch.arena, Icon_FA_FORWARD_STEP, str_lit("##reverse"));
          
          if (ui_button_flipped(forward_fast_reverse).press) {
            ui->state->scene.step.step_at = 0;
          }

          if (ui_button_flipped(forward_step_reverse).press) {
            ui->state->scene.step.step_at = i32_max(ui->state->scene.step.step_at - 1, 0);
          }
        }

        // ui_button(Icon_FA_PLAY);
        if (ui_button(Icon_FA_FORWARD_STEP).press) {
          ui->state->scene.step.step_at = i32_min(ui->state->scene.step.step_at + 1, ui->state->scene.step.step_count ? ui->state->scene.step.step_count - 1 : 0);
        }

        if (ui_button(Icon_FA_FORWARD_FAST).press) {
          ui->state->scene.step.step_at = ui->state->scene.step.step_count ? ui->state->scene.step.step_count - 1 : 0;
        }
      }
    }

    UI_Parent_Scope(ui_container(str_lit("##right"), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fit)) {
    }
  }
}

fn_internal void cfdr_ui_viewport(CFDR_UI_State *ui, I32 viewport_index) {
  
  char buffer[512];
  stbsp_snprintf(buffer, 512,"##viewport_%d", viewport_index);

  UI_Parent_Scope(ui_container(str_from_cstr(buffer), UI_Container_Box, Axis2_Y, UI_Size_Fill, UI_Size_Fill)) {
    cfdr_ui_viewport_menu_bar(ui, viewport_index);

    UI_Node *content = ui_container(str_lit("##content"), UI_Container_Box, Axis2_X, UI_Size_Fill, UI_Size_Fill);
    content->flags |= UI_Flag_Draw_Content_Hook | UI_Flag_Draw_Clip_Content;
    content->draw.content_hook      = cfdr_ui_viewport_draw_hook;
    content->draw.content_user_data = ui->state;

    if (content->response.press_secondary) {
      V2F spawn_at = v2f(pl_input()->mouse.position.x, pl_display()->resolution.y - pl_input()->mouse.position.y);
      ui_context_spawn(content, spawn_at);
    }

    if (pl_input()->keyboard.state[PL_KB_O].press) {
        ui->state->scene.view.camera.orthographic = !ui->state->scene.view.camera.orthographic;
    }

    UI_Context_Scope(content, str_lit("##context")) {
      UI_Node *container = ui_container(str_lit("##padding"), UI_Container_Box, Axis2_Y, UI_Size_Fit, UI_Size_Fit);

      container->layout.gap_border[Axis2_X] = 5;
      container->layout.gap_border[Axis2_Y] = 5;
      container->layout.gap_child = 5;

      UI_Parent_Scope(container) {
        UI_Parent_Scope(ui_container(str_lit("##viewport_tab"), UI_Container_None, Axis2_Y, UI_Size_Fill, UI_Size_Fit)) {
          ui_label(str_lit("Viewport"));
          ui_separator(str_lit("##break"));
        }

        ui_checkbox(str_lit("Orthographic View"), &ui->state->scene.view.camera.orthographic);
        ui_checkbox(str_lit("Fullscreen"), &ui->fullscreen);
        ui_checkbox(str_lit("Show Grid"), &ui->state->scene.view.grid.visible);
      }
    }

  }
}

fn_internal void cfdr_ui_layer_entry(CFDR_UI_State *ui, Str tag, B32 *visible, I32 entry_at, void *entry_ptr, void **active_ptr, Str icon) {
  char buffer[512] = { };
  stbsp_snprintf(buffer, 512,"##entry_%d", entry_at);

  UI_Parent_Scope(ui_container(str_from_cstr(buffer), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fit)) {
    UI_Node *node = ui_container(str_from_cstr(buffer), UI_Container_Box, Axis2_X, UI_Size_Fill, UI_Size_Fit);
    node->layout.gap_border[Axis2_X] = 5;
    node->layout.gap_border[Axis2_Y] = 5;
    node->flags |= UI_Flag_Draw_Rounded;

    if (entry_ptr != *active_ptr) {
      node->flags &= ~UI_Flag_Draw_Border;
    } else {
      node->palette.border = hsv_u32(200, 80, 100);
    }

    if (entry_at % 2) {
      node->palette.idle  = hsv_u32(235, 0, 15);
      node->palette.hover = hsv_u32(235, 0, 30);
      node->palette.down  = hsv_u32(200, 0, 50);
    } else {
      node->palette.idle  = hsv_u32(235, 0, 15 + 5);
      node->palette.hover = hsv_u32(235, 0, 30 + 5);
      node->palette.down  = hsv_u32(200, 0, 50 + 5);
    }

    if (entry_ptr == *active_ptr) {
      node->palette.idle  = hsv_u32(200, 80, 50);
      node->palette.hover = hsv_u32(200, 80, 50);
    }

    if (node->response.press) {
      *active_ptr = entry_ptr;
    }

    UI_Parent_Scope(node) {
      UI_Font_Scope(&ui->font_icon) { ui_label(icon); }
      ui_label(tag);
      ui_container(str_lit("##padding"), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fill);
    }

    UI_Font_Scope(&ui->font_icon) {

      F32 icon_1_width = fo_text_width(&ui->font_icon.font, Icon_FA_EYE);
      F32 icon_2_width = fo_text_width(&ui->font_icon.font, Icon_FA_EYE_SLASH);
      F32 icon_width   = f32_max(icon_1_width, icon_2_width);

      UI_Node *eye = ui_container(*visible ? Icon_FA_EYE : Icon_FA_EYE_SLASH, UI_Container_Box, Axis2_X, UI_Size_Fixed(icon_width), UI_Size_Fill);
      eye->flags  |= UI_Flag_Draw_Label;
      eye->flags  |= UI_Flag_Draw_Label_Centered;
      eye->flags  |= UI_Flag_Draw_Rounded;

      eye->palette = UI_Theme_Active.button;
      if (eye->response.press) {
        *visible = !*visible;
      }
    }
  }
}

fn_internal void cfdr_ui_layer_panel(CFDR_UI_State *ui) {
  UI_Parent_Scope(ui_container(str_lit("##layer_panel"), UI_Container_Box, Axis2_Y, UI_Size_Fill, UI_Size_Fit)) {
    UI_Node *tab_bar = ui_container(str_lit("##tab_bar"), UI_Container_None, Axis2_X, UI_Size_Fit, UI_Size_Fit);
    tab_bar->layout.gap_child = 2.f;
    UI_Parent_Scope(tab_bar) {
      UI_Node *object_tab = ui_container(str_lit("##object_tab"), UI_Container_Box, Axis2_X, UI_Size_Fit, UI_Size_Fit);
      object_tab->palette = UI_Theme_Active.button;
      if (!ui->overlay_tab) {
        object_tab->palette.idle = object_tab->palette.down;
        object_tab->palette.hover = hsv_u32(36, 70, 25);
      }

      if (object_tab->response.press) {
        ui->overlay_tab = 0;
      }

      UI_Parent_Scope(object_tab) {
        UI_Font_Scope(&ui->font_icon) { ui_label(Icon_FA_CUBES); }
        ui_label(str_lit("Objects"));
      }

      UI_Node *overlay_tab = ui_container(str_lit("##layer_tab"), UI_Container_Box, Axis2_X, UI_Size_Fit, UI_Size_Fit);
      overlay_tab->palette = UI_Theme_Active.button;

      if (ui->overlay_tab) {
        overlay_tab->palette.idle = overlay_tab->palette.down;
        overlay_tab->palette.hover = hsv_u32(36, 70, 25);
      }

      if (overlay_tab->response.press) {
        ui->overlay_tab = 1;
      }

      UI_Parent_Scope(overlay_tab) {
        UI_Font_Scope(&ui->font_icon) { ui_label(Icon_FA_OBJECT_GROUP); }
        ui_label(str_lit("Overlays"));
      }
    }

    ui_separator(str_lit("##sep_title"));
 
    UI_Node *layer_container = ui_container(str_lit("##layer_container"), UI_Container_None, Axis2_Y, UI_Size_Fill, UI_Size_Fit);
    UI_Parent_Scope(layer_container) {
      layer_container->layout.gap_border[Axis2_X] = 2;
      layer_container->layout.gap_border[Axis2_Y] = 5;
      layer_container->layout.gap_child = 0;

      I32 layer_at = 0;
      if (ui->overlay_tab) {
        for (CFDR_Overlay_Node *it = ui->state->overlay.first; it; it = it->next, ++layer_at) {
          void *active = ui->state->overlay.active;
          cfdr_ui_layer_entry(ui, it->tag, &it->visible, layer_at, it, &active, Icon_FA_FONT);
          ui->state->overlay.active = active;
        }
      } else {
        for (CFDR_Object_Node *it = ui->state->scene.first; it; it = it->next, ++layer_at) {
          void *active = ui->state->scene.active;
          cfdr_ui_layer_entry(ui, it->tag, &it->visible, layer_at, it, &active, Icon_FA_CUBE);
          ui->state->scene.active = active;
        }
      }
    }
  }
}

fn_internal void cfdr_ui_property_object(CFDR_UI_State *ui) {
  UI_Node *container = ui_container(str_lit("##property_object"), UI_Container_None, Axis2_Y, UI_Size_Fill, UI_Size_Fit);
  if (ui->state->scene.active) {
    UI_Parent_Scope(container) {
      container->layout.gap_border[Axis2_X] = 5;
      container->layout.gap_border[Axis2_Y] = 5;
      container->layout.gap_child = 5;

      CFDR_Object_Node *object = ui->state->scene.active;

      ui_label(str_lit("Base"));
      ui_separator(str_lit("##separator_1"));

      ui_color_hsv  (str_lit("Color"),    &object->color.hsv);
      ui_f32_edit   (str_lit("Opacity"),  &object->color.a, 0.0f, 1.0f, 0.01f);


      var_local_persist Str material_list[] = { str_lit("Flat"), str_lit("Matcap"), str_lit("Sample"), };
      ui_list(str_lit("Material"), &object->material, sarray_len(CFDR_Material_String_List), CFDR_Material_String_List); 

      UI_Parent_Scope(ui_container(str_lit("##scale"), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fit)) {
        ui_label(str_lit("Scale"));
        ui_container(str_lit("##padding"), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fit);
        ui_f32_edit_static(str_lit("##x"), &object->scale.x, -5000.0f, 5000.0f, 0.05f);
        ui_f32_edit_static(str_lit("##y"), &object->scale.y, -5000.0f, 5000.0f, 0.05f);
        ui_f32_edit_static(str_lit("##z"), &object->scale.z, -5000.0f, 5000.0f, 0.05f);
      }

      UI_Parent_Scope(ui_container(str_lit("##translate"), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fit)) {
        ui_label(str_lit("Translate"));
        ui_container(str_lit("##padding"), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fit);
        ui_f32_edit_static(str_lit("##x"), &object->translate.x, -5000.0f, 5000.0f, 0.05f);
        ui_f32_edit_static(str_lit("##y"), &object->translate.y, -5000.0f, 5000.0f, 0.05f);
        ui_f32_edit_static(str_lit("##z"), &object->translate.z, -5000.0f, 5000.0f, 0.05f);
      }

      if (object->flags & CFDR_Object_Flag_Draw_Volume) {
        ui_separator(str_lit("##separator_2"));
        ui_label(str_lit("Volume"));
        ui_separator(str_lit("##separator_3"));
        ui_f32_edit(str_lit("Volume Density"), &object->volume_density, 0.f, 10.f, 0.005f);
      }
    }
  }
}

fn_internal void cfdr_ui_property_overlay(CFDR_UI_State *ui) {
  UI_Node *container = ui_container(str_lit("##property_overlay"), UI_Container_None, Axis2_Y, UI_Size_Fill, UI_Size_Fit);
  if (ui->state->overlay.active) {
    UI_Parent_Scope(container) {
      container->layout.gap_border[Axis2_X] = 5;
      container->layout.gap_border[Axis2_Y] = 5;
      container->layout.gap_child = 5;

      var_local_persist Str x_align_list[] = { str_lit("Left"),   str_lit("Right"), str_lit("Center"), };
      var_local_persist Str y_align_list[] = { str_lit("Bottom"), str_lit("Top"),   str_lit("Center"), };

      CFDR_Overlay_Node *overlay = ui->state->overlay.active;

      ui_color_hsv(str_lit("Color"),  &overlay->color.hsv);
      ui_f32_edit(str_lit("Opacity"), &overlay->color.a, 0.0f, 1.0f, 0.01f);

      ui_f32_edit(str_lit("Scale"), &overlay->scale, 25.0f, 200.0f, 1.f);
      
      ui_list(str_lit("Position X"), &overlay->position_x, sarray_len(x_align_list), x_align_list); 
      ui_list(str_lit("Position Y"), &overlay->position_y, sarray_len(y_align_list), y_align_list); 

      ui_f32_edit(str_lit("Border X"), &overlay->border.x, 0.0f, 1000.0f, 1.0f);
      ui_f32_edit(str_lit("Border Y"), &overlay->border.y, 0.0f, 1000.0f, 1.0f);

      B32 shadow_flag = overlay->flags & CFDR_Overlay_Flag_Shadow;
      ui_checkbox(str_lit("Shadow"), &shadow_flag);
      if (shadow_flag) {
        overlay->flags |= CFDR_Overlay_Flag_Shadow;
      } else {
        overlay->flags &= ~CFDR_Overlay_Flag_Shadow;
      }

      if (shadow_flag) {
        UI_Node *shadow_container = ui_container(str_lit("##shadow"), UI_Container_Box, Axis2_Y, UI_Size_Fill, UI_Size_Fit);
        shadow_container->layout.gap_border[Axis2_X] = 5;
        shadow_container->layout.gap_border[Axis2_Y] = 5;
        shadow_container->layout.gap_child = 5;
        UI_Parent_Scope(shadow_container) {
          shadow_container->flags |= UI_Flag_Draw_Rounded;
          shadow_container->palette.idle  = hsv_u32(200, 10, 10);
          shadow_container->palette.hover = hsv_u32(210, 10, 15);
          shadow_container->palette.down  = hsv_u32(220, 10, 15);

          ui_color_hsv(str_lit("Color"),  &overlay->color_shadow.hsv);
          ui_f32_edit(str_lit("Offset"),  &overlay->shadow_offset, 0.0f, 100.0f, 0.1f);
        }
      }
    }
  }
}

fn_internal void cfdr_ui_property_surface(CFDR_UI_State *ui) {
  UI_Node *container = ui_container(str_lit("##property_panel"), UI_Container_None, Axis2_Y, UI_Size_Fill, UI_Size_Fit);
  UI_Parent_Scope(container) {
    container->layout.gap_border[Axis2_X] = 5;
    container->layout.gap_border[Axis2_Y] = 5;
    container->layout.gap_child = 5;

#if 0
    CFDR_Layer *layer = &CFDR_Res_State.layer_array.dat[CFDR_Res_State.layer_active];
    ui_color_hsv(str_lit("Color"), &layer->color.hsv);

    var_local_persist Str material_list[] = {
      str_lit("Flat"),
      str_lit("Position"),
    };

    ui_list(str_lit("Material"), &layer->material_type, sarray_len(material_list), material_list); 
    ui_checkbox(str_lit("Wireframe"), &layer->wireframe);
#endif
  }
}

fn_internal void cfdr_ui_property_volume(CFDR_UI_State *ui) {
  UI_Node *container = ui_container(str_lit("##property_panel"), UI_Container_None, Axis2_Y, UI_Size_Fill, UI_Size_Fit);
  UI_Parent_Scope(container) {
    container->layout.gap_border[Axis2_X] = 5;
    container->layout.gap_border[Axis2_Y] = 5;
    container->layout.gap_child = 5;

#if 0
    CFDR_Layer *layer = &CFDR_Res_State.layer_array.dat[CFDR_Res_State.layer_active];
    ui_f32_edit(str_lit("Density"), &layer->volume_density, 0.0f, 10.0f, 0.01f);
#endif
  }
}

fn_internal void cfdr_ui_property_panel(CFDR_UI_State *ui) {
  UI_Node *container = ui_container(str_lit("##property_panel"), UI_Container_Box, Axis2_Y, UI_Size_Fill, UI_Size_Fill);
  UI_Parent_Scope(container) {


  UI_Parent_Scope(ui_container(str_lit("##property_tab"), UI_Container_None, Axis2_X, UI_Size_Fit, UI_Size_Fit)) {
    UI_Font_Scope(&ui->font_icon) {
      ui_label(Icon_FA_WRENCH);
    }

    char buffer[512];
    ui_label(str_lit("Properties"));
  }

  ui_separator(str_lit("##sep_title"));

 
  if (ui->overlay_tab) {
    cfdr_ui_property_overlay(ui);
  } else {
    cfdr_ui_property_object(ui);
  }
#if 0
    if (layer->type == CFDR_Layer_Type_Text) {
      cfdr_ui_property_text(ui);
    } else if (layer->type == CFDR_Layer_Type_Surface) {
      cfdr_ui_property_surface(ui);
    } else if (layer->type == CFDR_Layer_Type_Volume) {
      cfdr_ui_property_volume(ui);
    }
#endif
  }
}

fn_internal void cfdr_ui_side_panel(CFDR_UI_State *ui) {
  UI_Parent_Scope(ui_container(str_lit("##side_panel"), UI_Container_None, Axis2_Y, UI_Size_Fit, UI_Size_Fill)) {
    cfdr_ui_layer_panel     (ui);
    cfdr_ui_property_panel  (ui);
  }
}

fn_internal void cfdr_ui_viewport_all(CFDR_UI_State *ui) {
  switch (ui->split_mode) {
    case CFDR_UI_Split_None: {
      cfdr_ui_viewport(ui, 0);
    } break;

    case CFDR_UI_Split_Dual: {
      cfdr_ui_viewport(ui, 0);
      cfdr_ui_viewport(ui, 1);
    } break;
  }
}

fn_internal void cfdr_ui_resource_panel(CFDR_UI_State *ui) {
  UI_Node *box = ui_container(str_lit("##resource_panel"), UI_Container_Box, Axis2_Y, UI_Size_Fill, UI_Size_Fill);
  UI_Parent_Scope(box) {

    UI_Parent_Scope(ui_container(str_lit("##resource_tab"), UI_Container_None, Axis2_X, UI_Size_Fit, UI_Size_Fit)) {
      UI_Font_Scope(&ui->font_icon) {
        ui_label(Icon_FA_BOX_ARCHIVE);
      }
      ui_label(str_lit("Resources"));
    }

    ui_separator(str_lit("##sep_title"));
  }
}

fn_internal void cfdr_ui_log_panel(CFDR_UI_State *ui) {
  UI_Node *box = ui_container(str_lit("##log_panel"), UI_Container_Box, Axis2_Y, UI_Size_Fill, UI_Size_Fill);
  UI_Parent_Scope(box) {

    UI_Parent_Scope(ui_container(str_lit("##log_tab"), UI_Container_None, Axis2_X, UI_Size_Fit, UI_Size_Fit)) {
      UI_Font_Scope(&ui->font_icon) {
        ui_label(Icon_FA_LIST_UL);
      }
      ui_label(str_lit("Log"));
    }

    ui_separator(str_lit("##sep_title"));

    UI_Font_Scope(&ui->font_mono) {
    }
  }
}

fn_internal void cfdr_ui_workspace(CFDR_UI_State *ui) {
  UI_Parent_Scope(ui_container(str_lit("##workspace"), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fill)) {
    cfdr_ui_side_panel(ui);
    UI_Parent_Scope(ui_container(str_lit("##container_1"), UI_Container_None, Axis2_Y, UI_Size_Fill, UI_Size_Fill)) {
      cfdr_ui_viewport_all(ui);

      UI_Parent_Scope(ui_container(str_lit("##container_2"), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fit)) {
        cfdr_ui_resource_panel(ui);
        cfdr_ui_log_panel(ui);
      }
    }
  }
}

fn_internal void cfdr_ui(CFDR_UI_State *ui) {
  cfdr_ui_update_fonts(ui);

  UI_Font_Scope(&ui->font_text) {
    if (pl_input()->keyboard.state[PL_KB_F].press) {
      ui->fullscreen = !ui->fullscreen;
    }

    if (ui->fullscreen) {
      cfdr_ui_viewport_all(ui);
    } else {
      UI_Parent_Scope(ui_container(str_lit("##global"), UI_Container_None, Axis2_Y, UI_Size_Fill, UI_Size_Fill)) {
        cfdr_ui_menu_bar(ui);
        cfdr_ui_workspace(ui);
        cfdr_ui_status_bar(ui);
      }
    }
  }
}

