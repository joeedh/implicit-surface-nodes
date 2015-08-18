#ifndef _FUNC_INTERFACE_H
#define _FUNC_INTERFACE_H

#ifdef _cplusplus
extern "C" {
#endif

typedef struct StateInfo {
  char error[2048];
  int except;
} StateInfo;

typedef union {
  int    i;
  double f;
  char   *string;
  void   *ptr;
} VarType;

typedef enum {
  INT      = 1,    //no chars or shorts
  DOUBLE   = 2, //no floats
  PYOBJECT = 4,
  STRING   = 8
} ARG_TYPES;

typedef struct BPYInterface {
  void *(*get_bpy)(StateInfo *state);
  char *(*get_exception)(StateInfo *state);
  
  void *(get_attr)(StateInfo *state, char *attr);
  int (*has_attr)(StateInfo *state, char *attr);
  
  void (*set_attr_f)(StateInfo *state, void *obj, char *attr, double f);
  void (*set_attr_s)(StateInfo *state, void *obj, char *attr, char *s);
  void (*set_attr_py)(StateInfo *state, void *obj, char *attr, void *pyobject);
  
  void *(*get_item_i)(StateInfo *state, void *obj, int item);
  void *(*get_item_s)(StateInfo *state, void *obj, char *item);
  
  int (*has_item_i)(StateInfo *state, void *obj, int item);
  int (*has_item_s)(StateInfo *state, void *obj, char *item);
  
  void (*set_item_f_s)(StateInfo *state, void *obj, char *item, double f);
  void (*set_item_f_i)(StateInfo *state, void *obj, int item, double f);
  
  void (*set_item_s_s)(StateInfo *state, void *obj, char *item, char *f);
  void (*set_item_s_i)(StateInfo *state, void *obj, int item, char *f);
  
  void (*set_item_py_s)(StateInfo *state, void *obj, char *item, void *f);
  void (*set_item_py_i)(StateInfo *state, void *obj, int item, void *f);
  
  int   (*len)(StateInfo *state, void *obj);
  void *(*iter)(StateInfo *state, void *obj);
  
  void *(*str)(StateInfo *state, void *obj);
  void *(*repr)(StateInfo *state, void *obj);
  
  void *(*dir)(StateInfo *state, void *obj);
  
  void *(*call)(StateInfo *state, void *obj, VarType **argv, int *argc, int *argtype);
  
  void (*ref)(StateInfo *state, void *obj);
  void (*unref)(StateInfo *state, void *obj);
};

#define MAXARG 64

typedef struct ArgTypes {
  char types[MAXARG];
} ArgTypes;

typedef struct OutStorage {
  char data[32];
} OutStorage;

typedef struct FuncType {
  char name[64];
  char argtypes[32];
  int totarg;
  int return_type;
  int (*func)(StateInfo *info, VarType **argv, int argc);
} FuncType;

FuncType getobj = {
  "get_object",
  {STRING},
  1,
  POINTER,
  NULL
};

//returns nonzero on error
int (*BpyInterfaceFunction)(StateInfo *state, char **argc, int argv);


#ifdef _cplusplus
}
#endif
#endif /* _FUNC_INTERFACE_H */
