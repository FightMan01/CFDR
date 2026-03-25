typedef struct Scan_Error {
  struct Scan_Error  *next;
  U08                     line_at;
  U08                     char_at;
  Str                     message;
} Scan_Error;

typedef struct Scan {
  Arena          *arena;
  Str             stream;
  U32             at;
  U32             line_at;
  U32             char_at;
  Scan_Error     *error_first;
  Scan_Error     *error_last;
} Scan;

fn_internal void scan_init(Scan *scan, Arena *arena, Str stream) {
  zero_fill(scan);
  scan->arena   = arena;
  scan->stream  = stream;
  scan->line_at = 1;
  scan->char_at = 0;
}

fn_internal void scan_error_push(Scan *scan, Str message) {
  Scan_Error *error = arena_push_type(scan->arena, Scan_Error);
  queue_push(scan->error_first, scan->error_last, error);
  
  error->line_at = scan->line_at;
  error->char_at = scan->char_at;
  error->message = arena_push_str(scan->arena, message);
}

fn_internal Scan_Error *scan_error(Scan *scan) {
  return scan->error_first;
}

fn_internal U08 scan_char(Scan *scan) {
  U08 result = scan->at < scan->stream.len ? scan->stream.txt[scan->at] : 0;
  return result;
}

fn_internal void scan_move(Scan *scan, U32 offset) {
  For_U64 (it, offset) {
    if (scan->at >= scan->stream.len) {
      break;
    }

    if (scan->stream.txt[scan->at] == '\n') {
      scan->line_at   += 1;
      scan->char_at   = 0;
    }

    scan->at       += 1;
    scan->char_at  += 1;
  }
}

fn_internal void scan_skip_whitespace(Scan *scan) {
  while (char_is_whitespace(scan_char(scan))) {
    scan_move(scan, 1);
  }
}

fn_internal void scan_skip_line(Scan *scan) {
  for (;;) {
    U08 c = scan_char(scan);
    if (char_is_linefeed(c) || c == 0) {
      break;
    }

    scan_move(scan, 1);
  }
}

fn_internal B32 scan_end(Scan *scan) {
  scan_skip_whitespace(scan);
  B32 result = scan_char(scan) == 0;
  return result;
}


fn_internal Str scan_identifier(Scan *scan) {
  Str result = { };
  scan_skip_whitespace(scan);

  U64 start = scan->at;
  U08 c = scan_char(scan);
  if (char_is_alpha(c) || c == '_') {
    for (;;) {
      scan_move(scan, 1);
      c = scan_char(scan);
      if (!(char_is_alpha(c) || char_is_digit(c) || c == '_')) {
        break;
      }
    }

    result = str_slice(scan->stream, start, scan->at - start);
  } else {
    scan_error_push(scan, str_lit("expected identifier"));
  }

  return result;
}

fn_internal U64 scan_u64(Scan *scan) {
  U64 result = 0;
  scan_skip_whitespace(scan);

  U64 start = scan->at;
  U08 c     = scan_char(scan);
  if (char_is_digit(c)) {
    for (;;) {
      scan_move(scan, 1);
      c = scan_char(scan);
      if (!char_is_digit(c)) {
        break;
      }
    }

    result = u64_from_str(str_slice(scan->stream, start, scan->at - start));
  }

  return result;
}

fn_internal F64 scan_f64(Scan *scan) {
  F64 result = 0;
  scan_skip_whitespace(scan);
  U64 start_at = scan->at;

  U08 c = scan_char(scan);
  if (c != '+' && c != '-' && !char_is_digit(c)) {
    scan_error_push(scan, str_lit("expected f64"));
  } else {
    if (c == '+' || c == '-') {
      scan_move(scan, 1);
    }

    for (;;) { 
      c = scan_char(scan);
      if (!c || !char_is_digit(c)) { break; }
      scan_move(scan, 1);
    }

    if (c == '.') {
      scan_move(scan, 1);
      for (;;) { 
        c = scan_char(scan);
        if (!c || !char_is_digit(c)) { break; }
        scan_move(scan, 1);
      }
    }
    
    if (c == 'e' || c == 'E') {
      scan_move(scan, 1);
      c = scan_char(scan);
      if (c == '+' || c == '-') {
        scan_move(scan, 1);
      }

      for (;;) { 
        c = scan_char(scan);
        if (!c || !char_is_digit(c)) { break; }
        scan_move(scan, 1);
      }
    }

    Str slice = str_slice(scan->stream, start_at, scan->at - start_at);
    result = f64_from_str(slice);
  }

  return result;
}

fn_internal B32 scan_require(Scan *scan, Str match) {
  scan_skip_whitespace(scan);
  B32 result = str_equals(match, str_slice(scan->stream, scan->at, match.len));
  if (!result) {
    char buffer[512];
    Str message = { .len = 0, .txt = (U08 *)buffer };
    message.len = stbsp_snprintf(buffer, 512, "expected \"%.*s\"", str_expand(match));
    scan_error_push(scan, message);
  } else {
    scan_move(scan, match.len);
  }

  return result;
}

// ------------------------------------------------------------
// #--

fn_internal void su2_parse_block_point(Scan *scan, Arena *arena, UG_Mesh *mesh) {
  scan_require(scan, str_lit("="));
  U64 point_count = scan_u64(scan);
  log_info("parsing %llu points...", point_count);
  if (!scan_error(scan)) {
    mesh->grid_count = point_count;
    mesh->grid_array = arena_push_count(arena, V2F, point_count);

    For_U64(it, point_count) {
      F64 g0 = scan_f64(scan);
      F64 g1 = scan_f64(scan);
      mesh->grid_array[it] = v2f(g0, g1);

      scan_skip_line(scan);
      if (scan_error(scan)) {
        break;
      }
    }
  }
}

fn_internal void su2_parse_block_element(Scan *scan, Arena *arena, UG_Mesh *mesh) {
  scan_require(scan, str_lit("="));
  U64 element_count = scan_u64(scan);
  log_info("parsing %llu elements...", element_count);
  if (!scan_error(scan)) {
    mesh->tri_count = element_count;
    mesh->tri_array = arena_push_count(arena, UG_Tri, element_count);

    For_U64(it, element_count) {
      U64 element_type = scan_u64(scan);
      if (element_type == 5) { // NOTE(cmat): Triangle
        U64 e0 = scan_u64(scan);
        U64 e1 = scan_u64(scan);
        U64 e2 = scan_u64(scan);
        mesh->tri_array[it] = (UG_Tri) { e0, e1, e2 };

      } else {
        scan_error_push(scan, str_lit("unsupported element type in element block"));
      }

      if (scan_error(scan)) {
        break;
      }

      scan_skip_line(scan);
    }
  }
}

fn_internal void su2_parse_block_mark(Scan *scan, Arena *arena, UG_Mesh *mesh) {
  scan_require(scan, str_lit("="));
  U64 mark_count = scan_u64(scan);
  log_info("parsing %llu marks...", mark_count);
  if (!scan_error(scan)) {
    For_U64(it, mark_count) {
      scan_require(scan, str_lit("MARKER_TAG"));
      scan_require(scan, str_lit("="));
      Str tag_name = scan_identifier(scan);

      scan_require(scan, str_lit("MARKER_ELEMS"));
      scan_require(scan, str_lit("="));
      U64 element_count = scan_u64(scan);

      For_U64(it_elem, element_count) {
        U64 type = scan_u64(scan);
        if (type == 3) {
          U64 l0 = scan_u64(scan);
          U64 l1 = scan_u64(scan);
        } else {
          scan_error_push(scan, str_lit("unsupported element type in marker block"));
        }

        if (scan_error(scan)) { break; }
      }

      if (scan_error(scan)) { break; }
    }
  }
}

fn_internal void su2_parse(Str content, Arena *arena, UG_Mesh *mesh) {
  Log_Zone_Scope("parsing su2 content") {
    Scratch scratch = { };
    Scratch_Scope(&scratch, arena) {
      Scan scan = { };
      scan_init(&scan, scratch.arena, content);

      scan_skip_whitespace(&scan);
      scan_require(&scan, str_lit("NDIME"));
      scan_require(&scan, str_lit("="));

      U64 dimension = scan_u64(&scan);
      log_info("dimension: %llu", dimension);

      if (!scan_error(&scan)) {
        for (;;) {
          if (scan_end(&scan) || scan_error(&scan)) {
            break;
          }

          Str block_type = scan_identifier(&scan);
          if (str_equals(block_type, str_lit("NPOIN"))) {
            su2_parse_block_point(&scan, arena, mesh);
          } else if (str_equals(block_type, str_lit("NELEM"))) {
            su2_parse_block_element(&scan, arena, mesh);
          } else if (str_equals(block_type, str_lit("NMARK"))) {
            su2_parse_block_mark(&scan, arena, mesh);
          } else {
            char buffer[512];
            Str message = { .len = 0, .txt = (U08 *)buffer };
            message.len = stbsp_snprintf(buffer, 512, "unexpected block \"%.*s\"", str_expand(block_type));
            scan_error_push(&scan, message);
          }
        }
      }

      for (Scan_Error *it = scan_error(&scan); it; it = it->next) {
        log_fatal("SU2 error: %u:%u: %.*s", it->line_at, it->char_at, str_expand(it->message));
      }
    }
  }

  ug_init(mesh);
}
