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

const ray_steps     = 256;
const ray_step_size = sqrt(sqrt(2) + 1) / ray_steps;

@fragment
fn fs_main(@location(0) X : vec3<f32>) -> @location(0) vec4<f32> {


  let s = (X - Vol_3D.Volume_Min) / (Vol_3D.Volume_Max - Vol_3D.Volume_Min);
  if (any(s < vec3<f32>(0, 0, 0)) || any(s > vec3<f32>(1, 1, 1))) {
    discard;
  }

  let sample_position = clamp(s, vec3<f32>(0.0), vec3<f32>(1.0));

  // TODO(cmat): This is very much temporary.
  var sample_uv = vec3<f32>(0, 0, 0);
  if (Vol_3D.Volume_XYZ == 0) {
    sample_uv = vec3<f32>(sample_position.x, sample_position.z, sample_position.y);
  } else {
    sample_uv = vec3<f32>(sample_position.x, sample_position.y, sample_position.z);
  }

  let sample = textureSample(Texture_Volume, Sampler, sample_uv).r;
  // if (sample == 0) { 
  //  discard;
  // }

  let data_min     = Vol_3D.Volume_Data_Bounds.x;
  let data_max     = Vol_3D.Volume_Data_Bounds.y;
  let sample_data  = mix(data_min, data_max, sample);

  let vis_min      = Vol_3D.Visualize_Range.x;
  let vis_max      = Vol_3D.Visualize_Range.y;
  let sample_clamp = clamp(sample_data, vis_min, vis_max);
  var sample_remap = (sample_clamp - vis_min) / (vis_max - vis_min);

  var pixel = textureSample(Texture, Sampler, vec2<f32>(sample_remap, 0));

  if (Vol_3D.Contour_Visible == 1) {
    let contour_value = Vol_3D.Contour_Value;
    let d             = abs(sample_data - contour_value);
    let fw            = fwidth(sample_data);
    let contour_width = fw * Vol_3D.Contour_Thickness;
    let contour       = 1.0 - smoothstep(0.0, contour_width, d);
    let contour_color = vec4<f32>(Vol_3D.Contour_Color, 1);
    var pixel2        = mix(pixel, contour_color, contour);
    return pixel2;
  } else {
    return pixel;
  }
}

