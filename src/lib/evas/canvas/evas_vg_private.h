#ifndef EVAS_VG_PRIVATE_H_
# define EVAS_VG_PRIVATE_H_

#include <Ector.h>

typedef struct _Evas_VG_Node_Data Evas_VG_Node_Data;
typedef struct _Evas_VG_Container_Data Evas_VG_Container_Data;
typedef struct _Evas_VG_Gradient_Data Evas_VG_Gradient_Data;

struct _Evas_VG_Node_Data
{
   Eina_Matrix3 *m;
   Evas_VG_Node *mask;
   Ector_Renderer *renderer;
   Evas_VG *eo_vg;

   Eina_Bool (*render_pre)(Eo *obj, Eina_Matrix3 *parent, Ector_Surface *s, void *data, Evas_VG_Node_Data *nd);
   void *data;

   double x, y;
   int r, g, b, a;

   Eina_Bool visibility : 1;
   Eina_Bool changed : 1;
};

struct _Evas_VG_Container_Data
{
   Eina_List *children;
};

struct _Evas_VG_Gradient_Data
{
   // FIXME: Later on we should deduplicate it somehow (Using Ector ?).
   Efl_Graphics_Gradient_Stop *colors;
   unsigned int colors_count;

   Efl_Graphics_Gradient_Spread s;
};

static inline Eina_Bool
_evas_vg_render_pre(Evas_VG_Node *node, Ector_Surface *s, Eina_Matrix3 *m)
{
   Evas_VG_Node_Data *nd;

   if (!node) return EINA_FALSE;

   // FIXME: Prevent infinite loop
   nd = eo_data_scope_get(node, EVAS_VG_NODE_CLASS);
   if (nd->render_pre) return nd->render_pre(node, m, s, nd->data, nd);
   else return EINA_FALSE;
}

#define EVAS_VG_COMPUTE_MATRIX(Current, Parent, Nd)              \
  Eina_Matrix3 *Current = Nd->m;                                 \
  Eina_Matrix3 _matrix_tmp;                                      \
                                                                 \
  if (Parent)                                                           \
    {                                                                   \
       if (Current)                                                     \
         {                                                              \
            eina_matrix3_compose(Parent, Current, &_matrix_tmp);        \
            Current = &_matrix_tmp;                                     \
         }                                                              \
       else                                                             \
         {                                                              \
            Current = Parent;                                           \
         }                                                              \
    }

#endif
