// $File: test.c
// $Last-Modified: "2025-08-29 11:14:12"
// $Author: Matyas Constans.
// $Notice: (C) Matyas Constans, Horvath Zoltan 2025 - All Rights Reserved.
// $License: You may use, distribute and modify this code under the terms of the MIT license.
// $Note: Test file.

// TODO(cmat):
// OK - 0. Camera movement.
// OK - 1. Load FAU 3D model.
// OK - 1.5. Metal Depth Buffer.
// OK - 1.75. Variable-based coloring.
// OK - 2. Animated model in time.
// - 3. Animation: interpolation over time.
// - 4. Menu-bar. Time slider.
// - 5. Vector field.

#include "alice/core/core_module.h"
#include "alice/core/core_module.c"

#include "alice/gfx/gfx_module.h"
#include "alice/gfx/gfx_module.c"

#include "alice/geo/geo_module.h"
#include "alice/geo/geo_module.c"

#include "alice/cfdr/cfdr_module.h"
#include "alice/cfdr/cfdr_module.c"

#define ENCAS_IMPLEMENTATION
#include "cfdr_external/encas.h"
#include "cfdr_layer.h"
#include "cfdr_menu.h"

#include "cfdr_layer.c"
#include "cfdr_menu.c"

Str08 case_file = {};

GFX_Font Font  = {};
Pool Pool_Font = {};

CFDR_Layer_Text  layer_text  = {};
CFDR_Layer_Image layer_image = {};
CFDR_Layer_Graph layer_graph = {};
CFDR_Menu        menu = {};

GPU_Buffer   model_world = {};
GPU_Pipeline model_pipeline = {};
F32          model_camera_elevation = .25f * PI_F32;
F32          model_camera_azimuth = .25f * PI_F32;
F32          model_camera_radius  = 4.0f;

void color_bar(GFX_Font *font, V2 bottom_left, V2 size, Str08 label, V2F range) {
  // GFX_IM_2D_Push_Rect(bottom_left, size, .color = v4(.5f, .5f, .5f, 1.f));
  F32 title_height = size.y * .30f;
  F32 title_border = size.y * .075f;
  F32 label_height = size.y * .20f;
  F32 label_border = size.y * 0.05f;
  F32 bar_height   = size.y - title_height - label_height - title_border - label_border;
  F32 label_font_height = .75f * label_height;
  
  V2 label_from = bottom_left;
  V2 bar_from   = v2_add(label_from, v2(0, label_height + label_border));
  V2 title_from = v2_add(bar_from, v2(0, bar_height + title_border));

  // GFX_IM_2D_Push_Rect(label_from, v2(size.x, label_height), .color = v4(.8f, .5f, .5f, 1.f));
  // GFX_IM_2D_Push_Rect(title_from, v2(size.x, title_height), .color = v4(.5f, .5f, .8f, 1.f));

  Iter_U32(it, 0, 512) {
    F32 t = (F32)it / (F32)512;
    V3 hsv = gfx_rgb_from_hsv(v3(t, 1, 1));
    GFX_IM_2D_Push_Rect(v2_add(bar_from, v2(it * size.x / 512.0f, 0)),
                        v2(size.x / 512.0f, bar_height), .color = v4(hsv.x, hsv.y, hsv.z, 1));
  }
  
  F32 font_width = gfx_font_text_width(font, label, title_height);

  GFX_IM_2D_Push_Text(label, &Font, v2(title_from.x + (size.x - font_width) / 2, title_from.y), title_height);

  F32 label_width = gfx_font_text_width(font, Str08_Literal("0.00e00"), label_font_height);
  I32 subdiv      = (I32)(size.x / (label_width * 1.5f));

  Iter_U32(it, 0, subdiv + 1) {
    F32 x = (size.x / (F32)subdiv) * it;
    F32 t = (F32)(it) / (F32)(subdiv + 1);
    
    char buffer[16];
    U32 buffer_len = stbsp_snprintf(buffer, 16, "%.2e", (1 - t) * range.x + t * range.y);
    Str08 label = { .txt = (U08 *)buffer, .len = buffer_len };
    
    F32 text_w = gfx_font_text_width(font, label, label_font_height);
    GFX_IM_2D_Push_Text(label, &Font, v2(label_from.x + x - .5f * text_w, label_from.y), label_font_height);
    GFX_IM_2D_Push_Rect(v2(label_from.x + x - .5f * Max(1, .05 * bar_height), bar_from.y - .25f * label_height), v2(Max(1, .05 * bar_height), .5f * label_height));
  }
}

typedef struct {
  GPU_Buffer         vertices;
  GPU_Buffer         indices;
  U32                vertex_count;
  U32                index_count;
  GPU_Vertex_XCU_3D *vertex_data;
  Encas_Case        *encas;
  Encas_MeshArray   *encas_mesh;
  Encas_ShellParams  encas_shell;
} CFDR_Shell;

Global_Variable CFDR_Shell Model_Shell;

CFDR_Shell cfdr_model_load_shell(void) {
  CFDR_Shell model = {};

  char file_buffer[512] = {};
  Memory_Copy(file_buffer, case_file.txt, case_file.len)
  
  Encas_Case *encas           = Encas_ReadCase(file_buffer);
  Encas_MeshArray *encas_mesh = Encas_LoadGeometry(encas, 0);

  Variable_Name_Count = encas->variable->len;
  Iter_U32(it, 0, encas->variable->len) {
    Log_Info("%u # Name :: %.*s, Type :: %u",
             it,
             encas->variable->elems[it]->description.len,
             encas->variable->elems[it]->description.buffer,
             encas->variable->elems[it]->type);

    Variable_Name_Array[it] = (Str08) {
      .txt = encas->variable->elems[it]->description.buffer,
      .len = encas->variable->elems[it]->description.len
    };

    Variable_Dim_Array[it] = encas->variable->elems[it]->type == 2 ? 1 : 3;
  }

  Timestamp_Count = encas->times->elems[0]->number_of_steps;
  
  Encas_ShellParams params = {
    .vbo = 0,
    .vbo_size = 0,
    .vbo_orig_idx = 0,
    .ebo = 0,
    .ebo_size = 0,
    .tria_global_idx = 0,
    .global_ebo_size = 0,
  };

  F32 *var_vbo = 0;
  Encas_LoadGeometryShell(encas, encas_mesh, &params);

  Pool work_pool = {};
  Pool_Init(&work_pool);

  V3 min_bounds = v3(F32_Maximum_Positive, F32_Maximum_Positive, F32_Maximum_Positive);
  V3 max_bounds = v3(F32_Smallest_Negative, F32_Smallest_Negative, F32_Smallest_Negative);

  Iter_U32(it, 0, params.vbo_size) {
    V3 X = v3(params.vbo[3*it + 0], params.vbo[3*it+1], params.vbo[3*it+2]);
    min_bounds = v3(Min(X.x, min_bounds.x), Min(X.y, min_bounds.y), Min(X.z, min_bounds.z));
    max_bounds = v3(Max(X.x, max_bounds.x), Max(X.y, max_bounds.y), Max(X.z, max_bounds.z));
  } 

  F32 largest_axis = Max(max_bounds.z - min_bounds.z, Max(max_bounds.x - min_bounds.x, max_bounds.y - min_bounds.y));
  
  GPU_Vertex_XCU_3D *model_vertex_data = Pool_Push_Array(&work_pool, GPU_Vertex_XCU_3D, params.ebo_size);
  Iter_U32(it, 0, params.ebo_size) {

    V3 X_src = v3(params.vbo[3 * params.ebo[it] + 0], params.vbo[3 * params.ebo[it]+1], params.vbo[3 * params.ebo[it]+2]);
      
    V3 X = {};
    X.x = X_src.x - min_bounds.x + .5f * (largest_axis - (max_bounds.x - min_bounds.x));
    X.y = X_src.y - min_bounds.y + .5f * (largest_axis - (max_bounds.y - min_bounds.y));
    X.z = X_src.z - min_bounds.z + .5f * (largest_axis - (max_bounds.z - min_bounds.z));

    X.x /= largest_axis;
    X.y /= largest_axis;
    X.z /= largest_axis;
       
    X.x = X.x * 2.f - 1.f;
    X.y = X.y * 2.f - 1.f;
    X.z = X.z * 2.f - 1.f;
    
    model_vertex_data[it] = (GPU_Vertex_XCU_3D) {
      .X = v3(X.x, X.z, X.y),
      .C = 0xFF000000,
      .U = v2(0, 0),
      .Texture_Slot = 0,
    };
  }

  U32 *model_index_data = Pool_Push_Array(&work_pool, U32, params.ebo_size);
  Iter_U32(it, 0, params.ebo_size) model_index_data[it] = it;
  
  model.vertices = gpu_buffer_allocate(&(GPU_Buffer_Config) { .capacity = sizeof(GPU_Vertex_XCU_3D) * params.ebo_size, .dynamic = 0 });
  gpu_buffer_download(&model.vertices, 0, sizeof(GPU_Vertex_XCU_3D) * params.vbo_size, model_vertex_data);

  model.indices = gpu_buffer_allocate(&(GPU_Buffer_Config) { .capacity = sizeof(U32) * params.ebo_size, .dynamic = 0 });
  gpu_buffer_download(&model.indices, 0, sizeof(U32) * params.ebo_size, model_index_data);

  model.index_count = params.ebo_size;
  model.vertex_count = params.ebo_size; // params.vbo_size;
  model.encas = encas;
  model.encas_mesh = encas_mesh;
  model.encas_shell = params;
  model.vertex_data = model_vertex_data;
  
  return model;
}

F32 Var_Min = 0;
F32 Var_Max = 0;

void cfdr_model_recolor_shell(CFDR_Shell *shell, U32 var_idx, U32 time_idx) {
  F32 *var_vbo = 0;
  //Encas_LoadVariableOnShell_Vertices(shell->encas, shell->encas_mesh, var_idx, time_idx, &shell->encas_shell, &var_vbo);
  Encas_LoadVariableOnShell_Elements(shell->encas, shell->encas_mesh, var_idx, time_idx, &shell->encas_shell, &var_vbo);
  
  F32 scalar_min = F32_Maximum_Positive;
  F32 scalar_max = F32_Smallest_Negative;


  U32 tri_count = shell->vertex_count / 3;
  
  Iter_U32(it, 0, tri_count) {
    F32 x = var_vbo[it];

    if (Variable_Dim_Array[var_idx] == 3) {
      x = square_root(var_vbo[it]                 * var_vbo[it] +
                      var_vbo[it + tri_count]     * var_vbo[it + tri_count] +
                      var_vbo[it + 2 * tri_count] * var_vbo[it + 2 * tri_count]);
    }
    
    
    scalar_min = Min(x, scalar_min);
    scalar_max = Max(x, scalar_max);
  }

  Var_Min = scalar_min;
  Var_Max = scalar_max;
  
  Iter_U32(it, 0, shell->vertex_count) {
    F32 x = var_vbo[it / 3];
    if (Variable_Dim_Array[var_idx] == 3) {
      x = square_root(var_vbo[it/3]                 * var_vbo[it/3] +
                      var_vbo[it/3 + tri_count]     * var_vbo[it/3 + tri_count] +
                      var_vbo[it/3 + 2 * tri_count] * var_vbo[it/3 + 2 * tri_count]);
    }
    
    F32 normalized_scalar = (x - scalar_min) / (scalar_max - scalar_min);
    GFX_RGB C = gfx_rgb_from_hsv(v3(normalized_scalar, 1.f, 1.f));
    shell->vertex_data[it].C = gfx_abgr_u32_from_rgba_f128(v4(C.r, C.g, C.b, 1.f));
  }
 
  ENCAS_FREE(var_vbo);

  gpu_buffer_download(&shell->vertices, 0, sizeof(GPU_Vertex_XCU_3D) * shell->vertex_count, shell->vertex_data);
}

#if 0

void model_load(void) {
  Encas_Case *encas           = Encas_ReadCase("export_2/combined.case");
  Encas_MeshArray *encas_mesh = Encas_LoadGeometry(encas, 0);

  Iter_U32(it, 0, encas->variable->len) {
    Log_Info("%u # Name :: %.*s, Type :: %u",
             it,
             encas->variable->elems[it]->description.len,
             encas->variable->elems[it]->description.buffer,
             encas->variable->elems[it]->type);
  }

  U32 last_time_index = encas->times->elems[0]->number_of_steps - 1;
  Iter_U32(it, 0, encas->times->elems[0]->number_of_steps) {
    Log_Info("Time :: %f", encas->times->elems[0]->time_values[it]);
  }
  
  Encas_ShellParams params = {
    .vbo = 0,
    .vbo_size = 0,
    .vbo_orig_idx = 0,
    .ebo = 0,
    .ebo_size = 0,
    .tria_global_idx = 0,
    .global_ebo_size = 0,
  };

  char var_buffer[512] = {};
  snprintf(var_buffer, 511, "%.*s",
           encas->variable->elems[0]->description.len,
           encas->variable->elems[0]->description.buffer);

  F32 *var_vbo = 0;
  Encas_LoadGeometryShell(encas, encas_mesh, &params);
  Encas_LoadVariableOnShell_Vertices(encas, encas_mesh, 0, last_time_index, &params, &var_vbo); 

  Pool work_pool = {};
  Pool_Init(&work_pool);

  V3 min_bounds = v3(F32_Maximum_Positive, F32_Maximum_Positive, F32_Maximum_Positive);
  V3 max_bounds = v3(F32_Smallest_Negative, F32_Smallest_Negative, F32_Smallest_Negative);
  Iter_U32(it, 0, params.vbo_size) {
    V3 X = v3(params.vbo[3*it + 0], params.vbo[3*it+1], params.vbo[3*it+2]);
    min_bounds = v3(Min(X.x, min_bounds.x), Min(X.y, min_bounds.y), Min(X.z, min_bounds.z));
    max_bounds = v3(Max(X.x, max_bounds.x), Max(X.y, max_bounds.y), Max(X.z, max_bounds.z));
  } 


  F32 scalar_min = F32_Maximum_Positive;
  F32 scalar_max = F32_Smallest_Negative;

  Iter_U32(it, 0, params.vbo_size) {
    scalar_min = Min(var_vbo[it], scalar_min);
    scalar_max = Max(var_vbo[it], scalar_max);
  }
  
  Log_Info("Bounds (%f %f %f), (%f %f %f)", min_bounds.x, min_bounds.y, min_bounds.z,
           max_bounds.x, max_bounds.y, max_bounds.z);

  F32 largest_axis = Max(max_bounds.z - min_bounds.z, Max(max_bounds.x - min_bounds.x, max_bounds.y - min_bounds.y));
  
  GPU_Vertex_XCU_3D *model_vertex_data = Pool_Push_Array(&work_pool, GPU_Vertex_XCU_3D, params.vbo_size);
  Iter_U32(it, 0, params.vbo_size) {
    V3 X_src = v3(params.vbo[3*it + 0], params.vbo[3*it+1], params.vbo[3*it+2]);

    V3 X = {};
    X.x = ((X_src.x - min_bounds.x) / largest_axis * 2.f) - 1.f;
    X.z = ((X_src.y - min_bounds.y) / largest_axis * 2.f) - 1.f;
    X.y = ((X_src.z - min_bounds.z) / largest_axis * 2.f) - 1.f;

    // Log_Info("%f, %f, %f", X.x, X.y, X.z);
    F32 normalized_scalar = (var_vbo[it] - scalar_min) / (scalar_max - scalar_min);
    GFX_RGB C = gfx_rgb_from_hsv(v3(normalized_scalar, 1.f, 1.f));

    
    model_vertex_data[it] = (GPU_Vertex_XCU_3D) {

      .X = X,
      .C = gfx_abgr_u32_from_rgba_f128(v4(C.r, C.g, C.b, 1.f)),
      .U = v2(0, 0),
      .Texture_Slot = 0,
    };
  }
  
  model_vertices = gpu_buffer_allocate(&(GPU_Buffer_Config) { .capacity = sizeof(GPU_Vertex_XCU_3D) * params.vbo_size, .dynamic = 0 });
  gpu_buffer_download(&model_vertices, 0, sizeof(GPU_Vertex_XCU_3D) * params.vbo_size, model_vertex_data);

  model_indices = gpu_buffer_allocate(&(GPU_Buffer_Config) { .capacity = sizeof(U32) * params.ebo_size, .dynamic = 0 });
  gpu_buffer_download(&model_indices, 0, sizeof(U32) * params.ebo_size, params.ebo);

  model_index_count = params.ebo_size;
}

#endif

void update_and_render(void *user_data) {
  gfx_im_2D_frame_begin();
  
  Local_Persist B32 init = 0;
  if (!init) {
    init = 1;
    Pool_Init(&Pool_Font);
    Font = gfx_font_load(&Pool_Font, "noto_serif.png", "noto_serif.bin");

    // model_load();
    Model_Shell = cfdr_model_load_shell();
    
    model_world = gpu_buffer_allocate(&(GPU_Buffer_Config) { .capacity = sizeof(GPU_Constant_Buffer_World_3D), .dynamic = 0 });
    
    model_pipeline = gpu_pipeline_create(&(GPU_Pipeline_Config) {
        .shader = GPU_Shader_Flat_3D,
        .format = GPU_Vertex_Format_XCU_3D,
      });
    
    layer_text = (CFDR_Layer_Text) {
      .font = &Font,
      .string = Str08_Literal("Strasbourg Demo - t = 130"),
      .underline = 1,
    };

    stbi_set_flip_vertically_on_load(1);
    I32 image_width, image_height;
    U08 *image_data = stbi_load("logo.png", &image_width, &image_height, 0, 4);
    GPU_Texture image_texture = gpu_texture_allocate(&(GPU_Texture_Config) {
      .format = GPU_Texture_Format_RGBA_U08_Normalized,
      .width  = image_width,
      .height = image_height,
    });

    gpu_texture_download(&image_texture, image_data);
    layer_image = (CFDR_Layer_Image) {
      .texture = image_texture,
      .width   = image_width,
      .height  = image_height,
    };

    layer_graph = (CFDR_Layer_Graph) {
      .title_font = &Font,
      .label_font = &Font,

      .title = Str08_Literal("FitzHugh–Nagumo Model"),
      .x_axis_label = Str08_Literal("Nodes"),
      
      .aspect_ratio = 1.5f,
    };


    cfdr_menu_init(&menu, &Font);
  }

  Local_Persist B32 cache_init = 0;
  Local_Persist U32 cache_var  = 0;
  Local_Persist U32 cache_time = 0;
 
  if (!cache_init) {
    cache_init = 1;
    cache_var =  Surface_Var_IDX_Array[0];
    cache_time = Timestamp_At;
    cfdr_model_recolor_shell(&Model_Shell, Surface_Var_IDX_Array[0], 130);
  }

  if (cache_var  != Surface_Var_IDX_Array[0] || cache_time != Timestamp_At) {
    cache_var =  Surface_Var_IDX_Array[0];
    cache_time = Timestamp_At;
    cfdr_model_recolor_shell(&Model_Shell, Surface_Var_IDX_Array[0], 130);
  }
  
  
  GPU_Command_Draw draw = {
    .index_count           = Model_Shell.index_count,
    .index_buffer_offset   = 0,
    .vertex_buffer         = Model_Shell.vertices,
    .index_buffer          = Model_Shell.indices,
    .pipeline              = model_pipeline,
    .texture_slots         = { },
    .sampler_slots         = { },
    .constant_buffer_count = 1,
    .constant_buffers      = model_world,
    .depth_testing         = 1,
  };

  Iter_U32(it, 0, GPU_Texture_Slot_Count) { draw.texture_slots[it] = GPU_Texture_White;        }
  Iter_U32(it, 0, GPU_Sampler_Slot_Count) { draw.sampler_slots[it] = GPU_Sampler_Linear_Clamp; }

  if (platform_input()->mouse.right.down) {
    model_camera_azimuth   -= .005f * platform_input()->mouse.position_delta.x;
    model_camera_elevation += .005f * platform_input()->mouse.position_delta.y;
  }

  model_camera_radius -= .1f * platform_input()->mouse.scroll_delta.y;
  model_camera_radius  = Clamp(model_camera_radius, 0.5f, 8.f);
  
  model_camera_elevation = Clamp(model_camera_elevation, .1f, PI_F32 - .1f);
  
  V3 camera_position = v3(model_camera_radius * cosf(model_camera_azimuth) * sinf(model_camera_elevation),
                          model_camera_radius * cosf(model_camera_elevation),
                          model_camera_radius * sinf(model_camera_azimuth) * sinf(model_camera_elevation));
  V3 camera_look_at  = v3(0, 0, 0);  
  V3 camera_up       = v3(0, 1, 0);
  
  V3 camera_basis_z = v3_noz(v3_sub(camera_look_at, camera_position));
  V3 camera_basis_x = v3_noz(v3_cross(camera_up, camera_basis_z));
  V3 camera_basis_y = v3_noz(v3_cross(camera_basis_z, camera_basis_x));
  
  M4 model_view       = m4_look_at(camera_basis_x, camera_basis_y, camera_basis_z, camera_position); 
  M4 model_projection = m4_projection(degrees_to_radians(60.f), platform_input()->display.aspect_ratio, .001f, 10.f);
  
  GPU_Constant_Buffer_World_3D world_3D = {
    .World_View_Projection = m4_transpose(m4_mul(model_view, model_projection)),
    .Eye_Position          = v3(0, 0, 0),
  };
  
  gpu_buffer_download(&model_world, 0, sizeof(world_3D), &world_3D);
  gpu_command_draw(&draw);

 
#if 1
  
  CFDR_Layer layer_array[] = {
    (CFDR_Layer) {
      .type       = CFDR_Layer_Type_Text,
      .layout_x   = CFDR_Layer_Layout_Center,
      .layout_y   = CFDR_Layer_Layout_Top,
      .margin_x   = { .metric = CFDR_Layer_Scale_Metric_Display_Ratio, .value = 1.0 },
      .margin_y   = { .metric = CFDR_Layer_Scale_Metric_Display_Ratio, .value = 1.0 },
      .scale      = { .metric = CFDR_Layer_Scale_Metric_Display_Ratio, .value = 6.0 },
      .visible    = 1,
      .color      = v4(.98f, .98f, .8f, 1),
      .layer_text = &layer_text,
    },

    
    (CFDR_Layer) {
      .type       = CFDR_Layer_Type_Image,
      .layout_x   = CFDR_Layer_Layout_Left,
      .layout_y   = CFDR_Layer_Layout_Bottom,
      .margin_x   = { .metric = CFDR_Layer_Scale_Metric_Display_Ratio, .value = 1.0 },
      .margin_y   = { .metric = CFDR_Layer_Scale_Metric_Display_Ratio, .value = 1.0 },
      .scale      = { .metric = CFDR_Layer_Scale_Metric_Display_Ratio, .value = 8.0 },
      .visible    = 1,
      .color      = v4(1, 1, 1, 1),
      .layer_image = &layer_image,
    },

    #if 0
    (CFDR_Layer) {
      .type       = CFDR_Layer_Type_Graph,
      .layout_x   = CFDR_Layer_Layout_Center,
      .layout_y   = CFDR_Layer_Layout_Center,
      .margin_x   = { .metric = CFDR_Layer_Scale_Metric_Display_Ratio, .value = 1.0 },
      .margin_y   = { .metric = CFDR_Layer_Scale_Metric_Display_Ratio, .value = 1.0 },
      .scale      = { .metric = CFDR_Layer_Scale_Metric_Display_Ratio, .value = 80.0 },
      .visible    = 1,
      .color      = v4(1, 1, 1, 1),
      .layer_graph = &layer_graph,
    },
    #endif

  };

  V2 color_bar_size = v2(4, 1);
  color_bar_size.x *= 0.1f * platform_input()->display.size.y;
  color_bar_size.y *= 0.1f * platform_input()->display.size.y;
  color_bar(&Font, v2(platform_input()->display.size.x - color_bar_size.x - 100, 25), color_bar_size, Variable_Name_Array[Surface_Var_IDX_Array[0]], v2(Var_Min, Var_Max));

  F32 menu_px     = 60;
  F32 timeline_px = 60;
  
  R2 content_region  = { .min = v2(0, 0), .max = v2(platform_input()->display.size.x, platform_input()->display.size.y - menu_px - timeline_px) };
  R2 timeline_region = { .min = v2(0, content_region.max.y), .max = v2(platform_input()->display.size.x, platform_input()->display.size.y - menu_px) };
  R2 menu_region     = { .min = v2(0, timeline_region.max.y), .max = v2(platform_input()->display.size.x, platform_input()->display.size.y ) };
  
  Iter_U32(it, 0, Stack_Array_Size(layer_array)) {
    cfdr_layer_render(&layer_array[it], content_region);
  }

  Local_Persist F32 time = 0;
  cfdr_timeline_render(&Font, &time, v2(0, 11), timeline_region);
  cfdr_menu_render(&menu, menu_region);
  
  gfx_im_2D_frame_end();
#endif
}

void platform_entry_point(Platform_Entry_Config *entry_config, Array_Str08 *command_line) {
  entry_config->next_frame = update_and_render;
  logger_push_hook(Logger_Default_Write_Hook, Logger_Default_Format_Hook);

  case_file = command_line->dat[1]; 
}
