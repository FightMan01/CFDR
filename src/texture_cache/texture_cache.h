// (C) Copyright 2025 Matyas Constans
// Licensed under the MIT License (https://opensource.org/license/mit/)

typedef struct TC_Packer {
  V2_U16       atlas_size;
  Array_V2_U16 nodes;
} TC_Packer;

fn_internal void  tc_packer_init  (TC_Packer *sk, Arena *arena, V2_U16 atlas_size);
fn_internal void  tc_packer_reset (TC_Packer *sk);
fn_internal B32   tc_packer_push  (TC_Packer *sk, V2_U16 rect, U16 border, V2_U16 *packed_position);

