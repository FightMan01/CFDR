#pragma pack(push, 1)
typedef struct UG_Cell {
  U64 v0, v1, v2;
} UG_Cell;

typedef struct UG_Face {
  U64 left_cell, right_cell;
} UG_Face;

#pragma pack(pop)

typedef struct UG_Mesh {
  Arena    arena;
  R2F      bounds;

  U64      grid_count;
  V2F     *grid_array;

  U64      cell_count;
  UG_Cell *cell_array;

  U64      face_count;
  UG_Face *face_array;
} UG_Mesh;

fn_internal void ug_compute_bounds(UG_Mesh *mesh) {
  mesh->bounds = r2f(f32_largest_positive, f32_largest_positive,
                     f32_largest_negative, f32_largest_negative);

  For_U64(it, mesh->grid_count) {
    V2F x = mesh->grid_array[it];
    mesh->bounds.min = v2f(f32_min(x.x, mesh->bounds.min.x), f32_min(x.y, mesh->bounds.min.y));
    mesh->bounds.max = v2f(f32_max(x.x, mesh->bounds.max.x), f32_max(x.y, mesh->bounds.max.y));
  }
}

fn_internal void ug_compute_faces(UG_Mesh *mesh) {
  Scratch scratch = { };
  Scratch_Scope(&scratch, 0) {

    // NOTE(cmat): Store all faces, including duplicates.
    U64      faces_all_len = 3 * mesh->cell_count;
    UG_Face *faces_all_dat = arena_push_count(scratch.arena, UG_Face, faces_all_len);

    // NOTE(cmat): Add all faces, sorting by left/right cell making duplicates identifiable.
    For_U64(it, mesh->cell_count) {
      UG_Cell *cell         = &mesh->cell_array[it];
      faces_all_dat[3 * it + 0] = (cell->v0 <= cell->v1) ? (UG_Face) { cell->v0, cell->v1 } : (UG_Face) { cell->v1, cell->v0 };
      faces_all_dat[3 * it + 1] = (cell->v1 <= cell->v2) ? (UG_Face) { cell->v1, cell->v2 } : (UG_Face) { cell->v2, cell->v1 };
      faces_all_dat[3 * it + 2] = (cell->v0 <= cell->v2) ? (UG_Face) { cell->v0, cell->v2 } : (UG_Face) { cell->v2, cell->v0 };
    }

    // NOTE(cmat): Now we radix-sort all faces resulting in duplicates being next to each other.
    u64_sort_radix(3 * mesh->cell_count, 2, ((U64 *)faces_all_dat) + 0);
    u64_sort_radix(3 * mesh->cell_count, 2, ((U64 *)faces_all_dat) + 1);

    // NOTE(cmat): Count how many duplicates we have.
    U64 duplicate_count = 0;
    For_U64(it, faces_all_len - 1) {
      duplicate_count += memory_compare(&faces_all_dat[it + 1], &faces_all_dat[it], sizeof(UG_Face));
    }

    // NOTE(cmat): Allocate de-duplicated face array.
    mesh->face_count = faces_all_len - duplicate_count;
    mesh->face_array = arena_push_count(&mesh->arena, UG_Face, mesh->face_count);

    U64 face_at = 0;
    For_U64(it, faces_all_len - 1) {
      if (!memory_compare(&faces_all_dat[it + 1], &faces_all_dat[it], sizeof(UG_Face))) {
        mesh->face_array[face_at++] = faces_all_dat[it];
      }
    }

    mesh->face_array[face_at++] = faces_all_dat[faces_all_len - 1];
  }
}

fn_internal void ug_init(UG_Mesh *mesh) {
  arena_init(&mesh->arena);

  ug_compute_bounds(mesh);
  ug_compute_faces(mesh);
}


