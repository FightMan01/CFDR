#include "core/core_build.h"
#include "core/core_build.c"

#include "base/base_build.h"
#include "base/base_build.c"
#include "base/base_test.c"

#include "platform/platform_build.h"
#include "platform/platform_build.c"

#include "render/render_build.h"
#include "render/render_build.c"

#include "texture_cache/texture_cache_build.h"
#include "texture_cache/texture_cache_build.c"

#include "font/font_build.h"
#include "font/font_build.c"

#include "graphics/graphics_build.h"
#include "graphics/graphics_build.c"

#include "ui/ui_build.h"
#include "ui/ui_build.c"

#include "geometry/geometry_build.h"
#include "geometry/geometry_build.c"

#include "tokenizer/tokenizer.h"
#include "tokenizer/tokenizer.c"

#include "http/http_wasm.c"
#include "mesh/mesh_stl.h"

#include "ug_mesh.c"
#include "su2.c"

var_global Str Project_Path = { };

#include "cfdr/cfdr_build.h"
#include "cfdr/cfdr_build.c"

var_global Arena          Permanent_Storage   = { };
var_global G2_Font        UI_Font_Text        = { };
var_global CFDR_UI_State  CFDR_UI             = { };
var_global CFDR_Resource  Project_File        = { };
var_global CFDR_State     State               = { };

fn_internal void init_frame(PL_Render_Context *render_context) {
  arena_init(&Permanent_Storage);

  lz4_init();
  r_init(render_context);
  g2_init();
  ui_init();

  cfdr_state_init(&State);
  cfdr_ui_init(&CFDR_UI, &State);
}

fn_internal V2F place_point(UG_Mesh *mesh, F32 largest_axis, F32 scale, V2F offset, V2F p) {
  p = v2f_sub(p, mesh->bounds.min);
  p = v2f_div(p, largest_axis);
  p = v2f_mul(scale, p);
  p = v2f_add(p, offset);
  return p;
}

fn_internal void next_frame(B32 first_frame, PL_Render_Context *render_context) {
  If_Unlikely(first_frame) {
    init_frame(render_context);

    U08 buffer[2000] = { };
    js_web_current_url(sarray_len(buffer) - 1, buffer);
    Str url = str_from_cstr((char *)buffer);

    Str project_file  = { };
    I32 anchor_at     = str_find(url, str_lit("?project="));
    if (anchor_at >= 0) {
      project_file = str_slice(url, anchor_at + str_lit("?project=").len, url.len - anchor_at - str_lit("?project=").len);
    }

    log_info("project file: %.*s", str_expand(project_file));
    Project_Path = arena_push_str(&Permanent_Storage, project_file);

    cfdr_resource_init(&Project_File, Project_Path);
  }

  CFDR_Resource_Data data = { };
  if (cfdr_resource_fetch(&Project_File, &data)) {
    cfdr_eval(&State, str(data.bytes_total, data.bytes_data));
  }

  cfdr_ui(&CFDR_UI);
  ui_frame_flush();
 
  g2_frame_flush();
  r_frame_flush();

  Resource_Downloading = 0;
}

fn_internal void log_co_context(void) {
  Log_Zone_Scope("hardware info") {
    log_info("CPU: %.*s",            str_expand(co_context()->cpu_name));
    log_info("Logical Cores: %llu",  co_context()->cpu_logical_cores);
    log_info("Page Size: %$$llu",    co_context()->mmu_page_bytes);
    log_info("RAM Capacity: %$$llu", co_context()->ram_capacity_bytes);
  }
}

fn_internal void pl_entry_point(Array_Str command_line, PL_Bootstrap *boot) {
  boot->next_frame = next_frame;
  boot->title = str_lit("CFDR");

  logger_push_hook(logger_write_entry_standard_stream, logger_format_entry_minimal);
  log_co_context();
} 

