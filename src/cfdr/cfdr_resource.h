typedef struct CFDR_Resource_Data {
  U64  bytes_downloaded;
  U64  bytes_total;
  U08 *bytes_data;
} CFDR_Resource_Data;

typedef struct CFDR_Resource {
  Arena        arena;
  B32          request_sent;
  HTTP_Request request;
  B32          complete;
  Str          path;
} CFDR_Resource;

fn_internal void cfdr_resource_init(CFDR_Resource *resource, Str path) {
  zero_fill(resource);
  arena_init(&resource->arena);
  resource->path = arena_push_str(&resource->arena, path);
 // resource->path = path;
}

var_global B32 Resource_Downloading             = 1;
var_global U64 Resource_Downloading_Bytes_Done  = 0;
var_global U64 Resource_Downloading_Bytes_Total = 0;

fn_internal B32 cfdr_resource_fetch(CFDR_Resource *resource, CFDR_Resource_Data *data) {
  B32 result = 0;
  if (!resource->complete) {
    Resource_Downloading = 1;
    Resource_Downloading_Bytes_Done  += resource->request.bytes_downloaded;
    Resource_Downloading_Bytes_Total += resource->request.bytes_total;
    if (!resource->request_sent) {
      resource->request_sent = 1;
      log_info("Resource Request: %.*s", str_expand(resource->path));
      http_request_send(&resource->request, &resource->arena, resource->path);
    }

    if (resource->request.status == HTTP_Status_Done) {
      resource->complete = 1;
      result = resource->complete;

      *data = (CFDR_Resource_Data) {
        .bytes_downloaded = resource->request.bytes_downloaded,
        .bytes_total      = resource->request.bytes_total,
        .bytes_data       = resource->request.bytes_data,
      };
    }
  }

  return result;
}

typedef struct CFDR_Resource_Surface {
  CFDR_Resource   resource;
  B32             valid;
  R3F             bounds;
  U32             index_count;
  R_Buffer        index_buffer;
  // R_Buffer        vertex_buffer;
  R_Buffer        X_buffer;
  R_Buffer        U_buffer;
  R_Buffer        N_buffer;

} CFDR_Resource_Surface;

fn_internal void cfdr_resource_surface_init(CFDR_Resource_Surface *surface, Str path) {
  zero_fill(surface);
  cfdr_resource_init(&surface->resource, path);
}

fn_internal void cfdr_resource_surface_update(CFDR_Resource_Surface *surface) {
  CFDR_Resource_Data data = { };
  if (cfdr_resource_fetch(&surface->resource, &data)) {
    surface->valid = 1;

    Scratch scratch = { };
    Scratch_Scope(&scratch, 0) {
      U32 triangle_count = 0;
      R_Vertex_XNUC_3D *vertices = stl_parse(&surface->resource.arena, data.bytes_total, data.bytes_data, &triangle_count);
      log_info("loaded surface resource \"%.*s\", %u triangles", str_expand(surface->resource.path), triangle_count);

      V4F *X_data = arena_push_count(scratch.arena, V4F, 3 * triangle_count);
      V4F *N_data = arena_push_count(scratch.arena, V4F, 3 * triangle_count);
      V2F *U_data = arena_push_count(scratch.arena, V2F, 3 * triangle_count);

      // NOTE(cmat): Deinterleave data.
      For_U64(it, 3 * triangle_count) {
        R_Vertex_XNUC_3D v = vertices[it];
        X_data[it] = v4f(v.X.x, v.X.y, v.X.z, 1);
        N_data[it] = v4f(v.N.x, v.N.y, v.N.z, 0);
        U_data[it] = v.U;
      }

      surface->X_buffer = r_buffer_allocate(3 * triangle_count * sizeof(V4F), R_Buffer_Mode_Static);
      r_buffer_download(surface->X_buffer, 0, 3 * triangle_count * sizeof(V4F), X_data);

      surface->N_buffer = r_buffer_allocate(3 * triangle_count * sizeof(V4F), R_Buffer_Mode_Static);
      r_buffer_download(surface->N_buffer, 0, 3 * triangle_count * sizeof(V4F), N_data);

      surface->U_buffer = r_buffer_allocate(3 * triangle_count * sizeof(V2F), R_Buffer_Mode_Static);
      r_buffer_download(surface->U_buffer, 0, 3 * triangle_count * sizeof(V2F), U_data);

      U32 *indices = (U32 *)arena_push_size(&surface->resource.arena, 3 * sizeof(U32) * triangle_count);
      For_U32 (it, 3 * triangle_count) {
        indices[it] = it;
      }

      surface->index_buffer = r_buffer_allocate(3 * sizeof(U32) * triangle_count, R_Buffer_Mode_Static);
      surface->index_count  = 3 * triangle_count;
      r_buffer_download(surface->index_buffer, 0, 3 * sizeof(U32) * triangle_count, indices);

      R3F bounds = r3f( f32_largest_positive, f32_largest_positive, f32_largest_positive,
                        f32_largest_negative, f32_largest_negative, f32_largest_negative);

      For_U64(it, 3 * triangle_count) {
        V3F x = vertices[it].X;
        bounds.min = v3f(f32_min(x.x, bounds.min.x), f32_min(x.y, bounds.min.y), f32_min(x.z, bounds.min.z));
        bounds.max = v3f(f32_max(x.x, bounds.max.x), f32_max(x.y, bounds.max.y), f32_max(x.z, bounds.max.z));
      }

      surface->bounds = bounds;
    }
  }
}

typedef struct CFDR_Resource_Volume {
  CFDR_Resource resource;
  B32           valid;
  R_Texture_3D  volume;
  V2F           data_range;
  // R_Texture_2D  color_map;
  R_Buffer      constant_buffer;
  // R_Bind_Group  bind_group;
} CFDR_Resource_Volume;

fn_internal void cfdr_resource_volume_init(CFDR_Resource_Volume *volume, Str path) {
  zero_fill(volume);
  cfdr_resource_init(&volume->resource, path);
}

fn_internal void cfdr_resource_volume_update(CFDR_Resource_Volume *volume) {
  CFDR_Resource_Data data = { };
  if (cfdr_resource_fetch(&volume->resource, &data)) {
    volume->valid = 1;

    Scratch scratch = { };
    Scratch_Scope(&scratch, 0) {
      U08 *data_view       = data.bytes_data;
      U08 *data_compressed = 0;
      U64 compressed_size   = 0;
      U64 decompressed_size = 0;
      U32 flags             = 0;
      U32 X = 0, Y = 0, Z = 0;
      F32 min_range = 0.0f, max_range = 0.0f;

      // NOTE(aron): vol header is either 72 bytes (new: magic+fmt+csz+dsz+flg+Y+X+Z+lo+hi+bounds)
      // or 64 bytes (old: csz+dsz+flg+Y+X+Z+lo+hi+bounds). the new format requires
      // compressed data to fit after the 72-byte header; the old format stores it
      // after 64 bytes. peek the new header and use it only if its compressed size
      // actually fits, otherwise fall back to the old layout.
      U08 *peek = data.bytes_data;
      U32 pe_magic = *(U32 *)peek; peek += 4;
      U32 pe_fmt   = *(U32 *)peek; peek += 4;
      U64 pe_csz   = *(U64 *)peek; peek += 8;
      U64 pe_dsz   = *(U64 *)peek; peek += 8;
      U32 pe_flg   = *(U32 *)peek; peek += 4;
      U32 pe_Y     = *(U32 *)peek; peek += 4;
      U32 pe_X     = *(U32 *)peek; peek += 4;
      U32 pe_Z     = *(U32 *)peek; peek += 4;
      F32 pe_lo    = *(F32 *)peek; peek += 4;
      F32 pe_hi    = *(F32 *)peek; peek += 4;
      peek += 12; /* min_bounds */
      peek += 12; /* max_bounds */

      B32 new_ok = pe_X && pe_Y && pe_Z && pe_dsz == (U64)pe_X * pe_Y * pe_Z &&
                   pe_csz && 72 + pe_csz <= data.bytes_total;

      if (new_ok) {
        data_view         = data.bytes_data + 72;  /* skip whole new-format header */
        compressed_size   = pe_csz;
        decompressed_size = pe_dsz;
        flags             = pe_flg;
        Y                 = pe_Y;
        X                 = pe_X;
        Z                 = pe_Z;
        min_range         = pe_lo;
        max_range         = pe_hi;
      } else {
        data_view = data.bytes_data;
        compressed_size   = *(U64 *)data_view; data_view += 8;
        decompressed_size = *(U64 *)data_view; data_view += 8;
        flags             = *(U32 *)data_view; data_view += 4;
        Y                 = *(U32 *)data_view; data_view += 4;
        X                 = *(U32 *)data_view; data_view += 4;
        Z                 = *(U32 *)data_view; data_view += 4;
        min_range         = *(F32 *)data_view; data_view += 4;
        max_range         = *(F32 *)data_view; data_view += 4;
        data_view        += 12;           /* min_bounds */
        data_view        += 12;           /* max_bounds */
      }
      data_compressed = data_view;

      log_info("Compressed Size   : %llu", compressed_size);
      log_info("Decompressed Size : %llu", decompressed_size);
      log_info("Compression Flags : %d", flags);
      log_info("Voxel Dimensions  : %u %u %u", X, Y, Z);
      log_info("Dataset Bounds    : %f %f", min_range, max_range);

      volume->data_range = v2f(min_range, max_range);

      U08 *data = arena_push_size(scratch.arena, decompressed_size);
      LZ4_decompress_safe((char *)data_compressed, (char *)data, compressed_size, decompressed_size);

      volume->volume = r_texture_3D_allocate(R_Texture_Format_R_U08_Normalized, X, Y, Z);
      r_texture_3D_download(volume->volume, R_Texture_Format_R_U08_Normalized, r3i(0, 0, 0, X, Y, Z), data);
    }
 
    volume->constant_buffer = r_buffer_allocate(sizeof(R_Constant_Buffer_Vol_3D), R_Buffer_Mode_Dynamic);
  }
}

typedef struct CFDR_Resource_Table {
  CFDR_Resource   resource;
  Arena           arena;
  B32             valid;
  U32             row_count;
  Str            *tag_array;
  V3F            *position_array;
} CFDR_Resource_Table;

fn_internal void cfdr_resource_table_init(CFDR_Resource_Table *table, Str path) {
  zero_fill(table);
  cfdr_resource_init(&table->resource, path);
  arena_init(&table->arena);
}

fn_internal void cfdr_resource_table_update(CFDR_Resource_Table *table) {
  CFDR_Resource_Data data = { };
  if (cfdr_resource_fetch(&table->resource, &data)) {
    table->valid = 1;

    Scratch scratch = { };
    Scratch_Scope(&scratch, 0) {

      Scan scan = { };
      scan_init(&scan, scratch.arena, str(data.bytes_total, data.bytes_data));

      // NOTE(cmat): Skip first line.
      scan_skip_whitespace(&scan);
      scan_skip_line(&scan);


      U32 row_count = 0;
      for (;;) {
        if (!scan_end(&scan) && !scan_error(&scan)) {
          scan_skip_whitespace(&scan);
          scan_skip_line(&scan);

          row_count += 1;
        } else {
          break;
        }
      }

      log_info("Table row count: %u", row_count);
      table->row_count = row_count;

      zero_fill(&scan);
      scan_init(&scan, scratch.arena, str(data.bytes_total, data.bytes_data));


      // NOTE(cmat): Skip first line.
      scan_skip_whitespace(&scan);
      scan_skip_line(&scan);

      table->tag_array      = arena_push_count(&table->arena, Str, row_count);
      table->position_array = arena_push_count(&table->arena, V3F, row_count);

      U32 row_at = 0;
      for (;;) {
        if (!scan_end(&scan) && !scan_error(&scan)) {
          scan_skip_whitespace(&scan);
          Str name  = scan_str(&scan); scan_require(&scan, str_lit(","));
          F32 x     = scan_f64(&scan); scan_require(&scan, str_lit(","));
          F32 y     = scan_f64(&scan); scan_require(&scan, str_lit(","));
          F32 z     = scan_f64(&scan);
          
          table->tag_array[row_at]      = arena_push_str(&table->arena, name);
          table->position_array[row_at] = v3f(x, y, z);

          scan_skip_line(&scan);
          row_at += 1;

        } else {
          break;
        }
      }

      for (Scan_Error *it = scan_error(&scan); it; it = it->next) {
        log_fatal("CSV error: %u:%u: %.*s", it->line_at, it->char_at, str_expand(it->message));
      }
    }
  }
}
