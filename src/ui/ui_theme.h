// (C) Copyright 2025 Matyas Constans
// Licensed under the MIT License (https://opensource.org/license/mit/)

typedef struct UI_Theme {
  UI_Color_Palette box;
  UI_Color_Palette separator;
  UI_Color_Palette button;
  UI_Color_Palette checkbox;
  UI_Color_Palette edit_value;
  UI_Color_Palette list;
} UI_Theme;

var_global UI_Theme UI_Theme_Dark = {
  .box = {
    .border = hsv_u32(36, 30, 20),
    .idle   = hsv_u32(232, 65, 5),
    .hover  = hsv_u32(232, 30, 10),
    .down   = hsv_u32(232, 30, 10),
  },

  .separator = {
    .border = hsv_u32(36,  86, 88),
    .idle   = hsv_u32(36,  86, 88),
  },

  .button = {
    .border = hsv_u32(36,  70, 88),
    .idle   = hsv_u32(36,  60, 60),
    .hover  = hsv_u32(46,  50, 80),
    .down   = hsv_u32(36,  70, 5),
  },

  .checkbox = {
    .border = hsv_u32(36,  70, 88),
    .idle   = hsv_u32(36,  60, 11),
    .hover  = hsv_u32(46,  80, 20),
    .down   = hsv_u32(36,  70, 5),
    .inner_fill   = hsv_u32(200,  50, 80),
  },

  .edit_value = {
    .border = hsv_u32(50,  70, 88),
    .idle   = hsv_u32(36,  60, 60),
    .hover  = hsv_u32(46,  80, 20),
    .down   = hsv_u32(36,  70, 5),
  },

  .list = {
    .border = hsv_u32(70,  70, 88),
    .idle   = hsv_u32(36,  60, 80),
    .hover  = hsv_u32(46,  80, 20),
    .down   = hsv_u32(36,  70, 5),
  },
};

var_global UI_Theme UI_Theme_Light = {
  .box = {
    .border = hsv_u32(232, 0, 90),
    .idle   = hsv_u32(232, 0, 95),
    .hover  = hsv_u32(232, 0, 97),
    .down   = hsv_u32(232, 0, 85),
  },

  .separator = {
    .border = hsv_u32(36,  0, 90),
    .idle   = hsv_u32(36,  0, 90),
  },

  .button = {
    .border = hsv_u32(36,  0, 88),
    .idle   = hsv_u32(36,  0, 80),
    .hover  = hsv_u32(46,  0, 20),
    .down   = hsv_u32(36,  0, 5),
  },

  .checkbox = {
    .border = hsv_u32(36,  0, 88),
    .idle   = hsv_u32(36,  0, 11),
    .hover  = hsv_u32(46,  0, 20),
    .down   = hsv_u32(36,  0, 5),
    .inner_fill   = hsv_u32(70,  0, 80),
  },

  .edit_value = {
    .border = hsv_u32(50,  0, 88),
    .idle   = hsv_u32(36,  0, 60),
    .hover  = hsv_u32(46,  0, 20),
    .down   = hsv_u32(36,  0, 5),
  },

  .list = {
    .border = hsv_u32(70,  0, 88),
    .idle   = hsv_u32(36,  0, 80),
    .hover  = hsv_u32(46,  0, 20),
    .down   = hsv_u32(36,  0, 5),
  },
};

var_global UI_Theme UI_Theme_Active = { };
