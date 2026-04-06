
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
    mesh->cell_count = element_count;
    mesh->cell_array = arena_push_count(arena, UG_Cell, element_count);

    For_U64(it, element_count) {
      U64 element_type = scan_u64(scan);
      if (element_type == 5) { // NOTE(cmat): Triangle
        U64 e0 = scan_u64(scan);
        U64 e1 = scan_u64(scan);
        U64 e2 = scan_u64(scan);
        mesh->cell_array[it] = (UG_Cell) { e0, e1, e2 };

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
