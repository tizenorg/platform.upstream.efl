#include "ecore_win_wayland_private.h"

//# include <Evas_Engine_Wayland_Shm.h>
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
static void _ecore_win_wl_move_resize(Ecore_Win *ewin, int x, int y, int w, int h);
static void _ecore_win_wl_show(Ecore_Win *ewin);
static void _ecore_win_wl_hide(Ecore_Win *ewin);
static void _ecore_win_wl_alpha_set(Ecore_Win *ewin, int alpha);
static void _ecore_win_wl_transparent_set(Ecore_Win *ewin, int transparent);
static void _ecore_win_wl_rotation_set(Ecore_Win *ewin, int rotation, int resize);

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
   EINA_LOG_ERR("ecore_win_wayland_new_internal start!!!");
   Ecore_Wl_Window *p = NULL;
   Ecore_Wl_Global *global;
   Eina_Inlist *globals;
//   Evas_Engine_Info_Wayland_Shm *einfo;
   Ecore_Win_Engine_Wl_Data *wdata;
   Ecore_Win_Interface_Wayland *iface;
   Ecore_Win *ewin;
   int method = 0;
   int fx = 0, fy = 0, fw = 0, fh = 0;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ecore_wl_init(disp_name))
     {
        EINA_LOG_ERR("Failed to initialize Ecore_Wayland");
        return NULL;
     }

   if (!(globals = ecore_wl_globals_get()))
     {
        EINA_LOG_ERR("Failed to get wayland globals");
        return NULL;
     }

   if (!(ewin = calloc(1, sizeof(Ecore_Win))))
     {
        EINA_LOG_ERR("Failed to allocate Ecore_Win");
        goto ewin_err;
     }
   if (!(wdata = calloc(1, sizeof(Ecore_Win_Engine_Wl_Data))))
     {
    EINA_LOG_ERR("Failed to allocate Ecore_Win_Engine_Wl_Data");
    free(ewin);
    goto ewin_err;
     }

   ECORE_MAGIC_SET(ewin, ECORE_MAGIC_WIN);

   _ecore_win_wl_common_init();

   ewin->engine.func = (Ecore_Win_Engine_Func *)&_ecore_wl_engine_func;
   ewin->engine.data = wdata;

   iface = _ecore_win_wl_interface_new();
   ewin->engine.ifaces = eina_list_append(ewin->engine.ifaces, iface);

   ewin->driver = "wayland";
   if (disp_name) ewin->name = strdup(disp_name);

   if (w < 1) w = 1;
   if (h < 1) h = 1;

   ewin->x = x;
   ewin->y = y;
   ewin->w = w;
   ewin->h = h;
   ewin->req.x = ewin->x;
   ewin->req.y = ewin->y;
   ewin->req.w = ewin->w;
   ewin->req.h = ewin->h;
   ewin->rotation = 0;
   ewin->prop.max.w = 32767;
   ewin->prop.max.h = 32767;
   ewin->prop.layer = 4;
   ewin->prop.request_pos = EINA_FALSE;
   ewin->prop.sticky = EINA_FALSE;
   ewin->prop.draw_frame = frame;
   ewin->prop.withdrawn = EINA_TRUE;
   ewin->prop.obscured = EINA_TRUE;
   ewin->alpha = EINA_FALSE;

   /* frame offset and size */
   if (ewin->prop.draw_frame)
     {
        fx = 4;
        fy = 18;
        fw = 8;
        fh = 22;
     }

   if (parent)
     {
        p = ecore_wl_window_find(parent);
        ewin->alpha = ecore_wl_window_alpha_get(p);
     }

   wdata->parent = p;
   wdata->win = ecore_wl_window_new(p, x, y, w + fw, h + fh, 
                         ECORE_WL_WINDOW_BUFFER_TYPE_SHM);
   ewin->prop.window = ecore_wl_window_id_get(wdata->win);
   ewin->prop.wl_disp = ecore_wl_display_get();
   ewin->prop.wl_surface = ecore_wl_window_surface_create(wdata->win);
   

   EINA_INLIST_FOREACH(globals, global)
     {
        if (!strcmp(global->interface, "tizen_policy_ext"))
         {
            ewin->prop.wm_rot.supported = 1;
            wdata->wm_rot.supported = 1;
            break;
         }
     }

//   if ((einfo = (Evas_Engine_Info_Wayland_Shm *)evas_engine_info_get(ewin->evas)))
//     {
//        einfo->info.wl_disp = ecore_wl_display_get();
//        einfo->info.wl_shm = ecore_wl_shm_get();
//        einfo->info.destination_alpha = EINA_TRUE;
//        einfo->info.rotation = ewin->rotation;
//        einfo->info.wl_surface = ecore_wl_window_surface_create(wdata->win);
//        if (!evas_engine_info_set(ewin->evas, (Evas_Engine_Info *)einfo))
//          {
//             EINA_LOG_ERR("Failed to set Evas Engine Info for '%s'", ewin->driver);
//             goto err;
//          }
//     }
//   else
//     {
//        EINA_LOG_ERR("Failed to get Evas Engine Info for '%s'", ewin->driver);
//        goto err;
//     }

   /* ecore_wl_animator_source_set(ECORE_ANIMATOR_SOURCE_CUSTOM); */

   _ecore_win_register(ewin);
   EINA_LOG_ERR("ecore_win_wayland_new_internal end!!!");
   return ewin;

 err:
   ecore_win_free(ewin);
   return NULL;

 ewin_err:
   ecore_wl_shutdown();
   return NULL;
}

static void 
_ecore_win_wl_move_resize(Ecore_Win *ewin, int x, int y, int w, int h)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ewin) return;
   if ((ewin->x != x) || (ewin->y != y))
     _ecore_win_wl_common_move(ewin, x, y);
   if ((ewin->w != w) || (ewin->h != h))
     _ecore_win_wl_common_resize(ewin, w, h);
}

static void
_ecore_win_wl_rotation_set(Ecore_Win *ewin, int rotation, int resize)
{

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (ewin->rotation == rotation) return;

   _ecore_win_wl_common_rotation_set(ewin, rotation, resize);

//   Evas_Engine_Info_Wayland_Shm *einfo;
//   einfo = (Evas_Engine_Info_Wayland_Shm *)evas_engine_info_get(ewin->evas);
//   if (!einfo) return;
//
//   einfo->info.rotation = rotation;
//
//   if (!evas_engine_info_set(ewin->evas, (Evas_Engine_Info *)einfo))
//     EINA_LOG_ERR("evas_engine_info_set() for engine '%s' failed.", ewin->driver);
}

static void
_ecore_win_wl_show(Ecore_Win *ewin)
{
   EINA_LOG_ERR("_ecore_win_wl_show start!!!!");
//   Evas_Engine_Info_Wayland_Shm *einfo;
   Ecore_Win_Engine_Wl_Data *wdata;
//   int fw, fh;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if ((!ewin) || (ewin->visible)) return;

//   evas_output_framespace_get(ewin->evas, NULL, NULL, &fw, &fh);
   wdata = ewin->engine.data;

   if (wdata->win)
     {
         EINA_LOG_ERR("_ecore_win_wl_show start----1");
        ecore_wl_window_show(wdata->win);
        ecore_wl_window_alpha_set(wdata->win, ewin->alpha);

//        einfo = (Evas_Engine_Info_Wayland_Shm *)evas_engine_info_get(ewin->evas);
//        if (einfo)
//          {
//             struct wl_surface *surf;
//
//             surf = ecore_wl_window_surface_get(wdata->win);
//             if ((!einfo->info.wl_surface) || (einfo->info.wl_surface != surf))
//               {
//                  einfo->info.wl_surface = surf;
//                  evas_engine_info_set(ewin->evas, (Evas_Engine_Info *)einfo);
//                  evas_damage_rectangle_add(ewin->evas, 0, 0, ewin->w + fw, ewin->h + fh);
//               }
//          }
     }

//   if (wdata->frame)
//     {
//        evas_object_show(wdata->frame);
//        evas_object_resize(wdata->frame, ewin->w + fw, ewin->h + fh);
//     }

   ewin->prop.withdrawn = EINA_FALSE;
   if (ewin->func.fn_state_change) ewin->func.fn_state_change(ewin);

   if (ewin->visible) return;
   ewin->visible = 1;
   ewin->should_be_visible = 1;
   ewin->draw_ok = EINA_TRUE;
   if (ewin->func.fn_show) ewin->func.fn_show(ewin);
   EINA_LOG_ERR("_ecore_win_wl_show start end");
}

static void 
_ecore_win_wl_hide(Ecore_Win *ewin)
{
//   Evas_Engine_Info_Wayland_Shm *einfo;
   Ecore_Win_Engine_Wl_Data *wdata;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if ((!ewin) || (!ewin->visible)) return;
   wdata = ewin->engine.data;

//   evas_sync(ewin->evas);
//   einfo = (Evas_Engine_Info_Wayland_Shm *)evas_engine_info_get(ewin->evas);
//   if (einfo)
//     {
//        einfo->info.wl_surface = NULL;
//        evas_engine_info_set(ewin->evas, (Evas_Engine_Info *)einfo);
//     }

   if (wdata->win) 
     ecore_wl_window_hide(wdata->win);

   if (ewin->prop.override)
     {
        ewin->prop.withdrawn = EINA_TRUE;
        if (ewin->func.fn_state_change) ewin->func.fn_state_change(ewin);
     }

   if (!ewin->visible) return;
   ewin->visible = 0;
   ewin->should_be_visible = 0;
   ewin->draw_ok = EINA_FALSE;

   if (ewin->func.fn_hide) ewin->func.fn_hide(ewin);
}

void
_ecore_win_wayland_alpha_do(Ecore_Win *ewin, int alpha)
{
//   Evas_Engine_Info_Wayland_Shm *einfo;
   Ecore_Win_Engine_Wl_Data *wdata;
//   int fw, fh;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ewin) return;
   if (ewin->alpha == alpha) return;
   ewin->alpha = alpha;
   wdata = ewin->engine.data;

   if (wdata->win) ecore_wl_window_alpha_set(wdata->win, ewin->alpha);

//   evas_output_framespace_get(ewin->evas, NULL, NULL, &fw, &fh);
//
//   if ((einfo = (Evas_Engine_Info_Wayland_Shm *)evas_engine_info_get(ewin->evas)))
//     {
//        einfo->info.destination_alpha = EINA_TRUE;//ewin->alpha;
//        if (!evas_engine_info_set(ewin->evas, (Evas_Engine_Info *)einfo))
//          EINA_LOG_ERR("evas_engine_info_set() for engine '%s' failed.", ewin->driver);
//        evas_damage_rectangle_add(ewin->evas, 0, 0, ewin->w + fw, ewin->h + fh);
//     }
}

static void
_ecore_win_wl_alpha_set(Ecore_Win *ewin, int alpha)
{
   if (ewin->in_async_render)
     {
        ewin->delayed.alpha = alpha;
        ewin->delayed.alpha_changed = EINA_TRUE;
        return;
     }
   _ecore_win_wayland_alpha_do(ewin, alpha);
}

void
_ecore_win_wayland_transparent_do(Ecore_Win *ewin, int transparent)
{
//   Evas_Engine_Info_Wayland_Shm *einfo;
   Ecore_Win_Engine_Wl_Data *wdata;
//   int fw, fh;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ewin) return;
   if (ewin->transparent == transparent) return;
   ewin->transparent = transparent;

   wdata = ewin->engine.data;
   if (wdata->win)
     ecore_wl_window_transparent_set(wdata->win, ewin->transparent);

//   evas_output_framespace_get(ewin->evas, NULL, NULL, &fw, &fh);
//
//   if ((einfo = (Evas_Engine_Info_Wayland_Shm *)evas_engine_info_get(ewin->evas)))
//     {
//        einfo->info.destination_alpha = EINA_TRUE;//ewin->transparent;
//        if (!evas_engine_info_set(ewin->evas, (Evas_Engine_Info *)einfo))
//          EINA_LOG_ERR("evas_engine_info_set() for engine '%s' failed.", ewin->driver);
//        evas_damage_rectangle_add(ewin->evas, 0, 0, ewin->w + fw, ewin->h + fh);
//     }
}

static void
_ecore_win_wl_transparent_set(Ecore_Win *ewin, int transparent)
{
   if (ewin->in_async_render)
     {
        ewin->delayed.transparent = transparent;
        ewin->delayed.transparent_changed = EINA_TRUE;
        return;
     }
   _ecore_win_wayland_transparent_do(ewin, transparent);
}

void 
_ecore_win_wayland_resize(Ecore_Win *ewin, int location)
{
   Ecore_Win_Engine_Wl_Data *wdata;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!ewin) return;
   wdata = ewin->engine.data;
   if (wdata->win) 
     {
        _ecore_win_wayland_resize_edge_set(ewin, location);

        if (ECORE_WIN_PORTRAIT(ewin))
          ecore_wl_window_resize(wdata->win, ewin->w, ewin->h, location);
        else
          ecore_wl_window_resize(wdata->win, ewin->w, ewin->h, location);
     }
}

void 
_ecore_win_wayland_resize_edge_set(Ecore_Win *ewin, int edge)
{
#if 0
   Evas_Engine_Info_Wayland_Shm *einfo;

   if ((einfo = (Evas_Engine_Info_Wayland_Shm *)evas_engine_info_get(ewin->evas)))
     einfo->info.edges = edge;
#endif      
}

void
_ecore_win_wayland_window_rotate(Ecore_Win *ewin, int rotation, int resize)
{
   if (!ewin) return;
   _ecore_win_wl_rotation_set(ewin, rotation, resize);
   if (ewin->func.fn_state_change) ewin->func.fn_state_change(ewin);
}

