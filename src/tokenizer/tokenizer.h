// (C) Copyright 2025 Matyas Constans
// Licensed under the MIT License (https://opensource.org/license/mit/)

#define TK_Token_X(name_, value_)

#define TK_Token_List_Fixed_X                                \
  TK_Token_X(Op_Exclamation,          "!")                   \
  TK_Token_X(Op_Backtick,             "`")                   \
  TK_Token_X(Op_Tilde,                "~")                   \
  TK_Token_X(Op_At,                   "@")                   \
  TK_Token_X(Op_Hash,                 "#")                   \
  TK_Token_X(Op_Dollar,               "$")                   \
  TK_Token_X(Op_Percent,              "%")                   \
  TK_Token_X(Op_Caret,                "^")                   \
  TK_Token_X(Op_Ampersand,            "&")                   \
  TK_Token_X(Op_Star,                 "*")                   \
  TK_Token_X(Op_Minus,                "-")                   \
  TK_Token_X(Op_Plus,                 "+")                   \
  TK_Token_X(Op_Assign,               "=")                   \
  TK_Token_X(Op_Comma,                ",")                   \
  TK_Token_X(Op_Colon,                ":")                   \
  TK_Token_X(Op_Semi_Colon,           ";")                   \
  TK_Token_X(Op_Question_Mark,        "?")                   \
  TK_Token_X(Op_Slash,                "/")                   \
  TK_Token_X(Op_Backslash,            "\\")                  \
  TK_Token_X(Op_Vertical_Bar,         "|")                   \
  TK_Token_X(Op_Period,               ".")                   \
  TK_Token_X(Op_Less_Than,            "<")                   \
  TK_Token_X(Op_Greater_Than,         ">")                   \
\
  TK_Token_X(Block_Open_Bracket,      "[")                   \
  TK_Token_X(Block_Close_Bracket,     "]")                   \
  TK_Token_X(Block_Open_Paren,        "(")                   \
  TK_Token_X(Block_Close_Paren,       ")")                   \
  TK_Token_X(Block_Open_Brace,        "{")                   \
  TK_Token_X(Block_Close_Brace,       "}")                   \
\
  TK_Token_X(Op_Less_Than_Equal,      "<=")                  \
  TK_Token_X(Op_Greater_Than_Equal,   ">=")                  \
  TK_Token_X(Op_Compare_Equal,        "==")                  \
  TK_Token_X(Op_Compare_Not_Equal,    "!=")                  \
  TK_Token_X(Op_Bool_And,             "&&")                  \
  TK_Token_X(Op_Bool_Or,              "||")                  \
  TK_Token_X(Op_Right_Arrow,          "->")                  \
  TK_Token_X(Op_Bind,                 "::")                  \
  TK_Token_X(Op_Right_Shift,          ">>")                  \
  TK_Token_X(Op_Left_Shift,           "<<")                  \
  TK_Token_X(Op_Increment,            "++")                  \
  TK_Token_X(Op_Decrement,            "--")                  \
  TK_Token_X(Op_Plus_Equal,           "+=")                  \
  TK_Token_X(Op_Minus_Equal,          "-=")                  \
  TK_Token_X(Op_Tilde_Equal,          "~=")                  \
  TK_Token_X(Op_Percent_Equal,        "%=")                  \
  TK_Token_X(Op_Star_Equal,           "*=")                  \
  TK_Token_X(Op_Slash_Equal,          "/=")                  \
  TK_Token_X(Op_Right_Shift_Equal,    ">>=")                 \
  TK_Token_X(Op_Left_Shift_Equal,     "<<=")                 \
  TK_Token_X(Op_Bar_Equal,            "|=")                  \
  TK_Token_X(Op_Ampersand_Equal,      "&=")                  \
  TK_Token_X(Op_Bool_And_Equal,       "&&=")                 \
  TK_Token_X(Op_Bool_Or_Equal,        "||=")                 \
  TK_Token_X(Op_Caret_Equal,          "^=")                  \

#define TK_Token_List_Variable_X                             \
  TK_Token_X(Identifier,              "identifier")          \
  TK_Token_X(Comment_Line,            "comment-line")        \
  TK_Token_X(Comment_Multi,           "comment-multi")       \
  TK_Token_X(Literal_Integer,         "literal-integer")     \
  TK_Token_X(Literal_Float,           "literal-float")       \
  TK_Token_X(Literal_Character,       "literal-character")   \
  TK_Token_X(Literal_String,          "literal-string")      \
  TK_Token_X(Invalid,                 "invalid")             \
  TK_Token_X(End_Of_Stream,           "end-of-stream")

#define TK_Token_List_X             \
  TK_Token_List_Fixed_X             \
  TK_Token_List_Variable_X

typedef U32 TK_Type;
enum {
# undef  TK_Token_X
# define TK_Token_X(name_, value_) TK_Type_ ## name_,
  TK_Token_List_X
};

var_global Str TK_Token_Type_Fixed_Str_List[] = {
# undef  TK_Token_X
# define TK_Token_X(name_, value_) str_lit(value_),
  TK_Token_List_Fixed_X
};

fn_internal inline Str tk_str_from_type(TK_Type type) {
  Str result;
  zero_fill(&result);

  switch (type) {
    Invalid_Default;
#   undef TK_Token_X
#   define TK_Token_X(name_, value_) case TK_Type_ ## name_: { result = str_lit(value_); } break;
    TK_Token_List_X
  }

  return result;
}

typedef union TK_Value {
  I64 i64;
  F64 f64;
  Str str;
} TK_Value;

typedef struct TK_Token {
  TK_Type  type;
  Str      text;
  TK_Value value;

  U32      line;
  U32      column;
} TK_Token;

typedef struct TK_Error {
  struct TK_Error  *next;
  U32               line;
  U32               column;
  Str               message;
} TK_Error;

typedef struct TK_Tokenizer {
  Arena    *arena;
  TK_Error *first_error;
  TK_Error *last_error;

  Str       text;
  U64       at;

  U32       line_at;
  U32       column_at;
} TK_Tokenizer;

typedef struct TK_Token_Node {
  struct TK_Token_Node *next;
  TK_Token token;
} TK_Token_Node;

typedef struct TK_Token_List {
  U64                    count;
  struct TK_Token_Node  *first;
  struct TK_Token_Node  *last;
} TK_Token_List;

typedef struct TK_Scan {
  TK_Token_List  list;
  TK_Token_Node *token_at;
} TK_Scan;

fn_internal void          tk_init           (TK_Tokenizer *tk, Arena *arena, Str text);
fn_internal TK_Token      tk_next           (TK_Tokenizer *tk);
fn_internal TK_Error     *tk_error_list     (TK_Tokenizer *tk);
fn_internal TK_Token_List tk_tokenize       (TK_Tokenizer *tk, Str text);
fn_internal TK_Scan       tk_scan_init      (TK_Token_List list);
fn_internal TK_Token      tk_scan_look      (TK_Scan *scan);
fn_internal TK_Token      tk_scan_next      (TK_Scan *scan);
fn_internal B32           tk_scan_require   (TK_Scan *scan, TK_Type type, TK_Token *token);

