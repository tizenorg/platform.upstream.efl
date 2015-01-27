/**
 * Simple Evas example illustrating a custom Evas box object
 *
 * You'll need at least one engine built for it (excluding the buffer
 * one). See stdout/stderr for output.
 *
 * @verbatim
 * gcc -o evas-box evas-box.c `pkg-config --libs --cflags evas ecore ecore-evas eina ector eo efl`
 * @endverbatim
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define PACKAGE_EXAMPLES_DIR "."
#endif

#define WIDTH 400
#define HEIGHT 400

#ifndef EFL_BETA_API_SUPPORT
#define EFL_BETA_API_SUPPORT 1
#endif

#ifndef EFL_EO_API_SUPPORT
#define EFL_EO_API_SUPPORT 1
#endif

#include <Eo.h>
#include <Efl.h>
#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Evas.h>

struct example_data
{
   Ecore_Evas  *ee;
   Evas        *evas;
   Evas_Object *bg;
   Evas_Object *vg;
};

static struct example_data d;

static void
_on_delete(Ecore_Evas *ee EINA_UNUSED)
{
   ecore_main_loop_quit();
}

static void /* adjust canvas' contents on resizes */
_canvas_resize_cb(Ecore_Evas *ee)
{
   int w, h;

   ecore_evas_geometry_get(ee, NULL, NULL, &w, &h);
   evas_object_resize(d.bg, w, h);
   evas_object_resize(d.vg, w, h);
}

static void
vector_set(int w, int h)
{
   Efl_Graphics_Path_Command *path_cmd = NULL;
   double *points = NULL;

   //Create VG Object
   d.vg = evas_object_vg_add(d.evas);
   evas_object_resize(d.vg, w, h);
   evas_object_show(d.vg);

   Evas_VG_Root_Node *root = evas_object_vg_root_node_get(d.vg);
   Evas_VG_Shape *shape = eo_add(EVAS_VG_SHAPE_CLASS, root);

   efl_graphics_path_append_circle(&path_cmd, &points, (w / 2), (h / 2), (w / 2));

   eo_do(shape,
         efl_graphics_shape_stroke_scale_set(5.0),
         efl_graphics_shape_stroke_width_set(3.0),
         efl_graphics_shape_stroke_color_set(255, 0, 0, 255),
         efl_graphics_shape_path_set(path_cmd, points));
}

int
main(void)
{
   if (!ecore_evas_init())
     return EXIT_FAILURE;

   /* this will give you a window with an Evas canvas under the first
    * engine available */
   d.ee = ecore_evas_new(NULL, 0, 0, WIDTH, HEIGHT, NULL);
   if (!d.ee)
     goto error;

   ecore_evas_callback_delete_request_set(d.ee, _on_delete);
   ecore_evas_callback_resize_set(d.ee, _canvas_resize_cb);
   ecore_evas_show(d.ee);

   d.evas = ecore_evas_get(d.ee);

   d.bg = evas_object_rectangle_add(d.evas);
   evas_object_color_set(d.bg, 255, 255, 255, 255); /* white bg */
   evas_object_show(d.bg);

   _canvas_resize_cb(d.ee);

   vector_set(WIDTH, HEIGHT);

   ecore_main_loop_begin();
   ecore_evas_shutdown();
   return 0;

error:
   ecore_evas_shutdown();
   return -1;
}

