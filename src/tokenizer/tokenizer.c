// (C) Copyright 2025 Matyas Constans
// Licensed under the MIT License (https://opensource.org/license/mit/)

fn_internal void tk_init(TK_Tokenizer *tk, Arena *arena, Str text) {
  zero_fill(tk);
  tk->arena   = arena;
  tk->text    = text;
  tk->at      = 0;
  tk->line_at = 1;
}

fn_internal U08 tk_peek(TK_Tokenizer *tk, U32 offset) {
  U08 result = 0;
  if (tk->at + offset < tk->text.len) {
    result = tk->text.txt[tk->at + offset];
  }

  return result;
}

fn_internal B32 tk_peek_match(TK_Tokenizer *tk, Str match) {
  B32 result = 0;

  U64 match_start = tk->at;
  U64 match_end   = tk->at + match.len;

  if (match_end < tk->text.len) {
    result = str_equals(str_slice(tk->text, match_start, match_end - match_start), match);
  }

  return result;
}

fn_internal void tk_move(TK_Tokenizer *tk, U32 offset) {
  For_U64 (it, offset) {
    if (tk->at >= tk->text.len) {
      break;
    }

    if (tk->text.txt[tk->at] == '\n') {
      tk->line_at   += 1;
      tk->column_at  = 0;
    }

    tk->at        += 1;
    tk->column_at += 1;
  }
}

fn_internal void tk_skip_whitespace(TK_Tokenizer *tk) {
  while (char_is_whitespace(tk_peek(tk, 0))) {
    tk_move(tk, 1);
  }
}

fn_internal void tk_skip_line(TK_Tokenizer *tk) {
  for (;;) {
    U08 c = tk_peek(tk, 0);
    if (char_is_linefeed(c) || c == 0) break;
    tk_move(tk, 1);
  }
}

fn_internal void tk_error_push(TK_Tokenizer *tk, U32 line_at, U32 column_at, Str message) {
  TK_Error *error = arena_push_type(tk->arena, TK_Error);
  error->line     = line_at;
  error->column   = column_at;
  error->message  = message;

  queue_push(tk->first_error, tk->last_error, error);
}

fn_internal TK_Error *tk_error_list(TK_Tokenizer *tk) {
  TK_Error *error = tk->first_error;
  return error;
}

fn_internal TK_Token tk_next(TK_Tokenizer *tk) {
  TK_Token token = {
    .type = TK_Type_Invalid
  };

  tk_skip_whitespace(tk);

  U64 token_start = tk->at;
  U08 c = tk_peek(tk, 0);  

  // NOTE(cmat): End of stream.
  if (c == 0) {
    token.type = TK_Type_End_Of_Stream;

  // NOTE(cmat): Single-line comment.
  } else if (c == '/' && tk_peek(tk, 1) == '/') {
    tk_move(tk, 2);

    token.type = TK_Type_Comment_Line;
    tk_skip_line(tk);

  // NOTE(cmat): Multi-line nested comments.
  } else if (c == '/' && tk_peek(tk, 1) == '*') {
    U32 line_at   = tk->line_at;
    U32 column_at = tk->column_at;

    tk_move(tk, 2);
    token.type = TK_Type_Comment_Multi;

    U32 depth = 1;
    for (;;) {
      c = tk_peek(tk, 0);
      if (c == 0) {
        tk_error_push(tk, line_at, column_at, str_lit("unclosed multi-line comment-block (*/ missing)"));
        break;
      } else if (c == '/' && tk_peek(tk, 1) == '*') { 
        tk_move(tk, 2);
        depth += 1;
      } else if (c == '*' && tk_peek(tk, 1) == '/') {
        tk_move(tk, 2);
        depth -= 1;
      }

      if (depth <= 0) {
        break;
      }

      tk_move(tk, 1);
    }

  // NOTE(cmat): Character literals
  } else if (c == '\'') {
    U32 line_at   = tk->line_at;
    U32 column_at = tk->column_at;

    tk_move(tk, 1);
    token.type = TK_Type_Literal_Character;

    c = tk_peek(tk, 0);

    if (c == '\'') {
      tk_error_push(tk, line_at, column_at, str_lit("empty character literal"));

    } else {
      tk_move(tk, 1);
      c = tk_peek(tk, 0);
      if (c != '\'') {
        tk_error_push(tk, line_at, column_at, str_lit("unclosed character literal"));
      } else  {
        tk_move(tk, 1);
      }
    }

  // NOTE(cmat): String literals
  } else if (c == '"') {
    U32 line_at   = tk->line_at;
    U32 column_at = tk->column_at;

    tk_move(tk, 1);
    token.type = TK_Type_Literal_String;

    for (;;){
      c = tk_peek(tk, 0);
      if (c == 0) {
        tk_error_push(tk, line_at, column_at, str_lit("unclosed string literal"));
        break;
      }

      if (c == '\\') {
        tk_move(tk, 1);
      }

      if (c == '"') {
        tk_move(tk, 1);
        break;
      }
 
      tk_move(tk, 1);
    }

  // NOTE(cmat): Identifier.
  } else if (char_is_alpha(c) || c == '_') {
    tk_move(tk, 1);
    token.type = TK_Type_Identifier;

    for (;;) {
      c = tk_peek(tk, 0);
      if (!(char_is_alpha(c) || char_is_digit(c) || c == '_')) {
        break;
      }

      tk_move(tk, 1);
    }
  }  else if (char_is_digit(c)) {
    tk_move(tk, 1);
    token.type = TK_Type_Literal_Integer;

    for (;;) {
      c = tk_peek(tk, 0);
      if (!(char_is_digit(c) || c == '_')) {
        break;
      }

      tk_move(tk, 1);
    }
  // NOTE(cmat): Fixed-length tokens.
  } else {
    switch (c) {
      case '`': { tk_move(tk, 1); token.type = TK_Type_Op_Backtick;        }  break;
      case '@': { tk_move(tk, 1); token.type = TK_Type_Op_At;              }  break;
      case '#': { tk_move(tk, 1); token.type = TK_Type_Op_Hash;            }  break;
      case '$': { tk_move(tk, 1); token.type = TK_Type_Op_Dollar;          }  break;
      case ',': { tk_move(tk, 1); token.type = TK_Type_Op_Comma;           }  break;
      case ';': { tk_move(tk, 1); token.type = TK_Type_Op_Semi_Colon;      }  break;
      case '?': { tk_move(tk, 1); token.type = TK_Type_Op_Question_Mark;   }  break;
      case '\\': { tk_move(tk, 1); token.type = TK_Type_Op_Backslash;      }  break;
      case '.': { tk_move(tk, 1); token.type = TK_Type_Op_Period;          }  break;

      case '[': { tk_move(tk, 1); token.type = TK_Type_Block_Open_Bracket;   }  break;
      case ']': { tk_move(tk, 1); token.type = TK_Type_Block_Close_Bracket;  }  break;
      case '(': { tk_move(tk, 1); token.type = TK_Type_Block_Open_Paren;     }  break;
      case ')': { tk_move(tk, 1); token.type = TK_Type_Block_Close_Paren;    }  break;
      case '{': { tk_move(tk, 1); token.type = TK_Type_Block_Open_Brace;     }  break;
      case '}': { tk_move(tk, 1); token.type = TK_Type_Block_Close_Brace;    }  break;

      case '<': {
        tk_move(tk, 1);
        token.type = TK_Type_Op_Less_Than;

        if (tk_peek(tk, 0) == '=') {
          tk_move(tk, 1);
          token.type = TK_Type_Op_Less_Than_Equal;
        } else if (tk_peek(tk, 0) == '<') {
          tk_move(tk, 1);
          token.type = TK_Type_Op_Left_Shift;
          if (tk_peek(tk, 0) == '=') {
            tk_move(tk, 1);
            token.type = TK_Type_Op_Left_Shift_Equal;
          }
        }
      }  break;

      case '>': {
        tk_move(tk, 1);
        token.type = TK_Type_Op_Greater_Than;
        if (tk_peek(tk, 0) == '=') {
          tk_move(tk, 1);
          token.type = TK_Type_Op_Greater_Than_Equal;
        } else if (tk_peek(tk, 0) == '>') {
          tk_move(tk, 1);
          token.type = TK_Type_Op_Right_Shift;
          if (tk_peek(tk, 0) == '=') {
            tk_move(tk, 1);
            token.type = TK_Type_Op_Right_Shift_Equal;
          }
        }
      } break;

      case '=': {
        tk_move(tk, 1);
        token.type = TK_Type_Op_Assign;
        if (tk_peek(tk, 0) == '=') {
          tk_move(tk, 1);
          token.type = TK_Type_Op_Compare_Equal;
        }
      } break;

      case '&': {
        tk_move(tk, 1);
        token.type = TK_Type_Op_Ampersand;
        if (tk_peek(tk, 0) == '&') {
          tk_move(tk, 1);
          token.type = TK_Type_Op_Bool_And;
          if (tk_peek(tk, 0) == '=') {
            tk_move(tk, 1);
            token.type = TK_Type_Op_Bool_And_Equal;
          }

        } else if (tk_peek(tk, 0) == '=') {
          tk_move(tk, 1);
          token.type = TK_Type_Op_Ampersand_Equal;
        }
      } break;
 
      case '|': {
        tk_move(tk, 1);
        token.type = TK_Type_Op_Ampersand;
        if (tk_peek(tk, 0) == '|') {
          tk_move(tk, 1);
          token.type = TK_Type_Op_Bool_Or;
          if (tk_peek(tk, 0) == '=') {
            tk_move(tk, 1);
            token.type = TK_Type_Op_Bool_Or_Equal;
          }
        } else if (tk_peek(tk, 0) == '=') {
          tk_move(tk, 1);
          token.type = TK_Type_Op_Bar_Equal;
        }
      } break;

      case '+': {
        tk_move(tk, 1);
        token.type = TK_Type_Op_Plus;
        if (tk_peek(tk, 0) == '>') {
          tk_move(tk, 1);
          token.type = TK_Type_Op_Increment;
        } else if (tk_peek(tk, 0) == '=') {
          tk_move(tk, 1);
          token.type = TK_Type_Op_Plus_Equal;
        }
      } break;

      case '-': {
        tk_move(tk, 1);
        token.type = TK_Type_Op_Minus;
        if (tk_peek(tk, 0) == '>') {
          tk_move(tk, 1);
          token.type = TK_Type_Op_Right_Arrow;
        } else if (tk_peek(tk, 0) == '-') {
          tk_move(tk, 1);
          token.type = TK_Type_Op_Decrement;
        } else if (tk_peek(tk, 0) == '=') {
          tk_move(tk, 1);
          token.type = TK_Type_Op_Minus_Equal;
        }
     } break;

      case ':': {
        tk_move(tk, 1);
        token.type = TK_Type_Op_Colon;
        if (tk_peek(tk, 0) == ':') {
          tk_move(tk, 1);
          token.type = TK_Type_Op_Bind;
        }
      } break;

      case '%': {
        tk_move(tk, 1);
        token.type = TK_Type_Op_Percent;
        if (tk_peek(tk, 0) == '=') {
          tk_move(tk, 1);
          token.type = TK_Type_Op_Percent_Equal;
        }
      } break;

      case '*': {
        tk_move(tk, 1);
        token.type = TK_Type_Op_Star;
        if (tk_peek(tk, 0) == '=') {
          tk_move(tk, 1);
          token.type = TK_Type_Op_Star_Equal;
        }
      } break;
 
      case '~': {
        tk_move(tk, 1);
        token.type = TK_Type_Op_Tilde;
        if (tk_peek(tk, 0) == '=') {
          tk_move(tk, 1);
          token.type = TK_Type_Op_Tilde_Equal;
        }
      } break;
 
      case '/': {
        tk_move(tk, 1);
        token.type = TK_Type_Op_Slash;
        if (tk_peek(tk, 0) == '=') {
          tk_move(tk, 1);
          token.type = TK_Type_Op_Slash_Equal;
        }
      } break;
 
      case '!': {
        tk_move(tk, 1);
        token.type = TK_Type_Op_Exclamation;
        if (tk_peek(tk, 0) == '=') {
          tk_move(tk, 1);
          token.type = TK_Type_Op_Compare_Not_Equal;
        }
      } break;
 
      case '^': {
        tk_move(tk, 1);
        token.type = TK_Type_Op_Caret;
        if (tk_peek(tk, 0) == '=') {
          tk_move(tk, 1);
          token.type = TK_Type_Op_Caret_Equal;
        }
      } break;
    }
  }

  U64 token_end = u32_min(tk->at, tk->text.len);
  token.text = str_slice(tk->text, token_start, token_end - token_start);

  if (token.type == TK_Type_Invalid) {
    tk_move(tk, 1);
  }

  // NOTE(cmat): Compute token value.
  switch (token.type) {
    case TK_Type_Literal_Integer: {
      token.value.i64 = (I64)u64_from_str(token.text);
    } break;
    case TK_Type_Literal_String: {
      token.value.str = str_slice(token.text, 1, token.text.len - 2);
    } break;
  }

  return token;
}

fn_internal B32 tk_require(TK_Tokenizer *tk, TK_Type type, TK_Token *token_ptr) {
  TK_Token token = tk_next(tk);
  if (token_ptr) {
    *token_ptr = token;
  }

  if (token.type != type) {
    Str type_got_str      = tk_str_from_type(token.type);
    Str type_expected_str = tk_str_from_type(type);
    log_fatal("expected %.*s, got %.*s (%.*s)",
        str_expand(type_expected_str),
        str_expand(token.text),
        str_expand(type_got_str));
  }

  return token.type == type;
}

fn_internal TK_Token_List tk_tokenize(TK_Tokenizer *tk, Str text) {
  TK_Token_List list = { };

  for (;;) {
    TK_Token token = tk_next(tk);
    TK_Token_Node *node = arena_push_type(tk->arena, TK_Token_Node);
    node->token = token;
    queue_push(list.first, list.last, node);

    if (node->token.type == TK_Type_End_Of_Stream) {
      break;
    }
  }

  return list;
}

fn_internal TK_Scan tk_scan_init(TK_Token_List list) {
  TK_Scan scan  = { };
  scan.list     = list;
  scan.token_at = scan.list.first;

  Assert(scan.token_at, "empty token list (should at least have an end of stream element)");
  return scan;
}

fn_internal void tk_scan_skip_comment(TK_Scan *scan) {
  for (;;) {
    if (scan->token_at->token.type  == TK_Type_Comment_Line ||
        scan->token_at->token.type  == TK_Type_Comment_Multi) {
      scan->token_at = scan->token_at->next ? scan->token_at->next : scan->token_at;
    } else {
      break;
    }
  }
}

fn_internal TK_Token tk_scan_next(TK_Scan *scan) {
  tk_scan_skip_comment(scan);
  TK_Token token = scan->token_at->token;
  scan->token_at = scan->token_at->next ? scan->token_at->next : scan->token_at;

  return token;
}

fn_internal TK_Token tk_scan_look(TK_Scan *scan) {
  tk_scan_skip_comment(scan);
  return scan->token_at->token;
}

fn_internal B32 tk_scan_require(TK_Scan *scan, TK_Type type, TK_Token *token) {
  TK_Token  value = scan->token_at->token;
  B32       match = value.type == type;

  if (!match) {
    Str expected_type   = tk_str_from_type(type);
    Str got_type        = tk_str_from_type(value.type);
    log_fatal("expected %.*s, got %.*s (%.*s)", str_expand(expected_type), str_expand(value.text), str_expand(got_type));
  }

  if (token) { *token  = value; }
  scan->token_at = scan->token_at->next ? scan->token_at->next : scan->token_at;

  return match;
}
