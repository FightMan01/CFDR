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

var_global B32 Resource_Downloading = 1;

fn_internal B32 cfdr_resource_fetch(CFDR_Resource *resource, CFDR_Resource_Data *data) {
  B32 result = 0;
  if (!resource->complete) {
    Resource_Downloading = 1;
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
  R_Buffer        vertex_buffer;
  R_Buffer        index_buffer;
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

      surface->vertex_buffer = r_buffer_allocate(3 * sizeof(R_Vertex_XNUC_3D) * triangle_count, R_Buffer_Mode_Static);
      r_buffer_download(surface->vertex_buffer, 0, 3 * sizeof(R_Vertex_XNUC_3D) * triangle_count, vertices);

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
      U08 *data_view = data.bytes_data;

      U32 magic_number      = *(U64 *)(data_view); data_view += sizeof(U32);
      U32 format_type       = *(U64 *)(data_view); data_view += sizeof(U32);
      U64 compressed_size   = *(U64 *)(data_view); data_view += sizeof(U64);
      U64 decompressed_size = *(U64 *)(data_view); data_view += sizeof(U64);
      U32 flags             = *(U32 *)(data_view); data_view += sizeof(U32);
      U32 X                 = *(U32 *)(data_view); data_view += sizeof(U32);
      U32 Y                 = *(U32 *)(data_view); data_view += sizeof(U32);
      U32 Z                 = *(U32 *)(data_view); data_view += sizeof(U32);
      F32 min_range         = *(F32 *)(data_view); data_view += sizeof(F32);
      F32 max_range         = *(F32 *)(data_view); data_view += sizeof(F32);
      V3F min_bounds        = *(V3F *)(data_view); data_view += sizeof(V3F);
      V3F max_bounds        = *(V3F *)(data_view); data_view += sizeof(V3F);
      U08 *data_compressed  = data_view;

      log_info("Compressed Size: %llu", compressed_size);
      log_info("Decompressed Size: %llu", decompressed_size);
      log_info("Compression Flags: %d", flags);
      log_info("Voxel Dimensions: %u %u %u", X, Y, Z);
      log_info("Dataset Bounds: %f %f", min_range, max_range);

      volume->data_range = v2f(min_range, max_range);

      U08 *data = arena_push_size(scratch.arena, decompressed_size);
      LZ4_decompress_safe((char *)data_compressed, (char *)data, compressed_size, decompressed_size);

      volume->volume = r_texture_3D_allocate(R_Texture_Format_R_U08_Normalized, X, Y, Z);
      r_texture_3D_download(volume->volume, R_Texture_Format_R_U08_Normalized, r3i(0, 0, 0, X, Y, Z), data);
    }

#if 0
    volume->color_map = r_texture_2D_allocate(R_Texture_Format_RGBA_U08_Normalized, 1024, 1);

    U32 texture_width = 1024;
    U08 *texture_data = arena_push_size(scratch.arena, 4 * texture_width);

    For_U32 (it, texture_width) {
      F32 t = (F32)it / (texture_width - 1);
      V3F c = { };
      c = rgb_from_hsv(v3f(t, 1.0f, 1.0f));

      texture_data[4 * it + 0] = 255 * c.r;
      texture_data[4 * it + 1] = 255 * c.g;
      texture_data[4 * it + 2] = 255 * c.b;
      texture_data[4 * it + 3] = 255;
    }

    r_texture_2D_download(volume->color_map, R_Texture_Format_RGBA_U08_Normalized, r2i(0, 0, 1024, 1), texture_data);

#endif

    volume->constant_buffer = r_buffer_allocate(sizeof(R_Constant_Buffer_World_3D), R_Buffer_Mode_Dynamic);
#if 0
    volume->bind_group = r_bind_group_create(&Flat_2D_Layout, &(R_Bind_Group_Entry_List) {
      .count      = 4,
      .entry_list = {
        { .binding = 0, .type = R_Binding_Type_Texture_2D, .resource = volume->color_map             },
        { .binding = 1, .type = R_Binding_Type_Sampler,    .resource = R_Sampler_Linear_Clamp        },
        { .binding = 2, .type = R_Binding_Type_Uniform,    .resource = volume->constant_buffer       },
        { .binding = 3, .type = R_Binding_Type_Texture_3D, .resource = volume->volume                },
      }
    });
#endif
  }
}


