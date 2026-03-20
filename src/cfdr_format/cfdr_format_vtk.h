
fn_internal void cfdr_format_vti_load_sub(Str content, Arena *arena) {
  Scratch_Scope(&scratch, arena) {

  }
}

tpyedef struct VTI_Error_Node {
  struct VTI_Error_Node *next;
  I32 line_at;
  I32 char_at;
  Str message;
} VTI_Error_Node;

typedef struct VTI_Scan {
  Str stream;
  U64 at;
  VIT_Error_Node *first;
  VIT_Error_Node *last;
} Str_Scan;

typedef struct VTI_State {
  // ...
} VTI_State;

fn_internal void vti_scan_require(VTI_Scan *scan, Arena *arena, Str match) {
  vti_scan_skip_whitespace(scan);

  if (at + match.len < arena->stream.len && str_equals(str(match.len, scan->stream.txt + at), match)) {
  } else {
    VTI_Error_Node *error = arena_push_type(arena, VTI_Error_Node);
    queue_push(scan->first, scan->last, error);
  }
}

typedef struct VTI_Field {
  Str  label;
  U32  channels;
  V3F *volume;
} VTI_Field;

typedef struct VTI_Result {
  V3I        dimension;
  U32        field_count;
  VTI_Field *field_array;
} VTI_Result;

typedef struct VTI_Value {
  VTI_Value_Type type;
} VTI_Value;

fn_internal VTI_Value cfdr_format_vti_eval(VTI_Result *result, Arena *work_arena, Arena *perm_arena, VTI_Scan *scan, B32 error) {
  if (!error) {
    VTI_Value result = { };

    vti_scan_require(&scatch, work_arena, str_lit("<"));
    Str identifier = vti_scan_identifier(&scratch, work_arena);

    if (str_equals(identifier, str_lit("ImageData"))) {
      result = cfdr_format_vti_eval_image_data(...);

    } else if (str_equals(identifier, str_lit("Cells"))) {
      for (...) {
        cfdr_format_vti_eval_cells(result, ...)
      }

    } else {
      // error
    }

    vti_scan_require(&scratch, work_arena, identifier);
    vti_scan_require(&scratch, work_arena, str_lit(">"));

    for (VTI_Error_Node *it = scan->first; it; it = it->next) {
      log_error("%.*s %d:%d: %.*s", str_expand(content_name), it->line, it->char, str_expand(it->message));
    }

    if (scan->first) {
      error = 1;
    }
  }

  return result;
}

fn_internal VTI_Result cfdr_format_vti_load(Str content_name, Str content, Arena *arena) {
  Scratch scratch = { };
  Scratch_Scope(&scratch, arena) {
    // vti_scan_require  (&scan, scratch.arena, str_lit("<"));
    // vti_scan_str      (&scan, scratch.arena, str_lit("ImageData"));
    // ...

  }
}
