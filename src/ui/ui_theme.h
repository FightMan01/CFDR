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
    .border = hsv_u32(220, 14, 24),
    .idle   = hsv_u32(220, 18, 13),
    .hover  = hsv_u32(220, 18, 15),
    .down   = hsv_u32(220, 18, 15),
    .label  = hsv_u32(220, 10, 90),
  },

  .separator = {
    .border = hsv_u32(220, 10, 28),
    .idle   = hsv_u32(220, 10, 28),
    .label  = hsv_u32(220, 8, 88),
  },

  .button = {
    .border = hsv_u32(215, 55, 58),
    .idle   = hsv_u32(215, 32, 24),
    .hover  = hsv_u32(215, 42, 34),
    .down   = hsv_u32(215, 48, 18),
    .inner_fill = hsv_u32(40, 50, 86),
    .label  = hsv_u32(215, 8, 96),
  },

  .checkbox = {
    .border     = hsv_u32(205, 50, 56),
    .idle       = hsv_u32(220, 18, 18),
    .hover      = hsv_u32(215, 30, 26),
    .down       = hsv_u32(215, 40, 12),
    .inner_fill = hsv_u32(195, 72, 86),
    .label      = hsv_u32(220, 8, 96),
  },

  .edit_value = {
    .border = hsv_u32(190, 50, 62),
    .idle   = hsv_u32(220, 16, 16),
    .hover  = hsv_u32(220, 24, 22),
    .down   = hsv_u32(220, 30, 10),
    .label  = hsv_u32(220, 8, 96),
  },

  .list = {
    .border = hsv_u32(200, 35, 50),
    .idle   = hsv_u32(220, 16, 15),
    .hover  = hsv_u32(220, 22, 21),
    .down   = hsv_u32(220, 28, 9),
    .label  = hsv_u32(220, 8, 96),
  },

  .label = {
    .label = hsv_u32(220, 8, 92),
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
