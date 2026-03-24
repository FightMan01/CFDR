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

      Random_Seed rng = 1234;

      STL_Binary_Triangle *triangles = (STL_Binary_Triangle *)(data + sizeof(STL_Binary_Header));
      For_U32 (it, header->tri_count) {
        STL_Binary_Triangle tri = triangles[it];

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

