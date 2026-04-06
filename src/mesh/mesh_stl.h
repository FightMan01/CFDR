// (C) Copyright 2025 Matyas Constans
// Licensed under the MIT License (https://opensource.org/license/mit/)

#pragma pack(push, 1)
typedef struct STL_Binary_Header {
  U08 header[80];
  U32 tri_count;
} STL_Binary_Header;

typedef struct STL_Binary_Triangle {
  V3F normal;
  V3F position_1;
  V3F position_2;
  V3F position_3;
  U16 attribute_byte_count;
} STL_Binary_Triangle;

#pragma pack(pop)

fn_internal R_Vertex_XNUC_3D *stl_parse_binary(Arena *arena, U64 bytes, U08 *data, U32 *tri_count) {
  R_Vertex_XNUC_3D *result = 0;
  if (bytes >= sizeof(STL_Binary_Header)) {
    STL_Binary_Header *header = (STL_Binary_Header *)data;

    U64 expected_bytes = 0;
    expected_bytes += sizeof(STL_Binary_Header);
    expected_bytes += header->tri_count * sizeof(STL_Binary_Triangle);

    if (bytes >= expected_bytes) {
      result = arena_push_count(arena, R_Vertex_XNUC_3D, 3 * header->tri_count);
      *tri_count = header->tri_count;

      STL_Binary_Triangle *triangles = (STL_Binary_Triangle *)(data + sizeof(STL_Binary_Header));
      For_U32 (it, header->tri_count) {
        STL_Binary_Triangle tri = triangles[it];

        if (v3f_len(tri.normal) < 0.00001f) {
          tri.normal = v3f_cross(v3f_noz(v3f_sub(tri.position_1, tri.position_2)),
                                 v3f_noz(v3f_sub(tri.position_3, tri.position_2)));
          tri.normal = v3f_mul(-1.f, tri.normal);
        }

        U32 C = 0xFFFFFFFF;
        result[3 * it + 0] = (R_Vertex_XNUC_3D) { .X = tri.position_1, .N = tri.normal, .C = 0xFFFFFFFF, .U = v2f(0, 0) };
        result[3 * it + 1] = (R_Vertex_XNUC_3D) { .X = tri.position_2, .N = tri.normal, .C = 0xFFFFFFFF, .U = v2f(1, 0) };
        result[3 * it + 2] = (R_Vertex_XNUC_3D) { .X = tri.position_3, .N = tri.normal, .C = 0xFFFFFFFF, .U = v2f(0, 1) };
      }
    } else {
      log_fatal("STL parse error");
    }
  } else {
    log_fatal("STL parse error");
  }

  return result;
}

fn_internal U32 stl_parse_ascii_count_triangles(Str content) {
  U32 tri_count = 0;
  Str match = str_lit("facet");
  For_U64(it, content.len) {
    Str slice = str_slice(content, it, match.len);
    if (str_starts_with(slice, match)) {
      tri_count += 1;
      it += match.len - 1;
    }
  }

  return tri_count;
}

fn_internal R_Vertex_XNUC_3D *stl_parse_ascii(Arena *arena, U64 bytes, U08 *data, U32 *tri_count) {
  R_Vertex_XNUC_3D *result = 0;
  Str content = str(bytes, data);

  Scratch scratch = { };
  Scratch_Scope(&scratch, 0) {

    // NOTE(cmat): Pass 1 -- Count triangles.
    Scan scan = { };
    scan_init(&scan, scratch.arena, content);

    *tri_count = stl_parse_ascii_count_triangles(content);
    U64 tri_at = 0;
    result = arena_push_count(arena, R_Vertex_XNUC_3D, 3 * (*tri_count));

    scan_require(&scan, str_lit("solid"));
    scan_skip_line(&scan);
    if (!scan_error(&scan)) {
      for (;;) {
        if (scan_error(&scan)) {
          break;
        }

        Str block = scan_identifier(&scan);
        if (str_equals(block, str_lit("endsolid"))) {
          break;
        } else if (str_equals(block, str_lit("facet"))) {
          scan_require(&scan, str_lit("normal"));

          F32 nx = (F32)scan_f64(&scan); // nx
          F32 ny = (F32)scan_f64(&scan); // ny
          F32 nz = (F32)scan_f64(&scan); // nz
          V3F N = v3f(nx, ny, nz);

          scan_skip_line(&scan);

          if (scan_error(&scan)) {
            break;
          }

          scan_require(&scan, str_lit("outer"));
          scan_require(&scan, str_lit("loop"));
          scan_skip_line(&scan);

          if (tri_at >= *tri_count) {
            scan_error_push(&scan, str_lit("superfluous \"facet\" qualifiers in STL file"));
            break;
          }

          R_Vertex_XNUC_3D *vertex = result + 3 * (tri_at++);
          var_local_persist V2F uv_lookup[] = { { 0, 0}, { 1, 0 }, { 0, 1 }, };

          V3F positions[3] = { };

          For_U32(it, 3) {
            scan_require(&scan, str_lit("vertex"));
            F32 x = (F32)scan_f64(&scan); // vx
            F32 y = (F32)scan_f64(&scan); // vy
            F32 z = (F32)scan_f64(&scan); // vz
            
            positions[it] = v3f(x, y, z);

            scan_skip_line(&scan);
            if (scan_error(&scan)) {
              break;
            }
          }

          if (v3f_len(N) < 0.00001f) {
            N = v3f_cross(v3f_noz(v3f_sub(positions[0], positions[1])),
                          v3f_noz(v3f_sub(positions[2], positions[1])));
            N = v3f_mul(-1.f, N);
          }

          vertex[0] = (R_Vertex_XNUC_3D) { .X = positions[0], .N = N, .C = 0xFFFFFFFF, .U = uv_lookup[0] };
          vertex[1] = (R_Vertex_XNUC_3D) { .X = positions[1], .N = N, .C = 0xFFFFFFFF, .U = uv_lookup[1] };
          vertex[2] = (R_Vertex_XNUC_3D) { .X = positions[2], .N = N, .C = 0xFFFFFFFF, .U = uv_lookup[2] };

          if (scan_error(&scan)) {
            break;
          }

          scan_require(&scan, str_lit("endloop"));
          scan_require(&scan, str_lit("endfacet"));

        } else {
          scan_error_push(&scan, str_lit("expected facet or endsolid"));
        }


      } 
    }

    for (Scan_Error *it = scan_error(&scan); it; it = it->next) {
      log_fatal("STL error: %u:%u: %.*s", it->line_at, it->char_at, str_expand(it->message));
    }
  }

  return result;
}

fn_internal R_Vertex_XNUC_3D *stl_parse(Arena *arena, U64 bytes, U08 *data, U32 *tri_count) {
  R_Vertex_XNUC_3D *result = { };

  Str content = str(bytes, data);
  if (str_starts_with(content, str_lit("solid"))) {
    result = stl_parse_ascii(arena, bytes, data, tri_count);
  } else {
    result = stl_parse_binary(arena, bytes, data, tri_count);
  }

  return result;
}

