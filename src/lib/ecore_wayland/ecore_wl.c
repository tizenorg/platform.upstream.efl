#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <fcntl.h>
#include "ecore_wl_private.h"

/*
 * The subsurface protocol was moved into Wayland Core
 * around v1.3.90 (i.e. v1.4.0).
 * Test if subsurface protocol is part of wayland-client.h.
 * If not, we include our own copy of the protocol header.
 */
#include <wayland-client.h>
#ifndef WL_SUBSURFACE_ERROR_ENUM
# include <subsurface-client-protocol.h>
#endif

#include <xdg-shell-client-protocol.h>
#define XDG_VERSION 4
#include <tizen-extension-client-protocol.h>

/* local function prototypes */
static int _ecore_wl_shutdown(Eina_Bool close);
static Eina_Bool _ecore_wl_cb_idle_enterer(void *data);
static Eina_Bool _ecore_wl_cb_handle_data(void *data, Ecore_Fd_Handler *hdl);
static void _ecore_wl_cb_handle_global(void *data, struct wl_registry *registry, unsigned int id, const char *interface, unsigned int version EINA_UNUSED);
static void _ecore_wl_cb_handle_global_remove(void *data, struct wl_registry *registry EINA_UNUSED, unsigned int id);
static Eina_Bool _ecore_wl_xkb_init(Ecore_Wl_Display *ewd);
static Eina_Bool _ecore_wl_xkb_shutdown(Ecore_Wl_Display *ewd);
static void _ecore_wl_sync_wait(Ecore_Wl_Display *ewd);
static void _ecore_wl_sync_callback(void *data, struct wl_callback *callback, uint32_t serial);
static void _ecore_wl_animator_tick_cb_begin(void *data EINA_UNUSED);
static void _ecore_wl_animator_tick_cb_end(void *data EINA_UNUSED);
static void _ecore_wl_animator_callback(void *data, struct wl_callback *callback, uint32_t serial EINA_UNUSED);
static Eina_Bool _ecore_wl_animator_window_add(const Eina_Hash *hash EINA_UNUSED, const void *key EINA_UNUSED, void *data, void *fdata EINA_UNUSED);
static void _ecore_wl_signal_exit(void);
static void _ecore_wl_signal_exit_free(void *data EINA_UNUSED, void *event);
static void _ecore_wl_init_callback(void *data, struct wl_callback *callback, uint32_t serial EINA_UNUSED);
// TIZEN_ONLY(20150722): Add ecore_wl_window_keygrab_* APIs
static void _ecore_wl_cb_keygrab_notify(void *data, struct tizen_keyrouter *tizen_keyrouter, struct wl_surface *surface, uint32_t key, uint32_t mode, uint32_t error);
//
static void _ecore_wl_cb_conformant(void *data EINA_UNUSED, struct tizen_policy *tizen_policy EINA_UNUSED, struct wl_surface *surface_resource, uint32_t is_conformant);
static void _ecore_wl_cb_conformant_area(void *data EINA_UNUSED, struct tizen_policy *tizen_policy EINA_UNUSED, struct wl_surface *surface_resource, uint32_t conformant_part, uint32_t state, int32_t x, int32_t y, int32_t w, int32_t h);
static void _ecore_wl_cb_notification_done(void *data, struct tizen_policy *tizen_policy, struct wl_surface *surface, int32_t level, uint32_t state);
static void _ecore_wl_cb_transient_for_done(void *data, struct tizen_policy *tizen_policy, uint32_t child_id);
static void _ecore_wl_cb_scr_mode_done(void *data, struct tizen_policy *tizen_policy, struct wl_surface *surface, uint32_t mode, uint32_t state);
static void _ecore_wl_window_conformant_area_send(Ecore_Wl_Window *win, uint32_t conformant_part, uint32_t state);

/* local variables */
static int _ecore_wl_init_count = 0;
static Eina_Bool _ecore_wl_animator_busy = EINA_FALSE;
static Eina_Bool _ecore_wl_fatal_error = EINA_FALSE;
static Eina_Bool _ecore_wl_server_mode = EINA_FALSE;
// TIZEN_ONLY(20150722): Add ecore_wl_window_keygrab_* APIs
static int _ecore_wl_keygrab_error = -1;
//

static const struct wl_registry_listener _ecore_wl_registry_listener =
{
   _ecore_wl_cb_handle_global,
   _ecore_wl_cb_handle_global_remove
};

static const struct wl_callback_listener _ecore_wl_sync_listener =
{
   _ecore_wl_sync_callback
};

static const struct wl_callback_listener _ecore_wl_init_sync_listener =
{
   _ecore_wl_init_callback
};

static const struct wl_callback_listener _ecore_wl_anim_listener =
{
   _ecore_wl_animator_callback
};

// TIZEN_ONLY(20150722): Add ecore_wl_window_keygrab_* APIs
static const struct tizen_keyrouter_listener _ecore_tizen_keyrouter_listener =
{
   _ecore_wl_cb_keygrab_notify
};
//

static const struct tizen_policy_listener _ecore_tizen_policy_listener =
{
   _ecore_wl_cb_conformant,
   _ecore_wl_cb_conformant_area,
   _ecore_wl_cb_notification_done,
   _ecore_wl_cb_transient_for_done,
   _ecore_wl_cb_scr_mode_done,
};
static void 
xdg_shell_ping(void *data EINA_UNUSED, struct xdg_shell *shell, uint32_t serial)
{
   xdg_shell_pong(shell, serial);
}

static const struct xdg_shell_listener xdg_shell_listener =
{
   xdg_shell_ping,
};

/* external variables */
int _ecore_wl_log_dom = -1;
Ecore_Wl_Display *_ecore_wl_disp = NULL;

EAPI int ECORE_WL_EVENT_MOUSE_IN = 0;
EAPI int ECORE_WL_EVENT_MOUSE_OUT = 0;
EAPI int ECORE_WL_EVENT_FOCUS_IN = 0;
EAPI int ECORE_WL_EVENT_FOCUS_OUT = 0;
EAPI int ECORE_WL_EVENT_WINDOW_CONFIGURE = 0;
EAPI int ECORE_WL_EVENT_WINDOW_ACTIVATE = 0;
EAPI int ECORE_WL_EVENT_WINDOW_DEACTIVATE = 0;
EAPI int ECORE_WL_EVENT_WINDOW_VISIBILITY_CHANGE = 0;
EAPI int ECORE_WL_EVENT_WINDOW_SHOW = 0;
EAPI int ECORE_WL_EVENT_WINDOW_HIDE = 0;
EAPI int ECORE_WL_EVENT_WINDOW_ROTATE = 0;
EAPI int ECORE_WL_EVENT_DND_ENTER = 0;
EAPI int ECORE_WL_EVENT_DND_POSITION = 0;
EAPI int ECORE_WL_EVENT_DND_LEAVE = 0;
EAPI int ECORE_WL_EVENT_DND_DROP = 0;
EAPI int ECORE_WL_EVENT_DND_OFFER = 0;
EAPI int ECORE_WL_EVENT_DND_END = 0;
EAPI int ECORE_WL_EVENT_DATA_SOURCE_TARGET = 0;
EAPI int ECORE_WL_EVENT_DATA_SOURCE_SEND = 0;
EAPI int ECORE_WL_EVENT_SELECTION_DATA_READY = 0;
EAPI int ECORE_WL_EVENT_DATA_SOURCE_CANCELLED = 0;
EAPI int ECORE_WL_EVENT_INTERFACES_BOUND = 0;
EAPI int ECORE_WL_EVENT_CONFORMANT_CHANGE = 0;

static void
_ecore_wl_init_callback(void *data, struct wl_callback *callback, uint32_t serial EINA_UNUSED)
{
   Ecore_Wl_Display *ewd = data;

   wl_callback_destroy(callback);
   ewd->init_done = EINA_TRUE;
}

static void
_ecore_wl_init_wait(void)
{
   while (!_ecore_wl_disp->init_done)
     wl_display_dispatch(_ecore_wl_disp->wl.display);
}

EAPI int
ecore_wl_init(const char *name)
{
   struct wl_callback *callback;
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (++_ecore_wl_init_count != 1) return _ecore_wl_init_count;

   if (!eina_init()) return --_ecore_wl_init_count;

   _ecore_wl_log_dom =
     eina_log_domain_register("ecore_wl", ECORE_WL_DEFAULT_LOG_COLOR);
   if (_ecore_wl_log_dom < 0)
     {
        EINA_LOG_ERR("Cannot create a log domain for Ecore Wayland");
        goto exit_eina;
     }

   if (!ecore_init())
     {
        ERR("Could not initialize ecore");
        goto exit_ecore;
     }

   if (!ecore_event_init())
     {
        ERR("Could not initialize ecore_event");
        goto exit_ecore_event;
     }

   if (!ECORE_WL_EVENT_MOUSE_IN)
     {
        ECORE_WL_EVENT_MOUSE_IN = ecore_event_type_new();
        ECORE_WL_EVENT_MOUSE_OUT = ecore_event_type_new();
        ECORE_WL_EVENT_FOCUS_IN = ecore_event_type_new();
        ECORE_WL_EVENT_FOCUS_OUT = ecore_event_type_new();
        ECORE_WL_EVENT_WINDOW_CONFIGURE = ecore_event_type_new();
        ECORE_WL_EVENT_WINDOW_ACTIVATE = ecore_event_type_new();
        ECORE_WL_EVENT_WINDOW_DEACTIVATE = ecore_event_type_new();
        ECORE_WL_EVENT_WINDOW_VISIBILITY_CHANGE = ecore_event_type_new();
        ECORE_WL_EVENT_WINDOW_SHOW = ecore_event_type_new();
        ECORE_WL_EVENT_WINDOW_HIDE = ecore_event_type_new();
        ECORE_WL_EVENT_WINDOW_ROTATE = ecore_event_type_new();
        ECORE_WL_EVENT_DND_ENTER = ecore_event_type_new();
        ECORE_WL_EVENT_DND_POSITION = ecore_event_type_new();
        ECORE_WL_EVENT_DND_LEAVE = ecore_event_type_new();
        ECORE_WL_EVENT_DND_DROP = ecore_event_type_new();
        ECORE_WL_EVENT_DND_OFFER = ecore_event_type_new();
        ECORE_WL_EVENT_DND_END = ecore_event_type_new();
        ECORE_WL_EVENT_DATA_SOURCE_TARGET = ecore_event_type_new();
        ECORE_WL_EVENT_DATA_SOURCE_SEND = ecore_event_type_new();
        ECORE_WL_EVENT_SELECTION_DATA_READY = ecore_event_type_new();
        ECORE_WL_EVENT_DATA_SOURCE_CANCELLED = ecore_event_type_new();
        ECORE_WL_EVENT_INTERFACES_BOUND = ecore_event_type_new();
        ECORE_WL_EVENT_CONFORMANT_CHANGE = ecore_event_type_new();
     }

   if (!(_ecore_wl_disp = malloc(sizeof(Ecore_Wl_Display))))
     {
        ERR("Could not allocate memory for Ecore_Wl_Display structure");
        goto exit_ecore_disp;
     }

   memset(_ecore_wl_disp, 0, sizeof(Ecore_Wl_Display));

   if (!(_ecore_wl_disp->wl.display = wl_display_connect(name)))
     {
        ERR("Could not connect to Wayland display");
        goto exit_ecore_disp_connect;
     }

   _ecore_wl_disp->fd = wl_display_get_fd(_ecore_wl_disp->wl.display);

   _ecore_wl_disp->fd_hdl =
     ecore_main_fd_handler_add(_ecore_wl_disp->fd, 
                               ECORE_FD_READ | ECORE_FD_WRITE | ECORE_FD_ERROR,
                               _ecore_wl_cb_handle_data, _ecore_wl_disp,
                               NULL, NULL);

   _ecore_wl_disp->idle_enterer =
     ecore_idle_enterer_add(_ecore_wl_cb_idle_enterer, _ecore_wl_disp);

   _ecore_wl_disp->wl.registry =
     wl_display_get_registry(_ecore_wl_disp->wl.display);
   wl_registry_add_listener(_ecore_wl_disp->wl.registry,
                            &_ecore_wl_registry_listener, _ecore_wl_disp);

   if (!_ecore_wl_xkb_init(_ecore_wl_disp))
     {
        ERR("Could not initialize XKB");
        goto exit_ecore_disp_connect;
     }

   _ecore_wl_window_init();
   _ecore_wl_events_init();

   _ecore_wl_disp->init_done = EINA_TRUE;
   if (!_ecore_wl_server_mode)
     {
        _ecore_wl_disp->init_done = EINA_FALSE;
        callback = wl_display_sync(_ecore_wl_disp->wl.display);
        wl_callback_add_listener(callback, &_ecore_wl_init_sync_listener,
                                 _ecore_wl_disp);
     }

   return _ecore_wl_init_count;

exit_ecore_disp_connect:
   free(_ecore_wl_disp);

exit_ecore_disp:
   ecore_event_shutdown();

exit_ecore_event:
   ecore_shutdown();

exit_ecore:
   eina_log_domain_unregister(_ecore_wl_log_dom);
   _ecore_wl_log_dom = -1;

exit_eina:
   eina_shutdown();
   return --_ecore_wl_init_count;
}

EAPI int
ecore_wl_shutdown(void)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   return _ecore_wl_shutdown(EINA_TRUE);
}

EAPI void
ecore_wl_flush(void)
{
   if ((!_ecore_wl_disp) || (!_ecore_wl_disp->wl.display)) return;
   wl_display_flush(_ecore_wl_disp->wl.display);
}

EAPI void
ecore_wl_sync(void)
{
   if ((!_ecore_wl_disp) || (!_ecore_wl_disp->wl.display)) return;
   _ecore_wl_sync_wait(_ecore_wl_disp);
   while (_ecore_wl_disp->sync_ref_count > 0)
     wl_display_dispatch(_ecore_wl_disp->wl.display);
}

EAPI struct wl_shm *
ecore_wl_shm_get(void)
{
   if (!_ecore_wl_disp) return NULL;

   _ecore_wl_init_wait();

   return _ecore_wl_disp->wl.shm;
}

EAPI struct wl_display *
ecore_wl_display_get(void)
{
   if ((!_ecore_wl_disp) || (!_ecore_wl_disp->wl.display))
     return NULL;
   return _ecore_wl_disp->wl.display;
}

EAPI Eina_Inlist *
ecore_wl_globals_get(void)
{
   if ((!_ecore_wl_disp) || (!_ecore_wl_disp->wl.display))
     return NULL;

   _ecore_wl_init_wait();

   return _ecore_wl_disp->globals;
}

EAPI struct wl_registry *
ecore_wl_registry_get(void)
{
   if ((!_ecore_wl_disp) || (!_ecore_wl_disp->wl.display))
     return NULL;
   return _ecore_wl_disp->wl.registry;
}

EAPI struct wl_compositor *
ecore_wl_compositor_get(void)
{
   return _ecore_wl_compositor_get();
}

struct wl_compositor *
_ecore_wl_compositor_get(void)
{
   if ((!_ecore_wl_disp) || (!_ecore_wl_disp->wl.display))
     return NULL;

   _ecore_wl_init_wait();

   return _ecore_wl_disp->wl.compositor;
}

struct wl_subcompositor *
_ecore_wl_subcompositor_get(void)
{
   if ((!_ecore_wl_disp) || (!_ecore_wl_disp->wl.display))
     return NULL;

   _ecore_wl_init_wait();

   return _ecore_wl_disp->wl.subcompositor;
}

EAPI void
ecore_wl_screen_size_get(int *w, int *h)
{
   Ecore_Wl_Output *out;
   Eina_Inlist *tmp;
   int ow = 0, oh = 0;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (w) *w = 0;
   if (h) *h = 0;

   if ((!_ecore_wl_disp) || (!_ecore_wl_disp->wl.display)) return;

   _ecore_wl_init_wait();

   // the first sync is in case registry replies are not back yet
   if (!_ecore_wl_disp->output)
     {
        // second sync is in case bound object replies in registry are not back
        ecore_wl_sync();
        if (!_ecore_wl_disp->output) ecore_wl_sync();
     }

   EINA_INLIST_FOREACH_SAFE(_ecore_wl_disp->outputs, tmp, out)
     {
        switch (out->transform)
          {
           case WL_OUTPUT_TRANSFORM_90:
           case WL_OUTPUT_TRANSFORM_270:
           case WL_OUTPUT_TRANSFORM_FLIPPED_90:
           case WL_OUTPUT_TRANSFORM_FLIPPED_270:
             /* Swap width and height */
             ow += out->allocation.h;
             oh += out->allocation.w;
             break;
           default:
             ow += out->allocation.w;
             oh += out->allocation.h;
          }
     }

   if (w) *w = ow;
   if (h) *h = oh;
}

/* @since 1.2 */
EAPI void
ecore_wl_pointer_xy_get(int *x, int *y)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   _ecore_wl_input_pointer_xy_get(x, y);
}

EAPI int
ecore_wl_dpi_get(void)
{
   int w, mw;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!_ecore_wl_disp) return 0;
   if (!_ecore_wl_disp->output) return 75;

   mw = _ecore_wl_disp->output->mw;
   if (mw <= 0) return 75;

   w = _ecore_wl_disp->output->allocation.w;
   /* FIXME: NB: Hrrrmmm, need to verify this. xorg code is using a different
    * formula to calc this */
   return (((w * 254) / mw) + 5) / 10;
}

EAPI void
ecore_wl_display_iterate(void)
{
   if ((!_ecore_wl_disp) || (!_ecore_wl_disp->wl.display)) return;
   if (!_ecore_wl_server_mode)
     wl_display_dispatch(_ecore_wl_disp->wl.display);
}

/* @since 1.8 */
EAPI Eina_Bool
ecore_wl_animator_source_set(Ecore_Animator_Source source)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* FIXME: check existing source. If custom, disable anim_callbacks */

   /* based on the animator source we are using, setup or destroy callbacks */
   switch (source)
     {
      case ECORE_ANIMATOR_SOURCE_CUSTOM:
        ecore_animator_custom_source_tick_begin_callback_set
          (_ecore_wl_animator_tick_cb_begin, NULL);
        ecore_animator_custom_source_tick_end_callback_set
          (_ecore_wl_animator_tick_cb_end, NULL);
        break;
      case ECORE_ANIMATOR_SOURCE_TIMER:
        ecore_animator_custom_source_tick_begin_callback_set(NULL, NULL);
        ecore_animator_custom_source_tick_end_callback_set(NULL, NULL);
        break;
      default:
        break;
     }

   /* set the source of the animator */
   ecore_animator_source_set(source);

   return EINA_TRUE;
}

EAPI struct wl_cursor *
ecore_wl_cursor_get(const char *cursor_name)
{
   if ((!_ecore_wl_disp) || (!_ecore_wl_disp->cursor_theme))
     return NULL;

   return wl_cursor_theme_get_cursor(_ecore_wl_disp->cursor_theme,
                                     cursor_name);
}

EAPI void
ecore_wl_server_mode_set(Eina_Bool on)
{
   _ecore_wl_server_mode = on;
}

/* local functions */
static int
_ecore_wl_shutdown(Eina_Bool close)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (_ecore_wl_init_count < 1)
     {
        ERR("Ecore_Wayland Shutdown called without Ecore_Wayland Init");
        return 0;
     }

   if (--_ecore_wl_init_count != 0) return _ecore_wl_init_count;
   if (!_ecore_wl_disp) return _ecore_wl_init_count;

   _ecore_wl_events_shutdown();
   _ecore_wl_window_shutdown();

   if (_ecore_wl_disp->fd_hdl)
     ecore_main_fd_handler_del(_ecore_wl_disp->fd_hdl);
   if (_ecore_wl_disp->idle_enterer)
      ecore_idle_enterer_del(_ecore_wl_disp->idle_enterer);

   if ((close) && (!_ecore_wl_fatal_error))
     {
        Ecore_Wl_Output *out;
        Ecore_Wl_Input *in;
        Ecore_Wl_Global *global;
        Eina_Inlist *tmp;

        EINA_INLIST_FOREACH_SAFE(_ecore_wl_disp->outputs, tmp, out)
          _ecore_wl_output_del(out);

        EINA_INLIST_FOREACH_SAFE(_ecore_wl_disp->inputs, tmp, in)
          _ecore_wl_input_del(in);

        EINA_INLIST_FOREACH_SAFE(_ecore_wl_disp->globals, tmp, global)
          {
             _ecore_wl_disp->globals = 
               eina_inlist_remove(_ecore_wl_disp->globals, 
                                  EINA_INLIST_GET(global));
             free(global->interface);
             free(global);
          }

        _ecore_wl_xkb_shutdown(_ecore_wl_disp);

#ifdef USE_IVI_SHELL
        if (_ecore_wl_disp->wl.ivi_application)
          ivi_application_destroy(_ecore_wl_disp->wl.ivi_application);
#endif
        if (_ecore_wl_disp->wl.xdg_shell)
          xdg_shell_destroy(_ecore_wl_disp->wl.xdg_shell);
        if (_ecore_wl_disp->wl.shell)
          wl_shell_destroy(_ecore_wl_disp->wl.shell);
        if (_ecore_wl_disp->wl.shm) wl_shm_destroy(_ecore_wl_disp->wl.shm);
        if (_ecore_wl_disp->wl.data_device_manager)
          wl_data_device_manager_destroy(_ecore_wl_disp->wl.data_device_manager);
        if (_ecore_wl_disp->wl.tz_policy)
          tizen_policy_destroy(_ecore_wl_disp->wl.tz_policy);
        if (_ecore_wl_disp->wl.tz_policy_ext)
          tizen_policy_ext_destroy(_ecore_wl_disp->wl.tz_policy_ext);
        if (_ecore_wl_disp->wl.tz_surf)
          tizen_surface_destroy(_ecore_wl_disp->wl.tz_surf);
        if (_ecore_wl_disp->wl.compositor)
          wl_compositor_destroy(_ecore_wl_disp->wl.compositor);
        if (_ecore_wl_disp->wl.subcompositor)
          wl_subcompositor_destroy(_ecore_wl_disp->wl.subcompositor);
// TIZEN_ONLY(20150722): Add ecore_wl_window_keygrab_* APIs
        if (_ecore_wl_disp->wl.keyrouter)
          tizen_keyrouter_destroy(_ecore_wl_disp->wl.keyrouter);
//
        if (_ecore_wl_disp->wl.display)
          {
             wl_registry_destroy(_ecore_wl_disp->wl.registry);
             wl_display_flush(_ecore_wl_disp->wl.display);
             wl_display_disconnect(_ecore_wl_disp->wl.display);
          }
        free(_ecore_wl_disp);
     }

   ecore_event_shutdown();
   ecore_shutdown();

   eina_log_domain_unregister(_ecore_wl_log_dom);
   _ecore_wl_log_dom = -1;
   eina_shutdown();

   return _ecore_wl_init_count;
}

static Eina_Bool
_ecore_wl_cb_idle_enterer(void *data)
{
   Ecore_Wl_Display *ewd;
   int ret = 0;

   if (_ecore_wl_fatal_error) return ECORE_CALLBACK_CANCEL;

   if (!(ewd = data)) return ECORE_CALLBACK_RENEW;

   ret = wl_display_get_error(ewd->wl.display);
   if (ret < 0) goto err;

   ret = wl_display_flush(ewd->wl.display);
   if ((ret < 0) && (errno == EAGAIN))
     ecore_main_fd_handler_active_set(ewd->fd_hdl,
                                      (ECORE_FD_READ | ECORE_FD_WRITE));

   ret = wl_display_dispatch_pending(ewd->wl.display);
   if (ret < 0) goto err;

   return ECORE_CALLBACK_RENEW;

err:
   if ((ret < 0) && ((errno != EAGAIN) && (errno != EINVAL)))
     {
        _ecore_wl_fatal_error = EINA_TRUE;

        /* raise exit signal */
        _ecore_wl_signal_exit();

        return ECORE_CALLBACK_CANCEL;
     }

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_ecore_wl_cb_handle_data(void *data, Ecore_Fd_Handler *hdl)
{
   Ecore_Wl_Display *ewd;
   int ret = 0;

   /* LOGFN(__FILE__, __LINE__, __FUNCTION__); */

   if (_ecore_wl_fatal_error) return ECORE_CALLBACK_CANCEL;

   if (!(ewd = data)) return ECORE_CALLBACK_RENEW;

   if (ecore_main_fd_handler_active_get(hdl, ECORE_FD_ERROR))
     {
        ERR("Received error on wayland display fd");
        _ecore_wl_fatal_error = EINA_TRUE;
        _ecore_wl_signal_exit();

        return ECORE_CALLBACK_CANCEL;
     }

   /* wl_display_dispatch_pending(ewd->wl.display); */

   if (ecore_main_fd_handler_active_get(hdl, ECORE_FD_READ))
     ret = wl_display_dispatch(ewd->wl.display);
   else if (ecore_main_fd_handler_active_get(hdl, ECORE_FD_WRITE))
     {
        ret = wl_display_flush(ewd->wl.display);
        if (ret == 0)
          ecore_main_fd_handler_active_set(hdl, ECORE_FD_READ);
     }

   if ((ret < 0) && ((errno != EAGAIN) && (errno != EINVAL)))
     {
        _ecore_wl_fatal_error = EINA_TRUE;

        /* raise exit signal */
        _ecore_wl_signal_exit();

        return ECORE_CALLBACK_CANCEL;
     }

   return ECORE_CALLBACK_RENEW;
}

static void
_ecore_wl_cb_handle_global(void *data, struct wl_registry *registry, unsigned int id, const char *interface, unsigned int version EINA_UNUSED)
{
   Ecore_Wl_Display *ewd;
   Ecore_Wl_Global *global;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ewd = data;

   if (!(global = calloc(1, sizeof(Ecore_Wl_Global)))) return;

   global->id = id;
   global->interface = strdup(interface);
   global->version = version;
   ewd->globals = eina_inlist_append(ewd->globals, EINA_INLIST_GET(global));

   if (!strcmp(interface, "wl_compositor"))
     {
        ewd->wl.compositor =
          wl_registry_bind(registry, id, &wl_compositor_interface, 3);
     }
   else if (!strcmp(interface, "wl_subcompositor"))
     {
        ewd->wl.subcompositor =
           wl_registry_bind(registry, id, &wl_subcompositor_interface, 1);
     }
   else if (!strcmp(interface, "wl_output"))
     _ecore_wl_output_add(ewd, id);
   else if (!strcmp(interface, "wl_seat"))
     _ecore_wl_input_add(ewd, id);
#ifdef USE_IVI_SHELL
   else if (!strcmp(interface, "ivi_application"))
     {
        ewd->wl.ivi_application =
          wl_registry_bind(registry, id, &ivi_application_interface, 1);
     }
#endif
   else if (!strcmp(interface, "xdg_shell") && !getenv("EFL_WAYLAND_DONT_USE_XDG_SHELL"))
     {
        ewd->wl.xdg_shell = 
          wl_registry_bind(registry, id, &xdg_shell_interface, 1);
        xdg_shell_use_unstable_version(ewd->wl.xdg_shell, XDG_VERSION);
        xdg_shell_add_listener(ewd->wl.xdg_shell, &xdg_shell_listener,
                               ewd->wl.display);
     }
   else if (!strcmp(interface, "wl_shell"))
     {
        ewd->wl.shell =
          wl_registry_bind(registry, id, &wl_shell_interface, 1);
     }
   else if (!strcmp(interface, "wl_shm"))
     {
        ewd->wl.shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);

        if (ewd->input)
          {
             ewd->cursor_theme =
               wl_cursor_theme_load(ewd->input->cursor_theme_name,
                                    ewd->input->cursor_size, ewd->wl.shm);
          }
        else
          {
             ewd->cursor_theme = 
               wl_cursor_theme_load(NULL, ECORE_WL_DEFAULT_CURSOR_SIZE, 
                                    ewd->wl.shm);
          }
     }
   else if (!strcmp(interface, "wl_data_device_manager"))
     {
        ewd->wl.data_device_manager =
          wl_registry_bind(registry, id, &wl_data_device_manager_interface, 1);
     }
   else if (!strcmp(interface, "tizen_policy"))
     {
        ewd->wl.tz_policy =
          wl_registry_bind(registry, id, &tizen_policy_interface, 1);
        if (ewd->wl.tz_policy)
          tizen_policy_add_listener(_ecore_wl_disp->wl.tz_policy, &_ecore_tizen_policy_listener, ewd->wl.display);
     }
   else if (!strcmp(interface, "tizen_policy_ext"))
     {
        ewd->wl.tz_policy_ext =
          wl_registry_bind(registry, id, &tizen_policy_ext_interface, 1);
     }
   else if (!strcmp(interface, "tizen_surface"))
     {
        ewd->wl.tz_surf =
          wl_registry_bind(registry, id, &tizen_surface_interface, 1);
     }
// TIZEN_ONLY(20150722): Add ecore_wl_window_keygrab_* APIs
   else if (!strcmp(interface, "tizen_keyrouter"))
     {
        ewd->wl.keyrouter =
          wl_registry_bind(registry, id, &tizen_keyrouter_interface, 1);
        if (ewd->wl.keyrouter)
          tizen_keyrouter_add_listener(_ecore_wl_disp->wl.keyrouter, &_ecore_tizen_keyrouter_listener, ewd->wl.display);
     }
//

   if ((ewd->wl.compositor) && (ewd->wl.shm) &&
       ((ewd->wl.shell) || (ewd->wl.xdg_shell)))
     {
        Ecore_Wl_Event_Interfaces_Bound *ev;

        if (!(ev = calloc(1, sizeof(Ecore_Wl_Event_Interfaces_Bound))))
          return;

        ev->compositor = (ewd->wl.compositor != NULL);
        ev->shm = (ewd->wl.shm != NULL);
        ev->shell = ((ewd->wl.shell != NULL) || (ewd->wl.xdg_shell != NULL));
        ev->output = (ewd->output != NULL);
        ev->seat = (ewd->input != NULL);
        ev->data_device_manager = (ewd->wl.data_device_manager != NULL);
        ev->policy = (ewd->wl.tz_policy != NULL);
        ev->policy_ext = (ewd->wl.tz_policy_ext != NULL);
        ev->subcompositor = (ewd->wl.subcompositor != NULL);

        ecore_event_add(ECORE_WL_EVENT_INTERFACES_BOUND, ev, NULL, NULL);
     }
}

static void
_ecore_wl_cb_handle_global_remove(void *data, struct wl_registry *registry EINA_UNUSED, unsigned int id)
{
   Ecore_Wl_Display *ewd;
   Ecore_Wl_Global *global;
   Eina_Inlist *tmp;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ewd = data;

   EINA_INLIST_FOREACH_SAFE(ewd->globals, tmp, global)
     {
        if (global->id != id) continue;
        ewd->globals =
          eina_inlist_remove(ewd->globals, EINA_INLIST_GET(global));
        free(global->interface);
        free(global);
     }
}

static Eina_Bool
_ecore_wl_xkb_init(Ecore_Wl_Display *ewd)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(ewd->xkb.context = xkb_context_new(0)))
     return EINA_FALSE;

   return EINA_TRUE;
}

static Eina_Bool
_ecore_wl_xkb_shutdown(Ecore_Wl_Display *ewd)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   xkb_context_unref(ewd->xkb.context);

   return EINA_TRUE;
}

static void
_ecore_wl_sync_callback(void *data, struct wl_callback *callback, uint32_t serial EINA_UNUSED)
{
   Ecore_Wl_Display *ewd = data;

   ewd->sync_ref_count--;
   wl_callback_destroy(callback);
}

static void
_ecore_wl_sync_wait(Ecore_Wl_Display *ewd)
{
   struct wl_callback *callback;

   ewd->sync_ref_count++;
   callback = wl_display_sync(ewd->wl.display);
   wl_callback_add_listener(callback, &_ecore_wl_sync_listener, ewd);
}

static void
_ecore_wl_animator_tick_cb_begin(void *data EINA_UNUSED)
{
   Eina_Hash *windows;

   _ecore_wl_animator_busy = EINA_TRUE;

   windows = _ecore_wl_window_hash_get();
   eina_hash_foreach(windows, _ecore_wl_animator_window_add, NULL);
}

static void
_ecore_wl_animator_tick_cb_end(void *data EINA_UNUSED)
{
   _ecore_wl_animator_busy = EINA_FALSE;
}

static void
_ecore_wl_animator_callback(void *data, struct wl_callback *callback, uint32_t serial EINA_UNUSED)
{
   Ecore_Wl_Window *win;

   if (!(win = data)) return;

   ecore_animator_custom_tick();

   wl_callback_destroy(callback);
   win->anim_callback = NULL;

   if (_ecore_wl_animator_busy)
     {
        win->anim_callback = wl_surface_frame(win->surface);
        wl_callback_add_listener(win->anim_callback,
                                 &_ecore_wl_anim_listener, win);
        ecore_wl_window_commit(win);
     }
}

static Eina_Bool
_ecore_wl_animator_window_add(const Eina_Hash *hash EINA_UNUSED, const void *key EINA_UNUSED, void *data, void *fdata EINA_UNUSED)
{
   Ecore_Wl_Window *win;

   if (!(win = data)) return EINA_TRUE;
   if (!win->surface) return EINA_TRUE;
   if (win->anim_callback) return EINA_TRUE;

   win->anim_callback = wl_surface_frame(win->surface);
   wl_callback_add_listener(win->anim_callback, &_ecore_wl_anim_listener, win);
   ecore_wl_window_commit(win);

   return EINA_TRUE;
}

static void
_ecore_wl_signal_exit(void)
{
   Ecore_Event_Signal_Exit *ev;

   if (!(ev = calloc(1, sizeof(Ecore_Event_Signal_Exit))))
     return;

   ev->quit = 1;
   ecore_event_add(ECORE_EVENT_SIGNAL_EXIT, ev,
                   _ecore_wl_signal_exit_free, NULL);
}

static void
_ecore_wl_signal_exit_free(void *data EINA_UNUSED, void *event)
{
   free(event);
}

// TIZEN_ONLY(20150722): Add ecore_wl_window_keygrab_* APIs
//Currently this function is only used in sink call, so use global value(_ecore_wl_keygrab_error) and just check the error is ok.
static void
_ecore_wl_cb_keygrab_notify(void *data EINA_UNUSED, struct tizen_keyrouter *tizen_keyrouter EINA_UNUSED, struct wl_surface *surface EINA_UNUSED, uint32_t key, uint32_t mode, uint32_t error)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   _ecore_wl_keygrab_error = error;
   INF("[PID:%d] key=%d, mode=%d, error=%d", getpid(), key, mode, error);
}

struct _Keycode_Map
{
   int num_keycodes;
   xkb_keysym_t keysym;
   Eina_List *keycodes;
};

typedef struct _Keycode_Map Keycode_Map;

static void find_keycode(struct xkb_keymap *keymap, xkb_keycode_t key, void *data)
{
   Keycode_Map *found_keycodes = (Keycode_Map *)data;
   xkb_keysym_t keysym = found_keycodes->keysym;
   int num_syms = 0;
   const xkb_keysym_t *syms_out = NULL;
   num_syms = xkb_keymap_key_get_syms_by_level(keymap, key, 0, 0, &syms_out);
   if ((num_syms) && (syms_out))
     {
        if ((*syms_out) == (keysym))
          {
             xkb_keycode_t *keycode = malloc(sizeof(xkb_keycode_t));
             found_keycodes->num_keycodes++;
             *keycode = key;
             found_keycodes->keycodes = eina_list_append(found_keycodes->keycodes, keycode);
          }
     }
}

//If there are several keycodes, ecore_wl only deals with first keycode.
static void
_ecore_wl_keycode_from_keysym(struct xkb_keymap *keymap, xkb_keysym_t keysym, xkb_keycode_t *keycode)
{
   Keycode_Map keycode_map = {0,};
   keycode_map.keysym = keysym;
   keycode_map.keycodes = NULL;
   xkb_keycode_t *data;

   //called fewer (max_keycode - min_keycode +1) times.
   xkb_keymap_key_for_each(keymap, find_keycode, &keycode_map);
   if (keycode_map.num_keycodes > 0)
     {
        *keycode = *((xkb_keycode_t *) eina_list_data_get(keycode_map.keycodes));
     }
   else
     *keycode = 0;

   INF("num of keycodes:%d keycode=%d", keycode_map.num_keycodes, *keycode);

   EINA_LIST_FREE(keycode_map.keycodes, data)
     free(data);
}

//I'm not sure that keygrab function should be changed to Ecore_evas_XXX.
//In the future, keyrouter feature can be added upstream or finish stabilizing.
//After that time, we maybe change API name or other thing.
//So do not use this API if you have trouble catch keyrouter feature or rule changes.

//Keyrouter support the situation when wl_win is not exist.
//But keyrouter also can be meet situation when there are several surfaces.
//So I decided to add keygrab feature into ecore_wl_window side like x system.

//Mod, not_mod, priority will be used future.
//But now we are not support, so just use 0 for this parameter.
//win can be NULL

EAPI Eina_Bool
ecore_wl_window_keygrab_set(Ecore_Wl_Window *win, const char *key, int mod EINA_UNUSED, int not_mod EINA_UNUSED, int priority EINA_UNUSED, Ecore_Wl_Window_Keygrab_Mode grab_mode)
{
   xkb_keycode_t keycode = 0;
   xkb_keysym_t keysym;

   Eina_Bool ret = EINA_FALSE;
   struct wl_surface *surface = NULL;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!key) return EINA_FALSE;
   if ((grab_mode < ECORE_WL_WINDOW_KEYGRAB_UNKNOWN) || (grab_mode > ECORE_WL_WINDOW_KEYGRAB_OVERRIDE_EXCLUSIVE))
     return EINA_FALSE;

   INF("win=%p key=%s mod=%d", win, key, grab_mode);

   keysym = xkb_keysym_from_name(key, XKB_KEYSYM_NO_FLAGS);

   if (keysym == XKB_KEY_NoSymbol)
     {
        WRN("Keysym of key(\"%s\") doesn't exist", key);
        return EINA_FALSE;
     }

   //We have to find the way to get keycode from keysym before keymap notify
   if (_ecore_wl_disp->input->xkb.keymap)
     _ecore_wl_keycode_from_keysym(_ecore_wl_disp->input->xkb.keymap, keysym, &keycode);
   else
     {
        WRN("Keymap is not ready");
        return EINA_FALSE;
     }

   if (keycode == 0)
     {
        WRN("Keycode of key(\"%s\") doesn't exist", key);
        return EINA_FALSE;
     }

   INF("keycode of key:(%d)", keycode);

   /* Request to grab a key */
   if (win)
     surface = ecore_wl_window_surface_get(win);

   tizen_keyrouter_set_keygrab(_ecore_wl_disp->wl.keyrouter, surface, keycode, grab_mode);

   /* Send sync to wayland compositor and register sync callback to exit while dispatch loop below */
   ecore_wl_sync();

   INF("After keygrab _ecore_wl_keygrab_error = %d", _ecore_wl_keygrab_error);
   if (!_ecore_wl_keygrab_error)
     {
        INF("[PID:%d]Succeed to get return value !", getpid());
        ret = EINA_TRUE;
     }
   else
     {
        WRN("[PID:%d]Failed to get return value ! ret=%d)", getpid(), _ecore_wl_keygrab_error);
        ret = EINA_FALSE;
     }
   _ecore_wl_keygrab_error = -1;
   return ret;
}

EAPI Eina_Bool
ecore_wl_window_keygrab_unset(Ecore_Wl_Window *win, const char *key, int mod EINA_UNUSED, int any_mod EINA_UNUSED)
{
   xkb_keycode_t keycode = 0;
   xkb_keysym_t keysym;

   Eina_Bool ret = EINA_FALSE;
   struct wl_surface *surface = NULL;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!key) return EINA_FALSE;
   INF("win=%p key=%s ", win, key);

   keysym = xkb_keysym_from_name(key, XKB_KEYSYM_NO_FLAGS);
   if (keysym == XKB_KEY_NoSymbol)
     {
        WRN("Keysym of key(\"%s\") doesn't exist", key);
        return EINA_FALSE;
     }

   //We have to find the way to get keycode from keysym before keymap notify
   if (_ecore_wl_disp->input->xkb.keymap)
     _ecore_wl_keycode_from_keysym(_ecore_wl_disp->input->xkb.keymap, keysym, &keycode);
   else
     {
        WRN("Keymap is not ready");
        return EINA_FALSE;
     }

   if (keycode == 0)
     {
        WRN("Keycode of key(\"%s\") doesn't exist", key);
        return EINA_FALSE;
     }

   INF("keycode of key:(%d)", keycode);

   /* Request to ungrab a key */
   if (win)
     surface = ecore_wl_window_surface_get(win);

   tizen_keyrouter_unset_keygrab(_ecore_wl_disp->wl.keyrouter, surface, keycode);

   /* Send sync to wayland compositor and register sync callback to exit while dispatch loop below */
   ecore_wl_sync();

   INF("After keygrab _ecore_wl_keygrab_error = %d", _ecore_wl_keygrab_error);
   if (!_ecore_wl_keygrab_error)
     {
        ret = EINA_TRUE;
        INF("[PID:%d] Succeed to get return value ! ", getpid());
     }
   else
     {
        ret = EINA_FALSE;
        WRN("[PID:%d] Failed to get return value ! (ret=%d)", getpid(), _ecore_wl_keygrab_error);
     }
   _ecore_wl_keygrab_error = -1;
   return ret;
}
//

static void
_ecore_wl_window_conformant_area_send(Ecore_Wl_Window *win, uint32_t conformant_part, uint32_t state)
{
   Ecore_Wl_Event_Conformant_Change *ev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(ev = calloc(1, sizeof(Ecore_Wl_Event_Conformant_Change)))) return;
   ev->win = win->id;
   ev->part_type = conformant_part;
   ev->state = state;
   ecore_event_add(ECORE_WL_EVENT_CONFORMANT_CHANGE, ev, NULL, NULL);
}

static void
_ecore_wl_cb_conformant(void *data EINA_UNUSED, struct tizen_policy *tizen_policy EINA_UNUSED, struct wl_surface *surface_resource, uint32_t is_conformant)
{
   struct wl_surface *surface = surface_resource;
   Ecore_Wl_Window *win = NULL;

   if (!surface) return;
   win = ecore_wl_window_surface_find(surface);
   if (win)
     win->conformant = is_conformant;
}

static void
_ecore_wl_cb_conformant_area(void *data EINA_UNUSED, struct tizen_policy *tizen_policy EINA_UNUSED, struct wl_surface *surface_resource, uint32_t conformant_part, uint32_t state, int32_t x, int32_t y, int32_t w, int32_t h)
{
   struct wl_surface *surface = surface_resource;
   Ecore_Wl_Window *win = NULL;
   int org_x, org_y, org_w, org_h;
   Eina_Bool changed;

   if (!surface) return;
   win = ecore_wl_window_surface_find(surface);
   if (!win) return;

   if (conformant_part == TIZEN_POLICY_CONFORMANT_PART_INDICATOR)
     {
        ecore_wl_window_indicator_geometry_get(win, &org_x, &org_y, &org_w, &org_h);
        if ((org_x != x) || (org_y != y) || (org_w != w) || (org_h != h))
          {
             ecore_wl_window_indicator_geometry_set(win, x, y, w, h);
             changed = EINA_TRUE;
          }
        if (state != ecore_wl_window_indicator_state_get(win))
          {
             ecore_wl_window_indicator_state_set(win, state);
             changed = EINA_TRUE;
          }
     }
   else if (conformant_part == TIZEN_POLICY_CONFORMANT_PART_KEYBOARD)
     {
        ecore_wl_window_keyboard_geometry_get(win, &org_x, &org_y, &org_w, &org_h);
        if ((org_x != x) || (org_y != y) || (org_w != w) || (org_h != h))
          {
             ecore_wl_window_keyboard_geometry_set(win, x, y, w, h);
             changed = EINA_TRUE;
          }
        if (state != ecore_wl_window_keyboard_state_get(win))
          {
             ecore_wl_window_keyboard_state_set(win, state);
             changed = EINA_TRUE;
          }
     }
   else if (conformant_part == TIZEN_POLICY_CONFORMANT_PART_CLIPBOARD)
     {
        ecore_wl_window_clipboard_geometry_get(win, &org_x, &org_y, &org_w, &org_h);
        if ((org_x != x) || (org_y != y) || (org_w != w) || (org_h != h))
          {
             ecore_wl_window_clipboard_geometry_set(win, x, y, w, h);
             changed = EINA_TRUE;
          }
        if (state != ecore_wl_window_clipboard_state_get(win))
          {
             ecore_wl_window_clipboard_state_set(win, state);
             changed = EINA_TRUE;
          }
     }

   if (changed)
     _ecore_wl_window_conformant_area_send(win, conformant_part, state);
}

static void
_ecore_wl_cb_notification_done(void *data EINA_UNUSED, struct tizen_policy *tizen_policy EINA_UNUSED, struct wl_surface *surface EINA_UNUSED, int32_t level EINA_UNUSED, uint32_t state EINA_UNUSED)
{
}

static void
_ecore_wl_cb_transient_for_done(void *data EINA_UNUSED, struct tizen_policy *tizen_policy EINA_UNUSED, uint32_t child_id EINA_UNUSED)
{
}

static void
_ecore_wl_cb_scr_mode_done(void *data EINA_UNUSED, struct tizen_policy *tizen_policy EINA_UNUSED, struct wl_surface *surface EINA_UNUSED, uint32_t mode EINA_UNUSED, uint32_t state EINA_UNUSED)
{
}
