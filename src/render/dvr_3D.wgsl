struct Volume_3D_Type {
  @align(16) World_View_Projection   : mat4x4<f32>,
  @align(16) World_Inverse_Transpose : mat4x4<f32>,
  @align(16) World                   : mat4x4<f32>,
  @align(16) Eye_Position            : vec3<f32>,
  @align(16) Volume_Density          : f32,
  @align(16) Volume_Min              : vec3<f32>,
  @align(16) Volume_Max              : vec3<f32>,
  @align(16) Volume_Data_Bounds      : vec2<f32>,
  @align(16) Visualize_Range         : vec2<f32>,
  @align(16) Volume_Saturate         : f32,
  @align(16) Volume_XYZ              : i32,
  @align(16) Contour_Color           : vec3<f32>,
  @align(16) Contour_Visible         : i32,
  @align(16) Contour_Value           : f32,
  @align(16) Contour_Thickness       : f32
};

@group(0) @binding(0) var<storage, read> X_Buffer : array<vec4<f32>>;
@group(0) @binding(1) var Texture                 : texture_2d<f32>;
@group(0) @binding(2) var Sampler                 : sampler;
@group(0) @binding(3) var<uniform> Vol_3D         : Volume_3D_Type;
@group(0) @binding(4) var Texture_Volume          : texture_3d<f32>;

fn vec4_unpack_u32(packed: u32) -> vec4<f32> {
  let r = f32((packed >> 0)  & 0xFFu) / 255.0;
  let g = f32((packed >> 8)  & 0xFFu) / 255.0;
  let b = f32((packed >> 16) & 0xFFu) / 255.0;
  let a = f32((packed >> 24) & 0xFFu) / 255.0;

    return vec4<f32>(r, g, b, a);
}

struct VS_Out {
    @builtin(position)  X_Clip : vec4<f32>,
    @location(0)        X      : vec3<f32>,
};

@vertex
fn vs_main(@builtin(vertex_index) idx : u32) -> VS_Out {
  var out : VS_Out;

  let X      = X_Buffer[idx];
  out.X_Clip = transpose(Vol_3D.World_View_Projection) * X;
  out.X      = (transpose(Vol_3D.World) * X).xyz;

  return out;
}

const ray_steps     = 64; // 256;
const ray_step_size = sqrt(sqrt(2) + 1) / ray_steps;

fn intersect_ray_box(ray_origin : vec3<f32>, ray_direction : vec3<f32>) -> vec2<f32> {
  let inv_direction = 1.f / ray_direction;

  let t0      = (vec3<f32>(0, 0, 0) - ray_origin) * inv_direction;
  let t1      = (vec3<f32>(1, 1, 1) - ray_origin) * inv_direction;

  let t_min   = min(t0, t1);
  let t_max   = max(t0, t1);

  let t_enter = max(max(t_min.x, t_min.y), t_min.z);
  let t_exit  = min(min(t_max.x, t_max.y), t_max.z);

  return vec2<f32>(t_enter, t_exit);
}


fn transfer_function(value : f32) -> vec4<f32> {
  let color = textureSampleLevel(Texture, Sampler, vec2<f32>(value, 0), 0.0);
  let alpha = 1.0 - exp(-value * ray_step_size);
  return vec4<f32>(color.rgb, alpha * color.a);
}

@fragment
fn fs_main(@location(0) X : vec3<f32>) -> @location(0) vec4<f32> {

  let world_min                         = Vol_3D.Volume_Min;
  let world_max                         = Vol_3D.Volume_Max;
  let extent                            = world_max - world_min;

  // NOTE(cmat): Transform to [0,1] ^ 3
  let ray_origin                        = (Vol_3D.Eye_Position - world_min) / extent;
  let ray_target                        = (X - world_min) / extent;
  let ray_direction                     = normalize(ray_target - ray_origin);
  let t_hit                             = intersect_ray_box(ray_origin, ray_direction);
  let t_enter                           = max(t_hit.x, 0.0);
  let t_exit                            = t_hit.y;

  if (t_exit < t_enter || t_exit < 0.0) {
    discard;
  }

  var accum_color = vec4<f32>(0.0);
  var ray_t       = max(t_enter, 0.0);

  for (var it = 0; it < ray_steps; it++) {
  
    // NOTE(cmat): If dynamic loops supported.
    // if ((ray_t > t_exit) || accum_color.a >  = 1.0) { break; }
    let mask                                    = select(1.0, 0.0, (ray_t > t_exit) || accum_color.a >= 1.0);

    let sample_position                         = ray_origin + ray_t * ray_direction;

    // TODO(cmat): This is very much temporary.
    var sample_uv = vec3<f32>(0, 0, 0);
    if (Vol_3D.Volume_XYZ == 0) {
      sample_uv = vec3<f32>(sample_position.x, sample_position.z, sample_position.y);
    } else {
      sample_uv = vec3<f32>(sample_position.x, sample_position.y, sample_position.z);
    }

    let sample                                  = Vol_3D.Volume_Density * (textureSample(Texture_Volume, Sampler, sample_uv).r);

    let data_min                                = Vol_3D.Volume_Data_Bounds.x;
    let data_max                                = Vol_3D.Volume_Data_Bounds.y;
    let sample_data                             = mix(data_min, data_max, sample);

    let vis_min                                 = Vol_3D.Visualize_Range.x;
    let vis_max                                 = Vol_3D.Visualize_Range.y;
    let sample_clamp                            = clamp(sample_data, vis_min, vis_max);
    let sample_remap                            = (sample_clamp - vis_min) / (vis_max - vis_min);


    let color                                   = Vol_3D.Volume_Saturate * transfer_function(sample_remap);
    accum_color                                += mask * (1.0 - accum_color.a) * vec4<f32>(color.rgb * color.a, color.a);
    ray_t                                      += ray_step_size;
  }

  return 2.f * accum_color;
}
