#include "evas_common_private.h"
#include "evas_private.h"

#include "evas_vg_private.h"

#include <strings.h>

#define MY_CLASS EVAS_VG_GRADIENT_CLASS

static void
_evas_vg_gradient_efl_graphics_gradient_stop_set(Eo *obj,
                                                 Evas_VG_Gradient_Data *pd,
                                                 const Efl_Graphics_Gradient_Stop *colors,
                                                 unsigned int length)
{
   pd->colors = realloc(pd->colors, length * sizeof(Efl_Graphics_Gradient_Stop));
   if (!pd->colors)
     {
        pd->colors_count = 0;
        return ;
     }

   memcpy(pd->colors, colors, length * sizeof(Efl_Graphics_Gradient_Stop));
   pd->colors_count = length;

   eo_do_super(obj, MY_CLASS, evas_vg_node_changed());
}

static void
_evas_vg_gradient_efl_graphics_gradient_stop_get(Eo *obj EINA_UNUSED,
                                                 Evas_VG_Gradient_Data *pd,
                                                 const Efl_Graphics_Gradient_Stop **colors,
                                                 unsigned int *length)
{
   if (colors) *colors = pd->colors;
   if (length) *length = pd->colors_count;
}

static void
_evas_vg_gradient_efl_graphics_gradient_spread_set(Eo *obj,
                                                   Evas_VG_Gradient_Data *pd,
                                                   Efl_Graphics_Gradient_Spread s)
{
   pd->s = s;
   eo_do_super(obj, MY_CLASS, evas_vg_node_changed());
}

static Efl_Graphics_Gradient_Spread
_evas_vg_gradient_efl_graphics_gradient_spread_get(Eo *obj EINA_UNUSED,
                                                   Evas_VG_Gradient_Data *pd)
{
   return pd->s;
}

#include "evas_vg_gradient.eo.c"
