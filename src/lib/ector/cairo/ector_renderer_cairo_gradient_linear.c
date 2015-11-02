#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include <Ector.h>
#include <cairo/Ector_Cairo.h>

#include "ector_private.h"
#include "ector_cairo_private.h"

static cairo_pattern_t *(*cairo_pattern_create_linear)(double x0, double y0,
                                                       double x1, double y1) = NULL;
static void (*cairo_set_source)(cairo_t *cr, cairo_pattern_t *source) = NULL;
static void (*cairo_fill)(cairo_t *cr) = NULL;
static void (*cairo_rectangle)(cairo_t *cr,
                               double x, double y,
                               double width, double height) = NULL;
<<<<<<< HEAD
static void (*cairo_pattern_add_color_stop_rgba)(cairo_pattern_t *pattern, double offset,
                                                 double red, double green, double blue, double alpha) = NULL;
=======
>>>>>>> opensource/master
static void (*cairo_pattern_destroy)(cairo_pattern_t *pattern) = NULL;

static void (*cairo_pattern_set_extend)(cairo_pattern_t *pattern, cairo_extend_t extend) = NULL;

typedef struct _Ector_Renderer_Cairo_Gradient_Linear_Data Ector_Renderer_Cairo_Gradient_Linear_Data;
struct _Ector_Renderer_Cairo_Gradient_Linear_Data
{
   Ector_Cairo_Surface_Data *parent;
<<<<<<< HEAD
   cairo_pattern_t *pat;
};

static Eina_Bool
_ector_renderer_cairo_gradient_linear_ector_renderer_generic_base_prepare(Eo *obj,
                                                                          Ector_Renderer_Cairo_Gradient_Linear_Data *pd)
{
   Ector_Renderer_Generic_Gradient_Linear_Data *gld;
   Ector_Renderer_Generic_Gradient_Data *gd;
   unsigned int i;

   eo_do_super(obj, ECTOR_RENDERER_CAIRO_GRADIENT_LINEAR_CLASS, ector_renderer_prepare());

   if (pd->pat) return EINA_FALSE;

   gld = eo_data_scope_get(obj, ECTOR_RENDERER_GENERIC_GRADIENT_LINEAR_MIXIN);
   gd = eo_data_scope_get(obj, ECTOR_RENDERER_GENERIC_GRADIENT_MIXIN);
   if (!gld || !gd) return EINA_FALSE;

   USE(obj, cairo_pattern_create_linear, EINA_FALSE);
   USE(obj, cairo_pattern_add_color_stop_rgba, EINA_FALSE);

   pd->pat = cairo_pattern_create_linear(gld->start.x, gld->start.y,
                                     gld->end.x, gld->end.y);
   int r,g,b,a;
   for (i = 0; i < gd->colors_count; i++)
     {
       r = gd->colors[i].r;
       g = gd->colors[i].g;
       b = gd->colors[i].b;
       a = gd->colors[i].a;
       ector_color_argb_unpremul(a, &r, &g, &b);
       cairo_pattern_add_color_stop_rgba(pd->pat, gd->colors[i].offset, r/255.0, g/255.0, b/255.0, a/255.0);
     }

   USE(obj, cairo_pattern_set_extend, EINA_FALSE);
   cairo_pattern_set_extend(pd->pat, _ector_cairo_extent_get(gd->s));
=======
};

static cairo_pattern_t *
_ector_renderer_cairo_gradient_linear_prepare(Eo *obj,
                                              Ector_Renderer_Generic_Gradient_Linear_Data *gld,
                                              Ector_Renderer_Generic_Gradient_Data *gd,
                                              unsigned int mul_col)
{
   cairo_pattern_t *pat;

   USE(obj, cairo_pattern_create_linear, NULL);

   pat = cairo_pattern_create_linear(gld->start.x, gld->start.y,
                                     gld->end.x, gld->end.y);
   if (!pat) return NULL;
   _ector_renderer_cairo_gradient_prepare(obj, pat, gd, mul_col);

   USE(obj, cairo_pattern_set_extend, NULL);
   cairo_pattern_set_extend(pat, _ector_cairo_extent_get(gd->s));

   return pat;
}

static Eina_Bool
_ector_renderer_cairo_gradient_linear_ector_renderer_generic_base_prepare(Eo *obj,
                                                                          Ector_Renderer_Cairo_Gradient_Linear_Data *pd)
{
   eo_do_super(obj, ECTOR_RENDERER_CAIRO_GRADIENT_LINEAR_CLASS, ector_renderer_prepare());
>>>>>>> opensource/master

   if (!pd->parent)
     {
        Eo *parent;

        eo_do(obj, parent = eo_parent_get());
        if (!parent) return EINA_FALSE;
        pd->parent = eo_data_xref(parent, ECTOR_CAIRO_SURFACE_CLASS, obj);
     }

   return EINA_FALSE;
}

static Eina_Bool
_ector_renderer_cairo_gradient_linear_ector_renderer_generic_base_draw(Eo *obj,
                                                                       Ector_Renderer_Cairo_Gradient_Linear_Data *pd,
                                                                       Ector_Rop op, Eina_Array *clips, unsigned int mul_col)
{
<<<<<<< HEAD
   if (pd->pat) return EINA_FALSE;

   Ector_Renderer_Generic_Gradient_Linear_Data *gld;

   // FIXME: don't ignore clipping !
   gld = eo_data_scope_get(obj, ECTOR_RENDERER_GENERIC_GRADIENT_LINEAR_MIXIN);
   if (!pd->pat || !gld) return EINA_FALSE;
=======
   Ector_Renderer_Generic_Gradient_Linear_Data *gld;
   Ector_Renderer_Generic_Gradient_Data *gd;
   cairo_pattern_t *pat;

   // FIXME: don't ignore clipping !
   gld = eo_data_scope_get(obj, ECTOR_RENDERER_GENERIC_GRADIENT_LINEAR_MIXIN);
   gd = eo_data_scope_get(obj, ECTOR_RENDERER_GENERIC_GRADIENT_MIXIN);
   if (!gd || !gld) return EINA_FALSE;

   pat = _ector_renderer_cairo_gradient_linear_prepare(obj, gld, gd, mul_col);
   if (!pat) return EINA_FALSE;
>>>>>>> opensource/master

   eo_do_super(obj, ECTOR_RENDERER_CAIRO_GRADIENT_LINEAR_CLASS, ector_renderer_draw(op, clips, mul_col));

   USE(obj, cairo_rectangle, EINA_FALSE);
   USE(obj, cairo_fill, EINA_FALSE);
<<<<<<< HEAD
=======
   USE(obj, cairo_pattern_destroy, EINA_FALSE);
   USE(obj, cairo_set_source, EINA_FALSE);
>>>>>>> opensource/master

   cairo_rectangle(pd->parent->cairo, gld->start.x, gld->start.y,
                   gld->end.x - gld->start.x,
                   gld->end.y - gld->start.y);
<<<<<<< HEAD
   eo_do(obj, ector_renderer_cairo_base_fill());
   cairo_fill(pd->parent->cairo);
=======
   cairo_set_source(pd->parent->cairo, pat);
   cairo_fill(pd->parent->cairo);
   cairo_pattern_destroy(pat);
>>>>>>> opensource/master

   return EINA_TRUE;
}

static Eina_Bool
_ector_renderer_cairo_gradient_linear_ector_renderer_cairo_base_fill(Eo *obj,
<<<<<<< HEAD
                                                                     Ector_Renderer_Cairo_Gradient_Linear_Data *pd)
{
   if (!pd->pat) return EINA_FALSE;

   USE(obj, cairo_set_source, EINA_FALSE);

   cairo_set_source(pd->parent->cairo, pd->pat);
=======
                                                                     Ector_Renderer_Cairo_Gradient_Linear_Data *pd,
                                                                     unsigned int mul_col)
{
   Ector_Renderer_Generic_Gradient_Linear_Data *gld;
   Ector_Renderer_Generic_Gradient_Data *gd;
   cairo_pattern_t *pat;

   gld = eo_data_scope_get(obj, ECTOR_RENDERER_GENERIC_GRADIENT_LINEAR_MIXIN);
   gd = eo_data_scope_get(obj, ECTOR_RENDERER_GENERIC_GRADIENT_MIXIN);
   if (!gd || !gld) return EINA_FALSE;

   pat = _ector_renderer_cairo_gradient_linear_prepare(obj, gld, gd, mul_col);
   if (!pat) return EINA_FALSE;

   USE(obj, cairo_set_source, EINA_FALSE);
   USE(obj, cairo_pattern_destroy, EINA_FALSE);

   cairo_set_source(pd->parent->cairo, pat);
   cairo_pattern_destroy(pat);
>>>>>>> opensource/master

   return EINA_TRUE;
}

static void
_ector_renderer_cairo_gradient_linear_ector_renderer_generic_base_bounds_get(Eo *obj,
                                                                             Ector_Renderer_Cairo_Gradient_Linear_Data *pd EINA_UNUSED,
                                                                             Eina_Rectangle *r)
{
   Ector_Renderer_Generic_Gradient_Linear_Data *gld;
   Ector_Renderer_Cairo_Base_Data *bd;

   gld = eo_data_scope_get(obj, ECTOR_RENDERER_GENERIC_GRADIENT_RADIAL_MIXIN);
   bd = eo_data_scope_get(obj, ECTOR_RENDERER_CAIRO_BASE_CLASS);
   EINA_RECTANGLE_SET(r,
                      bd->generic->origin.x + gld->start.x,
                      bd->generic->origin.y + gld->start.y,
                      gld->end.x - gld->start.x,
                      gld->end.y - gld->start.x);
}

void
_ector_renderer_cairo_gradient_linear_eo_base_destructor(Eo *obj,
                                                         Ector_Renderer_Cairo_Gradient_Linear_Data *pd)
{
   Eo *parent;

<<<<<<< HEAD
   USE(obj, cairo_pattern_destroy, );

   if (pd->pat) cairo_pattern_destroy(pd->pat);
   pd->pat = NULL;

=======
>>>>>>> opensource/master
   eo_do(obj, parent = eo_parent_get());
   eo_data_xunref(parent, pd->parent, obj);

   eo_do_super(obj, ECTOR_RENDERER_CAIRO_GRADIENT_LINEAR_CLASS, eo_destructor());
}

void
<<<<<<< HEAD
_ector_renderer_cairo_gradient_linear_efl_gfx_gradient_base_stop_set(Eo *obj, Ector_Renderer_Cairo_Gradient_Linear_Data *pd, const Efl_Gfx_Gradient_Stop *colors, unsigned int length)
{
   USE(obj, cairo_pattern_destroy, );

   if (pd->pat) cairo_pattern_destroy(pd->pat);
   pd->pat = NULL;

   eo_do_super(obj, ECTOR_RENDERER_CAIRO_GRADIENT_LINEAR_CLASS,
               efl_gfx_gradient_stop_set(colors, length));
=======
_ector_renderer_cairo_gradient_linear_efl_gfx_gradient_base_stop_set(Eo *obj,
                                                                     Ector_Renderer_Cairo_Gradient_Linear_Data *pd EINA_UNUSED,
                                                                     const Efl_Gfx_Gradient_Stop *colors,
                                                                     unsigned int length)
{
   eo_do_super(obj, ECTOR_RENDERER_CAIRO_GRADIENT_LINEAR_CLASS,
               efl_gfx_gradient_stop_set(colors, length));
}

static unsigned int
_ector_renderer_cairo_gradient_linear_ector_renderer_generic_base_crc_get(Eo *obj, Ector_Renderer_Cairo_Gradient_Linear_Data *pd EINA_UNUSED)
{
   Ector_Renderer_Generic_Gradient_Linear_Data *gld;
   Ector_Renderer_Generic_Gradient_Data *gd;
   unsigned int crc;

   eo_do_super(obj, ECTOR_RENDERER_CAIRO_GRADIENT_LINEAR_CLASS,
               crc = ector_renderer_crc_get());

   gld = eo_data_scope_get(obj, ECTOR_RENDERER_GENERIC_GRADIENT_LINEAR_MIXIN);
   gd = eo_data_scope_get(obj, ECTOR_RENDERER_GENERIC_GRADIENT_MIXIN);
   if (!gd || !gld) return crc;

   crc = eina_crc((void*) gd->s, sizeof (Efl_Gfx_Gradient_Spread), crc, EINA_FALSE);
   if (gd->colors_count)
     crc = eina_crc((void*) gd->colors, sizeof (Efl_Gfx_Gradient_Stop) * gd->colors_count, crc, EINA_FALSE);
   crc = eina_crc((void*) gld, sizeof (Ector_Renderer_Generic_Gradient_Linear_Data), crc, EINA_FALSE);

   return crc;
>>>>>>> opensource/master
}

#include "ector_renderer_cairo_gradient_linear.eo.c"
