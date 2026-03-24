typedef struct UG_Tri {
  U64 t0, t1, t2;
} UG_Tri;

typedef struct UG_Mesh {
  R2F     bounds;

  U64     grid_count;
  V2F    *grid_array;

  U64     tri_count;
  UG_Tri *tri_array;
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

}

fn_internal void ug_init(UG_Mesh *mesh) {
  ug_compute_bounds(mesh);
  ug_compute_faces(mesh);
}
