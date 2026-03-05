// (C) Copyright 2025 Matyas Constans
// Licensed under the MIT License (https://opensource.org/license/mit/)

// ------------------------------------------------------------
// #-- Render Commands

R_Command_Buffer R_Commands = {};

fn_internal void r_command_reset(void) {
  R_Commands.first  = 0;
  R_Commands.last   = 0;
  arena_clear(&R_Commands.arena);
}

fn_internal U08 *r_command_push(R_Command_Type type, U64 bytes) {
  var_local_persist B32 buffer_initialized = 0;
  if (!buffer_initialized) {
    buffer_initialized = 1;
    arena_init(&R_Commands.arena);
  }
  
  R_Command_Header *header = (R_Command_Header *)arena_push_size(&R_Commands.arena, bytes + sizeof(R_Command_Header));
  queue_push(R_Commands.first, R_Commands.last, header);
  return ((U08 *)header) + sizeof(R_Command_Header);
}

fn_internal void r_command_push_draw(R_Command_Draw *draw) {
  memory_copy(r_command_push(R_Command_Type_Draw, sizeof(R_Command_Draw)), draw, sizeof(R_Command_Draw));
}

