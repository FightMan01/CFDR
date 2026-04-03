#include "cfdr_lz4.c"

#pragma clang diagnostic ignored "-Wincompatible-pointer-types-discards-qualifiers"

#define LZ4_USER_MEMORY_FUNCTIONS
#define LZ4_FREESTANDING 1
#include "thirdparty/lz4.h"
#include "thirdparty/lz4.c"

#include "cfdr_resource.h"

#include "cfdr_camera.h"
#include "cfdr_render.h"

#include "cfdr_scene.h"
#include "cfdr_overlay.h"

#include "cfdr_scene.c"
#include "cfdr_overlay.c"

#include "cfdr_state.h"
#include "cfdr_state.c"

#include "cfdr_eval.h"
#include "cfdr_eval.c"

#include "cfdr_layer.h"

#include "cfdr_project.c"

// #include "cfdr_draw.c"
#include "cfdr_ui.c"

