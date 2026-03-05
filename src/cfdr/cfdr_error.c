typedef struct CFDR_Error_Node {
  struct CFDR_Error_Node *next;
} CFDR_Error_Node;

typedef struct CFDR_Error_List {
  U32              count;
  CFDR_Error_Node *first;
  CFDR_Error_Node *last;
} CFDR_Error_List;

#if 0
fn_internal void cfdr_error_push(CFDR_Error_List *error_list, Str reason) {
  queue_push(error_list->first, error_list->last, );
}
#endif
