#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "ecore_win_wayland_private.h"
#ifdef BUILD_ECORE_WIN_WAYLAND_EGL
# include <Evas_Engine_Wayland_Egl.h>
#endif

#define _smart_frame_type "ecore_win_wl_frame"

static const char *interface_wl_name = "wayland";
static const int interface_wl_version = 1;

/* local structures for the frame smart object */
typedef struct _EE_Wl_Smart_Data EE_Wl_Smart_Data;
struct _EE_Wl_Smart_Data
{
   Evas_Object_Smart_Clipped_Data base;
   Evas_Object *text;
   Evas_Coord x, y, w, h;
   Evas_Object *border[4]; // 0 = top, 1 = bottom, 2 = left, 3 = right
   Evas_Coord border_size[4]; // same as border
};

static const Evas_Smart_Cb_Description _smart_callbacks[] =
{
     {NULL, NULL}
};

EVAS_SMART_SUBCLASS_NEW(_smart_frame_type, _ecore_win_wl_frame,
                        Evas_Smart_Class, Evas_Smart_Class,
                        evas_object_smart_clipped_class_get, _smart_callbacks);

/* local variables */
static int _ecore_win_wl_init_count = 0;
static Ecore_Event_Handler *_ecore_win_wl_event_hdls[10];

static void _ecore_win_wayland_common_resize(Ecore_Win *ee, int location);

/* local function prototypes */
static int _ecore_win_wl_common_render_updates_process(Ecore_Win *ee, Eina_List *updates);
void _ecore_win_wl_common_render_updates(void *data, Evas *evas EINA_UNUSED, void *event);
static void _rotation_do(Ecore_Win *ee, int rotation, int resize);
static void _ecore_win_wayland_alpha_do(Ecore_Win *ee, int alpha);
static void _ecore_win_wayland_transparent_do(Ecore_Win *ee, int transparent);
static void _ecore_win_wl_common_border_update(Ecore_Win *ee);
static Eina_Bool _ecore_win_wl_common_wm_rot_manual_rotation_done_timeout(void *data);
static void      _ecore_win_wl_common_wm_rot_manual_rotation_done_timeout_update(Ecore_Win *ee);

/* local functions */
static void 
_ecore_win_wl_common_state_update(Ecore_Win *ee)
{
   if (ee->func.fn_state_change) ee->func.fn_state_change(ee);
}

static int 
_ecore_win_wl_common_render_updates_process(Ecore_Win *ee, Eina_List *updates)
{

   int rend = 0;
#if 0
   if (((ee->visible) && (ee->draw_ok)) ||
       ((ee->should_be_visible) && (ee->prop.fullscreen)) ||
       ((ee->should_be_visible) && (ee->prop.override)))
     {
        if (updates)
          {
             _ecore_win_idle_timeout_update(ee);
             rend = 1;
          }
     }
   else
     evas_norender(ee->evas);

   if (ee->func.fn_post_render) ee->func.fn_post_render(ee);
#endif
   return rend;
}

static Eina_Bool
_ecore_win_wl_common_cb_mouse_in(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
#if 0
   Ecore_Win *ee;
   Ecore_Wl_Event_Mouse_In *ev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ev = event;
   ee = ecore_event_window_match(ev->window);
   if ((!ee) || (ee->ignore_events)) return ECORE_CALLBACK_PASS_ON;
   if (ev->window != ee->prop.window) return ECORE_CALLBACK_PASS_ON;
   if (ee->in) return ECORE_CALLBACK_PASS_ON;

   if (ee->func.fn_mouse_in) ee->func.fn_mouse_in(ee);
   ecore_event_evas_modifier_lock_update(ee->evas, ev->modifiers);
   evas_event_feed_mouse_in(ee->evas, ev->timestamp, NULL);
   _ecore_win_mouse_move_process(ee, ev->x, ev->y, ev->timestamp);
   ee->in = EINA_TRUE;
#endif
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_ecore_win_wl_common_cb_mouse_out(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
#if 0
   Ecore_Win *ee;
   Ecore_Wl_Event_Mouse_Out *ev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ev = event;
   ee = ecore_event_window_match(ev->window);
   if ((!ee) || (ee->ignore_events)) return ECORE_CALLBACK_PASS_ON;
   if (ev->window != ee->prop.window) return ECORE_CALLBACK_PASS_ON;

   if (ee->in)
     {
        ecore_event_evas_modifier_lock_update(ee->evas, ev->modifiers);
        _ecore_win_mouse_move_process(ee, ev->x, ev->y, ev->timestamp);
        evas_event_feed_mouse_out(ee->evas, ev->timestamp, NULL);
        if (ee->func.fn_mouse_out) ee->func.fn_mouse_out(ee);
        if (ee->prop.cursor.object) evas_object_hide(ee->prop.cursor.object);
        ee->in = EINA_FALSE;
     }
#endif
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_ecore_win_wl_common_cb_focus_in(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
#if 0
   Ecore_Win *ee;
   Ecore_Wl_Event_Focus_In *ev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ev = event;
   ee = ecore_event_window_match(ev->win);
   if ((!ee) || (ee->ignore_events)) return ECORE_CALLBACK_PASS_ON;
   if (ev->win != ee->prop.window) return ECORE_CALLBACK_PASS_ON;
   if (ee->prop.focused) return ECORE_CALLBACK_PASS_ON;
   ee->prop.focused = EINA_TRUE;
   evas_focus_in(ee->evas);
   if (ee->func.fn_focus_in) ee->func.fn_focus_in(ee);
#endif
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_ecore_win_wl_common_cb_focus_out(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
#if 0
   Ecore_Win *ee;
   Ecore_Wl_Event_Focus_In *ev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ev = event;
   ee = ecore_event_window_match(ev->win);
   if ((!ee) || (ee->ignore_events)) return ECORE_CALLBACK_PASS_ON;
   if (ev->win != ee->prop.window) return ECORE_CALLBACK_PASS_ON;
   if (!ee->prop.focused) return ECORE_CALLBACK_PASS_ON;
   evas_focus_out(ee->evas);
   ee->prop.focused = EINA_FALSE;
   if (ee->func.fn_focus_out) ee->func.fn_focus_out(ee);
#endif
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_ecore_win_wl_common_cb_window_configure(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
#if 0
   Ecore_Win *ee;
   Ecore_Win_Engine_Wl_Data *wdata;
   Ecore_Wl_Event_Window_Configure *ev;
   int nw = 0, nh = 0;
   Eina_Bool prev_max, prev_full;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ev = event;
   ee = ecore_event_window_match(ev->win);
   if (!ee) return ECORE_CALLBACK_PASS_ON;
   if (ev->win != ee->prop.window) return ECORE_CALLBACK_PASS_ON;

   wdata = ee->engine.data;
   if (!wdata) return ECORE_CALLBACK_PASS_ON;

   prev_max = ee->prop.maximized;
   prev_full = ee->prop.fullscreen;
   ee->prop.maximized = ecore_wl_window_maximized_get(wdata->win);
   ee->prop.fullscreen = ecore_wl_window_fullscreen_get(wdata->win);

   nw = ev->w;
   nh = ev->h;
   if (nw < 1) nw = 1;
   if (nh < 1) nh = 1;

   if (prev_full != ee->prop.fullscreen)
     _ecore_win_wl_common_border_update(ee);

   if (ee->prop.fullscreen)
     {
        _ecore_win_wl_common_move(ee, ev->x, ev->y);
        _ecore_win_wl_common_resize(ee, nw, nh);

        if (prev_full != ee->prop.fullscreen)
          _ecore_win_wl_common_state_update(ee);

        return ECORE_CALLBACK_PASS_ON;
     }

   if ((ee->x != ev->x) || (ee->y != ev->y))
     _ecore_win_wl_common_move(ee, ev->x, ev->y);

   if ((ee->req.w != nw) || (ee->req.h != nh))
     _ecore_win_wl_common_resize(ee, nw, nh);

   if ((prev_max != ee->prop.maximized) ||
       (prev_full != ee->prop.fullscreen))
     _ecore_win_wl_common_state_update(ee);
#endif
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_ecore_win_wl_common_cb_conformant_change(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
#if 0
   Ecore_Win *ee;
   Ecore_Wl_Event_Conformant_Change *ev;

   ev = event;
   ee = ecore_event_window_match(ev->win);
   if (!ee) return ECORE_CALLBACK_PASS_ON;
   if (ev->win != ee->prop.window) return ECORE_CALLBACK_PASS_ON;

   if ((ev->part_type == ECORE_WL_INDICATOR_PART) && (ee->indicator_state != ev->state))
        ee->indicator_state = ev->state;
   else if ((ev->part_type == ECORE_WL_KEYBOARD_PART) && (ee->keyboard_state != ev->state))
     ee->keyboard_state = ev->state;
   else if ((ev->part_type == ECORE_WL_CLIPBOARD_PART) && (ee->clipboard_state != ev->state))
     ee->clipboard_state = ev->state;

   _ecore_win_wl_common_state_update(ee);
#endif
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_ecore_win_wl_common_cb_aux_hint_allowed(void *data  EINA_UNUSED, int type EINA_UNUSED, void *event)
{
#if 0
   Ecore_Win *ee;
   Ecore_Wl_Event_Aux_Hint_Allowed *ev;
   Eina_List *l;
   Ecore_Win_Aux_Hint *aux;

   ev = event;
   ee = ecore_event_window_match(ev->win);
   if (!ee) return ECORE_CALLBACK_PASS_ON;
   if (ev->win != ee->prop.window) return ECORE_CALLBACK_PASS_ON;

   EINA_LIST_FOREACH(ee->prop.aux_hint.hints, l, aux)
     {
        if (aux->id == ev->id)
          {
             aux->allowed = 1;
             if (!aux->notified)
               {
                  _ecore_win_wl_common_state_update(ee);
                  aux->notified = 1;
                }
             break;
          }
     }
#endif   
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_ecore_win_wl_common_cb_window_iconify_change(void *data  EINA_UNUSED, int type EINA_UNUSED, void *event)
{
#if 0
   Ecore_Win *ee;
   Ecore_Wl_Event_Window_Iconify_State_Change *ev;
   Ecore_Win_Engine_Wl_Data *wdata;

   ev = event;
   ee = ecore_event_window_match(ev->win);

   if (!ee) return ECORE_CALLBACK_PASS_ON;
   if (ev->win != ee->prop.window) return ECORE_CALLBACK_PASS_ON;

   if (ee->prop.iconified == ev->iconified)
     return ECORE_CALLBACK_PASS_ON;

   ee->prop.iconified = ev->iconified;

   wdata = ee->engine.data;
   if (wdata && ev->force)
     ecore_wl_window_iconify_state_update(wdata->win, ev->iconified, EINA_FALSE);
   _ecore_win_wl_common_state_update(ee);
#endif   
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_ecore_win_wl_common_cb_window_visibility_change(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
#if 0
   Ecore_Win *ee;
   Ecore_Wl_Event_Window_Visibility_Change *ev;

   ev = event;
   ee = ecore_event_window_match(ev->win);

   if (!ee) return ECORE_CALLBACK_PASS_ON;
   if (ev->win != ee->prop.window) return ECORE_CALLBACK_PASS_ON;

   if (ee->prop.obscured == ev->fully_obscured)
     return ECORE_CALLBACK_PASS_ON;

   ee->prop.obscured = ev->fully_obscured;
   _ecore_win_wl_common_state_update(ee);
#endif   
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_ecore_win_wl_common_cb_window_rotate(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
#if 0
   Ecore_Win *ee;
   Ecore_Wl_Event_Window_Rotate *ev;
   Ecore_Win_Engine_Wl_Data *wdata;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ev = event;
   ee = ecore_event_window_match(ev->win);
   if (!ee) return ECORE_CALLBACK_PASS_ON;
   if (ev->win != ee->prop.window) return ECORE_CALLBACK_PASS_ON;

   wdata = ee->engine.data;
   if (!wdata) return ECORE_CALLBACK_PASS_ON;

   if ((!ee->prop.wm_rot.supported) || (!ee->prop.wm_rot.app_set))
     return ECORE_CALLBACK_PASS_ON;

   wdata->wm_rot.request = 1;
   wdata->wm_rot.done = 0;

   if ((ee->w != ev->w) || (ee->h != ev->h))
     {
        _ecore_win_wl_common_resize(ee, ev->w , ev->h);
     }

   if (ee->prop.wm_rot.manual_mode.set)
     {
        ee->prop.wm_rot.manual_mode.wait_for_done = EINA_TRUE;
        _ecore_win_wl_common_wm_rot_manual_rotation_done_timeout_update(ee);
     }

   if (!strcmp(ee->driver, "wayland_shm"))
     {
#ifdef BUILD_ECORE_WIN_WAYLAND_SHM
        _ecore_win_wayland_shm_window_rotate(ee, ev->angle, 1);
#endif
     }
   else if (!strcmp(ee->driver, "wayland_egl"))
     {
#ifdef BUILD_ECORE_WIN_WAYLAND_EGL
        _ecore_win_wayland_egl_window_rotate(ee, ev->angle, 1);
#endif
     }

   wdata->wm_rot.done = 1;

   /* Fixme: rotation done send move to render flush */
   if (!ee->prop.wm_rot.manual_mode.set)
     {
        wdata->wm_rot.request = 0;
        wdata->wm_rot.done = 0;
        ecore_wl_window_rotation_change_done_send(wdata->win);
     }
#endif
   return ECORE_CALLBACK_PASS_ON;
}

static void
_rotation_do(Ecore_Win *ee, int rotation, int resize)
{
   Ecore_Win_Engine_Wl_Data *wdata;
   int rot_dif;

   wdata = ee->engine.data;

   /* calculate difference in rotation */
   rot_dif = ee->rotation - rotation;
   if (rot_dif < 0) rot_dif = -rot_dif;

   /* set ecore_wayland window rotation */
   ecore_wl_window_rotation_set(wdata->win, rotation);

   /* check if rotation is just a flip */
   if (rot_dif != 180)
     {
        int minw, minh, maxw, maxh;
        int basew, baseh, stepw, steph;

        /* check if we are rotating with resize */
        if (!resize)
          {
             int fw, fh;
             int ww, hh;

             /* resize the canvas based on rotation */
             if ((rotation == 0) || (rotation == 180))
                {
                   /* resize the ecore_wayland window */
                   ecore_wl_window_resize(wdata->win,
                                          ee->req.w, ee->req.h, 0);
                }
             else
                {
                   /* resize the ecore_wayland window */
                   ecore_wl_window_resize(wdata->win,
                                          ee->req.h, ee->req.w, 0);
                }

             ww = ee->h;
             hh = ee->w;
             ee->w = ww;
             ee->h = hh;
             ee->req.w = ww;
             ee->req.h = hh;
          }
        else
          {
             /* call the ecore_win' resize function */
             if (ee->func.fn_resize) ee->func.fn_resize(ee);

          }

        /* get min, max, base, & step sizes */
        ecore_win_size_min_get(ee, &minw, &minh);
        ecore_win_size_max_get(ee, &maxw, &maxh);
        ecore_win_size_base_get(ee, &basew, &baseh);
        ecore_win_size_step_get(ee, &stepw, &steph);

        /* record the current rotation of the ecore_win */
        ee->rotation = rotation;

        /* reset min, max, base, & step sizes */
        ecore_win_size_min_set(ee, minh, minw);
        ecore_win_size_max_set(ee, maxh, maxw);
        ecore_win_size_base_set(ee, baseh, basew);
        ecore_win_size_step_set(ee, steph, stepw);

        /* send a mouse_move process
         *
         * NB: Is This Really Needed ?
         * Yes, it's required to update the mouse position, relatively to
         * widgets. After a rotation change, e.g., the mouse might not be over
         * a button anymore. */
        _ecore_win_mouse_move_process(ee, ee->mouse.x, ee->mouse.y,
                                       ecore_loop_time_get());
     }
   else
     {
        /* resize the ecore_wayland window */
        ecore_wl_window_resize(wdata->win, ee->w, ee->h, 0);

        /* record the current rotation of the ecore_win */
        ee->rotation = rotation;

        /* send a mouse_move process
         *
         * NB: Is This Really Needed ? Yes, it's required to update the mouse
         * position, relatively to widgets. */
        _ecore_win_mouse_move_process(ee, ee->mouse.x, ee->mouse.y,
                                       ecore_loop_time_get());

        /* call the ecore_win' resize function */
        if (ee->func.fn_resize) ee->func.fn_resize(ee);

     }
}

void
_ecore_win_wl_common_rotation_set(Ecore_Win *ee, int rotation, int resize)
{
   if (ee->in_async_render)
     {
        ee->delayed.rotation = rotation;
        ee->delayed.rotation_resize = resize;
        ee->delayed.rotation_changed = EINA_TRUE;
        return;
     }
   _rotation_do(ee, rotation, resize);
}

int
_ecore_win_wl_common_init(void)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (++_ecore_win_wl_init_count != 1)
     return _ecore_win_wl_init_count;

   _ecore_win_wl_event_hdls[0] =
     ecore_event_handler_add(ECORE_WL_EVENT_MOUSE_IN,
                             _ecore_win_wl_common_cb_mouse_in, NULL);
   _ecore_win_wl_event_hdls[1] =
     ecore_event_handler_add(ECORE_WL_EVENT_MOUSE_OUT,
                             _ecore_win_wl_common_cb_mouse_out, NULL);
   _ecore_win_wl_event_hdls[2] =
     ecore_event_handler_add(ECORE_WL_EVENT_FOCUS_IN,
                             _ecore_win_wl_common_cb_focus_in, NULL);
   _ecore_win_wl_event_hdls[3] =
     ecore_event_handler_add(ECORE_WL_EVENT_FOCUS_OUT,
                             _ecore_win_wl_common_cb_focus_out, NULL);
   _ecore_win_wl_event_hdls[4] =
     ecore_event_handler_add(ECORE_WL_EVENT_WINDOW_CONFIGURE,
                             _ecore_win_wl_common_cb_window_configure, NULL);
   _ecore_win_wl_event_hdls[5] =
     ecore_event_handler_add(ECORE_WL_EVENT_CONFORMANT_CHANGE,
                             _ecore_win_wl_common_cb_conformant_change, NULL);
   _ecore_win_wl_event_hdls[6] =
     ecore_event_handler_add(ECORE_WL_EVENT_WINDOW_ROTATE,
                             _ecore_win_wl_common_cb_window_rotate, NULL);
   _ecore_win_wl_event_hdls[7] =
     ecore_event_handler_add(ECORE_WL_EVENT_AUX_HINT_ALLOWED,
                             _ecore_win_wl_common_cb_aux_hint_allowed, NULL);
   _ecore_win_wl_event_hdls[8] =
     ecore_event_handler_add(ECORE_WL_EVENT_WINDOW_ICONIFY_STATE_CHANGE,
                             _ecore_win_wl_common_cb_window_iconify_change, NULL);
   _ecore_win_wl_event_hdls[9] =
     ecore_event_handler_add(ECORE_WL_EVENT_WINDOW_VISIBILITY_CHANGE,
                             _ecore_win_wl_common_cb_window_visibility_change, NULL);

   ecore_event_evas_init();
   return _ecore_win_wl_init_count;
}

int
_ecore_win_wl_common_shutdown(void)
{
   unsigned int i = 0;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (--_ecore_win_wl_init_count != 0)
     return _ecore_win_wl_init_count;

   for (i = 0; i < sizeof(_ecore_win_wl_event_hdls) / sizeof(Ecore_Event_Handler *); i++)
     {
        if (_ecore_win_wl_event_hdls[i])
          ecore_event_handler_del(_ecore_win_wl_event_hdls[i]);
     }

   ecore_event_evas_shutdown();
   return _ecore_win_wl_init_count;
}

void
_ecore_win_wl_common_pre_free(Ecore_Win *ee)
{
#if 0
   Ecore_Win_Engine_Wl_Data *wdata;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ee) return;
   wdata = ee->engine.data;
   if (wdata->frame) evas_object_del(wdata->frame);
#endif   
}

void
_ecore_win_wl_common_free(Ecore_Win *ee)
{
   Ecore_Win_Engine_Wl_Data *wdata;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ee) return;
   wdata = ee->engine.data;
   if (wdata->anim_callback)
     wl_callback_destroy(wdata->anim_callback);
   if (wdata->win) ecore_wl_window_free(wdata->win);
   wdata->win = NULL;
   free(wdata);

   ecore_event_window_unregister(ee->prop.window);

   _ecore_win_wl_common_shutdown();
   ecore_wl_shutdown();
}

void
_ecore_win_wl_common_resize(Ecore_Win *ee, int w, int h)
{
   Ecore_Win_Engine_Wl_Data *wdata = ee->engine.data;
   int orig_w, orig_h;
   int ow, oh;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ee) return;
   if (w < 1) w = 1;
   if (h < 1) h = 1;

   ee->req.w = w;
   ee->req.h = h;

   if (wdata->win)
     ecore_wl_window_update_size(wdata->win, ee->req.w, ee->req.h);
}

void
_ecore_win_wl_common_callback_resize_set(Ecore_Win *ee, void (*func)(Ecore_Win *ee))
{
   if (!ee) return;
   ee->func.fn_resize = func;
}

void
_ecore_win_wl_common_callback_move_set(Ecore_Win *ee, void (*func)(Ecore_Win *ee))
{
   if (!ee) return;
   ee->func.fn_move = func;
}

void
_ecore_win_wl_common_callback_delete_request_set(Ecore_Win *ee, void (*func)(Ecore_Win *ee))
{
   if (!ee) return;
   ee->func.fn_delete_request = func;
}

void
_ecore_win_wl_common_callback_focus_in_set(Ecore_Win *ee, void (*func)(Ecore_Win *ee))
{
   if (!ee) return;
   ee->func.fn_focus_in = func;
}

void
_ecore_win_wl_common_callback_focus_out_set(Ecore_Win *ee, void (*func)(Ecore_Win *ee))
{
   if (!ee) return;
   ee->func.fn_focus_out = func;
}

void
_ecore_win_wl_common_callback_mouse_in_set(Ecore_Win *ee, void (*func)(Ecore_Win *ee))
{
   if (!ee) return;
   ee->func.fn_mouse_in = func;
}

void
_ecore_win_wl_common_callback_mouse_out_set(Ecore_Win *ee, void (*func)(Ecore_Win *ee))
{
   if (!ee) return;
   ee->func.fn_mouse_out = func;
}

void
_ecore_win_wl_common_move(Ecore_Win *ee, int x, int y)
{
   Ecore_Win_Engine_Wl_Data *wdata;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ee) return;

   wdata = ee->engine.data;
   ee->req.x = x;
   ee->req.y = y;

   if ((ee->x != x) || (ee->y != y))
     {
        ee->x = x;
        ee->y = y;
        if (ee->func.fn_move) ee->func.fn_move(ee);
     }

   ecore_wl_window_position_set(wdata->win, x, y);
}

/* Frame border:
 *
 * |------------------------------------------|
 * |                top border                |
 * |------------------------------------------|
 * |       |                         |        |
 * |       |                         |        |
 * |       |                         |        |
 * |       |                         |        |
 * |left   |                         | right  |
 * |border |                         | border |
 * |       |                         |        |
 * |       |                         |        |
 * |       |                         |        |
 * |------------------------------------------|
 * |                bottom border             |
 * |------------------------------------------|
 */
static void
_border_size_eval(Evas_Object *obj EINA_UNUSED, EE_Wl_Smart_Data *sd)
{
#if 0
   /* top border */
   if (sd->border[0])
     {
        evas_object_move(sd->border[0], sd->x, sd->y);
        evas_object_resize(sd->border[0], sd->w, sd->border_size[0]);
     }

   /* bottom border */
   if (sd->border[1])
     {
        evas_object_move(sd->border[1], sd->x,
                         sd->y + sd->h - sd->border_size[1]);
        evas_object_resize(sd->border[1], sd->w, sd->border_size[1]);
     }

   /* left border */
   if (sd->border[2])
     {
        evas_object_move(sd->border[2], sd->x, sd->y + sd->border_size[0]);
        evas_object_resize(sd->border[2], sd->border_size[2],
                           sd->h - sd->border_size[0] - sd->border_size[1]);
     }

   /* right border */
   if (sd->border[3])
     {
        evas_object_move(sd->border[3], sd->x + sd->w - sd->border_size[3],
                         sd->y + sd->border_size[0]);
        evas_object_resize(sd->border[3], sd->border_size[3],
                           sd->h - sd->border_size[0] - sd->border_size[1]);
     }
#endif      
}

static void
_ecore_win_wl_common_smart_add(Evas_Object *obj)
{
#if 0
   EE_Wl_Smart_Data *sd;
   Evas *evas;
   int i;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   EVAS_SMART_DATA_ALLOC(obj, EE_Wl_Smart_Data);

   _ecore_win_wl_frame_parent_sc->add(obj);

   sd = priv;

   evas = evas_object_evas_get(obj);

   sd->x = 0;
   sd->y = 0;
   sd->w = 1;
   sd->h = 1;

   for (i = 0; i < 4; i++)
     {
        sd->border[i] = NULL;
        sd->border_size[i] = 0;
     }

   sd->text = evas_object_text_add(evas);
   evas_object_color_set(sd->text, 0, 0, 0, 255);
   evas_object_text_style_set(sd->text, EVAS_TEXT_STYLE_PLAIN);
   evas_object_text_font_set(sd->text, "Sans", 10);
   evas_object_text_text_set(sd->text, "Smart Test");
   evas_object_show(sd->text);
   evas_object_smart_member_add(sd->text, obj);
#endif      
}

static void
_ecore_win_wl_common_smart_del(Evas_Object *obj)
{
#if 0
   EE_Wl_Smart_Data *sd;
   int i;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(sd = evas_object_smart_data_get(obj))) return;
   evas_object_del(sd->text);
   for (i = 0; i < 4; i++)
     {
        evas_object_del(sd->border[i]);
     }
   _ecore_win_wl_frame_parent_sc->del(obj);
#endif      
}

static void
_ecore_win_wl_common_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
#if 0
   EE_Wl_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   _ecore_win_wl_frame_parent_sc->move(obj, x, y);

   if (!(sd = evas_object_smart_data_get(obj))) return;
   if ((sd->x == x) && (sd->y == y)) return;
   sd->x = x;
   sd->y = y;

   evas_object_smart_changed(obj);
#endif      
}

static void
_ecore_win_wl_common_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
#if 0
   EE_Wl_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(sd = evas_object_smart_data_get(obj))) return;
   if ((sd->w == w) && (sd->h == h)) return;
   sd->w = w;
   sd->h = h;

   evas_object_smart_changed(obj);
#endif      
}

void
_ecore_win_wl_common_smart_calculate(Evas_Object *obj)
{
#if 0
   EE_Wl_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(sd = evas_object_smart_data_get(obj))) return;

   _border_size_eval(obj, sd);
#endif      
}

static void
_ecore_win_wl_frame_smart_set_user(Evas_Smart_Class *sc)
{
#if 0
   sc->add = _ecore_win_wl_common_smart_add;
   sc->del = _ecore_win_wl_common_smart_del;
   sc->move = _ecore_win_wl_common_smart_move;
   sc->resize = _ecore_win_wl_common_smart_resize;
   sc->calculate = _ecore_win_wl_common_smart_calculate;
#endif      
}

Evas_Object *
_ecore_win_wl_common_frame_add(Evas *evas)
{
#if 0
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   return evas_object_smart_add(evas, _ecore_win_wl_frame_smart_class_new());
#endif   
   return NULL;
}

/*
 * Size is received in the same format as it is used to set the framespace
 * offset size.
 */
void
_ecore_win_wl_common_frame_border_size_set(Evas_Object *obj, int fx, int fy, int fw, int fh)
{
#if 0
   EE_Wl_Smart_Data *sd;
   Evas *e;
   int i;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(sd = evas_object_smart_data_get(obj))) return;

   e = evas_object_evas_get(obj);

   sd->border_size[0] = fy;
   sd->border_size[1] = fh - fy;
   sd->border_size[2] = fx;
   sd->border_size[3] = fw - fx;

   for (i = 0; i < 4; i++)
     {
        if ((sd->border_size[i] <= 0) && (sd->border[i]))
          {
             evas_object_del(sd->border[i]);
             sd->border[i] = NULL;
          }
        else if ((sd->border_size[i] > 0) && (!sd->border[i]))
          {
             sd->border[i] = evas_object_rectangle_add(e);
             evas_object_color_set(sd->border[i], 249, 249, 249, 255);
             evas_object_show(sd->border[i]);
             evas_object_smart_member_add(sd->border[i], obj);
          }
     }
   evas_object_raise(sd->text);
#endif      
}

void
_ecore_win_wl_common_frame_border_size_get(Evas_Object *obj, int *fx, int *fy, int *fw, int *fh)
{
#if 0
   EE_Wl_Smart_Data *sd;
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(sd = evas_object_smart_data_get(obj))) return;

   if (fx) *fx = sd->border_size[2];
   if (fy) *fy = sd->border_size[0];
   if (fw) *fw = sd->border_size[2] + sd->border_size[3];
   if (fh) *fh = sd->border_size[0] + sd->border_size[1];
#endif      
}

void 
_ecore_win_wl_common_pointer_xy_get(const Ecore_Win *ee EINA_UNUSED, Evas_Coord *x, Evas_Coord *y)
{
#if 0
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ecore_wl_pointer_xy_get(x, y);
#endif      
}

void
_ecore_win_wl_common_wm_rot_preferred_rotation_set(Ecore_Win *ee, int rot)
{
#if 0
   Ecore_Win_Engine_Wl_Data *wdata;
   wdata = ee->engine.data;

   if (ee->prop.wm_rot.supported)
     {
        if (!ee->prop.wm_rot.app_set)
          {
             //Need?: App_set wayland rotation request?
             ee->prop.wm_rot.app_set = EINA_TRUE;
          }
        ecore_wl_window_rotation_preferred_rotation_set(wdata->win, rot);
        ee->prop.wm_rot.preferred_rot = rot;
     }
#endif      
}

void
_ecore_win_wl_common_wm_rot_available_rotations_set(Ecore_Win *ee, const int *rots, unsigned int count)
{
#if 0
   Ecore_Win_Engine_Wl_Data *wdata;
   wdata = ee->engine.data;

   if (ee->prop.wm_rot.supported)
     {
        if (!ee->prop.wm_rot.app_set)
          {
             //Need?: App_set wayland rotation request?
             ee->prop.wm_rot.app_set = EINA_TRUE;
          }

        if (ee->prop.wm_rot.available_rots)
          {
             free(ee->prop.wm_rot.available_rots);
             ee->prop.wm_rot.available_rots = NULL;
          }

        ee->prop.wm_rot.count = 0;

        if (count > 0)
          {
             ee->prop.wm_rot.available_rots = calloc(count, sizeof(int));
             if (!ee->prop.wm_rot.available_rots) return;

             memcpy(ee->prop.wm_rot.available_rots, rots, sizeof(int) * count);
          }

        ee->prop.wm_rot.count = count;

        ecore_wl_window_rotation_available_rotations_set(wdata->win, rots, count);
     }
#endif      
}

static void
_ecore_win_wl_common_wm_rot_manual_rotation_done_job(void *data)
{
#if 0
   Ecore_Win *ee = (Ecore_Win *)data;
   Ecore_Win_Engine_Wl_Data *wdata = ee->engine.data;

   wdata->wm_rot.manual_mode_job = NULL;
   ee->prop.wm_rot.manual_mode.wait_for_done = EINA_FALSE;

   ecore_wl_window_rotation_change_done_send(wdata->win);

   wdata->wm_rot.done = 0;
#endif      
}

static Eina_Bool
_ecore_win_wl_common_wm_rot_manual_rotation_done_timeout(void *data)
{
#if 0
   Ecore_Win *ee = data;

   ee->prop.wm_rot.manual_mode.timer = NULL;
   _ecore_win_wl_common_wm_rot_manual_rotation_done(ee);
#endif      
   return ECORE_CALLBACK_CANCEL;
}

static void
_ecore_win_wl_common_wm_rot_manual_rotation_done_timeout_update(Ecore_Win *ee)
{
#if 0
   if (ee->prop.wm_rot.manual_mode.timer)
     ecore_timer_del(ee->prop.wm_rot.manual_mode.timer);

   ee->prop.wm_rot.manual_mode.timer = ecore_timer_add
     (4.0f, _ecore_win_wl_common_wm_rot_manual_rotation_done_timeout, ee);
#endif
}

void
_ecore_win_wl_common_wm_rot_manual_rotation_done_set(Ecore_Win *ee, Eina_Bool set)
{
#if 0
   ee->prop.wm_rot.manual_mode.set = set;
#endif   
}

void
_ecore_win_wl_common_wm_rot_manual_rotation_done(Ecore_Win *ee)
{
#if 0
   Ecore_Win_Engine_Wl_Data *wdata = ee->engine.data;

   if ((ee->prop.wm_rot.supported) &&
       (ee->prop.wm_rot.app_set) &&
       (ee->prop.wm_rot.manual_mode.set))
     {
        if (ee->prop.wm_rot.manual_mode.wait_for_done)
          {
             if (ee->prop.wm_rot.manual_mode.timer)
               ecore_timer_del(ee->prop.wm_rot.manual_mode.timer);
             ee->prop.wm_rot.manual_mode.timer = NULL;

             if (wdata->wm_rot.manual_mode_job)
               ecore_job_del(wdata->wm_rot.manual_mode_job);
             wdata->wm_rot.manual_mode_job = ecore_job_add
               (_ecore_win_wl_common_wm_rot_manual_rotation_done_job, ee);
          }
     }
#endif      
}

void
_ecore_win_wl_common_raise(Ecore_Win *ee)
{
#if 0
   Ecore_Win_Engine_Wl_Data *wdata;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if ((!ee) || (!ee->visible)) return;
   wdata = ee->engine.data;
   ecore_wl_window_raise(wdata->win);
#endif      
}

void
_ecore_win_wl_common_lower(Ecore_Win *ee)
{
#if 0
   Ecore_Win_Engine_Wl_Data *wdata;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if ((!ee) || (!ee->visible)) return;
   wdata = ee->engine.data;
   ecore_wl_window_lower(wdata->win);
#endif      
}

void
_ecore_win_wl_common_activate(Ecore_Win *ee)
{
#if 0
   Ecore_Win_Engine_Wl_Data *wdata;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ee) return;
   wdata = ee->engine.data;
   ecore_win_show(ee);

   if (ee->prop.iconified)
     _ecore_win_wl_common_iconified_set(ee, EINA_FALSE);

   ecore_wl_window_activate(wdata->win);
#endif      
}

void
_ecore_win_wl_common_title_set(Ecore_Win *ee, const char *title)
{
#if 0
   Ecore_Win_Engine_Wl_Data *wdata;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ee) return;
   if (eina_streq(ee->prop.title, title)) return;
   if (ee->prop.title) free(ee->prop.title);
   ee->prop.title = NULL;
   if (title) ee->prop.title = strdup(title);
   wdata = ee->engine.data;
   if ((ee->prop.draw_frame) && (wdata->frame))
     {
        EE_Wl_Smart_Data *sd;

        if ((sd = evas_object_smart_data_get(wdata->frame)))
          evas_object_text_text_set(sd->text, ee->prop.title);
     }

   if (ee->prop.title)
     ecore_wl_window_title_set(wdata->win, ee->prop.title);
#endif      
}

void
_ecore_win_wl_common_name_class_set(Ecore_Win *ee, const char *n, const char *c)
{
#if 0
   Ecore_Win_Engine_Wl_Data *wdata;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ee) return;
   wdata = ee->engine.data;
   if (!eina_streq(ee->prop.name, n))
     {
        if (ee->prop.name) free(ee->prop.name);
        ee->prop.name = NULL;
        if (n) ee->prop.name = strdup(n);
     }
   if (!eina_streq(ee->prop.clas, c))
     {
        if (ee->prop.clas) free(ee->prop.clas);
        ee->prop.clas = NULL;
        if (c) ee->prop.clas = strdup(c);
     }
   if (ee->prop.clas)
     ecore_wl_window_class_name_set(wdata->win, ee->prop.clas);
#endif      
}

void
_ecore_win_wl_common_size_min_set(Ecore_Win *ee, int w, int h)
{
#if 0
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ee) return;
   if (w < 0) w = 0;
   if (h < 0) h = 0;
   if ((ee->prop.min.w == w) && (ee->prop.min.h == h)) return;
   ee->prop.min.w = w;
   ee->prop.min.h = h;
#endif      
}

void
_ecore_win_wl_common_size_max_set(Ecore_Win *ee, int w, int h)
{
#if 0
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ee) return;
   if (w < 0) w = 0;
   if (h < 0) h = 0;
   if ((ee->prop.max.w == w) && (ee->prop.max.h == h)) return;
   ee->prop.max.w = w;
   ee->prop.max.h = h;
#endif      
}

void
_ecore_win_wl_common_size_base_set(Ecore_Win *ee, int w, int h)
{
#if 0
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ee) return;
   if (w < 0) w = 0;
   if (h < 0) h = 0;
   if ((ee->prop.base.w == w) && (ee->prop.base.h == h)) return;
   ee->prop.base.w = w;
   ee->prop.base.h = h;
#endif      
}

void
_ecore_win_wl_common_size_step_set(Ecore_Win *ee, int w, int h)
{
#if 0
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ee) return;
   if (w < 0) w = 0;
   if (h < 0) h = 0;
   if ((ee->prop.step.w == w) && (ee->prop.step.h == h)) return;
   ee->prop.step.w = w;
   ee->prop.step.h = h;
#endif      
}

void 
_ecore_win_wl_common_aspect_set(Ecore_Win *ee, double aspect)
{
#if 0
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ee) return;
   if (ee->prop.aspect == aspect) return;
   ee->prop.aspect = aspect;
#endif      
}

static void
_ecore_win_object_cursor_del(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
#if 0
   Ecore_Win *ee;

   ee = data;
   if (ee) ee->prop.cursor.object = NULL;
#endif      
}

void
_ecore_win_wl_common_object_cursor_unset(Ecore_Win *ee)
{
#if 0
   evas_object_event_callback_del_full(ee->prop.cursor.object,
                                       EVAS_CALLBACK_DEL,
                                       _ecore_win_object_cursor_del, ee);
#endif   
}

void
_ecore_win_wl_common_object_cursor_set(Ecore_Win *ee, Evas_Object *obj, int layer, int hot_x, int hot_y)
{
#if 0
   int x, y, fx, fy;
   Ecore_Win_Engine_Wl_Data *wdata = ee->engine.data;
   Evas_Object *old;

   if (!ee) return;
   old = ee->prop.cursor.object;
   if (obj == NULL)
     {
        ecore_wl_window_pointer_set(wdata->win, NULL, 0, 0);
        ee->prop.cursor.object = NULL;
        ee->prop.cursor.layer = 0;
        ee->prop.cursor.hot.x = 0;
        ee->prop.cursor.hot.y = 0;
        goto end;
     }

   ee->prop.cursor.object = obj;
   ee->prop.cursor.layer = layer;
   ee->prop.cursor.hot.x = hot_x;
   ee->prop.cursor.hot.y = hot_y;

   evas_pointer_output_xy_get(ee->evas, &x, &y);

   if (obj != old)
     {
        ecore_wl_window_pointer_set(wdata->win, NULL, 0, 0);
        evas_object_layer_set(ee->prop.cursor.object, ee->prop.cursor.layer);
        evas_object_pass_events_set(ee->prop.cursor.object, 1);
        if (evas_pointer_inside_get(ee->evas))
          evas_object_show(ee->prop.cursor.object);
        evas_object_event_callback_add(obj, EVAS_CALLBACK_DEL,
                                       _ecore_win_object_cursor_del, ee);
     }

   evas_output_framespace_get(ee->evas, &fx, &fy, NULL, NULL);
   evas_object_move(ee->prop.cursor.object, x - fx - ee->prop.cursor.hot.x,
                    y - fy - ee->prop.cursor.hot.y);

end:
   if ((old) && (obj != old))
     {
        evas_object_event_callback_del_full
          (old, EVAS_CALLBACK_DEL, _ecore_win_object_cursor_del, ee);
        evas_object_del(old);
     }
#endif   
}

void
_ecore_win_wl_common_layer_set(Ecore_Win *ee, int layer)
{
#if 0
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ee) return;
   if (ee->prop.layer == layer) return;
   if (layer < 1) layer = 1;
   else if (layer > 255) layer = 255;
   ee->prop.layer = layer;
   _ecore_win_wl_common_state_update(ee);
#endif      
}

void
_ecore_win_wl_common_iconified_set(Ecore_Win *ee, Eina_Bool on)
{
#if 0
   Ecore_Win_Engine_Wl_Data *wdata;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ee) return;

   if (ee->prop.iconified == on) return;
   ee->prop.iconified = on;

   wdata = ee->engine.data;
   ecore_wl_window_iconified_set(wdata->win, on);
   _ecore_win_wl_common_state_update(ee);
#endif      
}

static void
_ecore_win_wl_common_border_update(Ecore_Win *ee)
{
#if 0
   Ecore_Win_Engine_Wl_Data *wdata;

   wdata = ee->engine.data;
   if (!wdata->frame)
     return;

   if ((ee->prop.borderless) || (ee->prop.fullscreen))
     {
        evas_object_hide(wdata->frame);
        evas_output_framespace_set(ee->evas, 0, 0, 0, 0);
     }
   else
     {
        int fx = 0, fy = 0, fw = 0, fh = 0;

        evas_object_show(wdata->frame);
        _ecore_win_wl_common_frame_border_size_get(wdata->frame,
                                                    &fx, &fy, &fw, &fh);
        evas_output_framespace_set(ee->evas, fx, fy, fw, fh);
     }
#endif      
}

void
_ecore_win_wl_common_borderless_set(Ecore_Win *ee, Eina_Bool on)
{
#if 0
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ee) return;
   if (ee->prop.borderless == on) return;
   ee->prop.borderless = on;

   _ecore_win_wl_common_border_update(ee);
   _ecore_win_wl_common_state_update(ee);
#endif      
}

void
_ecore_win_wl_common_maximized_set(Ecore_Win *ee, Eina_Bool on)
{
#if 0
   Ecore_Win_Engine_Wl_Data *wdata;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ee) return;
   if (ee->prop.maximized == on) return;
   wdata = ee->engine.data;
   ecore_wl_window_maximized_set(wdata->win, on);
//   _ecore_win_wl_common_state_update(ee);
#endif   
}

void
_ecore_win_wl_common_fullscreen_set(Ecore_Win *ee, Eina_Bool on)
{
#if 0
   Ecore_Win_Engine_Wl_Data *wdata;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ee) return;
   if (ee->prop.fullscreen == on) return;
   wdata = ee->engine.data;
   ecore_wl_window_fullscreen_set(wdata->win, on);
//   _ecore_win_wl_common_state_update(ee);
#endif   
}

void
_ecore_win_wl_common_ignore_events_set(Ecore_Win *ee, int ignore)
{
#if 0
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ee) return;
   ee->ignore_events = ignore;
   /* NB: Hmmm, may need to pass this to ecore_wl_window in the future */
#endif      
}

void
_ecore_win_wl_common_focus_skip_set(Ecore_Win *ee, Eina_Bool on)
{
#if 0
   Ecore_Win_Engine_Wl_Data *wdata;

   wdata = ee->engine.data;

   if (ee->prop.focus_skip == on) return;

   ee->prop.focus_skip = on;
   ecore_wl_window_focus_skip_set(wdata->win, on);
#endif      
}

int
_ecore_win_wl_common_pre_render(Ecore_Win *ee)
{
   int rend = 0;
#if 0
   Eina_List *ll = NULL;
   Ecore_Win *ee2 = NULL;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ee) return 0;
   if (ee->in_async_render)
     {
        /* EDBG("ee=%p is rendering asynchronously, skip", ee); */
        return 0;
     }

   EINA_LIST_FOREACH(ee->sub_ecore_win, ll, ee2)
     {
        if (ee2->func.fn_pre_render) ee2->func.fn_pre_render(ee2);
        if (ee2->engine.func->fn_render)
          rend |= ee2->engine.func->fn_render(ee2);
        if (ee2->func.fn_post_render) ee2->func.fn_post_render(ee2);
     }

   if (ee->func.fn_pre_render) ee->func.fn_pre_render(ee);
#endif   
   return rend;
}

static void
_anim_cb_animate(void *data, struct wl_callback *callback, uint32_t serial EINA_UNUSED)
{
#if 0
   Ecore_Win *ee = data;
   Ecore_Win_Engine_Wl_Data *wdata;

   wdata = ee->engine.data;
   wl_callback_destroy(callback);
   wdata->anim_callback = NULL;
   ecore_win_manual_render_set(ee, 0);
#endif      
}

static const struct wl_callback_listener _anim_listener =
{
   _anim_cb_animate
};

void
_ecore_win_wl_common_render_pre(void *data, Evas *evas EINA_UNUSED, void *event EINA_UNUSED)
{
#if 0
   Ecore_Win *ee = data;
   Ecore_Win_Engine_Wl_Data *wdata;

   if (!ee->visible) return;

   wdata = ee->engine.data;
   wdata->anim_callback =
     wl_surface_frame(ecore_wl_window_surface_get(wdata->win));
   wl_callback_add_listener(wdata->anim_callback, &_anim_listener, ee);
   ecore_win_manual_render_set(ee, 1);
#endif      
}

void 
_ecore_win_wl_common_render_updates(void *data, Evas *evas EINA_UNUSED, void *event)
{
#if 0
   Evas_Event_Render_Post *ev = event;
   Ecore_Win *ee = data;

   if (!(ee) || !(ev)) return;

   ee->in_async_render = EINA_FALSE;

   _ecore_win_wl_common_render_updates_process(ee, ev->updated_area);

   if (ee->delayed.alpha_changed)
     {
        _ecore_win_wayland_alpha_do(ee, ee->delayed.alpha);
        ee->delayed.alpha_changed = EINA_FALSE;
     }
   if (ee->delayed.transparent_changed)
     {
        _ecore_win_wayland_transparent_do(ee, ee->delayed.transparent);
        ee->delayed.transparent_changed = EINA_FALSE;
     }
   if (ee->delayed.rotation_changed)
     {
        _rotation_do(ee, ee->delayed.rotation, ee->delayed.rotation_resize);
        ee->delayed.rotation_changed = EINA_FALSE;
     }
#endif      
}

void
_ecore_win_wl_common_post_render(Ecore_Win *ee)
{
#if 0
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   _ecore_win_idle_timeout_update(ee);
   if (ee->func.fn_post_render) ee->func.fn_post_render(ee);
#endif   
}

//int
//_ecore_win_wl_common_render(Ecore_Win *ee)
//{
//   int rend = 0;
//#if 0
//   Eina_List *l;
//   Ecore_Win *ee2;
//   Ecore_Wl_Window *win = NULL;
//   Ecore_Win_Engine_Wl_Data *wdata;
//
//   if (!ee) return 0;
//   if (!(wdata = ee->engine.data)) return 0;
//   if (!(win = wdata->win)) return 0;
//
//   /* TODO: handle comp no sync */
//
//   if (ee->in_async_render) return 0;
//   if (!ee->visible)
//     {
//        evas_norender(ee->evas);
//        return 0;
//     }
//
//   EINA_LIST_FOREACH(ee->sub_ecore_win, l, ee2)
//     {
//        if (ee2->func.fn_pre_render) ee2->func.fn_pre_render(ee2);
//        if (ee2->engine.func->fn_render)
//          rend |= ee2->engine.func->fn_render(ee2);
//        if (ee2->func.fn_post_render) ee2->func.fn_post_render(ee2);
//     }
//
//   if (ee->func.fn_pre_render) ee->func.fn_pre_render(ee);
//
//   if (!ee->can_async_render)
//     {
//        Eina_List *updates;
//
//        updates = evas_render_updates(ee->evas);
//        rend = _ecore_win_wl_common_render_updates_process(ee, updates);
//        evas_render_updates_free(updates);
//     }
//   else if (evas_render_async(ee->evas))
//     {
//        ee->in_async_render = EINA_TRUE;
//        rend = 1;
//     }
//   else if (ee->func.fn_post_render)
//     ee->func.fn_post_render(ee);
//#endif
//   return rend;
//}

void
_ecore_win_wl_common_withdrawn_set(Ecore_Win *ee, Eina_Bool on)
{
#if 0
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (ee->prop.withdrawn == on) return;

   ee->prop.withdrawn = on;

   if (on)
     ecore_win_hide(ee);
   else
     ecore_win_show(ee);

   _ecore_win_wl_common_state_update(ee);
#endif   
}

void
_ecore_win_wl_common_screen_geometry_get(const Ecore_Win *ee EINA_UNUSED, int *x, int *y, int *w, int *h)
{
#if 0
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (x) *x = 0;
   if (y) *y = 0;
   ecore_wl_screen_size_get(w, h);
#endif   
}

void
_ecore_win_wl_common_screen_dpi_get(const Ecore_Win *ee EINA_UNUSED, int *xdpi, int *ydpi)
{
#if 0
   int dpi = 0;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (xdpi) *xdpi = 0;
   if (ydpi) *ydpi = 0;
   /* FIXME: Ideally this needs to get the DPI from a specific screen */
   dpi = ecore_wl_dpi_get();
   if (xdpi) *xdpi = dpi;
   if (ydpi) *ydpi = dpi;
#endif   
}

static void
_ecore_win_wayland_common_resize(Ecore_Win *ee, int location)
{
   if (!ee) return;
   if (!strcmp(ee->driver, "wayland_shm"))
     {
#ifdef BUILD_ECORE_WIN_WAYLAND_SHM
        _ecore_win_wayland_shm_resize(ee, location);
#endif
     }
   else if (!strcmp(ee->driver, "wayland_egl"))
     {
#ifdef BUILD_ECORE_WIN_WAYLAND_EGL
        _ecore_win_wayland_egl_resize(ee, location);
#endif
     }
}

static void
_ecore_win_wayland_alpha_do(Ecore_Win *ee, int alpha)
{
#if 0
   if (!ee) return;
   if (!strcmp(ee->driver, "wayland_shm"))
     {
#ifdef BUILD_ECORE_WIN_WAYLAND_SHM
        _ecore_win_wayland_shm_alpha_do(ee, alpha);
#endif
     }
#endif   
}

static void
_ecore_win_wayland_transparent_do(Ecore_Win *ee, int transparent)
{
#if 0
   if (!ee) return;
   if (!strcmp(ee->driver, "wayland_shm"))
     {
#ifdef BUILD_ECORE_WIN_WAYLAND_SHM
        _ecore_win_wayland_shm_transparent_do(ee, transparent);
#endif
     }
#endif   
}

static void
_ecore_win_wayland_move(Ecore_Win *ee, int x, int y)
{
   Ecore_Win_Engine_Wl_Data *wdata;

   if (!ee) return;
   if (!strncmp(ee->driver, "wayland", 7))
     {
	wdata = ee->engine.data;
        if (wdata->win)
          ecore_wl_window_move(wdata->win, x, y);
     }
}

static void
_ecore_win_wayland_type_set(Ecore_Win *ee, int type)
{
   Ecore_Win_Engine_Wl_Data *wdata;

   if (!ee) return;
   wdata = ee->engine.data;
   ecore_wl_window_type_set(wdata->win, type);
}

static Ecore_Wl_Window *
_ecore_win_wayland_window_get(const Ecore_Win *ee)
{
   Ecore_Win_Engine_Wl_Data *wdata;

   if (!(!strncmp(ee->driver, "wayland", 7)))
     return NULL;

   wdata = ee->engine.data;
   return wdata->win;
}


#ifdef BUILD_ECORE_WIN_WAYLAND_EGL
static void
_ecore_win_wayland_pre_post_swap_callback_set(const Ecore_Win *ee, void *data, void (*pre_cb) (void *data, Evas *e), void (*post_cb) (void *data, Evas *e))
{
#if 0
   Evas_Engine_Info_Wayland_Egl *einfo;

   if (!(!strcmp(ee->driver, "wayland_egl"))) return;

   if ((einfo = (Evas_Engine_Info_Wayland_Egl *)evas_engine_info_get(ee->evas)))
     {
        einfo->callback.pre_swap = pre_cb;
        einfo->callback.post_swap = post_cb;
        einfo->callback.data = data;
        if (!evas_engine_info_set(ee->evas, (Evas_Engine_Info *)einfo))
          ERR("evas_engine_info_set() for engine '%s' failed.", ee->driver);
     }
#endif   
}
#endif

static void
_ecore_win_wayland_pointer_set(Ecore_Win *ee EINA_UNUSED, int hot_x EINA_UNUSED, int hot_y EINA_UNUSED)
{

}

static void
_ecore_win_wayland_input_rect_set(Ecore_Win *ee, Eina_Rectangle *input_rect)
{
#if 0
   Ecore_Win_Engine_Wl_Data *wdata;

   if (!ee) return;
   wdata = ee->engine.data;
   ecore_wl_window_input_rect_set(wdata->win, input_rect);
#endif   
}

static void
_ecore_win_wayland_input_rect_add(Ecore_Win *ee, Eina_Rectangle *input_rect)
{
#if 0
   Ecore_Win_Engine_Wl_Data *wdata;

   if (!ee) return;
   wdata = ee->engine.data;
   ecore_wl_window_input_rect_add(wdata->win, input_rect);
#endif   
}

static void
_ecore_win_wayland_input_rect_subtract(Ecore_Win *ee, Eina_Rectangle *input_rect)
{
#if 0
   Ecore_Win_Engine_Wl_Data *wdata;

   if (!ee) return;
   wdata = ee->engine.data;
   ecore_wl_window_input_rect_subtract(wdata->win, input_rect);
#endif   
}

static void
_ecore_win_wayland_supported_aux_hints_get(Ecore_Win *ee)
{
#if 0
   Ecore_Win_Engine_Wl_Data *wdata;

   if (!ee) return;
   wdata = ee->engine.data;
   ee->prop.aux_hint.supported_list = ecore_wl_window_aux_hints_supported_get(wdata->win);
#endif   
}

static void
_ecore_win_wayland_aux_hint_add(Ecore_Win *ee, int id, const char *hint, const char *val)
{
#if 0
   Ecore_Win_Engine_Wl_Data *wdata;

   if (!ee) return;
   wdata = ee->engine.data;
   ecore_wl_window_aux_hint_add(wdata->win, id, hint, val);
#endif
}

static void
_ecore_win_wayland_aux_hint_change(Ecore_Win *ee, int id, const char *val)
{
#if 0
   Ecore_Win_Engine_Wl_Data *wdata;

   if (!ee) return;
   wdata = ee->engine.data;
   ecore_wl_window_aux_hint_change(wdata->win, id, val);
#endif   
}

static void
_ecore_win_wayland_aux_hint_del(Ecore_Win *ee, int id)
{
#if 0
   Ecore_Win_Engine_Wl_Data *wdata;

   if (!ee) return;
   wdata = ee->engine.data;
   ecore_wl_window_aux_hint_del(wdata->win, id);
#endif   
}

Ecore_Win_Interface_Wayland *
_ecore_win_wl_interface_new(void)
{
   Ecore_Win_Interface_Wayland *iface;
   iface = calloc(1, sizeof(Ecore_Win_Interface_Wayland));
   if (!iface) return NULL;

   iface->base.name = interface_wl_name;
   iface->base.version = interface_wl_version;

   iface->resize = _ecore_win_wayland_common_resize;
   iface->move = _ecore_win_wayland_move;
   iface->pointer_set = _ecore_win_wayland_pointer_set;
   iface->type_set = _ecore_win_wayland_type_set;
   iface->window_get = _ecore_win_wayland_window_get;
   return iface;
}
