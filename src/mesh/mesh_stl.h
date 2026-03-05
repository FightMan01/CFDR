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

        V3F R = rgb_from_hsv(v3f(it / (F32)header->tri_count, .8f, .8f));
        U32 C = abgr_u32_from_rgba_premul(v4f(R.x, R.y, R.z, 1.f));

        tri.position_1 = v3f_mul(.025f, v3f(tri.position_1.x, -tri.position_1.z, tri.position_1.y));
        tri.position_2 = v3f_mul(.025f, v3f(tri.position_2.x, -tri.position_2.z, tri.position_2.y));
        tri.position_3 = v3f_mul(.025f, v3f(tri.position_3.x, -tri.position_3.z, tri.position_3.y));

        V3F N = v3f(tri.normal.x, -tri.normal.z, tri.normal.y);

        result[3 * it + 0] = (R_Vertex_XNUC_3D) { .X = tri.position_1, .N = N, .C = 0xFFFFFFFF, .U = v2f(0, 0) };
        result[3 * it + 1] = (R_Vertex_XNUC_3D) { .X = tri.position_2, .N = N, .C = 0xFFFFFFFF, .U = v2f(1, 0) };
        result[3 * it + 2] = (R_Vertex_XNUC_3D) { .X = tri.position_3, .N = N, .C = 0xFFFFFFFF, .U = v2f(0, 1) };
      }
    } else {
      log_fatal("STL parse error");
    }
  } else {
    log_fatal("STL parse error");
  }

  return result;
}

