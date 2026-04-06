// (C) Copyright 2025 Matyas Constans
// Licensed under the MIT License (https://opensource.org/license/mit/)

typedef struct UI_Theme {
  UI_Color_Palette box;
  UI_Color_Palette separator;
  UI_Color_Palette button;
  UI_Color_Palette checkbox;
  UI_Color_Palette edit_value;
  UI_Color_Palette list;
  UI_Color_Palette label;
} UI_Theme;

var_global UI_Theme UI_Theme_Dark = {
  .box = {
    .border = hsv_u32(36, 30, 20),
    .idle   = hsv_u32(232, 65, 5),
    .hover  = hsv_u32(232, 30, 10),
    .down   = hsv_u32(232, 30, 10),
    .label  = hsv_u32(255, 0, 100),
  },

  .separator = {
    .border = hsv_u32(36,  86, 88),
    .idle   = hsv_u32(36,  86, 88),
    .label  = hsv_u32(255, 0, 100),
  },

  .button = {
    .border = hsv_u32(36,  70, 88),
    .idle   = hsv_u32(36,  60, 60),
    .hover  = hsv_u32(46,  50, 80),
    .down   = hsv_u32(36,  70, 5),
    .label  = hsv_u32(255, 0, 100),
  },

  .checkbox = {
    .border = hsv_u32(36,  70, 88),
    .idle   = hsv_u32(36,  60, 11),
    .hover  = hsv_u32(46,  80, 20),
    .down   = hsv_u32(36,  70, 5),
    .inner_fill   = hsv_u32(200,  50, 80),
    .label  = hsv_u32(255, 0, 100),
  },

  .edit_value = {
    .border = hsv_u32(50,  70, 88),
    .idle   = hsv_u32(36,  60, 60),
    .hover  = hsv_u32(46,  80, 20),
    .down   = hsv_u32(36,  70, 5),
    .label  = hsv_u32(255, 0, 100),
  },

  .list = {
    .border = hsv_u32(70,  70, 88),
    .idle   = hsv_u32(36,  60, 80),
    .hover  = hsv_u32(46,  80, 20),
    .down   = hsv_u32(36,  70, 5),
    .label  = hsv_u32(255, 0, 100),
  },

  .label = {
    .label  = hsv_u32(255, 0, 100),
  },
};

var_global UI_Theme UI_Theme_Light = {
  .box = {
    .border = hsv_u32(232, 0, 90),
    .idle   = hsv_u32(232, 0, 95),
    .hover  = hsv_u32(232, 0, 97),
    .down   = hsv_u32(232, 0, 85),
    .label  = hsv_u32(255, 0, 25),
  },

  .separator = {
    .border = hsv_u32(36,  0, 90),
    .idle   = hsv_u32(36,  0, 90),
    .label  = hsv_u32(255, 0, 25),
  },

  .button = {
    .border = hsv_u32(36,  0, 88),
    .idle   = hsv_u32(36,  0, 80),
    .hover  = hsv_u32(46,  0, 20),
    .down   = hsv_u32(36,  0, 5),
    .label  = hsv_u32(255, 0, 25),
  },

  .checkbox = {
    .border = hsv_u32(36,  0, 88),
    .idle   = hsv_u32(36,  0, 11),
    .hover  = hsv_u32(46,  0, 20),
    .down   = hsv_u32(36,  0, 5),
    .inner_fill   = hsv_u32(70,  0, 80),
    .label  = hsv_u32(255, 0, 25),
  },

  .edit_value = {
    .border = hsv_u32(50,  0, 88),
    .idle   = hsv_u32(36,  0, 60),
    .hover  = hsv_u32(46,  0, 20),
    .down   = hsv_u32(36,  0, 5),
    .label  = hsv_u32(255, 0, 25),
  },

  .list = {
    .border = hsv_u32(70,  0, 88),
    .idle   = hsv_u32(36,  0, 80),
    .hover  = hsv_u32(46,  0, 20),
    .down   = hsv_u32(36,  0, 5),
    .label  = hsv_u32(255, 0, 25),
  },

  .label = {
    .label  = hsv_u32(255, 0, 25),
  },
};

var_global UI_Theme UI_Theme_Active = { };
