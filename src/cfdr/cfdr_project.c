typedef struct CFDR_Viewport_Config {
  V3F background;
  B32 orthographic;
  B32 grid_enabled;
  V3F grid_color;
  F32 grid_level;
} CFDR_Viewport_Config;

typedef struct CFDR_Project {
  CFDR_Viewport_Config viewport_config;
} CFDR_Project;
