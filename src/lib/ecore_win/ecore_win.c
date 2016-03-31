#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef _MSC_VER
# include <unistd.h>
#endif

#if defined(HAVE_SYS_MMAN_H) || defined(HAVE_EVIL)
# include <sys/mman.h>
#endif

#ifdef HAVE_EVIL
# include <Evil.h>
#endif

#include <Ecore.h>
#include "ecore_private.h"
#include <Ecore_Input.h>
#include <Ecore_Input_Evas.h>

#include "Ecore_Win.h"
#include "ecore_win_private.h"
#include "ecore_win_x11.h"
#include "ecore_win_wayland.h"

EAPI int _ecore_win_log_dom = -1;
static int _ecore_win_init_count = 0;
static Ecore_Win *ecore_wines = NULL;

EAPI Ecore_Win *ecore_win_wayland_new(const char *disp_name, unsigned int parent, int x, int y, int w, int h, Eina_Bool frame);


EAPI Ecore_Win_Interface *
_ecore_win_interface_get(const Ecore_Win *ewin, const char *iname)
{
   Eina_List *l;
   Ecore_Win_Interface *i;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ewin, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(iname, NULL);

   EINA_LIST_FOREACH(ewin->engine.ifaces, l, i)
     {
	if (!strcmp(i->name, iname))
	  return i;
     }

   CRI("Ecore_Win %p (engine: %s) does not have interface '%s'",
        ewin, ewin->driver, iname);
   return NULL;
}

EAPI int
ecore_win_init(void)
{
   EINA_LOG_ERR(" ecore_win_init start!!");
   int fd;

   if (++_ecore_win_init_count != 1)
     return _ecore_win_init_count;

   if (!ecore_init())
     goto shutdown_init;

   _ecore_win_log_dom = eina_log_domain_register
     ("ecore_win", ECORE_WIN_DEFAULT_LOG_COLOR);
   if(_ecore_win_log_dom < 0)
     {
        EINA_LOG_ERR("Impossible to create a log domain for Ecore_Win.");
        goto shutdown_ecore;
     }

   _ecore_win_engine_init();

   eina_log_timing(_ecore_win_log_dom,
		   EINA_LOG_STATE_STOP,
		   EINA_LOG_STATE_INIT);

 shutdown_ecore:
   ecore_shutdown();
 shutdown_init:
   return --_ecore_win_init_count;
}

EAPI int
ecore_win_shutdown(void)
{
   if (--_ecore_win_init_count != 0)
     return _ecore_win_init_count;

   eina_log_timing(_ecore_win_log_dom,
		   EINA_LOG_STATE_START,
		   EINA_LOG_STATE_SHUTDOWN);

   _ecore_win_engine_shutdown();


   eina_log_domain_unregister(_ecore_win_log_dom);
   _ecore_win_log_dom = -1;
   ecore_shutdown();

   return _ecore_win_init_count;
}

struct ecore_win_engine {
   const char *name;
   Ecore_Win *(*constructor)(int x, int y, int w, int h, const char *extra_options);
};

/* inline is just to avoid need to ifdef around it */
static inline const char *
_ecore_win_parse_extra_options_str(const char *extra_options, const char *key, char **value)
{
   int len = strlen(key);

   while (extra_options)
     {
        const char *p;

        if (strncmp(extra_options, key, len) != 0)
          {
             extra_options = strchr(extra_options, ';');
             if (extra_options)
               extra_options++;
             continue;
          }

        extra_options += len;
        p = strchr(extra_options, ';');
        if (p)
          {
             len = p - extra_options;
             *value = malloc(len + 1);
             memcpy(*value, extra_options, len);
             (*value)[len] = '\0';
             extra_options = p + 1;
          }
        else
          {
             *value = strdup(extra_options);
             extra_options = NULL;
          }
     }
   return extra_options;
}

/* inline is just to avoid need to ifdef around it */
static inline const char *
_ecore_win_parse_extra_options_uint(const char *extra_options, const char *key, unsigned int *value)
{
   int len = strlen(key);

   while (extra_options)
     {
        const char *p;

        if (strncmp(extra_options, key, len) != 0)
          {
             extra_options = strchr(extra_options, ';');
             if (extra_options)
               extra_options++;
             continue;
          }

        extra_options += len;
        *value = strtol(extra_options, NULL, 0);

        p = strchr(extra_options, ';');
        if (p)
          extra_options = p + 1;
        else
          extra_options = NULL;
     }
   return extra_options;
}

static inline const char *
_ecore_win_parse_extra_options_x(const char *extra_options, char **disp_name, unsigned int *parent)
{
   _ecore_win_parse_extra_options_str(extra_options, "display=", disp_name);
   _ecore_win_parse_extra_options_uint(extra_options, "parent=", parent);
   return extra_options;
   return NULL;
}

static Ecore_Win *
_ecore_win_constructor_x11(int x, int y, int w, int h, const char *extra_options)
{
#if 0
   unsigned int parent = 0;
   char *disp_name = NULL;
   Ecore_Win *ewin;

   _ecore_win_parse_extra_options_x(extra_options, &disp_name, &parent);
   ewin = ecore_win_x11_new(disp_name, parent, x, y, w, h);
   free(disp_name);

   return ewin;
#endif
   return NULL;
}

static Ecore_Win *
_ecore_win_constructor_wayland(int x, int y, int w, int h, const char *extra_options)
{
   char *disp_name = NULL;
   unsigned int frame = 0, parent = 0;
   Ecore_Win *ewin;

   _ecore_win_parse_extra_options_str(extra_options, "display=", &disp_name);
   _ecore_win_parse_extra_options_uint(extra_options, "frame=", &frame);
   _ecore_win_parse_extra_options_uint(extra_options, "parent=", &parent);
   ewin = ecore_win_wayland_new(disp_name, parent, x, y, w, h, frame);
   free(disp_name);

   return ewin;
}


/* note: kewinp sorted by priority, highest first */
static const struct ecore_win_engine _engines[] = {
  /* unix */
  {"x11", _ecore_win_constructor_x11},
  {"wayland", _ecore_win_constructor_wayland},
  {NULL, NULL}
};

EAPI Eina_List *
ecore_win_engines_get(void)
{
   return eina_list_clone(_ecore_win_available_engines_get());
}

EAPI void
ecore_win_engines_free(Eina_List *engines)
{
   eina_list_free(engines);
}

EAPI Ecore_Win *
ecore_win_new(const char *engine_name, int x, int y, int w, int h, const char *extra_options)
{
   EINA_LOG_ERR(" ecore_win_new start!!");
   const struct ecore_win_engine *itr;

   if (!engine_name)
     {
        engine_name = getenv("ECORE_WIN_ENGINE");
        if (engine_name)
           EINA_LOG_ERR("no engine_name provided, using ECORE_WIN_ENGINE='%s'",
              engine_name);
     }

   for (itr = _engines; itr->name; itr++)
     if (strcmp(itr->name, engine_name) == 0)
       {
           EINA_LOG_ERR("using engine '%s', extra_options=%s",
              engine_name, extra_options ? extra_options : "(null)");
          return itr->constructor(x, y, w, h, extra_options);
       }

   EINA_LOG_ERR("unknown engine '%s'", engine_name);
   return NULL;
}


EAPI void
ecore_win_free(Ecore_Win *ewin)
{
   if (!ewin) return;
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_free");
        return;
     }
   _ecore_win_free(ewin);
   return;
}

#define IFC(_ee, _fn)  if (_ee->engine.func->_fn) {_ee->engine.func->_fn
#define IFE            return;}

EAPI void
ecore_win_callback_resize_set(Ecore_Win *ewin, Ecore_Win_Event_Cb func)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_callback_resize_set");
        return;
     }
   IFC(ewin, fn_callback_resize_set) (ewin, func);
   IFE;
   ewin->func.fn_resize = func;
}

EAPI void
ecore_win_callback_move_set(Ecore_Win *ewin, Ecore_Win_Event_Cb func)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_callback_move_set");
        return;
     }
   IFC(ewin, fn_callback_move_set) (ewin, func);
   IFE;
   ewin->func.fn_move = func;
}

EAPI void
ecore_win_callback_show_set(Ecore_Win *ewin, Ecore_Win_Event_Cb func)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_callback_show_set");
        return;
     }
   IFC(ewin, fn_callback_show_set) (ewin, func);
   IFE;
   ewin->func.fn_show = func;
}

EAPI void
ecore_win_callback_hide_set(Ecore_Win *ewin, Ecore_Win_Event_Cb func)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_callback_hide_set");
        return;
     }
   IFC(ewin, fn_callback_hide_set) (ewin, func);
   IFE;
   ewin->func.fn_hide = func;
}

EAPI void
ecore_win_callback_delete_request_set(Ecore_Win *ewin, Ecore_Win_Event_Cb func)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_callback_delete_request_set");
        return;
     }
   IFC(ewin, fn_callback_delete_request_set) (ewin, func);
   IFE;
   ewin->func.fn_delete_request = func;
}

EAPI void
ecore_win_callback_destroy_set(Ecore_Win *ewin, Ecore_Win_Event_Cb func)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_callback_destroy_set");
        return;
     }
   IFC(ewin, fn_callback_destroy_set) (ewin, func);
   IFE;
   ewin->func.fn_destroy = func;
}

EAPI void
ecore_win_callback_focus_in_set(Ecore_Win *ewin, Ecore_Win_Event_Cb func)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_callback_focus_in_set");
        return;
     }
   IFC(ewin, fn_callback_focus_in_set) (ewin, func);
   IFE;
   ewin->func.fn_focus_in = func;
}

EAPI void
ecore_win_callback_focus_out_set(Ecore_Win *ewin, Ecore_Win_Event_Cb func)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_callback_focus_out_set");
        return;
     }
   IFC(ewin, fn_callback_focus_out_set) (ewin, func);
   IFE;
   ewin->func.fn_focus_out = func;
}

EAPI void
ecore_win_callback_mouse_in_set(Ecore_Win *ewin, Ecore_Win_Event_Cb func)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_callback_mouse_in_set");
        return;
     }
   IFC(ewin, fn_callback_mouse_in_set) (ewin, func);
   IFE;
   ewin->func.fn_mouse_in = func;
}

EAPI void
ecore_win_callback_mouse_out_set(Ecore_Win *ewin, Ecore_Win_Event_Cb func)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_callback_mouse_out_set");
        return;
     }
   IFC(ewin, fn_callback_mouse_out_set) (ewin, func);
   IFE;
   ewin->func.fn_mouse_out = func;
}

EAPI void
ecore_win_callback_state_change_set(Ecore_Win *ewin, Ecore_Win_Event_Cb func)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_callback_state_change_set");
        return;
     }
   ewin->func.fn_state_change = func;
}

EAPI void
ecore_win_move(Ecore_Win *ewin, int x, int y)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_move");
        return;
     }
   if (ewin->prop.fullscreen) return;
   IFC(ewin, fn_move) (ewin, x, y);
   IFE;
}

EAPI void
ecore_win_resize(Ecore_Win *ewin, int w, int h)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_resize");
        return;
     }
   if (ewin->prop.fullscreen) return;
   if (w < 1) w = 1;
   if (h < 1) h = 1;
   if (ECORE_WIN_PORTRAIT(ewin))
     {
        IFC(ewin, fn_resize) (ewin, w, h);
        IFE;
     }
   else
     {
        IFC(ewin, fn_resize) (ewin, h, w);
        IFE;
     }
}

EAPI void
ecore_win_move_resize(Ecore_Win *ewin, int x, int y, int w, int h)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_move_resize");
        return;
     }
   if (ewin->prop.fullscreen) return;
   if (w < 1) w = 1;
   if (h < 1) h = 1;
   if (ECORE_WIN_PORTRAIT(ewin))
     {
        IFC(ewin, fn_move_resize) (ewin, x, y, w, h);
        IFE;
     }
   else
     {
        IFC(ewin, fn_move_resize) (ewin, x, y, h, w);
        IFE;
     }
}

EAPI void
ecore_win_geometry_get(const Ecore_Win *ewin, int *x, int *y, int *w, int *h)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_geometry_get");
        return;
     }
   if (ECORE_WIN_PORTRAIT(ewin))
     {
        if (x) *x = ewin->x;
        if (y) *y = ewin->y;
        if (w) *w = ewin->w;
        if (h) *h = ewin->h;
     }
   else
     {
        if (x) *x = ewin->x;
        if (y) *y = ewin->y;
        if (w) *w = ewin->h;
        if (h) *h = ewin->w;
     }
}

EAPI void
ecore_win_request_geometry_get(const Ecore_Win *ewin, int *x, int *y, int *w, int *h)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_request_geometry_get");
        return;
     }
   if (ECORE_WIN_PORTRAIT(ewin))
     {
        if (x) *x = ewin->req.x;
        if (y) *y = ewin->req.y;
        if (w) *w = ewin->req.w;
        if (h) *h = ewin->req.h;
     }
   else
     {
        if (x) *x = ewin->req.x;
        if (y) *y = ewin->req.y;
        if (w) *w = ewin->req.h;
        if (h) *h = ewin->req.w;
     }
}

EAPI void
ecore_win_shaped_set(Ecore_Win *ewin, Eina_Bool shaped)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_shaped_set");
        return;
     }
   IFC(ewin, fn_shaped_set) (ewin, shaped);
   IFE;
}

EAPI Eina_Bool
ecore_win_shaped_get(const Ecore_Win *ewin)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_shaped_get");
        return EINA_FALSE;
     }
   return ewin->shaped ? EINA_TRUE : EINA_FALSE;
}

EAPI void
ecore_win_alpha_set(Ecore_Win *ewin, Eina_Bool alpha)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_alpha_set");
        return;
     }
   IFC(ewin, fn_alpha_set) (ewin, alpha);
   IFE;
}

EAPI Eina_Bool
ecore_win_alpha_get(const Ecore_Win *ewin)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_alpha_get");
        return EINA_FALSE;
     }
   return ewin->alpha ? EINA_TRUE : EINA_FALSE;
}

EAPI void
ecore_win_transparent_set(Ecore_Win *ewin, Eina_Bool transparent)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_transparent_set");
        return;
     }
   IFC(ewin, fn_transparent_set) (ewin, transparent);
   IFE;
}

EAPI Eina_Bool
ecore_win_transparent_get(const Ecore_Win *ewin)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_transparent_get");
        return EINA_FALSE;
     }
   return ewin->transparent ? EINA_TRUE : 0;
}

EAPI void
ecore_win_show(Ecore_Win *ewin)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_show");
        return;
     }
   IFC(ewin, fn_show) (ewin);
   IFE;
}

EAPI void
ecore_win_hide(Ecore_Win *ewin)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_hide");
        return;
     }
   IFC(ewin, fn_hide) (ewin);
   IFE;
}

EAPI void
ecore_win_raise(Ecore_Win *ewin)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_raise");
        return;
     }
   IFC(ewin, fn_raise) (ewin);
   IFE;
}

EAPI void
ecore_win_lower(Ecore_Win *ewin)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_lower");
        return;
     }
   IFC(ewin, fn_lower) (ewin);
   IFE;
}

EAPI void
ecore_win_activate(Ecore_Win *ewin)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_activate");
        return;
     }
   IFC(ewin, fn_activate) (ewin);
   IFE;
}

EAPI void
ecore_win_focus_set(Ecore_Win *ewin, Eina_Bool on)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_focus_set");
        return;
     }
   IFC(ewin, fn_focus_set) (ewin, on);
   IFE;
}

EAPI Eina_Bool
ecore_win_focus_get(const Ecore_Win *ewin)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_focus_get");
        return EINA_FALSE;
     }
   return ewin->prop.focused ? EINA_TRUE : EINA_FALSE;
}

EAPI void
ecore_win_iconified_set(Ecore_Win *ewin, Eina_Bool on)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_iconified_set");
        return;
     }
   IFC(ewin, fn_iconified_set) (ewin, on);
   IFE;
}

EAPI Eina_Bool
ecore_win_iconified_get(const Ecore_Win *ewin)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_iconified_get");
        return EINA_FALSE;
     }
   return ewin->prop.iconified ? EINA_TRUE : EINA_FALSE;
}

EAPI void
ecore_win_maximized_set(Ecore_Win *ewin, Eina_Bool on)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_maximized_set");
        return;
     }
   IFC(ewin, fn_maximized_set) (ewin, on);
   IFE;
}

EAPI Eina_Bool
ecore_win_maximized_get(const Ecore_Win *ewin)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_maximized_get");
        return EINA_FALSE;
     }
   return ewin->prop.maximized ? EINA_TRUE : EINA_FALSE;
}

EAPI Eina_Bool
ecore_win_wm_rotation_supported_get(const Ecore_Win *ewin)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_wm_rotation_supported_get");
        return EINA_FALSE;
     }
   return ewin->prop.wm_rot.supported;
}

EAPI void
ecore_win_wm_rotation_preferred_rotation_set(Ecore_Win *ewin, int rotation)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_wm_rotation_preferred_rotation_set");
        return;
     }
   if (rotation != -1)
     {
        if (ewin->prop.wm_rot.available_rots)
          {
             Eina_Bool found = EINA_FALSE;
             unsigned int i;
             for (i = 0; i < ewin->prop.wm_rot.count; i++)
               {
                  if (ewin->prop.wm_rot.available_rots[i] == rotation)
                    {
                       found = EINA_TRUE;
                       break;
                    }
               }
             if (!found) return;
          }
     }
   IFC(ewin, fn_wm_rot_preferred_rotation_set) (ewin, rotation);
   IFE;
}

EAPI int
ecore_win_wm_rotation_preferred_rotation_get(const Ecore_Win *ewin)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_wm_rotation_preferred_rotation_get");
        return -1;
     }
   return ewin->prop.wm_rot.preferred_rot;
}

EAPI void
ecore_win_wm_rotation_available_rotations_set(Ecore_Win *ewin, const int *rotations, unsigned int count)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_wm_rotation_available_rotations_set");
        return;
     }
   IFC(ewin, fn_wm_rot_available_rotations_set) (ewin, rotations, count);
   IFE;
}

EAPI Eina_Bool
ecore_win_wm_rotation_available_rotations_get(const Ecore_Win *ewin, int **rotations, unsigned int *count)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_wm_rotation_available_rotations_get");
        return EINA_FALSE;
     }
   if ((!rotations) || (!count))
     return EINA_FALSE;

   if ((!ewin->prop.wm_rot.available_rots) || (ewin->prop.wm_rot.count == 0))
     return EINA_FALSE;

   *rotations = calloc(ewin->prop.wm_rot.count, sizeof(int));
   if (!*rotations) return EINA_FALSE;

   memcpy(*rotations, ewin->prop.wm_rot.available_rots, sizeof(int) * ewin->prop.wm_rot.count);
   *count = ewin->prop.wm_rot.count;
}

EAPI void
ecore_win_modal_set(Ecore_Win *ewin, Eina_Bool on)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "XXX");
        return;
     }

   IFC(ewin, fn_modal_set) (ewin, on);
   IFE;
}

EAPI Eina_Bool
ecore_win_modal_get(const Ecore_Win *ewin)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "XXX");
        return EINA_FALSE;
     }
   return ewin->prop.modal ? EINA_TRUE : EINA_FALSE;
}

EAPI Ecore_Window
ecore_win_window_get(const Ecore_Win *ewin)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_window_get");
        return 0;
     }

   return ewin->prop.window;
}


EAPI Ecore_Surface *
ecore_win_surface_get(const Ecore_Win *ewin)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_surface_get");
        return 0;
     }

   return ewin->prop.wl_surface;
}

EAPI Ecore_Display *
ecore_win_display_get(const Ecore_Win *ewin)
{
   if (!ECORE_MAGIC_CHECK(ewin, ECORE_MAGIC_WIN))
     {
        ECORE_MAGIC_FAIL(ewin, ECORE_MAGIC_WIN,
                         "ecore_win_display_get");
        return 0;
     }

   return ewin->prop.wl_disp;
}

EAPI void
_ecore_win_register(Ecore_Win *ewin)
{
   ewin->registered = 1;
   ecore_wines = (Ecore_Win *)eina_inlist_prepend
     (EINA_INLIST_GET(ecore_wines), EINA_INLIST_GET(ewin));

}

EAPI void
_ecore_win_ref(Ecore_Win *ewin)
{
   ewin->refcount++;
}

EAPI void
_ecore_win_unref(Ecore_Win *ewin)
{
   ewin->refcount--;
   if (ewin->refcount == 0)
     {
        if (ewin->deleted) _ecore_win_free(ewin);
     }
   else if (ewin->refcount < -1)
     ERR("Ecore_Win %p->refcount=%d < 0", ewin, ewin->refcount);
}

EAPI void
_ecore_win_free(Ecore_Win *ewin)
{
   Ecore_Win_Interface *iface;

   ewin->deleted = EINA_TRUE;
   if (ewin->refcount > 0) return;

   if (ewin->func.fn_pre_free) ewin->func.fn_pre_free(ewin);
   while (ewin->sub_ecore_win)
     {
        _ecore_win_free(ewin->sub_ecore_win->data);
     }
   if (ewin->data) eina_hash_free(ewin->data);
   ewin->data = NULL;
   if (ewin->name) free(ewin->name);
   ewin->name = NULL;
   if (ewin->prop.title) free(ewin->prop.title);
   ewin->prop.title = NULL;
   if (ewin->prop.name) free(ewin->prop.name);
   ewin->prop.name = NULL;
   if (ewin->prop.clas) free(ewin->prop.clas);
   ewin->prop.clas = NULL;
   if (ewin->prop.wm_rot.available_rots) free(ewin->prop.wm_rot.available_rots);
   ewin->prop.wm_rot.available_rots = NULL;
   if (ewin->prop.wm_rot.manual_mode.timer)
     ecore_timer_del(ewin->prop.wm_rot.manual_mode.timer);
   ewin->prop.wm_rot.manual_mode.timer = NULL;
   ECORE_MAGIC_SET(ewin, ECORE_MAGIC_NONE);
   ewin->driver = NULL;
   if (ewin->engine.func->fn_free) ewin->engine.func->fn_free(ewin);
   if (ewin->registered)
     {
        ecore_wines = (Ecore_Win *)eina_inlist_remove
          (EINA_INLIST_GET(ecore_wines), EINA_INLIST_GET(ewin));
     }

   EINA_LIST_FREE(ewin->engine.ifaces, iface)
     free(iface);

   ewin->engine.ifaces = NULL;
   free(ewin);
}

EAPI Ecore_Win *
ecore_win_x11_new(const char *disp_name, Ecore_X_Window parent, int x, int y, int w, int h)
{
#if 0
   Ecore_Win *(*new)(const char *, Ecore_X_Window, int, int, int, int);
   Eina_Module *m = _ecore_win_engine_load("x");
   EINA_SAFETY_ON_NULL_RETURN_VAL(m, NULL);

   new = eina_module_symbol_get(m, "ecore_win_x11_new_internal");
   EINA_SAFETY_ON_NULL_RETURN_VAL(new, NULL);

   return new(disp_name, parent, x, y, w, h);
   #endif 
   return NULL;
}

EAPI Ecore_X_Window
ecore_win_x11_window_get(const Ecore_Win *ewin)
{
#if 0
   Ecore_Win_Interface_Software_X11 *iface;
   iface = (Ecore_Win_Interface_Software_X11 *)_ecore_win_interface_get(ewin, "software_x11");
   EINA_SAFETY_ON_NULL_RETURN_VAL(iface, 0);

   return iface->window_get(ewin);
   #endif 
   return 0;
}

EAPI Ecore_Win *
ecore_win_wayland_new(const char *disp_name, unsigned int parent,
			   int x, int y, int w, int h, Eina_Bool frame)
{
   EINA_LOG_ERR("ecore_win_wayland_new ");
   Ecore_Win *(*new)(const char *, unsigned int, int, int, int, int, Eina_Bool);
   Eina_Module *m = _ecore_win_engine_load("wayland");
   EINA_SAFETY_ON_NULL_RETURN_VAL(m, NULL);

   new = eina_module_symbol_get(m, "ecore_win_wayland_new_internal");
   EINA_SAFETY_ON_NULL_RETURN_VAL(new, NULL);

   return new(disp_name, parent, x, y, w, h, frame);
}

EAPI void
ecore_win_wayland_resize(Ecore_Win *ewin, int location)
{
   Ecore_Win_Interface_Wayland *iface;
   iface = (Ecore_Win_Interface_Wayland *)_ecore_win_interface_get(ewin, "wayland");
   EINA_SAFETY_ON_NULL_RETURN(iface);

   iface->resize(ewin, location);
}

EAPI void
ecore_win_wayland_move(Ecore_Win *ewin, int x, int y)
{
   Ecore_Win_Interface_Wayland *iface;
   iface = (Ecore_Win_Interface_Wayland *)_ecore_win_interface_get(ewin, "wayland");
   EINA_SAFETY_ON_NULL_RETURN(iface);

   iface->move(ewin, x, y);
}

EAPI void
ecore_win_wayland_pointer_set(Ecore_Win *ewin, int hot_x, int hot_y)
{
   Ecore_Win_Interface_Wayland *iface;
   iface = (Ecore_Win_Interface_Wayland *)_ecore_win_interface_get(ewin, "wayland");
   EINA_SAFETY_ON_NULL_RETURN(iface);

   iface->pointer_set(ewin, hot_x, hot_y);
}

EAPI void
ecore_win_wayland_type_set(Ecore_Win *ewin, int type)
{
   Ecore_Win_Interface_Wayland *iface;
   iface = (Ecore_Win_Interface_Wayland *)_ecore_win_interface_get(ewin, "wayland");
   EINA_SAFETY_ON_NULL_RETURN(iface);

   iface->type_set(ewin, type);
}

EAPI Ecore_Wl_Window *
ecore_win_wayland_window_get(const Ecore_Win *ewin)
{
   Ecore_Win_Interface_Wayland *iface;
   iface = (Ecore_Win_Interface_Wayland *)_ecore_win_interface_get(ewin, "wayland");
   EINA_SAFETY_ON_NULL_RETURN_VAL(iface, NULL);

   return iface->window_get(ewin);
}
