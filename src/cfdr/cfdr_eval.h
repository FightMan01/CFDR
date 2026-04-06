typedef U32 CFDR_Value_Type;
enum {
  CFDR_Value_Type_None,
  CFDR_Value_Type_List,
  CFDR_Value_Type_Table,
  CFDR_Value_Type_Color,
  CFDR_Value_Type_Bool,
  CFDR_Value_Type_Integer,
  CFDR_Value_Type_String,
  CFDR_Value_Type_Align,
};

struct CFDR_List;
struct CFDR_Table;

typedef struct CFDR_Value {
  CFDR_Value_Type type;
  union {
    struct CFDR_List  *list;
    struct CFDR_Table *table;
    V3F                color;
    B32                b32;
    B32                i32;
    Str                str;
    Align2             align;
  };
} CFDR_Value;

typedef struct CFDR_List_Node {
  struct CFDR_List_Node *next;
  CFDR_Value             value;
} CFDR_List_Node;

typedef struct CFDR_List {
  U64             count;
  CFDR_List_Node *first;
  CFDR_List_Node *last;
} CFDR_List;

typedef struct CFDR_Table_Node {
  struct CFDR_Table_Node *next;
  Str        label;
  CFDR_Value value;
} CFDR_Table_Node;

typedef struct CFDR_Table {
  U64 count;
  CFDR_Table_Node *first;
  CFDR_Table_Node *last;
} CFDR_Table;

fn_internal CFDR_Value  cfdr_eval_expr  (Arena *arena, TK_Scan *scan);
fn_internal void        cfdr_eval       (CFDR_State *state, Str expr);

