var_global Arena LZ4_Arena = { };

void lz4_init(void) {
  arena_init(&LZ4_Arena);
}

void *LZ4_malloc(size_t s) {
  return arena_push_size(&LZ4_Arena, s, .flags = 0);
}

void *LZ4_calloc(size_t n, size_t s) {
  return arena_push_size(&LZ4_Arena, n * s);
}

void LZ4_free(void* p) {
}

#define LZ4_memcpy  memory_copy
#define LZ4_memmove memory_move
#define LZ4_memset  memory_fill
