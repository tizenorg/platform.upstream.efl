#include "ecore_win_wayland_private.h"

#ifdef BUILD_ECORE_WIN_WAYLAND_SHM
# include <Evas_Engine_Wayland_Shm.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/mman.h>

#ifdef EAPI
# undef EAPI
#endif

#ifdef _WIN32
# ifdef DLL_EXPORT
#  define EAPI __declspec(dllexport)
# else
#  define EAPI
# endif /* ! DLL_EXPORT */
#else
# ifdef __GNUC__
#  if __GNUC__ >= 4
#   define EAPI __attribute__ ((visibility("default")))
#  else
#   define EAPI
#  endif
# else
#  define EAPI
# endif
#endif /* ! _WIN32 */

/* local function prototypes */
static void _ecore_win_wl_move_resize(Ecore_Win *ee, int x, int y, int w, int h);
static void _ecore_win_wl_show(Ecore_Win *ee);
static void _ecore_win_wl_hide(Ecore_Win *ee);
static void _ecore_win_wl_alpha_set(Ecore_Win *ee, int alpha);
static void _ecore_win_wl_transparent_set(Ecore_Win *ee, int transparent);
static void _ecore_win_wl_rotation_set(Ecore_Win *ee, int rotation, int resize);

static Ecore_Win_Engine_Func _ecore_wl_engine_func =
{
   _ecore_win_wl_common_free,
   _ecore_win_wl_common_callback_resize_set,
   _ecore_win_wl_common_callback_move_set,
   NULL, 
   NULL,
   _ecore_win_wl_common_callback_delete_request_set,
   NULL,
   _ecore_win_wl_common_callback_focus_in_set,
   _ecore_win_wl_common_callback_focus_out_set,
   _ecore_win_wl_common_callback_mouse_in_set,
   _ecore_win_wl_common_callback_mouse_out_set,
   NULL, // sticky_set
   NULL, // unsticky_set
   NULL, // pre_render_set
   NULL, // post_render_set
   _ecore_win_wl_common_move,
   NULL, // managed_move
   _ecore_win_wl_common_resize,
   _ecore_win_wl_move_resize,
   _ecore_win_wl_rotation_set,
   NULL, // shaped_set
   _ecore_win_wl_show,
   _ecore_win_wl_hide,
   _ecore_win_wl_common_raise,
   _ecore_win_wl_common_lower,
   _ecore_win_wl_common_activate,
   _ecore_win_wl_common_title_set,
   _ecore_win_wl_common_name_class_set,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL, // focus set
   _ecore_win_wl_common_iconified_set,
   NULL,
   NULL, // override set
   _ecore_win_wl_common_maximized_set,
   NULL,
   NULL, // func avoid_damage set
   NULL,
   NULL, // func sticky set
   NULL,
   _ecore_win_wl_alpha_set,
   _ecore_win_wl_transparent_set,
   NULL, // func profiles set
   NULL, // func profile set
   NULL, // window group set
   NULL,
   NULL, // urgent set
   NULL, // modal set
   NULL, // demand attention set
   NULL,
   NULL, //_ecore_win_wl_common_render,
   _ecore_win_wl_common_screen_geometry_get,
   _ecore_win_wl_common_screen_dpi_get,
   NULL, // func msg parent send
   NULL, // func msg send

   _ecore_win_wl_common_pointer_xy_get,
   NULL, // pointer_warp

   _ecore_win_wl_common_wm_rot_preferred_rotation_set,
   _ecore_win_wl_common_wm_rot_available_rotations_set,
   _ecore_win_wl_common_wm_rot_manual_rotation_done_set,
   _ecore_win_wl_common_wm_rot_manual_rotation_done,

   NULL  // aux_hints_set
};

/* external variables */

/* external functions */
EAPI Ecore_Win *
ecore_win_wayland_new_internal(const char *disp_name, unsigned int parent, int x, int y, int w, int h, Eina_Bool frame)
{
#if 0
   Ecore_Wl_Window *p = NULL;
   Ecore_Wl_Global *global;
   Eina_Inlist *globals;
   Evas_Engine_Info_Wayland_Shm *einfo;
   Ecore_Win_Engine_Wl_Data *wdata;
   Ecore_Win_Interface_Wayland *iface;
   Ecore_Win *ee;
   int method = 0;
   int fx = 0, fy = 0, fw = 0, fh = 0;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ecore_wl_init(disp_name))
     {
        ERR("Failed to initialize Ecore_Wayland");
        return NULL;
     }

   if (!(globals = ecore_wl_globals_get()))
     {
        ERR("Failed to get wayland globals");
        return NULL;
     }

   if (!(ee = calloc(1, sizeof(Ecore_Win))))
     {
        ERR("Failed to allocate Ecore_Win");
        goto ee_err;
     }
   if (!(wdata = calloc(1, sizeof(Ecore_Win_Engine_Wl_Data))))
     {
    ERR("Failed to allocate Ecore_Win_Engine_Wl_Data");
	free(ee);
	goto ee_err;
     }

   ECORE_MAGIC_SET(ee, ECORE_MAGIC_EVAS);

   _ecore_win_wl_common_init();

   ee->engine.func = (Ecore_Win_Engine_Func *)&_ecore_wl_engine_func;
   ee->engine.data = wdata;

   iface = _ecore_win_wl_interface_new();
   ee->engine.ifaces = eina_list_append(ee->engine.ifaces, iface);

   ee->driver = "wayland";
   if (disp_name) ee->name = strdup(disp_name);

   if (w < 1) w = 1;
   if (h < 1) h = 1;

   ee->x = x;
   ee->y = y;
   ee->w = w;
   ee->h = h;
   ee->req.x = ee->x;
   ee->req.y = ee->y;
   ee->req.w = ee->w;
   ee->req.h = ee->h;
   ee->rotation = 0;
   ee->prop.max.w = 32767;
   ee->prop.max.h = 32767;
   ee->prop.layer = 4;
   ee->prop.request_pos = EINA_FALSE;
   ee->prop.sticky = EINA_FALSE;
   ee->prop.draw_frame = frame;
   ee->prop.withdrawn = EINA_TRUE;
   ee->prop.obscured = EINA_TRUE;
   ee->alpha = EINA_FALSE;

   if (getenv("ECORE_WIN_FORCE_SYNC_RENDER"))
     ee->can_async_render = 0;
   else
     ee->can_async_render = 1;

   /* frame offset and size */
   if (ee->prop.draw_frame)
     {
        fx = 4;
        fy = 18;
        fw = 8;
        fh = 22;
     }

   if (parent)
     {
        p = ecore_wl_window_find(parent);
        ee->alpha = ecore_wl_window_alpha_get(p);
     }

   wdata->parent = p;
   wdata->win = 
     ecore_wl_window_new(p, x, y, w + fw, h + fh, 
                         ECORE_WL_WINDOW_BUFFER_TYPE_SHM);
   ee->prop.window = ecore_wl_window_id_get(wdata->win);

   EINA_INLIST_FOREACH(globals, global)
     {
        if (!strcmp(global->interface, "tizen_policy_ext"))
         {
            ee->prop.wm_rot.supported = 1;
            wdata->wm_rot.supported = 1;
            break;
         }
     }

   ee->evas = evas_new();
   evas_data_attach_set(ee->evas, ee);
   evas_output_method_set(ee->evas, method);
   evas_output_size_set(ee->evas, ee->w + fw, ee->h + fh);
   evas_output_viewport_set(ee->evas, 0, 0, ee->w + fw, ee->h + fh);

   if (ee->can_async_render)
     evas_event_callback_add(ee->evas, EVAS_CALLBACK_RENDER_POST,
                 _ecore_win_wl_common_render_updates, ee);

   evas_event_callback_add(ee->evas, EVAS_CALLBACK_RENDER_PRE,
                 _ecore_win_wl_common_render_pre, ee);

   /* FIXME: This needs to be set based on theme & scale */
   if (ee->prop.draw_frame)
     evas_output_framespace_set(ee->evas, fx, fy, fw, fh);

   if ((einfo = (Evas_Engine_Info_Wayland_Shm *)evas_engine_info_get(ee->evas)))
     {
        einfo->info.wl_disp = ecore_wl_display_get();
        einfo->info.wl_shm = ecore_wl_shm_get();
        einfo->info.destination_alpha = EINA_TRUE;
        einfo->info.rotation = ee->rotation;
        einfo->info.wl_surface = ecore_wl_window_surface_create(wdata->win);
        if (!evas_engine_info_set(ee->evas, (Evas_Engine_Info *)einfo))
          {
             ERR("Failed to set Evas Engine Info for '%s'", ee->driver);
             goto err;
          }
     }
   else 
     {
        ERR("Failed to get Evas Engine Info for '%s'", ee->driver);
        goto err;
     }

   /* ecore_wl_animator_source_set(ECORE_ANIMATOR_SOURCE_CUSTOM); */

   ecore_win_callback_pre_free_set(ee, _ecore_win_wl_common_pre_free);

   if (ee->prop.draw_frame)
     {
        wdata->frame = _ecore_win_wl_common_frame_add(ee->evas);
        _ecore_win_wl_common_frame_border_size_set(wdata->frame, fx, fy, fw, fh);
        evas_object_move(wdata->frame, -fx, -fy);
        evas_object_layer_set(wdata->frame, EVAS_LAYER_MAX - 1);
     }

   ee->engine.func->fn_render = _ecore_win_wl_common_render;

   _ecore_win_register(ee);
   ecore_win_input_event_register(ee);

   ecore_event_window_register(ee->prop.window, ee, ee->evas, 
                               (Ecore_Event_Mouse_Move_Cb)_ecore_win_mouse_move_process,
                               (Ecore_Event_Multi_Move_Cb)_ecore_win_mouse_multi_move_process,
                               (Ecore_Event_Multi_Down_Cb)_ecore_win_mouse_multi_down_process,
                               (Ecore_Event_Multi_Up_Cb)_ecore_win_mouse_multi_up_process);

   return ee;

 err:
   ecore_win_free(ee);
   return NULL;

 ee_err:
   ecore_wl_shutdown();
   return NULL;
#endif      
   return NULL;
}

static void 
_ecore_win_wl_move_resize(Ecore_Win *ee, int x, int y, int w, int h)
{
#if 0
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ee) return;
   if ((ee->x != x) || (ee->y != y))
     _ecore_win_wl_common_move(ee, x, y);
   if ((ee->w != w) || (ee->h != h))
     _ecore_win_wl_common_resize(ee, w, h);
#endif      
}

static void
_ecore_win_wl_rotation_set(Ecore_Win *ee, int rotation, int resize)
{
#if 0
   Evas_Engine_Info_Wayland_Shm *einfo;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (ee->rotation == rotation) return;

   _ecore_win_wl_common_rotation_set(ee, rotation, resize);

   einfo = (Evas_Engine_Info_Wayland_Shm *)evas_engine_info_get(ee->evas);
   if (!einfo) return;

   einfo->info.rotation = rotation;

   if (!evas_engine_info_set(ee->evas, (Evas_Engine_Info *)einfo))
     ERR("evas_engine_info_set() for engine '%s' failed.", ee->driver);
#endif      
}

static void
_ecore_win_wl_show(Ecore_Win *ee)
{
#if 0
   Evas_Engine_Info_Wayland_Shm *einfo;
   Ecore_Win_Engine_Wl_Data *wdata;
   int fw, fh;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if ((!ee) || (ee->visible)) return;

   evas_output_framespace_get(ee->evas, NULL, NULL, &fw, &fh);
   wdata = ee->engine.data;

   if (wdata->win)
     {
        ecore_wl_window_show(wdata->win);
        ecore_wl_window_alpha_set(wdata->win, ee->alpha);

        einfo = (Evas_Engine_Info_Wayland_Shm *)evas_engine_info_get(ee->evas);
        if (einfo)
          {
             struct wl_surface *surf;

             surf = ecore_wl_window_surface_get(wdata->win);
             if ((!einfo->info.wl_surface) || (einfo->info.wl_surface != surf))
               {
                  einfo->info.wl_surface = surf;
                  evas_engine_info_set(ee->evas, (Evas_Engine_Info *)einfo);
                  evas_damage_rectangle_add(ee->evas, 0, 0, ee->w + fw, ee->h + fh);
               }
          }
     }

   if (wdata->frame)
     {
        evas_object_show(wdata->frame);
        evas_object_resize(wdata->frame, ee->w + fw, ee->h + fh);
     }

   ee->prop.withdrawn = EINA_FALSE;
   if (ee->func.fn_state_change) ee->func.fn_state_change(ee);

   if (ee->visible) return;
   ee->visible = 1;
   ee->should_be_visible = 1;
   ee->draw_ok = EINA_TRUE;
   if (ee->func.fn_show) ee->func.fn_show(ee);
#endif      
}

static void 
_ecore_win_wl_hide(Ecore_Win *ee)
{
#if 0
   Evas_Engine_Info_Wayland_Shm *einfo;
   Ecore_Win_Engine_Wl_Data *wdata;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if ((!ee) || (!ee->visible)) return;
   wdata = ee->engine.data;

   evas_sync(ee->evas);

   einfo = (Evas_Engine_Info_Wayland_Shm *)evas_engine_info_get(ee->evas);
   if (einfo)
     {
        einfo->info.wl_surface = NULL;
        evas_engine_info_set(ee->evas, (Evas_Engine_Info *)einfo);
     }

   if (wdata->win) 
     ecore_wl_window_hide(wdata->win);

   if (ee->prop.override)
     {
        ee->prop.withdrawn = EINA_TRUE;
        if (ee->func.fn_state_change) ee->func.fn_state_change(ee);
     }

   if (!ee->visible) return;
   ee->visible = 0;
   ee->should_be_visible = 0;
   ee->draw_ok = EINA_FALSE;

   if (ee->func.fn_hide) ee->func.fn_hide(ee);
#endif      
}

void
_ecore_win_wayland_alpha_do(Ecore_Win *ee, int alpha)
{
#if 0
   Evas_Engine_Info_Wayland_Shm *einfo;
   Ecore_Win_Engine_Wl_Data *wdata;
   int fw, fh;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ee) return;
   if (ee->alpha == alpha) return;
   ee->alpha = alpha;
   wdata = ee->engine.data;

   if (wdata->win) ecore_wl_window_alpha_set(wdata->win, ee->alpha);

   evas_output_framespace_get(ee->evas, NULL, NULL, &fw, &fh);

   if ((einfo = (Evas_Engine_Info_Wayland_Shm *)evas_engine_info_get(ee->evas)))
     {
        einfo->info.destination_alpha = EINA_TRUE;//ee->alpha;
        if (!evas_engine_info_set(ee->evas, (Evas_Engine_Info *)einfo))
          ERR("evas_engine_info_set() for engine '%s' failed.", ee->driver);
        evas_damage_rectangle_add(ee->evas, 0, 0, ee->w + fw, ee->h + fh);
     }
#endif      
}

static void
_ecore_win_wl_alpha_set(Ecore_Win *ee, int alpha)
{
#if 0
   if (ee->in_async_render)
     {
        ee->delayed.alpha = alpha;
        ee->delayed.alpha_changed = EINA_TRUE;
        return;
     }
   _ecore_win_wayland_alpha_do(ee, alpha);
#endif      
}

void
_ecore_win_wayland_transparent_do(Ecore_Win *ee, int transparent)
{
#if 0
   Evas_Engine_Info_Wayland_Shm *einfo;
   Ecore_Win_Engine_Wl_Data *wdata;
   int fw, fh;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ee) return;
   if (ee->transparent == transparent) return;
   ee->transparent = transparent;

   wdata = ee->engine.data;
   if (wdata->win)
     ecore_wl_window_transparent_set(wdata->win, ee->transparent);

   evas_output_framespace_get(ee->evas, NULL, NULL, &fw, &fh);

   if ((einfo = (Evas_Engine_Info_Wayland_Shm *)evas_engine_info_get(ee->evas)))
     {
        einfo->info.destination_alpha = EINA_TRUE;//ee->transparent;
        if (!evas_engine_info_set(ee->evas, (Evas_Engine_Info *)einfo))
          ERR("evas_engine_info_set() for engine '%s' failed.", ee->driver);
        evas_damage_rectangle_add(ee->evas, 0, 0, ee->w + fw, ee->h + fh);
     }
#endif      
}

static void
_ecore_win_wl_transparent_set(Ecore_Win *ee, int transparent)
{
#if 0
   if (ee->in_async_render)
     {
        ee->delayed.transparent = transparent;
        ee->delayed.transparent_changed = EINA_TRUE;
        return;
     }
   _ecore_win_wayland_transparent_do(ee, transparent);
#endif      
}

void 
_ecore_win_wayland_resize(Ecore_Win *ee, int location)
{
#if 0
   Ecore_Win_Engine_Wl_Data *wdata;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ee) return;
   wdata = ee->engine.data;
   if (wdata->win) 
     {
        _ecore_win_wayland_resize_edge_set(ee, location);

        if (ECORE_WIN_PORTRAIT(ee))
          ecore_wl_window_resize(wdata->win, ee->w, ee->h, location);
        else
          ecore_wl_window_resize(wdata->win, ee->w, ee->h, location);
     }
#endif      
}

void 
_ecore_win_wayland_resize_edge_set(Ecore_Win *ee, int edge)
{
#if 0
   Evas_Engine_Info_Wayland_Shm *einfo;

   if ((einfo = (Evas_Engine_Info_Wayland_Shm *)evas_engine_info_get(ee->evas)))
     einfo->info.edges = edge;
#endif      
}

void
_ecore_win_wayland_window_rotate(Ecore_Win *ee, int rotation, int resize)
{
#if 0
   if (!ee) return;
   _ecore_win_wl_rotation_set(ee, rotation, resize);
   if (ee->func.fn_state_change) ee->func.fn_state_change(ee);
#endif      
}

#endif
