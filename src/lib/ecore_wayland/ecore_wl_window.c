#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "ecore_wl_private.h"
#include "xdg-shell-client-protocol.h"
#include "session-recovery-client-protocol.h"
#include <tizen-extension-client-protocol.h>

/* local function prototypes */
static void _ecore_wl_window_cb_ping(void *data EINA_UNUSED, struct wl_shell_surface *shell_surface, unsigned int serial);
static void _ecore_wl_window_cb_configure(void *data, struct wl_shell_surface *shell_surface EINA_UNUSED, unsigned int edges, int w, int h);
static void _ecore_wl_window_cb_popup_done(void *data, struct wl_shell_surface *shell_surface EINA_UNUSED);
static void _ecore_wl_window_configure_send(Ecore_Wl_Window *win, int w, int h, int edges);
static void _ecore_wl_window_show_send(Ecore_Wl_Window *win);
static void _ecore_wl_window_hide_send(Ecore_Wl_Window *win);
static char *_ecore_wl_window_id_str_get(unsigned int win_id);
static void _ecore_xdg_handle_surface_configure(void *data, struct xdg_surface *xdg_surface, int32_t width, int32_t height,struct wl_array *states, uint32_t serial);
static void _ecore_xdg_handle_surface_delete(void *data, struct xdg_surface *xdg_surface);
static void _ecore_xdg_handle_surface_delete(void *data, struct xdg_surface *xdg_surface);
static void _ecore_xdg_handle_popup_done(void *data, struct xdg_popup *xdg_popup);
static void _ecore_session_recovery_uuid(void *data, struct session_recovery *session_recovery, const char *uuid);
static void _ecore_wl_window_cb_visibility_change(void *data, struct tizen_visibility *tizen_visibility, uint32_t visibility);
static void _ecore_wl_window_cb_position_change(void *data, struct tizen_position *tizen_position, int32_t x, int32_t y);
static void _ecore_wl_window_cb_available_angles_done(void *data, struct tizen_rotation *tizen_rotation, uint32_t angles);
static void _ecore_wl_window_cb_preferred_angle_done(void *data, struct tizen_rotation *tizen_rotation, uint32_t angle);
static void _ecore_wl_window_cb_angle_change(void *data, struct tizen_rotation *tizen_rotation, uint32_t angle, uint32_t serial);
static void _ecore_wl_window_cb_resource_id(void *data, struct tizen_resource *tizen_resource, uint32_t id);
static void _ecore_wl_window_iconified_set(Ecore_Wl_Window *win, Eina_Bool iconified, Eina_Bool send_event);


/* local variables */
static Eina_Hash *_windows = NULL;

/* wayland listeners */
static const struct wl_shell_surface_listener _ecore_wl_shell_surface_listener =
{
   _ecore_wl_window_cb_ping,
   _ecore_wl_window_cb_configure,
   _ecore_wl_window_cb_popup_done
};

static const struct xdg_surface_listener _ecore_xdg_surface_listener =
{
   _ecore_xdg_handle_surface_configure,
   _ecore_xdg_handle_surface_delete,
};

static const struct xdg_popup_listener _ecore_xdg_popup_listener =
{
   _ecore_xdg_handle_popup_done,
};

static const struct tizen_visibility_listener _ecore_tizen_visibility_listener =
{
   _ecore_wl_window_cb_visibility_change,
};

static const struct tizen_position_listener _ecore_tizen_position_listener =
{
   _ecore_wl_window_cb_position_change,
};
static const struct tizen_rotation_listener _ecore_tizen_rotation_listener =
{
   _ecore_wl_window_cb_available_angles_done,
   _ecore_wl_window_cb_preferred_angle_done,
   _ecore_wl_window_cb_angle_change,
};

static const struct tizen_resource_listener _ecore_tizen_resource_listener =
{
   _ecore_wl_window_cb_resource_id,
};

static const struct session_recovery_listener _ecore_session_recovery_listener =
{
   _ecore_session_recovery_uuid,
};

/* internal functions */
void
_ecore_wl_window_init(void)
{
   if (!_windows)
     _windows = eina_hash_string_superfast_new(NULL);
}

void
_ecore_wl_window_shutdown(void)
{
   eina_hash_free(_windows);
   _windows = NULL;
}

Eina_Hash *
_ecore_wl_window_hash_get(void)
{
   return _windows;
}

void
_ecore_wl_window_shell_surface_init(Ecore_Wl_Window *win)
{
#ifdef USE_IVI_SHELL
   char *env;
#endif

   if ((win->type != ECORE_WL_WINDOW_TYPE_DND) &&
       (win->type != ECORE_WL_WINDOW_TYPE_NONE))
     {
#ifdef USE_IVI_SHELL
        if ((!win->ivi_surface) && (_ecore_wl_disp->wl.ivi_application))
          {
             if (win->parent && win->parent->ivi_surface)
               win->ivi_surface_id = win->parent->ivi_surface_id + 1;
             else if ((env = getenv("ECORE_IVI_SURFACE_ID")))
               win->ivi_surface_id = atoi(env);
             else
               win->ivi_surface_id = IVI_SURFACE_ID + getpid();

             win->ivi_surface =
                ivi_application_surface_create(_ecore_wl_disp->wl.ivi_application,
                                               win->ivi_surface_id, win->surface);
          }

        if (!win->ivi_surface)
          {
#endif
             if (_ecore_wl_disp->wl.xdg_shell)
               {
                  if (win->xdg_surface) return;
                  win->xdg_surface =
                     xdg_shell_get_xdg_surface(_ecore_wl_disp->wl.xdg_shell,
                                               win->surface);
                  if (!win->xdg_surface) return;
                  if (win->title)
                    xdg_surface_set_title(win->xdg_surface, win->title);
                  if (win->class_name)
                    xdg_surface_set_app_id(win->xdg_surface, win->class_name);
                  xdg_surface_set_user_data(win->xdg_surface, win);
                  xdg_surface_add_listener(win->xdg_surface,
                                           &_ecore_xdg_surface_listener, win);
               }
             else if (_ecore_wl_disp->wl.shell)
               {
                  if (win->shell_surface) return;
                  win->shell_surface =
                     wl_shell_get_shell_surface(_ecore_wl_disp->wl.shell,
                                                win->surface);
                  if (!win->shell_surface) return;

                  if (win->title)
                    wl_shell_surface_set_title(win->shell_surface, win->title);

                  if (win->class_name)
                    wl_shell_surface_set_class(win->shell_surface, win->class_name);
               }

             if (win->shell_surface)
               wl_shell_surface_add_listener(win->shell_surface,
                                             &_ecore_wl_shell_surface_listener, win);
#ifdef USE_IVI_SHELL
          }
#endif
     }

   if (_ecore_wl_disp->wl.tz_policy)
     {
        if (!win->tz_visibility)
          {
             win->tz_visibility =
                tizen_policy_get_visibility(_ecore_wl_disp->wl.tz_policy,
                                            win->surface);
             if (!win->tz_visibility) return;
             tizen_visibility_add_listener(win->tz_visibility,
                                           &_ecore_tizen_visibility_listener,
                                           win);
          }
        if (!win->tz_position)
          {

             win->tz_position =
                tizen_policy_get_position(_ecore_wl_disp->wl.tz_policy,
                                          win->surface);

             if (!win->tz_position) return;
             tizen_position_add_listener(win->tz_position,
                                         &_ecore_tizen_position_listener, win);
             if (win->surface)
               tizen_position_set(win->tz_position,
                                  win->allocation.x, win->allocation.y);
          }
        if (win->role)
          {
             if (win->surface)
               tizen_policy_set_role(_ecore_wl_disp->wl.tz_policy,
                                     win->surface,
                                     win->role);
          }
        if (win->focus_skip)
          {
             if (win->surface)
               tizen_policy_set_focus_skip(_ecore_wl_disp->wl.tz_policy, win->surface);
          }
        else
          {
             if (win->surface)
               tizen_policy_unset_focus_skip(_ecore_wl_disp->wl.tz_policy, win->surface);
          }
        if (win->floating)
          {
             if (win->surface)
               tizen_policy_set_floating_mode(_ecore_wl_disp->wl.tz_policy,
                                              win->surface);
          }
     }
   if ((!win->tz_rot.resource) && (_ecore_wl_disp->wl.tz_policy_ext))
     {
        int i = 0, w, h, rot;
        win->tz_rot.resource =
        tizen_policy_ext_get_rotation(_ecore_wl_disp->wl.tz_policy_ext,
                                      win->surface);
        if (!win->tz_rot.resource) return;
        tizen_rotation_add_listener(win->tz_rot.resource,
                                    &_ecore_tizen_rotation_listener, win);

        if (win->tz_rot.preferred != TIZEN_ROTATION_ANGLE_NONE)
          {
             tizen_rotation_set_preferred_angle(win->tz_rot.resource,
                                                win->tz_rot.preferred);
          }

        if (win->tz_rot.available != TIZEN_ROTATION_ANGLE_NONE)
          {
             tizen_rotation_set_available_angles(win->tz_rot.resource,
                                                 win->tz_rot.available);
          }

        rot = ecore_wl_window_rotation_get(win);
        if ((rot % 90 == 0) && (rot / 90 <= 3) && (rot >= 0))
          {
             i = rot / 90;
             w = win->rotation_geometry_hints[i].w;
             h = win->rotation_geometry_hints[i].h;

             if ((win->rotation_geometry_hints[i].valid) &&
                 ((win->allocation.w != w) || (win->allocation.h != h)))
               {
                  _ecore_wl_window_configure_send(win,
                                                  w, h, 0);
               }
          }
     }

   if ((!win->tz_resource) && (_ecore_wl_disp->wl.tz_surf))
     {
        win->tz_resource =
           tizen_surface_get_tizen_resource(_ecore_wl_disp->wl.tz_surf, win->surface);
        if (!win->tz_resource) return;
        tizen_resource_add_listener(win->tz_resource,
                                    &_ecore_tizen_resource_listener, win);
     }

   /* trap for valid shell surface */
   if ((!win->xdg_surface) && (!win->shell_surface)) return;

   switch (win->type)
     {
      case ECORE_WL_WINDOW_TYPE_FULLSCREEN:
        if (win->xdg_surface)
          xdg_surface_set_fullscreen(win->xdg_surface, NULL);
        else if (win->shell_surface)
          wl_shell_surface_set_fullscreen(win->shell_surface,
                                          WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT,
                                          0, NULL);
        break;
      case ECORE_WL_WINDOW_TYPE_MAXIMIZED:
        if (win->xdg_surface)
          xdg_surface_set_maximized(win->xdg_surface);
        else if (win->shell_surface)
          wl_shell_surface_set_maximized(win->shell_surface, NULL);
        break;
      case ECORE_WL_WINDOW_TYPE_TRANSIENT:
        if (win->xdg_surface)
          xdg_surface_set_parent(win->xdg_surface, win->parent->xdg_surface);
        else if (win->shell_surface)
          wl_shell_surface_set_transient(win->shell_surface,
                                         win->parent->surface,
                                         win->allocation.x,
                                         win->allocation.y, 0);
        break;
      case ECORE_WL_WINDOW_TYPE_MENU:
        if (win->xdg_surface)
          {
             win->xdg_popup =
               xdg_shell_get_xdg_popup(_ecore_wl_disp->wl.xdg_shell,
                                       win->surface,
                                       win->parent->surface,
                                       _ecore_wl_disp->input->seat,
                                       _ecore_wl_disp->serial,
                                       win->allocation.x, win->allocation.y);
             if (!win->xdg_popup) return;
             xdg_popup_set_user_data(win->xdg_popup, win);
             xdg_popup_add_listener(win->xdg_popup,
                                    &_ecore_xdg_popup_listener, win);
          }
        else if (win->shell_surface)
          wl_shell_surface_set_popup(win->shell_surface,
                                     _ecore_wl_disp->input->seat,
                                     _ecore_wl_disp->serial,
                                     win->parent->surface,
                                     win->allocation.x, win->allocation.y, 0);
        break;
      case ECORE_WL_WINDOW_TYPE_TOPLEVEL:
        if (win->xdg_surface)
          xdg_surface_set_parent(win->xdg_surface, NULL);
        else if (win->shell_surface)
          wl_shell_surface_set_toplevel(win->shell_surface);
        break;
      default:
        break;
     }

   if (!win->visible)
     {
        _ecore_wl_window_show_send(win);
        win->visible = EINA_TRUE;
     }
}

EAPI Ecore_Wl_Window *
ecore_wl_window_new(Ecore_Wl_Window *parent, int x, int y, int w, int h, int buffer_type)
{
   Ecore_Wl_Window *win;
   static int _win_id = 1;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(win = calloc(1, sizeof(Ecore_Wl_Window))))
     {
        ERR("Failed to allocate an Ecore Wayland Window");
        return NULL;
     }

   win->display = _ecore_wl_disp;
   win->parent = parent;
   win->allocation.x = x;
   win->allocation.y = y;
   win->allocation.w = w;
   win->allocation.h = h;
   win->saved.w = w;
   win->saved.h = h;
   win->configured.w = w;
   win->configured.h = h;
   win->configured.edges = 0;
   win->transparent = EINA_FALSE;
   if (parent)
     win->type = ECORE_WL_WINDOW_TYPE_TRANSIENT;
   else
     win->type = ECORE_WL_WINDOW_TYPE_TOPLEVEL;
   win->buffer_type = buffer_type;
   win->id = _win_id++;
   win->rotation = 0;

   win->opaque.x = x;
   win->opaque.y = y;
   win->opaque.w = w;
   win->opaque.h = h;

   win->title = NULL;
   win->class_name = NULL;
   win->role = NULL;
   win->focus_skip = EINA_FALSE;

   eina_hash_add(_windows, _ecore_wl_window_id_str_get(win->id), win);
   return win;
}

void
_ecore_wl_window_aux_hint_free(Ecore_Wl_Window *win)
{
   char *supported;
   EINA_LIST_FREE(win->supported_aux_hints, supported)
     if (supported) eina_stringshare_del(supported);
}

EAPI void 
ecore_wl_window_free(Ecore_Wl_Window *win)
{
   Ecore_Wl_Input *input;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;

   eina_hash_del(_windows, _ecore_wl_window_id_str_get(win->id), win);

   EINA_INLIST_FOREACH(_ecore_wl_disp->inputs, input)
     {
        if ((input->pointer_focus) && (input->pointer_focus == win))
          input->pointer_focus = NULL;
        if ((input->keyboard_focus) && (input->keyboard_focus == win))
          {
             input->keyboard_focus = NULL;
             ecore_timer_del(input->repeat.tmr);
             input->repeat.tmr = NULL;
          }
     }

   if (win->visible) _ecore_wl_window_hide_send(win);
   win->visible = EINA_FALSE;

   if (win->anim_callback) wl_callback_destroy(win->anim_callback);
   win->anim_callback = NULL;

   if (win->subsurfs) _ecore_wl_subsurfs_del_all(win);

#ifdef USE_IVI_SHELL
   if (win->ivi_surface) ivi_surface_destroy(win->ivi_surface);
   win->ivi_surface = NULL;
#endif
   if (win->tz_visibility) tizen_visibility_destroy(win->tz_visibility);
   win->tz_visibility = NULL;
   if (win->tz_rot.resource) tizen_rotation_destroy(win->tz_rot.resource);
   win->tz_rot.resource = NULL;

   if (win->tz_position) tizen_position_destroy(win->tz_position);
   win->tz_position = NULL;

   if (win->xdg_surface) xdg_surface_destroy(win->xdg_surface);
   win->xdg_surface = NULL;
   if (win->xdg_popup) xdg_popup_destroy(win->xdg_popup);
   win->xdg_popup = NULL;

   if (win->shell_surface) wl_shell_surface_destroy(win->shell_surface);
   win->shell_surface = NULL;
   if (win->surface) wl_surface_destroy(win->surface);
   win->surface = NULL;

   if (win->title) eina_stringshare_del(win->title);
   if (win->class_name) eina_stringshare_del(win->class_name);
   if (win->role) eina_stringshare_del(win->role);

   _ecore_wl_window_aux_hint_free(win);
   if (win->input_region) wl_region_destroy(win->input_region);
   win->input_region = NULL;
   /* HMMM, why was this disabled ? */
   free(win);
}

EAPI void
ecore_wl_window_move(Ecore_Wl_Window *win, int x, int y)
{
   Ecore_Wl_Input *input;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;

   input = win->keyboard_device;
   ecore_wl_window_update_location(win, x, y);

   if ((!input) && (win->parent))
     {
        if (!(input = win->parent->keyboard_device))
          input = win->parent->pointer_device;
     }

   if ((!input) || (!input->seat)) return;

   _ecore_wl_input_grab_release(input, win);

   if (win->xdg_surface)
     xdg_surface_move(win->xdg_surface, input->seat, input->display->serial);
   else if (win->shell_surface)
     wl_shell_surface_move(win->shell_surface, input->seat,
                           input->display->serial);
}

EAPI void
ecore_wl_window_resize(Ecore_Wl_Window *win, int w EINA_UNUSED, int h EINA_UNUSED, int location)
{
   Ecore_Wl_Input *input;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;

   input = win->keyboard_device;

   if ((!input) && (win->parent))
     {
        if (!(input = win->parent->keyboard_device))
          input = win->parent->pointer_device;
     }

   if ((!input) || (!input->seat)) return;

   _ecore_wl_input_grab_release(input, win);

   if (win->xdg_surface)
     xdg_surface_resize(win->xdg_surface, input->seat,
                        input->display->serial, location);
   else if (win->shell_surface)
     wl_shell_surface_resize(win->shell_surface, input->seat,
                             input->display->serial, location);
}

EAPI void
ecore_wl_window_damage(Ecore_Wl_Window *win, int x, int y, int w, int h)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;
   if (win->surface) wl_surface_damage(win->surface, x, y, w, h);
}

EAPI void
ecore_wl_window_commit(Ecore_Wl_Window *win)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;

   if ((win->surface))// && (win->has_buffer))
     wl_surface_commit(win->surface);
}

EAPI void
ecore_wl_window_buffer_attach(Ecore_Wl_Window *win, struct wl_buffer *buffer, int x, int y)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;

   switch (win->buffer_type)
     {
      case ECORE_WL_WINDOW_BUFFER_TYPE_EGL_WINDOW:
        break;
      case ECORE_WL_WINDOW_BUFFER_TYPE_EGL_IMAGE:
      case ECORE_WL_WINDOW_BUFFER_TYPE_SHM:
        if (win->surface)
          {
             win->has_buffer = (buffer != NULL);

             /* if (buffer) */
             wl_surface_attach(win->surface, buffer, x, y);
             wl_surface_damage(win->surface, 0, 0,
                               win->allocation.w, win->allocation.h);
             ecore_wl_window_commit(win);
          }
        break;
      default:
        return;
     }
}

EAPI struct wl_surface *
ecore_wl_window_surface_create(Ecore_Wl_Window *win)
{
   struct wl_compositor *wlcomp;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return NULL;
   if (win->surface) return win->surface;

   if (_ecore_wl_disp->wl.session_recovery)
     session_recovery_add_listener(_ecore_wl_disp->wl.session_recovery, &_ecore_session_recovery_listener, win);
   wlcomp = _ecore_wl_compositor_get();
   if (wlcomp)
     win->surface = wl_compositor_create_surface(wlcomp);
   if (!win->surface) return NULL;
   win->surface_id = wl_proxy_get_id((struct wl_proxy *)win->surface);
   return win->surface;
}

EAPI void
ecore_wl_window_show(Ecore_Wl_Window *win)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;

   ecore_wl_window_surface_create(win);

   _ecore_wl_window_shell_surface_init(win);
}

EAPI void
ecore_wl_window_hide(Ecore_Wl_Window *win)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;

   if (win->visible)
     {
        _ecore_wl_window_hide_send(win);
        win->visible = EINA_FALSE;
     }

   if (win->tz_visibility) tizen_visibility_destroy(win->tz_visibility);
   win->tz_visibility = NULL;

   if (win->tz_rot.resource) tizen_rotation_destroy(win->tz_rot.resource);
   win->tz_rot.resource = NULL;

   if (win->tz_position) tizen_position_destroy(win->tz_position);
   win->tz_position = NULL;

   if (win->xdg_surface) xdg_surface_destroy(win->xdg_surface);
   win->xdg_surface = NULL;

   if (win->xdg_popup) xdg_popup_destroy(win->xdg_popup);
   win->xdg_popup = NULL;

   if (win->shell_surface) wl_shell_surface_destroy(win->shell_surface);
   win->shell_surface = NULL;
}

EAPI void
ecore_wl_window_raise(Ecore_Wl_Window *win)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;
   /* FIXME: This should raise the xdg surface also */

   if ((win->surface) && (_ecore_wl_disp->wl.tz_policy))
     tizen_policy_raise(_ecore_wl_disp->wl.tz_policy, win->surface);
   if (win->shell_surface) 
     wl_shell_surface_set_toplevel(win->shell_surface);
}

EAPI void 
ecore_wl_window_lower(Ecore_Wl_Window *win)
{
   Ecore_Wl_Event_Window_Lower *ev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;
   /* FIXME: This should lower the xdg surface also */
   if (_ecore_wl_disp->wl.tz_policy)
     {
        tizen_policy_lower(_ecore_wl_disp->wl.tz_policy, win->surface);

        if (!(ev = calloc(1, sizeof(Ecore_Wl_Event_Window_Lower)))) return;

        ev->win = win->id;
        ecore_event_add(ECORE_WL_EVENT_WINDOW_LOWER, ev, NULL, NULL);
     }
}

EAPI void 
ecore_wl_window_activate(Ecore_Wl_Window *win)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;

   Eina_Bool iconic = EINA_FALSE;
   iconic = ecore_wl_window_iconified_get(win);
   if (iconic)
     ecore_wl_window_iconified_set(win, EINA_FALSE);

   if (_ecore_wl_disp->wl.tz_policy)
     tizen_policy_activate(_ecore_wl_disp->wl.tz_policy, win->surface);
}

EAPI void
ecore_wl_window_maximized_set(Ecore_Wl_Window *win, Eina_Bool maximized)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;

   if ((win->type == ECORE_WL_WINDOW_TYPE_MAXIMIZED) == maximized) return;

   if (win->type == ECORE_WL_WINDOW_TYPE_TOPLEVEL)
     {
        if (win->xdg_surface)
          {
             xdg_surface_set_maximized(win->xdg_surface);
             win->type = ECORE_WL_WINDOW_TYPE_MAXIMIZED;
          }
        else if (win->shell_surface)
          {
             wl_shell_surface_set_maximized(win->shell_surface, NULL);
             win->type = ECORE_WL_WINDOW_TYPE_MAXIMIZED;
          }
     }
   else if (win->type == ECORE_WL_WINDOW_TYPE_MAXIMIZED)
     {
        if (win->xdg_surface)
          {
             xdg_surface_unset_maximized(win->xdg_surface);
             win->type = ECORE_WL_WINDOW_TYPE_TOPLEVEL;
             _ecore_wl_window_configure_send(win,
                                             win->saved.w,
                                             win->saved.h,
                                             0);
          }
        else if (win->shell_surface)
          {
             wl_shell_surface_set_toplevel(win->shell_surface);
             win->type = ECORE_WL_WINDOW_TYPE_TOPLEVEL;
             _ecore_wl_window_configure_send(win,
                                             win->saved.w,
                                             win->saved.h,
                                             0);
          }
     }
}

EAPI Eina_Bool
ecore_wl_window_maximized_get(Ecore_Wl_Window *win)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return EINA_FALSE;

   return (win->maximized) || (win->type == ECORE_WL_WINDOW_TYPE_MAXIMIZED);
}

EAPI void
ecore_wl_window_fullscreen_set(Ecore_Wl_Window *win, Eina_Bool fullscreen)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;
   if ((win->type == ECORE_WL_WINDOW_TYPE_FULLSCREEN) == fullscreen) return;
   if (fullscreen)
     {
        win->type = ECORE_WL_WINDOW_TYPE_FULLSCREEN;

        if (win->xdg_surface)
          xdg_surface_set_fullscreen(win->xdg_surface, NULL);

        if (win->shell_surface)
          wl_shell_surface_set_fullscreen(win->shell_surface,
                                          WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT,
                                          0, NULL);
     }
   else
     {
        if (win->xdg_surface)
          xdg_surface_unset_fullscreen(win->xdg_surface);
        else if (win->shell_surface)
          wl_shell_surface_set_toplevel(win->shell_surface);

        win->type = ECORE_WL_WINDOW_TYPE_TOPLEVEL;
        _ecore_wl_window_configure_send(win,
                                        win->saved.w, win->saved.h, 0);
     }
}

EAPI Eina_Bool
ecore_wl_window_fullscreen_get(Ecore_Wl_Window *win)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return EINA_FALSE;

   return win->fullscreen || (win->type == ECORE_WL_WINDOW_TYPE_FULLSCREEN);
}

EAPI void
ecore_wl_window_transparent_set(Ecore_Wl_Window *win, Eina_Bool transparent)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;
   win->transparent = transparent;
   if (!win->transparent)
     ecore_wl_window_opaque_region_set(win, win->opaque.x, win->opaque.y,
                                       win->opaque.w, win->opaque.h);
   else
     ecore_wl_window_opaque_region_set(win, win->opaque.x, win->opaque.y, 0, 0);
}

EAPI Eina_Bool
ecore_wl_window_alpha_get(Ecore_Wl_Window *win)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return EINA_FALSE;

   return win->alpha;
}

EAPI void
ecore_wl_window_alpha_set(Ecore_Wl_Window *win, Eina_Bool alpha)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;
   win->alpha = alpha;
   if (!win->alpha)
     ecore_wl_window_opaque_region_set(win, win->opaque.x, win->opaque.y,
                                       win->opaque.w, win->opaque.h);
   else
     {
        ecore_wl_window_opaque_region_set(win, win->opaque.x, win->opaque.y, 0, 0);
        if (win->surface)
          wl_surface_set_opaque_region(win->surface, NULL);
     }
}

EAPI Eina_Bool
ecore_wl_window_transparent_get(Ecore_Wl_Window *win)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return EINA_FALSE;

   return win->transparent;
}

EAPI void
ecore_wl_window_update_size(Ecore_Wl_Window *win, int w, int h)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;
   win->allocation.w = w;
   win->allocation.h = h;
   win->configured.w = w;
   win->configured.h = h;
   win->configured.edges = 0;
   if ((!ecore_wl_window_maximized_get(win)) && (!win->fullscreen))
     {
        win->saved.w = w;
        win->saved.h = h;
     }

   if (win->xdg_surface)
     xdg_surface_set_window_geometry(win->xdg_surface,
                                     win->allocation.x, win->allocation.y,
                                     win->allocation.w, win->allocation.h);
}

EAPI void
ecore_wl_window_update_location(Ecore_Wl_Window *win, int x, int y)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;
   win->allocation.x = x;
   win->allocation.y = y;

   if (win->xdg_surface)
     xdg_surface_set_window_geometry(win->xdg_surface,
                                     win->allocation.x, win->allocation.y,
                                     win->allocation.w, win->allocation.h);
}

EAPI struct wl_surface *
ecore_wl_window_surface_get(Ecore_Wl_Window *win)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return NULL;
   return win->surface;
}

/* @since 1.2 */
EAPI struct wl_shell_surface *
ecore_wl_window_shell_surface_get(Ecore_Wl_Window *win)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return NULL;
   return win->shell_surface;
}

/* @since 1.11 */
EAPI struct xdg_surface *
ecore_wl_window_xdg_surface_get(Ecore_Wl_Window *win)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return NULL;
   return win->xdg_surface;
}

EAPI Ecore_Wl_Window *
ecore_wl_window_find(unsigned int id)
{
   Ecore_Wl_Window *win = NULL;

   if (!_windows) return NULL;
   win = eina_hash_find(_windows, _ecore_wl_window_id_str_get(id));
   return win;
}

EAPI Ecore_Wl_Window_Type
ecore_wl_window_type_get(Ecore_Wl_Window *win)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return ECORE_WL_WINDOW_TYPE_NONE;
   return win->type;
}

EAPI void 
ecore_wl_window_type_set(Ecore_Wl_Window *win, Ecore_Wl_Window_Type type)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;
   win->type = type;

   if ((win->surface) && (_ecore_wl_disp->wl.tz_policy))
     tizen_policy_set_type(_ecore_wl_disp->wl.tz_policy, win->surface, (uint32_t)type);
}

EAPI void
ecore_wl_window_pointer_set(Ecore_Wl_Window *win, struct wl_surface *surface, int hot_x, int hot_y)
{
   Ecore_Wl_Input *input;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;

   win->pointer.surface = surface;
   win->pointer.hot_x = hot_x;
   win->pointer.hot_y = hot_y;
   win->pointer.set = EINA_TRUE;

   if ((input = win->pointer_device))
     ecore_wl_input_pointer_set(input, surface, hot_x, hot_y);
}

EAPI Eina_Bool
ecore_wl_window_pointer_warp(Ecore_Wl_Window *win, int x, int y)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win || !win->surface || !win->visible) return EINA_FALSE;
   if (!_ecore_wl_disp->wl.tz_input_device_manager) return EINA_FALSE;

   tizen_input_device_manager_pointer_warp(_ecore_wl_disp->wl.tz_input_device_manager,
                                           win->surface, wl_fixed_from_int(x), wl_fixed_from_int(y));

   return EINA_TRUE;
}

EAPI void
ecore_wl_window_cursor_from_name_set(Ecore_Wl_Window *win, const char *cursor_name)
{
   Ecore_Wl_Input *input;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;

   win->pointer.set = EINA_FALSE;

   if (!(input = win->pointer_device))
     return;

   eina_stringshare_replace(&win->cursor_name, cursor_name);

   if ((input->cursor_name) && (strcmp(input->cursor_name, win->cursor_name)))
     ecore_wl_input_cursor_from_name_set(input, cursor_name);
}

EAPI void
ecore_wl_window_cursor_default_restore(Ecore_Wl_Window *win)
{
   Ecore_Wl_Input *input;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;

   win->pointer.set = EINA_FALSE;

   if ((input = win->pointer_device))
     ecore_wl_input_cursor_default_restore(input);
}

/* @since 1.2 */
EAPI void
ecore_wl_window_parent_set(Ecore_Wl_Window *win, Ecore_Wl_Window *parent)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   win->parent = parent;
   if (win->parent)
     {
        win->type = ECORE_WL_WINDOW_TYPE_TRANSIENT;
        if (win->xdg_surface)
          xdg_surface_set_parent(win->xdg_surface, win->parent->xdg_surface);
        else if (win->shell_surface)
          wl_shell_surface_set_transient(win->shell_surface,
                                         win->parent->surface,
                                         win->allocation.x,
                                         win->allocation.y, 0);
     }
   else
     {
        win->type = ECORE_WL_WINDOW_TYPE_TOPLEVEL;
        if (win->xdg_surface)
          xdg_surface_set_parent(win->xdg_surface, NULL);
        else if (win->shell_surface)
          wl_shell_surface_set_toplevel(win->shell_surface);
     }
}

EAPI void
ecore_wl_window_position_set(Ecore_Wl_Window *win, int x, int y)
{
   win->allocation.x = x;
   win->allocation.y = y;

   if ((win->surface) && (win->tz_position))
     {
        tizen_position_set(win->tz_position, win->allocation.x, win->allocation.y);
     }
}

EAPI void
ecore_wl_window_focus_skip_set(Ecore_Wl_Window *win, Eina_Bool focus_skip)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;
   if (win->focus_skip == focus_skip) return;

   win->focus_skip = focus_skip;

   if (focus_skip)
     {
        if ((win->surface) && (_ecore_wl_disp->wl.tz_policy))
          tizen_policy_set_focus_skip(_ecore_wl_disp->wl.tz_policy, win->surface);
      }
   else
     {
        if ((win->surface) && (_ecore_wl_disp->wl.tz_policy))
          tizen_policy_unset_focus_skip(_ecore_wl_disp->wl.tz_policy, win->surface);
     }
 }

EAPI void
ecore_wl_window_role_set(Ecore_Wl_Window *win, const char *role)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;
   eina_stringshare_replace(&win->role, role);

   if ((win->surface) && (_ecore_wl_disp->wl.tz_policy))
     tizen_policy_set_role(_ecore_wl_disp->wl.tz_policy, win->surface, win->role);
}

/* @since 1.12 */
EAPI void
ecore_wl_window_iconified_set(Ecore_Wl_Window *win, Eina_Bool iconified)
{
   _ecore_wl_window_iconified_set(win, iconified, EINA_TRUE);
}

EAPI Eina_Bool
ecore_wl_window_iconified_get(Ecore_Wl_Window *win)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return EINA_FALSE;

   if (_ecore_wl_disp->wl.tz_policy)
     return win->iconified;
   else
     return win->minimized;
}

EAPI void
ecore_wl_window_iconify_state_update(Ecore_Wl_Window *win, Eina_Bool iconified, Eina_Bool send_event)
{
   _ecore_wl_window_iconified_set(win, iconified, send_event);
}

EAPI Ecore_Wl_Window *
ecore_wl_window_surface_find(struct wl_surface *surface)
{
   Eina_Iterator *itr;
   Ecore_Wl_Window *win = NULL;
   void *data;

   itr = eina_hash_iterator_data_new(_windows);
   while (eina_iterator_next(itr, &data))
     {
        if (((Ecore_Wl_Window *)data)->surface == surface)
          {
             win = data;
             break;
          }
     }

   eina_iterator_free(itr);

   return win;
}

EAPI void
ecore_wl_window_input_rect_set(Ecore_Wl_Window *win, Eina_Rectangle *input_rect)
{
   if (!win) return;
   if (!input_rect) return;
   if (win->input_region)
     {
        wl_region_destroy(win->input_region);
        win->input_region = NULL;
     }

   win->input.x = input_rect->x;
   win->input.y = input_rect->y;
   win->input.w = input_rect->w;
   win->input.h = input_rect->h;

   if (win->type != ECORE_WL_WINDOW_TYPE_DND)
     {
        struct wl_region *region;
        region = wl_compositor_create_region(_ecore_wl_compositor_get());
        if (!region) return;

        wl_region_add(region, input_rect->x, input_rect->y, input_rect->w, input_rect->h);
        wl_surface_set_input_region(win->surface, region);
        wl_region_destroy(region);
     }
}

EAPI void
ecore_wl_window_input_rect_add(Ecore_Wl_Window *win, Eina_Rectangle *input_rect)
{
   if (!win) return;
   if (!input_rect) return;
   if (input_rect->x < 0 || input_rect->y < 0) return;

   if (win->type != ECORE_WL_WINDOW_TYPE_DND)
     {
        if (!win->input_region)
          {
             struct wl_region *region;
             region = wl_compositor_create_region(_ecore_wl_compositor_get());
             if (!region) return;

             win->input_region = region;
          }

        wl_region_add(win->input_region, input_rect->x, input_rect->y, input_rect->w, input_rect->h);
        wl_surface_set_input_region(win->surface, win->input_region);
     }
}

EAPI void
ecore_wl_window_input_rect_subtract(Ecore_Wl_Window *win, Eina_Rectangle *input_rect)
{
   if (!win) return;
   if (!input_rect) return;
   if (input_rect->x < 0 || input_rect->y < 0) return;
   if (!win->input_region) return;

   if (win->type != ECORE_WL_WINDOW_TYPE_DND)
     {
        wl_region_subtract(win->input_region, input_rect->x, input_rect->y, input_rect->w, input_rect->h);
        wl_surface_set_input_region(win->surface, win->input_region);
     }
}

/* @since 1.8 */
EAPI void
ecore_wl_window_input_region_set(Ecore_Wl_Window *win, int x, int y, int w, int h)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;

   win->input.x = x;
   win->input.y = y;
   win->input.w = w;
   win->input.h = h;

   if (win->type != ECORE_WL_WINDOW_TYPE_DND)
     {
        struct wl_region *region;

        region = wl_compositor_create_region(_ecore_wl_compositor_get());
        if (!region) return;

        switch (win->rotation)
          {
           case 0:
             wl_region_add(region, x, y, w, h);
             break;
           case 180:
             wl_region_add(region, x, x + y, w, h);
             break;
           case 90:
             wl_region_add(region, y, x, h, w);
             break;
           case 270:
             wl_region_add(region, x + y, x, h, w);
             break;
          }

        wl_surface_set_input_region(win->surface, region);
        wl_region_destroy(region);
     }
}

/* @since 1.8 */
EAPI void
ecore_wl_window_opaque_region_set(Ecore_Wl_Window *win, int x, int y, int w, int h)
{
   struct wl_region *region;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;

   win->opaque.x = x;
   win->opaque.y = y;
   win->opaque.w = w;
   win->opaque.h = h;

   if ((win->transparent) || (win->alpha)) return;

   region = wl_compositor_create_region(_ecore_wl_compositor_get());
   if (!region) return;

   switch (win->rotation)
     {
      case 0:
        wl_region_add(region, x, y, w, h);
        break;
      case 180:
        wl_region_add(region, x, x + y, w, h);
        break;
      case 90:
        wl_region_add(region, y, x, h, w);
        break;
      case 270:
        wl_region_add(region, x + y, x, h, w);
        break;
     }

   wl_surface_set_opaque_region(win->surface, region);
   wl_region_destroy(region);
}

/* @since 1.8 */
EAPI void
ecore_wl_window_rotation_set(Ecore_Wl_Window *win, int rotation)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;
   win->rotation = rotation;
}

/* @since 1.8 */
EAPI int
ecore_wl_window_rotation_get(Ecore_Wl_Window *win)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return 0;
   return win->rotation;
}

/* @since 1.8 */
EAPI int
ecore_wl_window_id_get(Ecore_Wl_Window *win)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return 0;
   return win->id;
}

/* @since 1.8 */
EAPI int
ecore_wl_window_surface_id_get(Ecore_Wl_Window *win)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return 0;
   return win->surface_id;
}

/* @since 1.8 */
EAPI void
ecore_wl_window_title_set(Ecore_Wl_Window *win, const char *title)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;
   eina_stringshare_replace(&win->title, title);

   if ((win->xdg_surface) && (win->title))
     xdg_surface_set_title(win->xdg_surface, win->title);
   else if ((win->shell_surface) && (win->title))
     wl_shell_surface_set_title(win->shell_surface, win->title);
}

/* @since 1.8 */
EAPI void
ecore_wl_window_class_name_set(Ecore_Wl_Window *win, const char *class_name)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;
   eina_stringshare_replace(&win->class_name, class_name);

   if ((win->xdg_surface) && (win->class_name))
     xdg_surface_set_app_id(win->xdg_surface, win->class_name);
   else if ((win->shell_surface) && (win->class_name))
     wl_shell_surface_set_class(win->shell_surface, win->class_name);
}

/* @since 1.8 */
/* Maybe we need an ecore_wl_window_pointer_get() too */
EAPI Ecore_Wl_Input *
ecore_wl_window_keyboard_get(Ecore_Wl_Window *win)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return 0;
   return win->keyboard_device;
}

EAPI void
ecore_wl_window_stack_mode_set(Ecore_Wl_Window *win, Ecore_Wl_Window_Stack_Mode mode)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;

   if ((win->surface) && (_ecore_wl_disp->wl.tz_policy))
     tizen_policy_set_stack_mode(_ecore_wl_disp->wl.tz_policy, win->surface, mode);
}

EAPI void
ecore_wl_window_rotation_preferred_rotation_set(Ecore_Wl_Window *win, int rot)
{
   enum tizen_rotation_angle angle = TIZEN_ROTATION_ANGLE_NONE;

   if (!win) return;

   switch (rot)
     {
        case 0:
          angle = TIZEN_ROTATION_ANGLE_0;
          break;
        case 90:
          angle = TIZEN_ROTATION_ANGLE_90;
          break;
        case 180:
          angle = TIZEN_ROTATION_ANGLE_180;
          break;
        case 270:
          angle = TIZEN_ROTATION_ANGLE_270;
          break;
        default:
          break;
     }

   if (win->tz_rot.preferred == angle)
     return;

   win->tz_rot.preferred = angle;

   if (win->tz_rot.resource)
     tizen_rotation_set_preferred_angle(win->tz_rot.resource, (uint32_t)angle);
}

EAPI void
ecore_wl_window_rotation_available_rotations_set(Ecore_Wl_Window *win, const int *rots, unsigned int count)
{
   uint32_t angles = 0;
   unsigned int i = 0;

   if (!win) return;

   for (i = 0; i < count ; i++)
     {
        switch (rots[i])
          {
             case 0:
                angles |= (uint32_t)TIZEN_ROTATION_ANGLE_0;
                break;
             case 90:
                angles |= (uint32_t)TIZEN_ROTATION_ANGLE_90;
                break;
             case 180:
                angles |= (uint32_t)TIZEN_ROTATION_ANGLE_180;
                break;
             case 270:
                angles |= (uint32_t)TIZEN_ROTATION_ANGLE_270;
                break;
             default:
                break;
          }
     }

   if (win->tz_rot.available == angles)
     return;

   win->tz_rot.available = angles;

   if (win->tz_rot.resource)
     tizen_rotation_set_available_angles(win->tz_rot.resource, angles);
}

EAPI void
ecore_wl_window_rotation_change_done_send(Ecore_Wl_Window *win)
{
   if (!win) return;
   if (!win->tz_rot.resource) return;

   tizen_rotation_ack_angle_change(win->tz_rot.resource, win->tz_rot.serial);
}


EAPI void
ecore_wl_window_rotation_geometry_set(Ecore_Wl_Window *win, int rot, int x, int y, int w, int h)
{
   int i = 0;
   int rotation = 0;
   if (!win) return;

   if ((rot % 90 != 0) || (rot / 90 > 3) || (rot < 0)) return;

   i = rot / 90;
   win->rotation_geometry_hints[i].x = x;
   win->rotation_geometry_hints[i].y = y;
   win->rotation_geometry_hints[i].w = w;
   win->rotation_geometry_hints[i].h = h;
   win->rotation_geometry_hints[i].valid = EINA_TRUE;

   if (!win->tz_rot.resource) return;
   rotation = ecore_wl_window_rotation_get(win);
   if ((rotation % 90 != 0) || (rotation / 90 > 3) || (rotation < 0)) return;
   if ((i == (rotation / 90)) &&
       ((win->allocation.w != w) || (win->allocation.h != h)))
     {
        _ecore_wl_window_configure_send(win,
                                        w, h, 0);
     }
}

/* local functions */
static void
_ecore_wl_window_cb_ping(void *data EINA_UNUSED, struct wl_shell_surface *shell_surface, unsigned int serial)
{
   if (!shell_surface) return;
   wl_shell_surface_pong(shell_surface, serial);
}

static void
_ecore_wl_window_cb_configure(void *data, struct wl_shell_surface *shell_surface EINA_UNUSED, unsigned int edges, int w, int h)
{
   Ecore_Wl_Window *win;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(win = data)) return;

   if ((w <= 0) || (h <= 0)) return;

   if ((win->allocation.w != w) || (win->allocation.h != h))
     {
        win->configured.w = w;
        win->configured.h = h;
        win->configured.edges = edges;

        _ecore_wl_window_configure_send(win,
                                        w, h, edges);
     }
}

static void
_ecore_xdg_handle_surface_configure(void *data, struct xdg_surface *xdg_surface EINA_UNUSED, int32_t width, int32_t height, struct wl_array *states, uint32_t serial)
{
   Ecore_Wl_Window *win;
   uint32_t *p;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(win = data)) return;

   win->maximized = EINA_FALSE;
   win->fullscreen = EINA_FALSE;
   win->resizing = EINA_FALSE;
   win->focused = EINA_FALSE;

   wl_array_for_each(p, states)
     {
        uint32_t state = *p;
        switch (state)
          {
           case XDG_SURFACE_STATE_MAXIMIZED:
             win->maximized = EINA_TRUE;
             break;
           case XDG_SURFACE_STATE_FULLSCREEN:
             win->fullscreen = EINA_TRUE;
             break;
           case XDG_SURFACE_STATE_RESIZING:
             win->resizing = EINA_TRUE;
             break;
           case XDG_SURFACE_STATE_ACTIVATED:
             win->focused = EINA_TRUE;
             win->minimized = EINA_FALSE;
             break;
           default:
             break;
          }
     }
   if ((width > 0) && (height > 0))
     _ecore_wl_window_configure_send(win, width, height, 0);

   if (win->xdg_surface)
     xdg_surface_ack_configure(win->xdg_surface, serial);
}

static void
_ecore_wl_window_cb_position_change(void *data, struct tizen_position *tizen_position EINA_UNUSED, int32_t x, int32_t y)
{
   Ecore_Wl_Window *win;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(win = data)) return;

   if ((x != win->allocation.x) || (y != win->allocation.y))
     {
        ecore_wl_window_update_location(win, x, y);
        _ecore_wl_window_configure_send(win,
                                        win->configured.w,
                                        win->configured.h,
                                        win->configured.edges);
     }
}

static void
_ecore_wl_window_cb_visibility_change(void *data, struct tizen_visibility *tizen_visibility EINA_UNUSED, uint32_t visibility)
{
   Ecore_Wl_Window *win;
   Ecore_Wl_Event_Window_Visibility_Change *ev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(win = data)) return;
   if (!(ev = calloc(1, sizeof(Ecore_Wl_Event_Window_Visibility_Change)))) return;

   ev->win = win->id;
   if (visibility == TIZEN_VISIBILITY_VISIBILITY_FULLY_OBSCURED)
     ev->fully_obscured = 1;
   else
     ev->fully_obscured = 0;

   ecore_event_add(ECORE_WL_EVENT_WINDOW_VISIBILITY_CHANGE, ev, NULL, NULL);
}

static void
_ecore_wl_window_cb_available_angles_done(void *data EINA_UNUSED, struct tizen_rotation *tizen_rotation EINA_UNUSED, uint32_t angles EINA_UNUSED)
{
   return;
}

static void
_ecore_wl_window_cb_preferred_angle_done(void *data EINA_UNUSED, struct tizen_rotation *tizen_rotation EINA_UNUSED, uint32_t angle EINA_UNUSED)
{
   return;
}

static void
_ecore_wl_window_cb_angle_change(void *data, struct tizen_rotation *tizen_rotation EINA_UNUSED, uint32_t angle, uint32_t serial)
{
   Ecore_Wl_Window *win;
   Ecore_Wl_Event_Window_Rotate *ev;
   int i = 0;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(win = data)) return;
   if (!(ev = calloc(1, sizeof(Ecore_Wl_Event_Window_Rotate)))) return;

   win->tz_rot.serial = serial;

   ev->win = win->id;
   ev->w = win->configured.w;
   ev->h = win->configured.h;

   switch (angle)
     {
       case TIZEN_ROTATION_ANGLE_0:
         ev->angle = 0;
         break;
       case TIZEN_ROTATION_ANGLE_90:
         ev->angle = 90;
         break;
       case TIZEN_ROTATION_ANGLE_180:
         ev->angle = 180;
         break;
       case TIZEN_ROTATION_ANGLE_270:
         ev->angle = 270;
         break;
       default:
         ev->angle = 0;
         break;
     }

   i = ev->angle / 90;
   if (win->rotation_geometry_hints[i].valid)
     {
        ev->w = win->rotation_geometry_hints[i].w;
        ev->h = win->rotation_geometry_hints[i].h;
     }

   ecore_event_add(ECORE_WL_EVENT_WINDOW_ROTATE, ev, NULL, NULL);
   ecore_wl_window_rotation_set(win, ev->angle);
}

static void
_ecore_wl_window_cb_resource_id(void *data, struct tizen_resource *tizen_resource EINA_UNUSED, uint32_t id)
{
   Ecore_Wl_Window *win;
   Ecore_Wl_Event_Window_Show *ev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(win = data)) return;
   if (!(ev = calloc(1, sizeof(Ecore_Wl_Event_Window_Show)))) return;
   ev->win = win->id;
   if (win->parent)
       ev->parent_win = win->parent->id;
   else
       ev->parent_win = 0;
   ev->event_win = win->id;
   ev->data[0] = (unsigned int)id;
   win->resource_id = (unsigned int)id;
   ecore_event_add(ECORE_WL_EVENT_WINDOW_SHOW, ev, NULL, NULL);
}

static void
_ecore_wl_window_iconified_set(Ecore_Wl_Window *win, Eina_Bool iconified, Eina_Bool send_event)
{
   struct wl_array states;
   uint32_t *s;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!win) return;

   if (iconified)
     {
        if ((win->surface) && (_ecore_wl_disp->wl.tz_policy))
          {
             if (send_event)
               tizen_policy_iconify(_ecore_wl_disp->wl.tz_policy, win->surface);
             win->iconified = EINA_TRUE;
          }
        else if (win->xdg_surface)
          {
             xdg_surface_set_minimized(win->xdg_surface);
             win->minimized = iconified;
          }
        else if (win->shell_surface)
          {
             /* TODO: handle case of iconifying a wl_shell surface */
          }
     }
   else
     {
        if ((win->surface) && (_ecore_wl_disp->wl.tz_policy))
          {
             if (send_event)
               tizen_policy_uniconify(_ecore_wl_disp->wl.tz_policy, win->surface);
             win->iconified = EINA_FALSE;
          }
        else if (win->xdg_surface)
          {
             win->type = ECORE_WL_WINDOW_TYPE_TOPLEVEL;
             wl_array_init(&states);
             s = wl_array_add(&states, sizeof(*s));
             *s = XDG_SURFACE_STATE_ACTIVATED;
             _ecore_xdg_handle_surface_configure(win, win->xdg_surface, win->saved.w, win->saved.h, &states, 0);
             wl_array_release(&states);
          }
        else if (win->shell_surface)
          {
             wl_shell_surface_set_toplevel(win->shell_surface);
             win->type = ECORE_WL_WINDOW_TYPE_TOPLEVEL;
             _ecore_wl_window_configure_send(win,
                                             win->saved.w, win->saved.h, 0);
          }
     }

   if (send_event)
     {
        Ecore_Wl_Event_Window_Iconify_State_Change *ev;

        if (!(ev = calloc(1, sizeof(Ecore_Wl_Event_Window_Iconify_State_Change)))) return;
        ev->win = win->id;
        ev->iconified = iconified;
        ev->force = 0;
        ecore_event_add(ECORE_WL_EVENT_WINDOW_ICONIFY_STATE_CHANGE, ev, NULL, NULL);
     }
}

static void 
_ecore_xdg_handle_surface_delete(void *data, struct xdg_surface *xdg_surface EINA_UNUSED)
{
   Ecore_Wl_Window *win;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(win = data)) return;
   ecore_wl_window_free(win);
}

static void
_ecore_wl_window_cb_popup_done(void *data, struct wl_shell_surface *shell_surface)
{
   Ecore_Wl_Window *win;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!shell_surface) return;
   if (!(win = data)) return;
   ecore_wl_input_ungrab(win->pointer_device);
}

static void
_ecore_xdg_handle_popup_done(void *data, struct xdg_popup *xdg_popup)
{
   Ecore_Wl_Window *win;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!xdg_popup) return;
   if (!(win = data)) return;
   ecore_wl_input_ungrab(win->pointer_device);
}

static void
_ecore_session_recovery_uuid(void *data EINA_UNUSED, struct session_recovery *session_recovery, const char *uuid)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!session_recovery) return;
   DBG("UUID event received from compositor with UUID: %s", uuid);
}

static void
_ecore_wl_window_configure_send(Ecore_Wl_Window *win, int w, int h, int edges)
{
   Ecore_Wl_Event_Window_Configure *ev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(ev = calloc(1, sizeof(Ecore_Wl_Event_Window_Configure)))) return;
   ev->win = win->id;
   ev->event_win = win->id;
   ev->x = win->allocation.x;
   ev->y = win->allocation.y;
   ev->w = w;
   ev->h = h;
   ev->edges = edges;

   win->configured.w = w;
   win->configured.h = h;
   win->configured.edges = edges;

   ecore_event_add(ECORE_WL_EVENT_WINDOW_CONFIGURE, ev, NULL, NULL);
}

static void
_ecore_wl_window_show_send(Ecore_Wl_Window *win)
{
   Ecore_Wl_Event_Window_Show *ev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(ev = calloc(1, sizeof(Ecore_Wl_Event_Window_Show)))) return;
   ev->win = win->id;
   if (win->parent)
       ev->parent_win = win->parent->id;
   else
       ev->parent_win = 0;
   ev->event_win = win->id;
   ecore_event_add(ECORE_WL_EVENT_WINDOW_SHOW, ev, NULL, NULL);
}

static void
_ecore_wl_window_hide_send(Ecore_Wl_Window *win)
{
   Ecore_Wl_Event_Window_Hide *ev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(ev = calloc(1, sizeof(Ecore_Wl_Event_Window_Hide)))) return;
   ev->win = win->id;
   if (win->parent)
       ev->parent_win = win->parent->id;
   else
       ev->parent_win = 0;
   ev->event_win = win->id;
   ecore_event_add(ECORE_WL_EVENT_WINDOW_HIDE, ev, NULL, NULL);
}

static char *
_ecore_wl_window_id_str_get(unsigned int win_id)
{
   const char *vals = "qWeRtYuIoP5$&<~";
   static char id[9];
   unsigned int val;

   val = win_id;
   id[0] = vals[(val >> 28) & 0xf];
   id[1] = vals[(val >> 24) & 0xf];
   id[2] = vals[(val >> 20) & 0xf];
   id[3] = vals[(val >> 16) & 0xf];
   id[4] = vals[(val >> 12) & 0xf];
   id[5] = vals[(val >> 8) & 0xf];
   id[6] = vals[(val >> 4) & 0xf];
   id[7] = vals[(val) & 0xf];
   id[8] = 0;

   return id;
}

void
ecore_wl_window_indicator_geometry_set(Ecore_Wl_Window *win, int x, int y, int w, int h)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   if(!win) return;

   win->indicator.x = x;
   win->indicator.y = y;
   win->indicator.w = w;
   win->indicator.h = h;
}

Eina_Bool
ecore_wl_window_indicator_geometry_get(Ecore_Wl_Window *win, int *x, int *y, int *w, int *h)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   if(!win) return EINA_FALSE;

   if (x)
     *x = win->indicator.x;
   if (y)
     *y = win->indicator.y;
   if (w)
     *w = win->indicator.w;
   if (h)
     *h = win->indicator.h;

   return EINA_TRUE;
}

EAPI void
ecore_wl_window_indicator_state_set(Ecore_Wl_Window *win, Ecore_Wl_Indicator_State state)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   if (!win) return;

   win->indicator.state = state;
}

EAPI Ecore_Wl_Indicator_State
ecore_wl_window_indicator_state_get(Ecore_Wl_Window *win)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   if (!win) return EINA_FALSE;

   return win->indicator.state;
}

EAPI void
ecore_wl_window_indicator_opacity_set(Ecore_Wl_Window *win, Ecore_Wl_Indicator_Opacity_Mode mode)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   if (!win) return;

   win->indicator.mode = mode;
}

EAPI Ecore_Wl_Indicator_Opacity_Mode
ecore_wl_window_indicator_opacity_get(Ecore_Wl_Window *win)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   if (!win) return EINA_FALSE;

   return win->indicator.mode;
}

void
ecore_wl_window_clipboard_geometry_set(Ecore_Wl_Window *win, int x, int y, int w, int h)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   if(!win) return;

   win->clipboard.x = x;
   win->clipboard.y = y;
   win->clipboard.w = w;
   win->clipboard.h = h;
}

Eina_Bool
ecore_wl_window_clipboard_geometry_get(Ecore_Wl_Window *win, int *x, int *y, int *w, int *h)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   if (!win) return EINA_FALSE;

   if (x)
     *x = win->clipboard.x;
   if (y)
     *y = win->clipboard.y;
   if (w)
     *w = win->clipboard.w;
   if (h)
     *h = win->clipboard.h;

   return EINA_TRUE;
}

EAPI void
ecore_wl_window_clipboard_state_set(Ecore_Wl_Window *win, Ecore_Wl_Clipboard_State state)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   if (!win) return;

   win->clipboard.state = state;
}

EAPI Ecore_Wl_Clipboard_State
ecore_wl_window_clipboard_state_get(Ecore_Wl_Window *win)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   if (!win) return EINA_FALSE;

   return win->clipboard.state;
}

void
ecore_wl_window_keyboard_geometry_set(Ecore_Wl_Window *win, int x, int y, int w, int h)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   if(!win) return;

   win->keyboard.x= x;
   win->keyboard.y = y;
   win->keyboard.w = w;
   win->keyboard.h = h;
}

Eina_Bool
ecore_wl_window_keyboard_geometry_get(Ecore_Wl_Window *win, int *x, int *y, int *w, int *h)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   if (!win) return EINA_FALSE;

   if (x)
     *x = win->keyboard.x;
   if (y)
     *y = win->keyboard.y;
   if (w)
     *w = win->keyboard.w;
   if (h)
     *h = win->keyboard.h;

   return EINA_TRUE;
}

EAPI void
ecore_wl_window_keyboard_state_set(Ecore_Wl_Window *win, Ecore_Wl_Virtual_Keyboard_State state)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   if (!win) return;

   win->keyboard.state = state;
}

EAPI Ecore_Wl_Virtual_Keyboard_State
ecore_wl_window_keyboard_state_get(Ecore_Wl_Window *win)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   if (!win) return EINA_FALSE;

   return win->keyboard.state;
}

EAPI void
ecore_wl_window_conformant_set(Ecore_Wl_Window *win, unsigned int is_conformant)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   if (!win) return;
   if (!win->surface) return;
   if (!_ecore_wl_disp->wl.tz_policy) return;

   if (is_conformant)
     tizen_policy_set_conformant(_ecore_wl_disp->wl.tz_policy, win->surface);
   else
     tizen_policy_unset_conformant(_ecore_wl_disp->wl.tz_policy, win->surface);
}

EAPI Eina_Bool
ecore_wl_window_conformant_get(Ecore_Wl_Window *win)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   if (!win) return 0;
   if (!win->surface) return 0;
   if (!_ecore_wl_disp->wl.tz_policy) return 0;

   tizen_policy_get_conformant(_ecore_wl_disp->wl.tz_policy, win->surface);

   ecore_wl_sync();

   return win->conformant;
}

EAPI Eina_List *
ecore_wl_window_aux_hints_supported_get(Ecore_Wl_Window *win)
{
   Eina_List *res = NULL;
   Eina_List *ll;
   char *supported_hint = NULL;
   const char *hint = NULL;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   if (!win) return 0;
   if (!win->surface) return 0;
   if (!_ecore_wl_disp->wl.tz_policy) return 0;

   tizen_policy_get_supported_aux_hints(_ecore_wl_disp->wl.tz_policy, win->surface);

   ecore_wl_sync();

   EINA_LIST_FOREACH(win->supported_aux_hints, ll, supported_hint)
     {
        hint = eina_stringshare_add(supported_hint);
        res = eina_list_append(res, hint);
     }
   return res;
}

EAPI void
ecore_wl_window_aux_hint_add(Ecore_Wl_Window *win, int id, const char *hint, const char *val)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   if (!win) return;
   if ((win->surface) && (_ecore_wl_disp->wl.tz_policy))
     tizen_policy_add_aux_hint(_ecore_wl_disp->wl.tz_policy, win->surface, id, hint, val);
}

EAPI void
ecore_wl_window_aux_hint_change(Ecore_Wl_Window *win, int id, const char *val)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   if (!win) return;
   if ((win->surface) && (_ecore_wl_disp->wl.tz_policy))
     tizen_policy_change_aux_hint(_ecore_wl_disp->wl.tz_policy, win->surface, id, val);
}

EAPI void
ecore_wl_window_aux_hint_del(Ecore_Wl_Window *win, int id)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   if (!win) return;
   if ((win->surface) && (_ecore_wl_disp->wl.tz_policy))
     tizen_policy_del_aux_hint(_ecore_wl_disp->wl.tz_policy, win->surface, id);
}

EAPI void
ecore_wl_window_floating_mode_set(Ecore_Wl_Window *win, Eina_Bool floating)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   if (!win) return;

   win->floating = floating;
   if ((win->surface) && (_ecore_wl_disp->wl.tz_policy))
     {
        if (floating)
          tizen_policy_set_floating_mode(_ecore_wl_disp->wl.tz_policy,
                                         win->surface);
        else
          tizen_policy_unset_floating_mode(_ecore_wl_disp->wl.tz_policy,
                                           win->surface);
     }
}
