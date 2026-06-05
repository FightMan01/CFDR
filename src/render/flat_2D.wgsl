struct Quad {
  position    : vec2<f32>,
  size        : vec2<f32>,
  uv_position : vec2<f32>,
  uv_size     : vec2<f32>,
  color       : array<u32, 4>,
  rounding    : vec4<f32>,
};

@group(0) @binding(0)
var<storage, read> Quad_Buffer: array<Quad>;

@group(0) @binding(1)
var Texture : texture_2d<f32>;
  
@group(0) @binding(2)
var Sampler : sampler;

struct Viewport_2D_Type {
  @align(16) NDC_From_Screen : mat4x4<f32>,
  @align(16) Viewport_Size   : vec2<f32>,
};

@group(0) @binding(3)
var<uniform> Viewport_2D : Viewport_2D_Type;

@group(0) @binding(4)
var Texture_Volume : texture_3d<f32>;

fn vec4_unpack_u32(packed: u32) -> vec4<f32> {
  let r = f32((packed >> 0)  & 0xFFu) / 255.0;
  let g = f32((packed >> 8)  & 0xFFu) / 255.0;
  let b = f32((packed >> 16) & 0xFFu) / 255.0;
  let a = f32((packed >> 24) & 0xFFu) / 255.0;

    return vec4<f32>(r, g, b, a);
}

struct VS_Out {
    @builtin(position)              X             : vec4<f32>,
    @location(0)                    U             : vec2<f32>,
    @location(1)                    C             : vec4<f32>,
    @location(2)                    Quad_X        : vec2<f32>,
    @location(3)                    Quad_Radius   : vec2<f32>,
    @location(4)                    Quad_Rounding : f32,
};

@vertex
fn vs_main(@builtin(vertex_index) idx : u32) -> VS_Out {
  const Vertices = array<vec2<f32>, 4>(vec2<f32>(0, 0), vec2<f32>(1, 0), vec2<f32>(1, 1), vec2<f32>(0, 1));

  var quad      = Quad_Buffer[idx / 4];
  let V         = Vertices[idx % 4];
  let X         = quad.position    + quad.size    * V;
  let U         = quad.uv_position + quad.uv_size * V;
  let C         = vec4_unpack_u32(quad.color[idx % 4]);

  var vout : VS_Out;
  vout.X             = transpose(Viewport_2D.NDC_From_Screen) * vec4<f32>(X, 0.0, 1.0);
  vout.U             = U;
  vout.C             = C;
  vout.Quad_Radius   = 0.5 * quad.size;
  vout.Quad_X        = (2.0 * V - vec2<f32>(1.0, 1.0)) * vout.Quad_Radius;
  vout.Quad_Rounding = quad.rounding[idx % 4];

  return vout;
}

fn sdf_rect(x : vec2<f32>, r : vec2<f32>, rounding : f32) -> f32 {
  return length(max(abs(x) - r + rounding, vec2<f32>(0.0, 0.0))) - rounding;
}

@fragment
fn fs_main(@location(0)                     U             : vec2<f32>,
           @location(1)                     C             : vec4<f32>,
           @location(2)                     Quad_X        : vec2<f32>,
           @location(3)                     Quad_Radius   : vec2<f32>,
           @location(4)                     Quad_Rounding : f32,
           ) -> @location(0) vec4<f32> {

  let color_texture = textureSample(Texture, Sampler, U);
  let color         = color_texture * C;

  var alpha = 1.0;
  if (Quad_Rounding > 0) {
    // NOTE(cmat): Make -1.f a bias parameter
    let sdf   = sdf_rect(Quad_X, Quad_Radius - 1.0f, Quad_Rounding);
    alpha = 1.0 - smoothstep(0, 2, sdf);
  }

  return alpha * color;
}
