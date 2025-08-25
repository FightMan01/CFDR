// $File: cfdr_menu.h
// $Last-Modified: "2025-08-08 01:39:03"
// $Author: Matyas Constans.
// $Notice: (C) Matyas Constans, Horvath Zoltan 2025 - All Rights Reserved.
// $License: You may use, distribute and modify this code under the terms of the MIT license.
// $Note: CFDR menu bar.

typedef struct {
  GFX_Font *font;
} CFDR_Menu;

void cfdr_menu_init(CFDR_Menu *menu, GFX_Font *font);
void cfdr_menu_render(CFDR_Menu *menu, R2 menu_region);
void cfdr_timeline_render(GFX_Font *font, F32 *time, V2 time_range, R2 region);
