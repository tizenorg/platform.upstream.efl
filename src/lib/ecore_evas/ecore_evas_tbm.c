#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <Ecore_Evas.h>
#include <Evas.h>
#include <Evas_Engine_Tbm.h>
#include <Ecore.h>
#include "ecore_private.h"
#include <Ecore_Input.h>
#include <tbm_bufmgr.h>
#include <tbm_surface_queue.h>

#include "Ecore_Evas.h"
#include "ecore_evas_tbm.h"
#include "ecore_evas_private.h"

//EAPI Ecore_Evas * ecore_evas_tbm_ext_new(const char *engine, const void *tbm_surf_queue, void* data);

static void
_ecore_evas_tbm_free(Ecore_Evas *ee)
{
   Ecore_Evas_Engine_Tbm_Data *tbm_data = ee->engine.data;

   if (tbm_data->image)
     {
        Ecore_Evas *ee2;

        ee2 = evas_object_data_get(tbm_data->image, "Ecore_Evas_Parent");
        evas_object_del(tbm_data->image);
        if (ee2)
          ee2->sub_ecore_evas = eina_list_remove(ee2->sub_ecore_evas, ee);
     }
   else
     {
        if (tbm_data->tbm_queue)
          tbm_data->free_func(tbm_data->data,tbm_data->tbm_queue);
     }

   free(tbm_data);
}

static void
_ecore_evas_resize(Ecore_Evas *ee, int w, int h)
{
#if 0
   Evas_Engine_Info_Tbm *einfo;
   Ecore_Evas_Engine_Tbm_Data *tbm_data = ee->engine.data;
   int stride = 0;

   if (w < 1) w = 1;
   if (h < 1) h = 1;
   ee->req.w = w;
   ee->req.h = h;
   if ((w == ee->w) && (h == ee->h)) return;
   ee->w = w;
   ee->h = h;
   evas_output_size_set(ee->evas, ee->w, ee->h);
   evas_output_viewport_set(ee->evas, 0, 0, ee->w, ee->h);
   evas_damage_rectangle_add(ee->evas, 0, 0, ee->w, ee->h);

   if (tbm_data->image)
     {
        tbm_data->pixels = evas_object_image_data_get(tbm_data->image, 1);
        stride = evas_object_image_stride_get(tbm_data->image);
     }
   else
     {
        if (tbm_data->pixels)
          tbm_data->free_func(tbm_data->data,
                                      tbm_data->pixels);
        tbm_data->pixels = tbm_data->alloc_func(tbm_data->data,
                      ee->w * ee->h * sizeof(int));
        stride = ee->w * sizeof(int);
     }

   einfo = (Evas_Engine_Info_Tbm *)evas_engine_info_get(ee->evas);
   if (einfo)
     {
        if (ee->alpha)
          einfo->info.depth_type = EVAS_ENGINE_BUFFER_DEPTH_ARGB32;
        else
          einfo->info.depth_type = EVAS_ENGINE_BUFFER_DEPTH_RGB32;
        einfo->info.dest_tbm = tbm_data->pixels;
        einfo->info.dest_tbm_row_bytes = stride;
        einfo->info.use_color_key = 0;
        einfo->info.alpha_threshold = 0;
        einfo->info.func.new_update_region = NULL;
        einfo->info.func.free_update_region = NULL;
        if (!evas_engine_info_set(ee->evas, (Evas_Engine_Info *)einfo))
          {
             ERR("evas_engine_info_set() for engine '%s' failed.", ee->driver);
          }
     }
   if (tbm_data->image)
      evas_object_image_data_set(tbm_data->image, tbm_data->pixels);
   if (ee->func.fn_resize) ee->func.fn_resize(ee);
#endif
}

static void
_ecore_evas_move_resize(Ecore_Evas *ee, int x EINA_UNUSED, int y EINA_UNUSED, int w, int h)
{
   _ecore_evas_resize(ee, w, h);
}

static void
_ecore_evas_show(Ecore_Evas *ee)
{
   Ecore_Evas_Engine_Tbm_Data *tbm_data = ee->engine.data;

   if (tbm_data->image) return;
   if (ee->prop.focused) return;
   ee->prop.focused = EINA_TRUE;
   ee->prop.withdrawn = EINA_FALSE;
   if (ee->func.fn_state_change) ee->func.fn_state_change(ee);
   evas_focus_in(ee->evas);
   if (ee->func.fn_focus_in) ee->func.fn_focus_in(ee);
}

static int
_ecore_evas_tbm_render(Ecore_Evas *ee)
{
   Eina_List *updates = NULL, *l, *ll;
   Ecore_Evas_Engine_Tbm_Data *tbm_data;
   Ecore_Evas *ee2;
   int rend = 0;

   tbm_data = ee->engine.data;
   EINA_LIST_FOREACH(ee->sub_ecore_evas, ll, ee2)
     {
        if (ee2->func.fn_pre_render) ee2->func.fn_pre_render(ee2);
        if (ee2->engine.func->fn_render)
           rend |= ee2->engine.func->fn_render(ee2);
        if (ee2->func.fn_post_render) ee2->func.fn_post_render(ee2);
     }
#if 0
   if (tbm_data->image)
     {
        int w, h;

        evas_object_image_size_get(tbm_data->image, &w, &h);
        if ((w != ee->w) || (h != ee->h))
           _ecore_evas_resize(ee, w, h);
        tbm_data->pixels = evas_object_image_data_get(tbm_data->image, 1);
     }
#endif
   if (tbm_data->tbm_queue)
     {
        updates = evas_render_updates(ee->evas);
     }
#if 0
   if (tbm_data->image)
     {
        Eina_Rectangle *r;

        evas_object_image_data_set(tbm_data->image, tbm_data->pixels);
        EINA_LIST_FOREACH(updates, l, r)
           evas_object_image_data_update_add(tbm_data->image,
                                             r->x, r->y, r->w, r->h);
     }
#endif
   if (updates)
     {
        evas_render_updates_free(updates);
        _ecore_evas_idle_timeout_update(ee);
     }

   if (ee->func.fn_post_render) ee->func.fn_post_render(ee);

   return updates ? 1 : rend;
}

EAPI int
ecore_evas_tbm_render(Ecore_Evas *ee)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(ee, 0);
   return _ecore_evas_tbm_render(ee);
}

// NOTE: if you fix this, consider fixing ecore_evas_ews.c as it is similar!
static void
_ecore_evas_tbm_coord_translate(Ecore_Evas *ee, Evas_Coord *x, Evas_Coord *y)
{
   Ecore_Evas_Engine_Tbm_Data *tbm_data = ee->engine.data;
   Evas_Coord xx, yy, ww, hh, fx, fy, fw, fh;

   evas_object_geometry_get(tbm_data->image, &xx, &yy, &ww, &hh);
   evas_object_image_fill_get(tbm_data->image, &fx, &fy, &fw, &fh);

   if (fw < 1) fw = 1;
   if (fh < 1) fh = 1;

   if (evas_object_map_get(tbm_data->image) &&
       evas_object_map_enable_get(tbm_data->image))
     {
        fx = 0; fy = 0;
        fw = ee->w; fh = ee->h;
        ww = ee->w; hh = ee->h;
     }

   if ((fx == 0) && (fy == 0) && (fw == ww) && (fh == hh))
     {
        *x = (ee->w * (*x - xx)) / fw;
        *y = (ee->h * (*y - yy)) / fh;
     }
   else
     {
        xx = (*x - xx) - fx;
        while (xx < 0) xx += fw;
        while (xx > fw) xx -= fw;
        *x = (ee->w * xx) / fw;

        yy = (*y - yy) - fy;
        while (yy < 0) yy += fh;
        while (yy > fh) yy -= fh;
        *y = (ee->h * yy) / fh;
     }
}

static void
_ecore_evas_tbm_transfer_modifiers_locks(Evas *e, Evas *e2)
{
   const char *mods[] =
     { "Shift", "Control", "Alt", "Meta", "Hyper", "Super", NULL };
   const char *locks[] =
     { "Scroll_Lock", "Num_Lock", "Caps_Lock", NULL };
   int i;

   for (i = 0; mods[i]; i++)
     {
        if (evas_key_modifier_is_set(evas_key_modifier_get(e), mods[i]))
          evas_key_modifier_on(e2, mods[i]);
        else
          evas_key_modifier_off(e2, mods[i]);
     }
   for (i = 0; locks[i]; i++)
     {
        if (evas_key_lock_is_set(evas_key_lock_get(e), locks[i]))
          evas_key_lock_on(e2, locks[i]);
        else
          evas_key_lock_off(e2, locks[i]);
     }
}

static void
_ecore_evas_tbm_cb_mouse_in(void *data, Evas *e, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Ecore_Evas *ee;
   Evas_Event_Mouse_In *ev;

   ee = data;
   ev = event_info;
   _ecore_evas_tbm_transfer_modifiers_locks(e, ee->evas);
   evas_event_feed_mouse_in(ee->evas, ev->timestamp, NULL);
}

static void
_ecore_evas_tbm_cb_mouse_out(void *data, Evas *e, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Ecore_Evas *ee;
   Evas_Event_Mouse_Out *ev;

   ee = data;
   ev = event_info;
   _ecore_evas_tbm_transfer_modifiers_locks(e, ee->evas);
   evas_event_feed_mouse_out(ee->evas, ev->timestamp, NULL);
}

static void
_ecore_evas_tbm_cb_mouse_down(void *data, Evas *e, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Ecore_Evas *ee;
   Evas_Event_Mouse_Down *ev;

   ee = data;
   ev = event_info;
   _ecore_evas_tbm_transfer_modifiers_locks(e, ee->evas);
   evas_event_feed_mouse_down(ee->evas, ev->button, ev->flags, ev->timestamp, NULL);
}

static void
_ecore_evas_tbm_cb_mouse_up(void *data, Evas *e, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Ecore_Evas *ee;
   Evas_Event_Mouse_Up *ev;

   ee = data;
   ev = event_info;
   _ecore_evas_tbm_transfer_modifiers_locks(e, ee->evas);
   evas_event_feed_mouse_up(ee->evas, ev->button, ev->flags, ev->timestamp, NULL);
}

static void
_ecore_evas_tbm_cb_mouse_move(void *data, Evas *e, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Ecore_Evas *ee;
   Evas_Event_Mouse_Move *ev;
   Evas_Coord x, y;

   ee = data;
   ev = event_info;
   x = ev->cur.canvas.x;
   y = ev->cur.canvas.y;
   _ecore_evas_tbm_coord_translate(ee, &x, &y);
   _ecore_evas_tbm_transfer_modifiers_locks(e, ee->evas);
   _ecore_evas_mouse_move_process(ee, x, y, ev->timestamp);
}

static void
_ecore_evas_tbm_cb_mouse_wheel(void *data, Evas *e, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Ecore_Evas *ee;
   Evas_Event_Mouse_Wheel *ev;

   ee = data;
   ev = event_info;
   _ecore_evas_tbm_transfer_modifiers_locks(e, ee->evas);
   evas_event_feed_mouse_wheel(ee->evas, ev->direction, ev->z, ev->timestamp, NULL);
}

static void
_ecore_evas_tbm_cb_multi_down(void *data, Evas *e, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Ecore_Evas *ee;
   Evas_Event_Multi_Down *ev;
   Evas_Coord x, y, xx, yy;
   double xf, yf;

   ee = data;
   ev = event_info;
   x = ev->canvas.x;
   y = ev->canvas.y;
   xx = x;
   yy = y;
   _ecore_evas_tbm_coord_translate(ee, &x, &y);
   xf = (ev->canvas.xsub - (double)xx) + (double)x;
   yf = (ev->canvas.ysub - (double)yy) + (double)y;
   _ecore_evas_tbm_transfer_modifiers_locks(e, ee->evas);
   evas_event_feed_multi_down(ee->evas, ev->device, x, y, ev->radius, ev->radius_x, ev->radius_y, ev->pressure, ev->angle, xf, yf, ev->flags, ev->timestamp, NULL);
}

static void
_ecore_evas_tbm_cb_multi_up(void *data, Evas *e, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Ecore_Evas *ee;
   Evas_Event_Multi_Up *ev;
   Evas_Coord x, y, xx, yy;
   double xf, yf;

   ee = data;
   ev = event_info;
   x = ev->canvas.x;
   y = ev->canvas.y;
   xx = x;
   yy = y;
   _ecore_evas_tbm_coord_translate(ee, &x, &y);
   xf = (ev->canvas.xsub - (double)xx) + (double)x;
   yf = (ev->canvas.ysub - (double)yy) + (double)y;
   _ecore_evas_tbm_transfer_modifiers_locks(e, ee->evas);
   evas_event_feed_multi_up(ee->evas, ev->device, x, y, ev->radius, ev->radius_x, ev->radius_y, ev->pressure, ev->angle, xf, yf, ev->flags, ev->timestamp, NULL);
}

static void
_ecore_evas_tbm_cb_multi_move(void *data, Evas *e, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Ecore_Evas *ee;
   Evas_Event_Multi_Move *ev;
   Evas_Coord x, y, xx, yy;
   double xf, yf;

   ee = data;
   ev = event_info;
   x = ev->cur.canvas.x;
   y = ev->cur.canvas.y;
   xx = x;
   yy = y;
   _ecore_evas_tbm_coord_translate(ee, &x, &y);
   xf = (ev->cur.canvas.xsub - (double)xx) + (double)x;
   yf = (ev->cur.canvas.ysub - (double)yy) + (double)y;
   _ecore_evas_tbm_transfer_modifiers_locks(e, ee->evas);
   evas_event_feed_multi_move(ee->evas, ev->device, x, y, ev->radius, ev->radius_x, ev->radius_y, ev->pressure, ev->angle, xf, yf, ev->timestamp, NULL);
}

static void
_ecore_evas_tbm_cb_free(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Ecore_Evas *ee;

   ee = data;
   if (ee->driver) _ecore_evas_free(ee);
}

static void
_ecore_evas_tbm_cb_key_down(void *data, Evas *e, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Ecore_Evas *ee;
   Evas_Event_Key_Down *ev;

   ee = data;
   ev = event_info;
   _ecore_evas_tbm_transfer_modifiers_locks(e, ee->evas);
   evas_event_feed_key_down(ee->evas, ev->keyname, ev->key, ev->string, ev->compose, ev->timestamp, NULL);
}

static void
_ecore_evas_tbm_cb_key_up(void *data, Evas *e, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Ecore_Evas *ee;
   Evas_Event_Key_Up *ev;

   ee = data;
   ev = event_info;
   _ecore_evas_tbm_transfer_modifiers_locks(e, ee->evas);
   evas_event_feed_key_up(ee->evas, ev->keyname, ev->key, ev->string, ev->compose, ev->timestamp, NULL);
}

static void
_ecore_evas_tbm_cb_focus_in(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Ecore_Evas *ee;

   ee = data;
   ee->prop.focused = EINA_TRUE;
   evas_focus_in(ee->evas);
   if (ee->func.fn_focus_in) ee->func.fn_focus_in(ee);
}

static void
_ecore_evas_tbm_cb_focus_out(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Ecore_Evas *ee;

   ee = data;
   ee->prop.focused = EINA_FALSE;
   evas_focus_out(ee->evas);
   if (ee->func.fn_focus_out) ee->func.fn_focus_out(ee);
}

static void
_ecore_evas_tbm_cb_show(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Ecore_Evas *ee;

   ee = data;
   ee->prop.withdrawn = EINA_FALSE;
   if (ee->func.fn_state_change) ee->func.fn_state_change(ee);
   ee->visible = 1;
   if (ee->func.fn_show) ee->func.fn_show(ee);
}

static void
_ecore_evas_tbm_cb_hide(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Ecore_Evas *ee;

   ee = data;
   ee->prop.withdrawn = EINA_TRUE;
   if (ee->func.fn_state_change) ee->func.fn_state_change(ee);
   ee->visible = 0;
   if (ee->func.fn_hide) ee->func.fn_hide(ee);
}

static void
_ecore_evas_tbm_alpha_set(Ecore_Evas *ee, int alpha)
{
#if 0
   Ecore_Evas_Engine_Tbm_Data *tbm_data = ee->engine.data;
   if (((ee->alpha) && (alpha)) || ((!ee->alpha) && (!alpha))) return;
   ee->alpha = alpha;
   if (tbm_data->image)
      evas_object_image_alpha_set(tbm_data->image, ee->alpha);
   else
     {
        Evas_Engine_Info_Tbm *einfo;

        einfo = (Evas_Engine_Info_Tbm *)evas_engine_info_get(ee->evas);
        if (einfo)
          {
             if (ee->alpha)
               einfo->info.depth_type = EVAS_ENGINE_BUFFER_DEPTH_ARGB32;
             else
               einfo->info.depth_type = EVAS_ENGINE_BUFFER_DEPTH_RGB32;
             evas_engine_info_set(ee->evas, (Evas_Engine_Info *)einfo);
          }
     }
#endif
}

static void
_ecore_evas_tbm_profile_set(Ecore_Evas *ee, const char *profile)
{
   _ecore_evas_window_profile_free(ee);
   ee->prop.profile.name = NULL;

   if (profile)
     {
        ee->prop.profile.name = (char *)eina_stringshare_add(profile);

        /* just change ee's state.*/
        if (ee->func.fn_state_change)
          ee->func.fn_state_change(ee);
     }
}

static void
_ecore_evas_tbm_msg_parent_send(Ecore_Evas *ee, int msg_domain, int msg_id, void *data, int size)
{
   Ecore_Evas *parent_ee = NULL;
   parent_ee = ecore_evas_data_get(ee, "parent");

   if (parent_ee)
     {
        if (parent_ee->func.fn_msg_parent_handle)
          parent_ee ->func.fn_msg_parent_handle(parent_ee, msg_domain, msg_id, data, size);
     }
   else
     {
        if (ee->func.fn_msg_parent_handle)
          ee ->func.fn_msg_parent_handle(ee, msg_domain, msg_id, data, size);
     }
}

static void
_ecore_evas_tbm_msg_send(Ecore_Evas *ee, int msg_domain, int msg_id, void *data, int size)
{
   Ecore_Evas *child_ee = NULL;
   child_ee = ecore_evas_data_get(ee, "child");

   if (child_ee)
     {
        if (child_ee->func.fn_msg_handle)
          child_ee->func.fn_msg_handle(child_ee, msg_domain, msg_id, data, size);
     }
   else
     {
        if (ee->func.fn_msg_handle)
          ee->func.fn_msg_handle(ee, msg_domain, msg_id, data, size);
     }
}

static Ecore_Evas_Engine_Func _ecore_tbm_engine_func =
{
     _ecore_evas_tbm_free,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     _ecore_evas_resize,
     _ecore_evas_move_resize,
     NULL,
     NULL,
     _ecore_evas_show,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     _ecore_evas_tbm_alpha_set,
     NULL, //transparent
     NULL, // profiles_set
     _ecore_evas_tbm_profile_set,

     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,

     _ecore_evas_tbm_render,
     NULL, // screen_geometry_get
     NULL,  // screen_dpi_get
     _ecore_evas_tbm_msg_parent_send,
     _ecore_evas_tbm_msg_send,

     NULL, // pointer_xy_get
     NULL, // pointer_warp

     NULL, // wm_rot_preferred_rotation_set
     NULL, // wm_rot_available_rotations_set
     NULL, // wm_rot_manual_rotation_done_set
     NULL, // wm_rot_manual_rotation_done

     NULL  // aux_hints_set
};

static tbm_surface_queue_h *
_ecore_evas_tbm_queue_alloc(void *data, int w, int h)
{
   return tbm_surface_queue_create(3, w, h, TBM_FORMAT_ARGB8888, TBM_BO_DEFAULT);
}

static void
_ecore_evas_tbm_queue_free(void *data,void *tbm_queue)
{
    tbm_surface_queue_destroy(tbm_queue);
   //free(pix);
}

EAPI Ecore_Evas *
ecore_evas_tbm_ext_new(const char *engine, const void *tbm_surf_queue, void* data)
{
    Evas_Engine_Info_Tbm *einfo;
    Ecore_Evas_Engine_Tbm_Data *tbm_data;
    Ecore_Evas *ee;
    int rmethod;
    int w, h;

    EINA_SAFETY_ON_NULL_RETURN_VAL(tbm_surf_queue, NULL);

    rmethod = evas_render_method_lookup("tbm");
    EINA_SAFETY_ON_TRUE_RETURN_VAL(rmethod == 0, NULL);

    ee = calloc(1, sizeof(Ecore_Evas));
    EINA_SAFETY_ON_NULL_RETURN_VAL(ee, NULL);

    tbm_data = calloc(1, sizeof(Ecore_Evas_Engine_Tbm_Data));
    if (!tbm_data)
      {
        free(ee);
        return NULL;
      }

    ECORE_MAGIC_SET(ee, ECORE_MAGIC_EVAS);

    ee->engine.func = (Ecore_Evas_Engine_Func *)&_ecore_tbm_engine_func;
    ee->engine.data = tbm_data;
    tbm_data->alloc_func = NULL;
    tbm_data->free_func = NULL;
    tbm_data->data = (void *)data;
    tbm_data->tbm_queue = tbm_surf_queue;
    tbm_data->ext_tbm_queue = EINA_TRUE;

    ee->driver = "tbm";

    w = tbm_surface_queue_get_width(tbm_data->tbm_queue);
    h = tbm_surface_queue_get_height(tbm_data->tbm_queue);

    if (w < 1) w = 1;
    if (h < 1) h = 1;
    ee->rotation = 0;
    ee->visible = 1;
    ee->w = w;
    ee->h = h;
    ee->req.w = ee->w;
    ee->req.h = ee->h;
    ee->profile_supported = 1;

    ee->prop.max.w = 0;
    ee->prop.max.h = 0;
    ee->prop.layer = 0;
    ee->prop.focused = EINA_TRUE;
    ee->prop.borderless = EINA_TRUE;
    ee->prop.override = EINA_TRUE;
    ee->prop.maximized = EINA_TRUE;
    ee->prop.fullscreen = EINA_FALSE;
    ee->prop.withdrawn = EINA_FALSE;
    ee->prop.sticky = EINA_FALSE;

    /* init evas here */
    ee->evas = evas_new();
    evas_data_attach_set(ee->evas, ee);
    evas_output_method_set(ee->evas, rmethod);
    evas_output_size_set(ee->evas, w, h);
    evas_output_viewport_set(ee->evas, 0, 0, w, h);

    einfo = (Evas_Engine_Info_Tbm *)evas_engine_info_get(ee->evas);
    if (einfo)
      {
         einfo->info.tbm_queue = tbm_data->tbm_queue;
         einfo->info.destination_alpha = EINA_TRUE;
         einfo->info.ext_tbm_queue = EINA_FALSE;
         einfo->info.rotation = 0;
         einfo->info.depth = 32;
         if (!evas_engine_info_set(ee->evas, (Evas_Engine_Info *)einfo))
           {
              ERR("evas_engine_info_set() for engine '%s' failed.", ee->driver);
              ecore_evas_free(ee);
              return NULL;
           }
      }
    else
      {
         ERR("evas_engine_info_set() init engine '%s' failed.", ee->driver);
         ecore_evas_free(ee);
         return NULL;
      }
    evas_key_modifier_add(ee->evas, "Shift");
    evas_key_modifier_add(ee->evas, "Control");
    evas_key_modifier_add(ee->evas, "Alt");
    evas_key_modifier_add(ee->evas, "Meta");
    evas_key_modifier_add(ee->evas, "Hyper");
    evas_key_modifier_add(ee->evas, "Super");
    evas_key_lock_add(ee->evas, "Caps_Lock");
    evas_key_lock_add(ee->evas, "Num_Lock");
    evas_key_lock_add(ee->evas, "Scroll_Lock");

    evas_event_feed_mouse_in(ee->evas, 0, NULL);

    _ecore_evas_register(ee);

    evas_event_feed_mouse_in(ee->evas, (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) & 0xffffffff), NULL);

    return ee;
}

EAPI Ecore_Evas *
ecore_evas_tbm_allocfunc_new(int w, int h,
                                void *(*alloc_func) (void *data, int w, int h),
                                void (*free_func) (void *data, void *tbm_queue),
                                const void *data)
{
   Evas_Engine_Info_Tbm *einfo;
   Ecore_Evas_Engine_Tbm_Data *tbm_data;
   Ecore_Evas *ee;
   int rmethod;

   EINA_SAFETY_ON_NULL_RETURN_VAL(alloc_func, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(free_func, NULL);

   rmethod = evas_render_method_lookup("tbm");
   EINA_SAFETY_ON_TRUE_RETURN_VAL(rmethod == 0, NULL);

   ee = calloc(1, sizeof(Ecore_Evas));
   EINA_SAFETY_ON_NULL_RETURN_VAL(ee, NULL);

   tbm_data = calloc(1, sizeof(Ecore_Evas_Engine_Tbm_Data));
   if (!tbm_data)
     {
       free(ee);
       return NULL;
     }

   ECORE_MAGIC_SET(ee, ECORE_MAGIC_EVAS);

   ee->engine.func = (Ecore_Evas_Engine_Func *)&_ecore_tbm_engine_func;
   ee->engine.data = tbm_data;
   tbm_data->alloc_func = alloc_func;
   tbm_data->free_func = free_func;
   tbm_data->data = (void *)data;
   tbm_data->ext_tbm_queue = EINA_FALSE;

   ee->driver = "tbm";

   if (w < 1) w = 1;
   if (h < 1) h = 1;
   ee->rotation = 0;
   ee->visible = 1;
   ee->w = w;
   ee->h = h;
   ee->req.w = ee->w;
   ee->req.h = ee->h;
   ee->profile_supported = 1;

   ee->prop.max.w = 0;
   ee->prop.max.h = 0;
   ee->prop.layer = 0;
   ee->prop.focused = EINA_TRUE;
   ee->prop.borderless = EINA_TRUE;
   ee->prop.override = EINA_TRUE;
   ee->prop.maximized = EINA_TRUE;
   ee->prop.fullscreen = EINA_FALSE;
   ee->prop.withdrawn = EINA_FALSE;
   ee->prop.sticky = EINA_FALSE;

   /* init evas here */
   ee->evas = evas_new();
   evas_data_attach_set(ee->evas, ee);
   evas_output_method_set(ee->evas, rmethod);
   evas_output_size_set(ee->evas, w, h);
   evas_output_viewport_set(ee->evas, 0, 0, w, h);

   tbm_data->tbm_queue = tbm_data->alloc_func(tbm_data->data, w, h);

   einfo = (Evas_Engine_Info_Tbm *)evas_engine_info_get(ee->evas);
   if (einfo)
     {
        einfo->info.tbm_queue = tbm_data->tbm_queue;
        einfo->info.destination_alpha = EINA_TRUE;
        einfo->info.ext_tbm_queue = EINA_FALSE;
        einfo->info.rotation = 0;
        einfo->info.depth = 32;
        if (!evas_engine_info_set(ee->evas, (Evas_Engine_Info *)einfo))
          {
             ERR("evas_engine_info_set() for engine '%s' failed.", ee->driver);
             ecore_evas_free(ee);
             return NULL;
          }
     }
   else
     {
        ERR("evas_engine_info_set() init engine '%s' failed.", ee->driver);
        ecore_evas_free(ee);
        return NULL;
     }
   evas_key_modifier_add(ee->evas, "Shift");
   evas_key_modifier_add(ee->evas, "Control");
   evas_key_modifier_add(ee->evas, "Alt");
   evas_key_modifier_add(ee->evas, "Meta");
   evas_key_modifier_add(ee->evas, "Hyper");
   evas_key_modifier_add(ee->evas, "Super");
   evas_key_lock_add(ee->evas, "Caps_Lock");
   evas_key_lock_add(ee->evas, "Num_Lock");
   evas_key_lock_add(ee->evas, "Scroll_Lock");

   evas_event_feed_mouse_in(ee->evas, 0, NULL);

   _ecore_evas_register(ee);

   evas_event_feed_mouse_in(ee->evas, (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) & 0xffffffff), NULL);

   return ee;
}

EAPI Ecore_Evas *
ecore_evas_tbm_new(int w, int h)
{
    return ecore_evas_tbm_allocfunc_new
     (w, h,_ecore_evas_tbm_queue_alloc, _ecore_evas_tbm_queue_free, NULL);
}

EAPI const void *
ecore_evas_tbm_pixels_get(Ecore_Evas *ee)
{
   Ecore_Evas_Engine_Tbm_Data *tbm_data;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ee, NULL);

   tbm_data = ee->engine.data;
   _ecore_evas_tbm_render(ee);
   return tbm_data->tbm_queue;
}

EAPI Ecore_Evas *
ecore_evas_tbm_ecore_evas_parent_get(Ecore_Evas *ee)
{
   Ecore_Evas_Engine_Tbm_Data *tbm_data;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ee, NULL);

   tbm_data = ee->engine.data;
   return evas_object_data_get(tbm_data->image, "Ecore_Evas_Parent");
}

EAPI Evas_Object *
ecore_evas_tbm_object_image_new(Ecore_Evas *ee_target)
{
   Evas_Object *o;
#if 0
   Ecore_Evas_Engine_Tbm_Data *tbm_data;
   Evas_Engine_Info_Tbm *einfo;
   Ecore_Evas *ee;
   int rmethod;
   int w = 1, h = 1;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ee_target, NULL);

   rmethod = evas_render_method_lookup("tbm");
   EINA_SAFETY_ON_TRUE_RETURN_VAL(rmethod == 0, NULL);

   ee = calloc(1, sizeof(Ecore_Evas));
   EINA_SAFETY_ON_NULL_RETURN_VAL(ee, NULL);

   tbm_data = calloc(1, sizeof(Ecore_Evas_Engine_Tbm_Data));
   if (!tbm_data)
     {
    free(ee);
    return NULL;
     }

   ee->engine.data = tbm_data;

   o = evas_object_image_add(ee_target->evas);
   evas_object_image_content_hint_set(o, EVAS_IMAGE_CONTENT_HINT_DYNAMIC);
   evas_object_image_colorspace_set(o, EVAS_COLORSPACE_ARGB8888);
   evas_object_image_alpha_set(o, 0);
   evas_object_image_size_set(o, w, h);

   ECORE_MAGIC_SET(ee, ECORE_MAGIC_EVAS);

   ee->engine.func = (Ecore_Evas_Engine_Func *)&_ecore_tbm_engine_func;

   ee->driver = "tbm";

   ee->rotation = 0;
   ee->visible = 0;
   ee->w = w;
   ee->h = h;
   ee->req.w = ee->w;
   ee->req.h = ee->h;
   ee->profile_supported = 1;

   ee->prop.max.w = 0;
   ee->prop.max.h = 0;
   ee->prop.layer = 0;
   ee->prop.focused = EINA_FALSE;
   ee->prop.borderless = EINA_TRUE;
   ee->prop.override = EINA_TRUE;
   ee->prop.maximized = EINA_FALSE;
   ee->prop.fullscreen = EINA_FALSE;
   ee->prop.withdrawn = EINA_TRUE;
   ee->prop.sticky = EINA_FALSE;

   /* init evas here */
   ee->evas = evas_new();
   evas_data_attach_set(ee->evas, ee);
   evas_output_method_set(ee->evas, rmethod);
   evas_output_size_set(ee->evas, w, h);
   evas_output_viewport_set(ee->evas, 0, 0, w, h);

   tbm_data->image = o;
   evas_object_data_set(tbm_data->image, "Ecore_Evas", ee);
   evas_object_data_set(tbm_data->image, "Ecore_Evas_Parent", ee_target);
   evas_object_event_callback_add(tbm_data->image,
                                  EVAS_CALLBACK_MOUSE_IN,
                                  _ecore_evas_tbm_cb_mouse_in, ee);
   evas_object_event_callback_add(tbm_data->image,
                                  EVAS_CALLBACK_MOUSE_OUT,
                                  _ecore_evas_tbm_cb_mouse_out, ee);
   evas_object_event_callback_add(tbm_data->image,
                                  EVAS_CALLBACK_MOUSE_DOWN,
                                  _ecore_evas_tbm_cb_mouse_down, ee);
   evas_object_event_callback_add(tbm_data->image,
                                  EVAS_CALLBACK_MOUSE_UP,
                                  _ecore_evas_tbm_cb_mouse_up, ee);
   evas_object_event_callback_add(tbm_data->image,
                                  EVAS_CALLBACK_MOUSE_MOVE,
                                  _ecore_evas_tbm_cb_mouse_move, ee);
   evas_object_event_callback_add(tbm_data->image,
                                  EVAS_CALLBACK_MOUSE_WHEEL,
                                  _ecore_evas_tbm_cb_mouse_wheel, ee);
   evas_object_event_callback_add(tbm_data->image,
                                  EVAS_CALLBACK_MULTI_DOWN,
                                  _ecore_evas_tbm_cb_multi_down, ee);
   evas_object_event_callback_add(tbm_data->image,
                                  EVAS_CALLBACK_MULTI_UP,
                                  _ecore_evas_tbm_cb_multi_up, ee);
   evas_object_event_callback_add(tbm_data->image,
                                  EVAS_CALLBACK_MULTI_MOVE,
                                  _ecore_evas_tbm_cb_multi_move, ee);
   evas_object_event_callback_add(tbm_data->image,
                                  EVAS_CALLBACK_FREE,
                                  _ecore_evas_tbm_cb_free, ee);
   evas_object_event_callback_add(tbm_data->image,
                                  EVAS_CALLBACK_KEY_DOWN,
                                  _ecore_evas_tbm_cb_key_down, ee);
   evas_object_event_callback_add(tbm_data->image,
                                  EVAS_CALLBACK_KEY_UP,
                                  _ecore_evas_tbm_cb_key_up, ee);
   evas_object_event_callback_add(tbm_data->image,
                                  EVAS_CALLBACK_FOCUS_IN,
                                  _ecore_evas_tbm_cb_focus_in, ee);
   evas_object_event_callback_add(tbm_data->image,
                                  EVAS_CALLBACK_FOCUS_OUT,
                                  _ecore_evas_tbm_cb_focus_out, ee);
   evas_object_event_callback_add(tbm_data->image,
                                  EVAS_CALLBACK_SHOW,
                                  _ecore_evas_tbm_cb_show, ee);
   evas_object_event_callback_add(tbm_data->image,
                                  EVAS_CALLBACK_HIDE,
                                  _ecore_evas_tbm_cb_hide, ee);
   einfo = (Evas_Engine_Info_Tbm *)evas_engine_info_get(ee->evas);
   if (einfo)
     {
        tbm_data->pixels = evas_object_image_data_get(o, 1);
        einfo->info.depth_type = EVAS_ENGINE_BUFFER_DEPTH_ARGB32;
        einfo->info.dest_tbm = tbm_data->pixels;
        einfo->info.dest_tbm_row_bytes = evas_object_image_stride_get(o);
        einfo->info.use_color_key = 0;
        einfo->info.alpha_threshold = 0;
        einfo->info.func.new_update_region = NULL;
        einfo->info.func.free_update_region = NULL;
        evas_object_image_data_set(o, tbm_data->pixels);
        if (!evas_engine_info_set(ee->evas, (Evas_Engine_Info *)einfo))
          {
             ERR("evas_engine_info_set() for engine '%s' failed.", ee->driver);
             ecore_evas_free(ee);
             return NULL;
          }
     }
   else
     {
        ERR("evas_engine_info_set() for engine '%s' failed.", ee->driver);
        ecore_evas_free(ee);
        return NULL;
     }
   evas_key_modifier_add(ee->evas, "Shift");
   evas_key_modifier_add(ee->evas, "Control");
   evas_key_modifier_add(ee->evas, "Alt");
   evas_key_modifier_add(ee->evas, "Meta");
   evas_key_modifier_add(ee->evas, "Hyper");
   evas_key_modifier_add(ee->evas, "Super");
   evas_key_lock_add(ee->evas, "Caps_Lock");
   evas_key_lock_add(ee->evas, "Num_Lock");
   evas_key_lock_add(ee->evas, "Scroll_Lock");

   ee_target->sub_ecore_evas = eina_list_append(ee_target->sub_ecore_evas, ee);
#endif
   return o;
}
