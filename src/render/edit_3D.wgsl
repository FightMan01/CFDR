// (C) Copyright 2025 Matyas Constans
// Licensed under the MIT License (https://opensource.org/license/mit/)

@group(0) @binding(0)
var Texture : texture_2d<f32>;
  
@group(0) @binding(1)
var Sampler : sampler;

struct World_3D_Type {
  @align(16) World_View_Projection : mat4x4<f32>,
  @align(16) World_Inverse_Transpose: mat4x4<f32>,
  @align(16) World:                   mat4x4<f32>,
  @align(16) Eye_Position          : vec3<f32>,
  @align(16) Volume_Density        : f32,
  @align(16) Grid_Scale            : f32,
  @align(16) Color                 : vec4<f32>,
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
    @location(0)        X : vec3<f32>,
    @location(1)        N : vec3<f32>,
    @location(2)        C : vec4<f32>,
    @location(3)        U : vec2<f32>,
};

@vertex
fn vs_main(@location(0) X : vec3<f32>,
           @location(1) N : vec3<f32>,
           @location(2) U : vec2<f32>,
           @location(3) C : u32) -> VS_Out {

   var out : VS_Out;

   out.X_Clip = transpose(World_3D.World_View_Projection) * vec4<f32>(X, 1.0);
   out.X      = (transpose(World_3D.World) * vec4<f32>(X, 1.0)).xyz;
   out.N      = (transpose(World_3D.World_Inverse_Transpose) * vec4<f32>(N, 0.0)).xyz;
   out.C      = World_3D.Color * vec4_unpack_u32(C);
   out.U      = U;

   return out;
}

@fragment
fn fs_main(@location(0) X : vec3<f32>,
           @location(1) N_in : vec3<f32>,
           @location(2) C : vec4<f32>,
           @location(3) U : vec2<f32> ) -> @location(0) vec4<f32> {

  let E      = World_3D.Eye_Position;
  let N      = normalize(N_in);

  let V      = normalize(E - X);
  let L      = V;
  let H      = normalize(L + V);

  let diffuse = max(dot(N, L), 0.0);
  
  let shininess = 100.0;
  let specular_strength = 0.01;

  let specular = pow(max(dot(N, H), 0.0), shininess) * specular_strength;
  let up = vec3<f32>(0.0, 1.0, 0.0);

  let hemi = 0.5 * dot(N, up) + 0.5;

  let ambient = mix(vec3<f32>(0.12, 0.12, 0.14), vec3<f32>(0.22, 0.24, 0.28), hemi);
  let lighting = ambient + diffuse + specular;

  let color_texture = textureSample(Texture, Sampler, U);
  let pixel         = color_texture * C * vec4<f32>(lighting, 1.0);

  return pixel;
}

