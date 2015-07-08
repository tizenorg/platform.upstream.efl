#include "evas_common_private.h"
#include "evas_private.h"
#include "evas_vg_private.h"
#include "evas_vg_root_node.eo.h"

#include <string.h>

#define MY_CLASS EVAS_VG_ROOT_NODE_CLASS

typedef struct _Evas_VG_Root_Node_Data Evas_VG_Root_Node_Data;
struct _Evas_VG_Root_Node_Data
{
};

static void
evas_vg_root_node_vg_set(Evas_VG_Root_Node *root, Evas_VG *vg)
{
   Evas_Object_Protected_Data *obj_vg;
   Evas_VG_Node_Data *nd;

   nd = eo_data_scope_get(root, EVAS_VG_NODE_CLASS);
   nd->eo_vg = vg;
   if (!vg) return;
   obj_vg = eo_data_scope_get(vg, EVAS_OBJECT_CLASS);
   evas_object_change(vg, obj_vg);
}

static Eina_Bool
_evas_vg_root_node_render_pre(Eo *obj EINA_UNUSED,
                              Eina_Matrix3 *parent,
                              Ector_Surface *s,
                              void *data EINA_UNUSED,
                              Evas_VG_Node_Data *nd)
{
   Evas_VG_Container_Data *cd = eo_data_scope_get(obj, EVAS_VG_CONTAINER_CLASS);
   Eina_List *l;
   Eo *child;
   Eina_Bool change = EINA_FALSE;
   EVAS_VG_COMPUTE_MATRIX(current, parent, nd);

   EINA_LIST_FOREACH(cd->children, l, child)
     change |= _evas_vg_render_pre(child, s, current);

   return change;
}

void
_evas_vg_root_node_eo_base_parent_set(Eo *obj,
                                      Evas_VG_Root_Node_Data *pd EINA_UNUSED,
                                      Eo *parent)
{
   // Nice little hack, jump over parent parent_set in Evas_VG_Root
   eo_do_super(obj, EVAS_VG_NODE_CLASS, eo_parent_set(parent));
   if (parent && !eo_isa(parent, EVAS_VG_CLASS))
     eo_error_set(obj);
}

void
_evas_vg_root_node_eo_base_constructor(Eo *obj,
                                       Evas_VG_Root_Node_Data *pd EINA_UNUSED)
{
   Eo *parent;
   Evas_VG_Node_Data *nd;

   // Nice little hack, jump over parent constructor in Evas_VG_Root
   eo_do_super(obj, EVAS_VG_NODE_CLASS, eo_constructor());
   eo_do(obj, parent = eo_parent_get());
   nd = eo_data_scope_get(obj, EVAS_VG_NODE_CLASS);
   nd->render_pre = &_evas_vg_root_node_render_pre;

   evas_vg_root_node_vg_set(obj, parent);
}

void
_evas_vg_root_node_eo_base_destructor(Eo *obj,
                                      Evas_VG_Root_Node_Data *pd EINA_UNUSED)
{
   evas_vg_root_node_vg_set(obj, NULL);
   eo_do_super(obj, MY_CLASS, eo_destructor());
}

#include "evas_vg_root_node.eo.c"
