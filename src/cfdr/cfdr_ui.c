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
Icon_X(FA_OBJECT_GROUP,                          "\xef\x89\x87") \
Icon_X(FA_BARS,                                  "\xef\x83\x89") \
Icon_X(FA_BUG,                                   "\xef\x86\x88") \
Icon_X(FA_COG,                                   "\xef\x80\x93")

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

typedef I32 CFDR_Tab_Mode;
enum {
  CFDR_Tab_Object,
  CFDR_Tab_Overlay,
  CFDR_Tab_CMap,
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
  CFDR_Tab_Mode      tab_mode;

  B32 profile_view;
  F32 frame_rate_avg;
  U32 frame_rate_at;
  F32 frame_rate_buffer[256];

  V2I          viewport_resolution;
  R_Texture_2D viewport_color_ms;
  R_Texture_2D viewport_depth_ms;
  R_Texture_2D viewport_color;
  G2_Material  viewport_mat;

  Arena        render_image_arena;
  B32          render_image_waiting;
  U32          render_image_status;
  U08         *render_image_buffer;
  V2I          render_image_size;
  R2I          render_image_sample_region;

} CFDR_UI_State;

fn_internal void cfdr_viewport_render(UI_Response *response, R2F draw_region, CFDR_UI_State *ui) {
  V2I old_viewport_resolution = ui->viewport_resolution;
  ui->viewport_resolution = r2i_size(pl_display_region());
  if (old_viewport_resolution.x != ui->viewport_resolution.x || old_viewport_resolution.y != ui->viewport_resolution.y) {
    g2_material_delete(&ui->viewport_mat);
    r_texture_2D_destroy(&ui->viewport_color_ms);
    r_texture_2D_destroy(&ui->viewport_depth_ms);
    r_texture_2D_destroy(&ui->viewport_color);

    ui->viewport_color_ms   = r_texture_2D_allocate (R_Texture_Format_Backbuffer, ui->viewport_resolution.x, ui->viewport_resolution.y, 4);
    ui->viewport_depth_ms   = r_texture_2D_allocate (R_Texture_Format_Depth_F24,  ui->viewport_resolution.x, ui->viewport_resolution.y, 4);
    ui->viewport_color      = r_texture_2D_allocate (R_Texture_Format_Backbuffer, ui->viewport_resolution.x, ui->viewport_resolution.y, 1);
    ui->viewport_mat        = g2_material_create    (ui->viewport_color, R_Sampler_Nearest_Clamp);
  }

  ui->render_image_sample_region = r2i_from_r2f(draw_region);
  ui->render_image_sample_region.y0 = pl_display()->resolution.y - draw_region.y1;
  ui->render_image_sample_region.y1 = pl_display()->resolution.y - draw_region.y0;

  r_target_begin(ui->viewport_color_ms, ui->viewport_color, ui->viewport_depth_ms); {
    CFDR_State    *state  = ui->state;

    cfdr_scene_draw(&state->render, &state->cmap_table, response, &state->scene, draw_region);
    cfdr_overlay_draw(&state->overlay, &state->cmap_table, &state->scene, draw_region);

    if (ui->profile_view) {
      g2_draw_rect(draw_region.min, r2f_size(draw_region), .color = v4f(.0f, .0f, .0f, .6f));

      F32 max_value = f32_largest_negative;
      F32 min_value = f32_largest_positive;
      For_U32(it, sarray_len(ui->frame_rate_buffer)) {
        max_value = f32_max(ui->frame_rate_buffer[it], max_value);
        min_value = f32_min(ui->frame_rate_buffer[it], min_value);
      }

      V2F draw_at     = v2f_add(draw_region.min, v2f(10, 10));
      F32 max_height  = .25f * r2f_size(draw_region).y;
      For_U32(it, sarray_len(ui->frame_rate_buffer)) {
        F32 h = (ui->frame_rate_buffer[it] / max_value) * max_height;

        V4F c = rgba_from_hsva(v4f(h / max_height, .8f, 1.f, 1.f));

        g2_draw_rect(draw_at, v2f(2.f, h), .color = c);
        draw_at.x += 3.f;
      }

      F32 avg_h = draw_at.y + ((1.f / ui->frame_rate_avg) / max_value) * max_height;
      g2_draw_rect(v2f(0, avg_h), v2f(draw_at.x + 100, 2), .color = v4f(1, 1, 1, 1));

      char buffer[512] = { };
      stbsp_snprintf(buffer, 512, "Average: %.2f FPS, Highest: %.2f FPS, Lowest: %.2f FPS", ui->frame_rate_avg, 1.f / min_value, 1.f / max_value);
      g2_draw_text(str_from_cstr(buffer), &ui->font_mono, v2f(draw_region.min.x + 10, draw_region.min.y + max_height + 50), .color = v4f(1, 1, 1, 1));
    }

    g2_submit_draw();
    g2_frame_flush();
    r_frame_flush();
  } r_target_end();
}

fn_internal void cfdr_ui_viewport_draw_hook(UI_Response *response, R2F draw_region, void *user_data) {
  CFDR_UI_State *ui     = (CFDR_UI_State *)user_data;

  V2F uv_bl = v2f(draw_region.x0 / pl_display()->resolution.x, (pl_display()->resolution.y - draw_region.y0) / pl_display()->resolution.y);
  V2F uv_tr = v2f(draw_region.x1 / pl_display()->resolution.x, (pl_display()->resolution.y - draw_region.y1) / pl_display()->resolution.y);

  g2_draw_rect(draw_region.min, r2f_size(draw_region), .uv_bl = uv_bl, .uv_tr = uv_tr, .mat = ui->viewport_mat, .color=v4f(1, 1, 1, 1));
}

fn_internal void cfdr_ui_update_fonts(CFDR_UI_State *ui) {
  F32 font_size = 20.f * js_web_device_pixel_ratio();
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

  For_U32(it, sarray_len(ui->frame_rate_buffer)) {
    ui->frame_rate_buffer[it] = f32_smallest_positive;
  }

  ui->viewport_resolution = v2i(1024, 1024);
  ui->viewport_color_ms   = r_texture_2D_allocate (R_Texture_Format_Backbuffer, ui->viewport_resolution.x, ui->viewport_resolution.y, 4);
  ui->viewport_depth_ms   = r_texture_2D_allocate (R_Texture_Format_Depth_F24,  ui->viewport_resolution.x, ui->viewport_resolution.y, 4);
  ui->viewport_color      = r_texture_2D_allocate (R_Texture_Format_Backbuffer, ui->viewport_resolution.x, ui->viewport_resolution.y, 1);
  ui->viewport_mat        = g2_material_create    (ui->viewport_color, R_Sampler_Nearest_Clamp);

  arena_init(&ui->render_image_arena);
}

fn_internal void cfdr_ui_menu_bar(CFDR_UI_State *ui) {
  UI_Node *menu_bar = ui_container(str_lit("##menu_bar"), UI_Container_Box, Axis2_X, UI_Size_Fill, UI_Size_Fit);

  UI_Parent_Scope(menu_bar) {

    var_local_persist Str demo_names[] = {
      str_lit("        -        "),
      str_lit("SZE - Stockholm A"),
      str_lit("SZE - Stockholm B"),
      str_lit("SZE - Poznan A"),
      str_lit("FAU - Lattice Boltzman Large"),
    };

    var_local_persist Str demo_paths[] = {
      str_lit(""),
      str_lit("stockholm_A.cfdr"),
      str_lit("stockholm_B.cfdr"),
      str_lit("poznan_A.cfdr"),
      str_lit("fau.cfdr"),
    };

    var_local_persist I32 demo_index = 0;
    if (ui_list_fixed(str_lit("Examples"), &demo_index, sarray_len(demo_names), demo_names)) {
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
#if 0
      {
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
        }
      }
#endif

      {
        F32 icon_1_width = fo_text_width(&ui->font_icon.font, Icon_FA_BUG);
        F32 icon_2_width = fo_text_width(&ui->font_icon.font, Icon_FA_COG);
        F32 icon_width   = f32_max(icon_1_width, icon_2_width);

        UI_Node *theme_button = ui_container(ui->profile_view ? Icon_FA_BUG : Icon_FA_COG, UI_Container_Box, Axis2_X, UI_Size_Fixed(icon_width), UI_Size_Fill);
        theme_button->flags  |= UI_Flag_Draw_Label;
        theme_button->flags  |= UI_Flag_Draw_Label_Centered;
        theme_button->flags  |= UI_Flag_Draw_Rounded;

        theme_button->palette.idle  = hsv_u32(235, 27, 25);
        theme_button->palette.hover = hsv_u32(235, 24, 44);
        theme_button->palette.down  = hsv_u32(235, 10, 15);

        if (theme_button->response.press) {
          ui->profile_view = !ui->profile_view;
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

    if (Resource_Downloading) {
      char buffer[512] = {};
      F32 downloaded_ratio = (F32)Resource_Downloading_Bytes_Done / (F32)Resource_Downloading_Bytes_Total;
      stbsp_snprintf(buffer, 512, "Downloading Resources: %.2f%%###download", 100.f * downloaded_ratio);

      if (downloaded_ratio == downloaded_ratio) {
        ui_progress_bar(str_from_cstr(buffer), downloaded_ratio);
      } else {
        ui_progress_bar(str_from_cstr(buffer), 0);
      }
    } else {
      ui_progress_bar(str_lit("Downloading Resources - Done###download"), 1.f);
    }

    // ui_container(str_lit("##pad_left"), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fit);

    UI_Font_Scope(&ui->font_mono) {
      // char buffer[512];
      // stbsp_snprintf(buffer, 512, "%.2f FPS###FRAMERATE", ui->frame_rate_avg);

      // ui_label(str_from_cstr(buffer));
      ui_container(str_lit("##spacing"), UI_Container_None, Axis2_X, UI_Size_Fixed(10), UI_Size_Fit);
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

      ui_container(str_lit("##spacing_1"), Axis2_X, UI_Container_None, UI_Size_Fixed(10), UI_Size_Fit);

      if (ui->state->scene.step.var_count) {
        I32 variable_index = ui->state->scene.step.var_at;
        ui_list_fixed(str_lit("Variable"), &ui->state->scene.step.var_at, ui->state->scene.step.var_count, ui->state->scene.step.var_names); 
      }

      ui_container(str_lit("##spacing_2"), Axis2_X, UI_Container_None, UI_Size_Fixed(10), UI_Size_Fit);

      Str cmap = ui->state->scene.cmap;
      I32 colormap_index = 0;
      For_U64(it, ui->state->cmap_table.key_count) {
        if (str_equals(cmap, ui->state->cmap_table.key_list[it])) {
          colormap_index = it;
          break;
        }
      }

      ui_list_fixed(str_lit("Color Map"), &colormap_index, ui->state->cmap_table.key_count, ui->state->cmap_table.key_list); 
      ui->state->scene.cmap = ui->state->cmap_table.key_list[colormap_index];
    }

    UI_Node *right = ui_container(str_lit("##right"), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fit);
    right->layout.gap_child = 2.f;
    UI_Parent_Scope(right) {
      ui_container(str_lit("##padding_1"), Axis2_X, UI_Container_None, UI_Size_Fill, UI_Size_Fit);

      if (ui_button(str_lit("Render Image")).press) {
        if (!ui->render_image_waiting) { 
          log_info("Rendering Image...");
          ui->render_image_waiting  = 1;
          ui->render_image_status   = 0;

          // R2I read_region       = r2i(0, 0, ui->viewport_resolution.x, ui->viewport_resolution.y);
          R2I read_region = ui->render_image_sample_region;
          V2I read_region_size  = r2i_size(read_region);

          ui->render_image_size = read_region_size;

          arena_clear(&ui->render_image_arena);
          ui->render_image_buffer = arena_push_count(&ui->render_image_arena, U08, read_region_size.x * read_region_size.y * 4);
          r_texture_2D_read(ui->viewport_color, read_region, ui->render_image_buffer, &ui->render_image_status);
        }
      }

      if (ui->render_image_status) {
        log_info("Exporting Image");
        ui->render_image_waiting = 0;
        ui->render_image_status  = 0;

        IM_Bitmap image = {
          .width    = ui->render_image_size.width,
          .height   = ui->render_image_size.height,
          .channels = 4,
          .dat      = ui->render_image_buffer,
        };

        Scratch scratch = { };
        Scratch_Scope(&scratch, 0) {
#if 0
          char header[512] = { };
          U64 header_len   = stbsp_snprintf(header, 512, "P6\n%d %d\n255\n", image.width, image.height);

          U08 *buffer = arena_push_count(scratch.arena, U08, header_len + 3 * image.width * image.height);
          memory_copy(buffer, header, header_len);

          For_U64(h, image.height) {
            For_U64(w, image.width) {
              U64 dst_base = h                      * image.width + w;
              U64 src_base = (image.height - h - 1) * image.width + w;
              buffer[header_len + 3 * dst_base + 0] = image.dat[4 * src_base + 0];
              buffer[header_len + 3 * dst_base + 1] = image.dat[4 * src_base + 1];
              buffer[header_len + 3 * dst_base + 2] = image.dat[4 * src_base + 2];
            }
          }

          char name_buffer[512] = { };
          U64 name_len = stbsp_snprintf(name_buffer, 512, "%.*s.ppm", str_expand(ui->state->project));
          js_web_download(name_len, (U08 *)name_buffer, header_len + 3 * image.width * image.height, buffer);
#else
      U32 pixel_data_size = image.width * image.height * 4;
      U32 file_size = 54 + pixel_data_size;

      // Allocate full file buffer
      U08 *buffer_data = arena_push_size(scratch.arena, file_size);

      buffer_data[0] = 'B';
      buffer_data[1] = 'M';

      // file size
      *(U32*)(buffer_data + 2) = file_size;

      // reserved
      *(U16*)(buffer_data + 6) = 0;
      *(U16*)(buffer_data + 8) = 0;

      // pixel data offset
      *(U32*)(buffer_data + 10) = 54;

      // -------------------------
      // BITMAP INFO HEADER (40 bytes)
      // -------------------------
      U08 *info = buffer_data + 14;

      // header size
      *(U32*)(info + 0) = 40;

      // width / height
      *(I32*)(info + 4) = (I32)image.width;

      // top-down bitmap (negative height)
      *(I32*)(info + 8) = -(I32)image.height;

      // planes
      *(U16*)(info + 12) = 1;

      // bits per pixel (BGRA = 32-bit)
      *(U16*)(info + 14) = 32;

      // compression (BI_RGB)
      *(U32*)(info + 16) = 0;

      // image size (can be 0 for BI_RGB, but set explicitly for safety)
      *(U32*)(info + 20) = pixel_data_size;

      // resolution (2835 ppm = ~72 DPI)
      *(I32*)(info + 24) = 2835;
      *(I32*)(info + 28) = 2835;

      // colors used / important
      *(U32*)(info + 32) = 0;
      *(U32*)(info + 36) = 0;

 //     memory_copy(buffer_data + 54, image.dat, pixel_data_size);


      For_U64(h, image.height) {
        For_U64(w, image.width) {
          U64 dst_base = h                      * image.width + w;
          U64 src_base = (image.height - h - 1) * image.width + w;
          buffer_data[54 + 4 * dst_base + 0] = image.dat[4 * src_base + 2];
          buffer_data[54 + 4 * dst_base + 1] = image.dat[4 * src_base + 1];
          buffer_data[54 + 4 * dst_base + 2] = image.dat[4 * src_base + 0];
          buffer_data[54 + 4 * dst_base + 3] = image.dat[4 * src_base + 3];
        }
      }



      

      char name_buffer[512] = { };
      U64 name_len = stbsp_snprintf(
          name_buffer,
          512,
          "%.*s.bmp",
          str_expand(ui->state->project)
      );

      js_web_download(
          name_len,
          (U08*)name_buffer,
          file_size,
          buffer_data
      );

#endif
          
        }
      }
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
    content->draw.content_user_data = ui;

    // NOTE(cmat): Render viewport into off-screen render-target.
    cfdr_viewport_render(&content->response, content->solved.region_absolute, ui);

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

fn_internal void cfdr_ui_cmap_draw_hook(UI_Response *response, R2F draw_region, void *user_data) {
  CFDR_CMap *cmap = (CFDR_CMap *)user_data;
  if (cmap) {

    V2F region_size = r2f_size(draw_region);

    F32 cell_size = 12.f;
    U32 cells_x   = (U32)(region_size.x / cell_size) + 1;
    U32 cells_y   = (U32)(region_size.y / cell_size) + 1;

    For_U32(h, cells_y) {
      For_U32(w, cells_x) {
        g2_draw_rect(v2f(draw_region.min.x + cell_size * w, draw_region.min.y + cell_size * h), v2f(cell_size, cell_size), .color = (w + h) % 2 ? v4f(.1f, .1f, .1f, 1.f) : v4f(.5f, .5f, .5f, 1.f));
      }
    }

    g2_draw_rect(draw_region.min, r2f_size(draw_region), .mat = cmap->material);
    g2_submit_draw();
  }
}

fn_internal void cfdr_ui_cmap_panel(CFDR_UI_State *ui) {
  UI_Node *box = ui_container(str_lit("##colormap_panel"), UI_Container_Box, Axis2_Y, UI_Size_Fill, UI_Size_Fill);
  UI_Parent_Scope(box) {
    UI_Parent_Scope(ui_container(str_lit("##colormap_tab"), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fit)) {
      UI_Font_Scope(&ui->font_icon) {
        ui_label(Icon_FA_BARS);
      }

      ui_label(str_lit("Color Map"));
    }

    ui_separator(str_lit("##sep_title"));
    ui_container(str_lit("##padding_1"), UI_Container_None, Axis2_X, UI_Size_Fit, UI_Size_Fixed(5));

    CFDR_CMap *cmap = cfdr_cmap_table_get(&ui->state->cmap_table, ui->state->scene.cmap);

    ui_container(str_lit("##force_size"), UI_Container_None, Axis2_X, UI_Size_Fixed(350), UI_Size_Fit);

    UI_Node *cmap_view = ui_container(str_lit("##colormap_preview"), UI_Container_Box, Axis2_X, UI_Size_Fill, UI_Size_Fixed(40));
    cmap_view->flags |= UI_Flag_Draw_Content_Hook | UI_Flag_Draw_Clip_Content;
    cmap_view->draw.content_hook      = cfdr_ui_cmap_draw_hook;
    cmap_view->draw.content_user_data = cmap;

    if (cmap) {
      B32 update_cmap = 0;

      B32 interpolate = cmap->interpolate;
      ui_checkbox(str_lit("Interpolate"), &cmap->interpolate);
      update_cmap = update_cmap || (interpolate != cmap->interpolate);

      B32 map_opacity = cmap->map_opacity;
      ui_checkbox(str_lit("Map Opacity"), &cmap->map_opacity);
      update_cmap = update_cmap || (map_opacity != cmap->map_opacity);

      B32 data_scale = cmap->data_scale;
      ui_f32_edit(str_lit("Data Scale"), &cmap->data_scale, -1000, 1000, 0.001f);
      update_cmap = update_cmap || (data_scale != cmap->data_scale);

      B32 data_shift = cmap->data_shift;
      ui_f32_edit(str_lit("Data Shift"), &cmap->data_shift, -1000, 1000, 0.001f);
      update_cmap = update_cmap || (data_shift != cmap->data_shift);

      ui_container(str_lit("##gap_1"), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fixed(10));
      ui_separator(str_lit("##seperator_1"));
      ui_container(str_lit("##gap_2"), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fixed(10));

      ui_checkbox(str_lit("Contour Lines"), &cmap->contour_visible);
      if (cmap->contour_visible) {

        UI_Node *container = ui_container(str_lit("##container"), UI_Container_Box, Axis2_Y, UI_Size_Fill, UI_Size_Fit);
        container->layout.gap_border[Axis2_X] = 5;
        container->layout.gap_border[Axis2_Y] = 5;
        container->layout.gap_child           = 5;
        container->flags                     |= UI_Flag_Draw_Rounded;
        container->palette.idle               = hsv_u32(200, 10, 10);
        container->palette.hover              = hsv_u32(210, 10, 15);
        container->palette.down               = hsv_u32(220, 10, 15);
        UI_Parent_Scope(container) {

          ui_f32_edit(str_lit("Value"), &cmap->contour_value, 0.0f, 100.0f, 0.001f);
          ui_f32_edit(str_lit("Thickness"), &cmap->contour_thickness, 0.01f, 100.0f, 0.01f);
          ui_color_hsv(str_lit("Color"),  &cmap->contour_color);
        }
      }

      ui_container(str_lit("##gap_3"), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fixed(10));
      ui_separator(str_lit("##seperator_2"));
      ui_container(str_lit("##gap_4"), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fixed(10));


      ui_label(str_lit("Map Nodes"));

      For_I32(it, cmap->node_len) {
        char buffer[512] = { };
        stbsp_snprintf(buffer, 512, "##Container_%d", it);
        UI_Node *container = ui_container(str_from_cstr(buffer), UI_Container_Box, Axis2_X, UI_Size_Fill, UI_Size_Fit);
        UI_Parent_Scope(container) {
          stbsp_snprintf(buffer, 512, "Step %d##Label", it);
          ui_label(str_from_cstr(buffer));
          ui_container(str_from_cstr("##Padding"), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fit);

          F32 step = cmap->node_dat[it].step_t;
          ui_f32_edit_static(str_lit("##Step"), &cmap->node_dat[it].step_t, -1000, 1000, 0.001f); 
          update_cmap = update_cmap || (step != cmap->node_dat[it].step_t);

          HSVA color = cmap->node_dat[it].color;
          ui_color_hsv_fixed(str_lit("##Color"), &cmap->node_dat[it].color.hsv);
          update_cmap = update_cmap || ((color.x != cmap->node_dat[it].color.x) ||
                                        (color.y != cmap->node_dat[it].color.y) ||
                                        (color.z != cmap->node_dat[it].color.z) ||
                                        (color.w != cmap->node_dat[it].color.w));
        }
      }

      if (update_cmap) {
        cfdr_cmap_update(cmap);
      }
    }


#if 0
    if (cmap) {
      HSV c0 = cmap->c0.hsv;
      HSV c1 = cmap->c0.hsv;

      ui_color_hsv(str_lit("Color 0"), &cmap->c0.hsv);
      ui_color_hsv(str_lit("Color 1"), &cmap->c1.hsv);

      I32 interpolate_mode = cmap->interpolate;
      I32 opacity_mode     = cmap->opacity;
      ui_list(str_lit("Color Mapping"),   &cmap->interpolate, sarray_len(CFDR_CMap_Interpolate_String), CFDR_CMap_Interpolate_String); 
      ui_list(str_lit("Opacity Mapping"), &cmap->opacity,     sarray_len(CFDR_CMap_Opacity_String),     CFDR_CMap_Opacity_String); 
      ui_list(str_lit("Data Mapping"), &cmap->map_mode, sarray_len(CFDR_CMap_Map_String), CFDR_CMap_Map_String); 
      if (cmap->map_mode == CFDR_CMap_Map_Custom) {
        UI_Node *box = ui_container(str_lit("##custom_range"), UI_Container_Box, Axis2_X, UI_Size_Fill, UI_Size_Fit);
        box->palette.idle  = hsv_u32(200, 10, 10);
        box->palette.hover = hsv_u32(210, 10, 15);
        box->palette.down  = hsv_u32(220, 10, 15);
        UI_Parent_Scope(box) {
          ui_f32_edit(str_lit("Map Range##MIN"), &cmap->map_custom.x, -5000.f, +5000.f, 0.01f);
          ui_f32_edit_static(str_lit("##MAX"), &cmap->map_custom.y, -5000.f, +5000.f, 0.01f);
        }
      }
      
      if (interpolate_mode != cmap->interpolate ||
          opacity_mode     != cmap->opacity ||
          !memory_compare(&c0, &cmap->c0.hsv, sizeof(V3F)) ||
          !memory_compare(&c1, &cmap->c1.hsv, sizeof(V3F))) {
        cfdr_cmap_edit_colors(cmap, cmap->interpolate, cmap->opacity, cmap->c0, cmap->c1);
      }
    }
#endif
  }
}



fn_internal void cfdr_ui_layer_panel(CFDR_UI_State *ui) {
  UI_Parent_Scope(ui_container(str_lit("##layer_panel"), UI_Container_Box, Axis2_Y, UI_Size_Fill, UI_Size_Fit)) {
    UI_Node *tab_bar = ui_container(str_lit("##tab_bar"), UI_Container_None, Axis2_X, UI_Size_Fit, UI_Size_Fit);
    tab_bar->layout.gap_child = 2.f;
    UI_Parent_Scope(tab_bar) {
      UI_Node *object_tab = ui_container(str_lit("##object_tab"), UI_Container_Box, Axis2_X, UI_Size_Fit, UI_Size_Fit);
      object_tab->palette = UI_Theme_Active.button;
      if (ui->tab_mode == CFDR_Tab_Object) {
        object_tab->palette.idle = object_tab->palette.down;
      }

      if (object_tab->response.press) {
        ui->tab_mode = CFDR_Tab_Object;
      }

      UI_Parent_Scope(object_tab) {
        UI_Font_Scope(&ui->font_icon) { ui_label(Icon_FA_CUBES); }
        ui_label(str_lit("Objects"));
      }

      UI_Node *overlay_tab = ui_container(str_lit("##layer_tab"), UI_Container_Box, Axis2_X, UI_Size_Fit, UI_Size_Fit);
      overlay_tab->palette = UI_Theme_Active.button;

      if (ui->tab_mode == CFDR_Tab_Overlay) {
        overlay_tab->palette.idle = overlay_tab->palette.down;
      }

      if (overlay_tab->response.press) {
        ui->tab_mode = CFDR_Tab_Overlay;
      }

      UI_Parent_Scope(overlay_tab) {
        UI_Font_Scope(&ui->font_icon) { ui_label(Icon_FA_OBJECT_GROUP); }
        ui_label(str_lit("Overlays"));
      }

      UI_Node *cmap_tab = ui_container(str_lit("##cmap_tab"), UI_Container_Box, Axis2_X, UI_Size_Fit, UI_Size_Fit);
      cmap_tab->palette = UI_Theme_Active.button;

      UI_Parent_Scope(cmap_tab) {
        UI_Font_Scope(&ui->font_icon) { ui_label(Icon_FA_OBJECT_GROUP); }
        ui_label(str_lit("Color Map"));
      }

      if (ui->tab_mode == CFDR_Tab_CMap) {
        cmap_tab->palette.idle  = cmap_tab->palette.down;
      }

      if (cmap_tab->response.press) {
        ui->tab_mode = CFDR_Tab_CMap;
      }
    }

    ui_separator(str_lit("##sep_title"));
 
    UI_Node *layer_container = ui_container(str_lit("##layer_container"), UI_Container_None, Axis2_Y, UI_Size_Fill, UI_Size_Fit);
    UI_Parent_Scope(layer_container) {
      layer_container->layout.gap_border[Axis2_X] = 2;
      layer_container->layout.gap_border[Axis2_Y] = 5;
      layer_container->layout.gap_child = 0;

      I32 layer_at = 0;
      if (ui->tab_mode == CFDR_Tab_Overlay) {
        for (CFDR_Overlay_Node *it = ui->state->overlay.first; it; it = it->next, ++layer_at) {
          void *active = ui->state->overlay.active;
          cfdr_ui_layer_entry(ui, it->tag, &it->visible, layer_at, it, &active, Icon_FA_FONT);
          ui->state->overlay.active = active;
        }
      } else if (ui->tab_mode == CFDR_Tab_Object) {
        for (CFDR_Object_Node *it = ui->state->scene.first; it; it = it->next, ++layer_at) {
          void *active = ui->state->scene.active;
          cfdr_ui_layer_entry(ui, it->tag, &it->visible, layer_at, it, &active, Icon_FA_CUBE);
          ui->state->scene.active = active;
        }
      } else if (ui->tab_mode == CFDR_Tab_CMap) {
        cfdr_ui_cmap_panel(ui);
      }
    }
  }

  if (pl_input()->keyboard.state[PL_KB_Escape].press) {
    ui->state->scene.active   = 0;
    ui->state->overlay.active = 0;
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

      var_local_persist Str material_list[] = { str_lit("Flat"), str_lit("Matcap"), str_lit("Sample"), };
      ui_list(str_lit("Material"), &object->material, sarray_len(CFDR_Material_String_List), CFDR_Material_String_List); 

      if (object->material != CFDR_Material_Sample) {
        ui_color_hsv  (str_lit("Color"),    &object->color.hsv);
        ui_f32_edit   (str_lit("Opacity"),  &object->color.a, 0.0f, 1.0f, 0.01f);
      }

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

      UI_Parent_Scope(ui_container(str_lit("##rotate"), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fit)) {
        ui_label(str_lit("Rotate"));
        ui_container(str_lit("##padding"), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fit);
        ui_f32_edit_static(str_lit("##x"), &object->rotate.x, -5000.f, 5000.f, 0.05f);
        ui_f32_edit_static(str_lit("##y"), &object->rotate.y, -5000.f, 5000.f, 0.05f);
        ui_f32_edit_static(str_lit("##z"), &object->rotate.z, -5000.f, 5000.f, 0.05f);
      }

      if (object->flags & CFDR_Object_Flag_Draw_Volume) {
        ui_separator(str_lit("##separator_2"));
        ui_label(str_lit("Volume"));
        ui_separator(str_lit("##separator_3"));
        ui_f32_edit(str_lit("Volume Density"), &object->volume_density, 0.f, 10.f, 0.005f);
        ui_f32_edit(str_lit("Volume Saturate"), &object->volume_saturate, 0.f, 10.f, 0.005f);
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

 
  if (ui->tab_mode == CFDR_Tab_Overlay) {
    cfdr_ui_property_overlay(ui);
  } else if (ui->tab_mode == CFDR_Tab_Object) {
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
    // cfdr_ui_cmap_panel      (ui);
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

      UI_Parent_Scope(ui_container(str_lit("##container_1.5"), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fill)) {
        // cfdr_ui_cmap_panel(ui);
        cfdr_ui_viewport_all(ui);
      }

      UI_Parent_Scope(ui_container(str_lit("##container_2"), UI_Container_None, Axis2_X, UI_Size_Fill, UI_Size_Fit)) {
        // cfdr_ui_resource_panel(ui);
        // cfdr_ui_log_panel(ui);
      }
    }
  }
}

fn_internal void cfdr_ui(CFDR_UI_State *ui) {
  cfdr_ui_update_fonts(ui);

  ui->frame_rate_buffer[ui->frame_rate_at] = pl_display()->frame_delta;
  ui->frame_rate_at = (ui->frame_rate_at + 1) % sarray_len(ui->frame_rate_buffer);
  ui->frame_rate_avg = 0;
  For_U32(it, sarray_len(ui->frame_rate_buffer)) {
    ui->frame_rate_avg += ui->frame_rate_buffer[it];
  }

  ui->frame_rate_avg /= sarray_len(ui->frame_rate_buffer);
  ui->frame_rate_avg = 1.f / ui->frame_rate_avg;

  if (ui->dark_mode) {
    UI_Theme_Active = UI_Theme_Dark;
  } else {
    UI_Theme_Active = UI_Theme_Light;
  }

  UI_Font_Scope(&ui->font_text) {
    if (pl_input()->keyboard.state[PL_KB_F].press && pl_input()->keyboard.state[PL_KB_Shift_Left].down) {
      ui->fullscreen = !ui->fullscreen;
    }

    if (pl_input()->keyboard.state[PL_KB_T].press && pl_input()->keyboard.state[PL_KB_Shift_Left].down) {
      ui->dark_mode = !ui->dark_mode;
    }


    if (pl_input()->keyboard.state[PL_KB_D].press && pl_input()->keyboard.state[PL_KB_Shift_Left].down) {
      ui->profile_view = !ui->profile_view;
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

  // NOTE(cmat): Render ui onto backbuffer.
  r_target_begin(0, 0, 0); {
    g2_draw_rect(v2f(0, 0), pl_display()->resolution, .color = v4f(0, 0, 0, 1));
    ui_frame_flush();
    g2_frame_flush();
    r_frame_flush();
  } r_target_end();
}

