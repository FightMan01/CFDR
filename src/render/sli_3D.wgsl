@group(0) @binding(0)
var Texture : texture_2d<f32>;

@group(0) @binding(1)
var Sampler : sampler;

struct World_3D_Type {
  @align(16) World_View_Projection   : mat4x4<f32>,
  @align(16) World_Inverse_Transpose : mat4x4<f32>,
  @align(16) World                   : mat4x4<f32>,
  @align(16) Eye_Position            : vec3<f32>,
  @align(16) Volume_Density          : f32,
  @align(16) Grid_Scale              : f32,
  @align(16) Color                   : vec4<f32>,
  @align(16) Volume_Min              : vec3<f32>,
  @align(16) Volume_Max              : vec3<f32>,
  @align(16) Volume_Data_Bounds      : vec2<f32>,
  @align(16) Visualize_Range         : vec2<f32>,
  @align(16) Volume_Saturate         : f32,
  @align(16) Volume_XYZ              : i32,
};

@group(0) @binding(2)
var<uniform> World_3D : World_3D_Type;

@group(0) @binding(3)
var Texture_Volume : texture_3d<f32>;

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
    @location(1)        C      : vec4<f32>,
    @location(2)        U      : vec2<f32>,
};

@vertex
fn vs_main(@location(0) X : vec3<f32>,
           @location(1) N : vec3<f32>,
           @location(2) U : vec2<f32>,
           @location(3) C : u32) -> VS_Out {

   var out : VS_Out;

   out.X_Clip = transpose(World_3D.World_View_Projection) * vec4<f32>(X, 1.0);
   out.X      = (transpose(World_3D.World) * vec4<f32>(X, 1.0)).xyz;
   out.C      = vec4_unpack_u32(C);
   out.U      = U;

   return out;
}

const ray_steps     = 256;
const ray_step_size = sqrt(sqrt(2) + 1) / ray_steps;

@fragment
fn fs_main(@location(0) X : vec3<f32>,
           @location(1) C : vec4<f32>,
           @location(2) U : vec2<f32> ) -> @location(0) vec4<f32> {

  let sample_position = clamp((X - World_3D.Volume_Min) / (World_3D.Volume_Max - World_3D.Volume_Min), vec3<f32>(0.0), vec3<f32>(1.0));
  // let p  = vec3<f32>(p2.z, 1.0 - p2.x, p2.y);

  // TODO(cmat): This is very much temporary.
  var sample_uv       = vec3<f32>(0, 0, 0);
  if (World_3D.Volume_XYZ == 0) {
    // sample_uv       = vec3<f32>(sample_position.z, 1.0 - sample_position.x, sample_position.y);
    sample_uv       = vec3<f32>(sample_position.x, sample_position.z, sample_position.y);
  } else {
    sample_uv       = vec3<f32>(sample_position.x, sample_position.y, sample_position.z);
  }

  let sample = textureSample(Texture_Volume, Sampler, sample_uv).r;
  // if (sample == 0) { 
  //  discard;
  // }

  // let sample_range = World_3D.Visualize_Range * (World_3D.Volume_Data_Bounds.y - World_3D.Volume_Data_Bounds.x) - World_3D.Volume_Data_Bounds.x;
  // let sample_clamp = clamp(sample, sample_range.x, sample_range.y);
  // let sample_remap = sample_clamp * (sample_range.y - sample_range.x) - sample_range.x;
 
  let data_min     = World_3D.Volume_Data_Bounds.x;
  let data_max     = World_3D.Volume_Data_Bounds.y;
  let sample_data  = mix(data_min, data_max, sample);

  let vis_min      = World_3D.Visualize_Range.x;
  let vis_max      = World_3D.Visualize_Range.y;
  let sample_clamp = clamp(sample_data, vis_min, vis_max);
  let sample_remap = (sample_clamp - vis_min) / (vis_max - vis_min);
  
  let pixel = textureSample(Texture, Sampler, vec2<f32>(sample_remap, 0));
  return pixel;
}

