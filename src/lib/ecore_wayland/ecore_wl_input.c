#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/*
 * NB: Events that receive a 'serial' instead of timestamp
 *
 * input_device_attach (for pointer image)
 * input_device_button_event (button press/release)
 * input_device_key_press
 * input_device_pointer_enter
 * input_device_pointer_leave
 * input_device_keyboard_enter
 * input_device_keyboard_leave
 * input_device_touch_down
 * input_device_touch_up
 *
 **/

#include "ecore_wl_private.h"
#include <sys/mman.h>
#include <ctype.h>

/* FIXME: This gives BTN_LEFT/RIGHT/MIDDLE for linux systems ...
 *        What about other OSs ?? */
#ifdef __linux__
# include <linux/input.h>
#else
# define BTN_LEFT 0x110
# define BTN_RIGHT 0x111
# define BTN_MIDDLE 0x112
# define BTN_SIDE 0x113
# define BTN_EXTRA 0x114
# define BTN_FORWARD 0x115
# define BTN_BACK 0x116
#endif

typedef struct _Ecore_Wl_Mouse_Down_Info
{
   EINA_INLIST;
   int dev;
   int last_win;
   int last_last_win;
   int last_event_win;
   int last_last_event_win;
   int sx, sy;
   unsigned int last_time;
   unsigned int last_last_time;
   Eina_Bool did_double : 1;
   Eina_Bool did_triple : 1;
} Ecore_Wl_Mouse_Down_Info;

/* FIXME: This should be a global setting, used by wayland and X */
static double _ecore_wl_double_click_time = 0.25;
static Eina_Inlist *_ecore_wl_mouse_down_info_list = NULL;

/* local function prototypes */
static void _ecore_wl_input_seat_handle_capabilities(void *data, struct wl_seat *seat, enum wl_seat_capability caps);
static void _ecore_wl_input_seat_handle_name(void *data EINA_UNUSED, struct wl_seat *seat EINA_UNUSED, const char *name EINA_UNUSED);

static void _ecore_wl_input_cb_pointer_enter(void *data, struct wl_pointer *pointer EINA_UNUSED, unsigned int serial, struct wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy);
static void _ecore_wl_input_cb_pointer_leave(void *data, struct wl_pointer *pointer EINA_UNUSED, unsigned int serial, struct wl_surface *surface);
static void _ecore_wl_input_cb_pointer_motion(void *data, struct wl_pointer *pointer EINA_UNUSED, unsigned int timestamp, wl_fixed_t sx, wl_fixed_t sy);
static void _ecore_wl_input_cb_pointer_button(void *data, struct wl_pointer *pointer EINA_UNUSED, unsigned int serial, unsigned int timestamp, unsigned int button, unsigned int state);
static void _ecore_wl_input_cb_pointer_axis(void *data, struct wl_pointer *pointer EINA_UNUSED, unsigned int timestamp, unsigned int axis, wl_fixed_t value);
static void _ecore_wl_input_cb_pointer_frame(void *data, struct wl_callback *callback, unsigned int timestamp EINA_UNUSED);
static Eina_Bool _ecore_wl_input_keymap_update_send(Ecore_Wl_Input *input);
static void _ecore_wl_input_cb_keyboard_keymap(void *data, struct wl_keyboard *keyboard EINA_UNUSED, unsigned int format, int fd, unsigned int size);
static void _ecore_wl_input_cb_keyboard_enter(void *data, struct wl_keyboard *keyboard EINA_UNUSED, unsigned int serial, struct wl_surface *surface, struct wl_array *keys EINA_UNUSED);
static void _ecore_wl_input_cb_keyboard_leave(void *data, struct wl_keyboard *keyboard EINA_UNUSED, unsigned int serial, struct wl_surface *surface);
static void _ecore_wl_input_cb_keyboard_key(void *data, struct wl_keyboard *keyboard, unsigned int serial, unsigned int timestamp, unsigned int key, unsigned int state);
static void _ecore_wl_input_cb_keyboard_modifiers(void *data, struct wl_keyboard *keyboard EINA_UNUSED, unsigned int serial EINA_UNUSED, unsigned int depressed, unsigned int latched, unsigned int locked, unsigned int group);
static void _ecore_wl_input_cb_keyboard_repeat_setup(void *data, struct wl_keyboard *keyboard EINA_UNUSED, int32_t rate, int32_t delay);
static Eina_Bool _ecore_wl_input_cb_keyboard_repeat(void *data);
static void _ecore_wl_input_cb_touch_down(void *data, struct wl_touch *touch, unsigned int serial, unsigned int timestamp, struct wl_surface *surface EINA_UNUSED, int id EINA_UNUSED, wl_fixed_t x, wl_fixed_t y);
static void _ecore_wl_input_cb_touch_up(void *data, struct wl_touch *touch, unsigned int serial, unsigned int timestamp, int id EINA_UNUSED);
static void _ecore_wl_input_cb_touch_motion(void *data, struct wl_touch *touch EINA_UNUSED, unsigned int timestamp, int id, wl_fixed_t x, wl_fixed_t y);
static void _ecore_wl_input_cb_touch_frame(void *data EINA_UNUSED, struct wl_touch *touch EINA_UNUSED);
static void _ecore_wl_input_cb_touch_cancel(void *data EINA_UNUSED, struct wl_touch *touch EINA_UNUSED);
static void _ecore_wl_input_cb_data_offer(void *data, struct wl_data_device *data_device, struct wl_data_offer *offer);
static void _ecore_wl_input_cb_data_enter(void *data, struct wl_data_device *data_device, unsigned int timestamp, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y, struct wl_data_offer *offer);
static void _ecore_wl_input_cb_data_leave(void *data, struct wl_data_device *data_device);
static void _ecore_wl_input_cb_data_motion(void *data, struct wl_data_device *data_device, unsigned int timestamp, wl_fixed_t x, wl_fixed_t y);
static void _ecore_wl_input_cb_data_drop(void *data, struct wl_data_device *data_device);
static void _ecore_wl_input_cb_data_selection(void *data, struct wl_data_device *data_device, struct wl_data_offer *offer);

static void _ecore_wl_input_mouse_move_send(Ecore_Wl_Input *input, Ecore_Wl_Window *win, unsigned int timestamp, int device);
static void _ecore_wl_input_mouse_in_send(Ecore_Wl_Input *input, Ecore_Wl_Window *win, unsigned int timestamp);
static void _ecore_wl_input_mouse_out_send(Ecore_Wl_Input *input, Ecore_Wl_Window *win, unsigned int timestamp);
static void _ecore_wl_input_focus_in_send(Ecore_Wl_Input *input EINA_UNUSED, Ecore_Wl_Window *win, unsigned int timestamp);
static void _ecore_wl_input_focus_out_send(Ecore_Wl_Input *input EINA_UNUSED, Ecore_Wl_Window *win, unsigned int timestamp);
static void _ecore_wl_input_mouse_down_send(Ecore_Wl_Input *input, Ecore_Wl_Window *win, int device, unsigned int button, unsigned int timestamp);
static void _ecore_wl_input_mouse_up_send(Ecore_Wl_Input *input, Ecore_Wl_Window *win, int device, unsigned int button, unsigned int timestamp);
static void _ecore_wl_input_mouse_wheel_send(Ecore_Wl_Input *input, unsigned int axis, int value, unsigned int timestamp);
static Ecore_Wl_Mouse_Down_Info *_ecore_wl_mouse_down_info_get(int dev);
static void _ecore_wl_input_device_manager_cb_device_add(void *data EINA_UNUSED, struct tizen_input_device_manager *tizen_input_device_manager EINA_UNUSED, unsigned int serial EINA_UNUSED, const char *identifier, struct tizen_input_device *device, struct wl_seat *seat);
static void _ecore_wl_input_device_manager_cb_device_remove(void *data EINA_UNUSED, struct tizen_input_device_manager *tizen_input_device_manager EINA_UNUSED, unsigned int serial EINA_UNUSED, const char *identifier, struct tizen_input_device *device, struct wl_seat *seat);
static void _ecore_wl_input_device_manager_cb_error(void *data EINA_UNUSED, struct tizen_input_device_manager *tizen_input_device_manager EINA_UNUSED, uint32_t errorcode EINA_UNUSED);
static void _ecore_wl_input_device_manager_cb_block_expired(void *data EINA_UNUSED, struct tizen_input_device_manager *tizen_input_device_manager EINA_UNUSED);
static Ecore_Device *_ecore_wl_input_get_ecore_device(const char *name, Ecore_Device_Class clas);
static void _ecore_wl_input_device_cb_device_info(void *data, struct tizen_input_device *tizen_input_device EINA_UNUSED, const char *name, uint32_t clas, uint32_t subclas, struct wl_array *axes EINA_UNUSED);
static void _ecore_wl_input_device_cb_event_device(void *data, struct tizen_input_device *tizen_input_device EINA_UNUSED, unsigned int serial EINA_UNUSED, const char *name EINA_UNUSED, uint32_t time EINA_UNUSED);
static void _ecore_wl_input_device_cb_axis(void *data EINA_UNUSED, struct tizen_input_device *tizen_input_device EINA_UNUSED, uint32_t axis_type EINA_UNUSED, wl_fixed_t value EINA_UNUSED);

static void _ecore_wl_input_key_conversion_clean_up(void);
static void _ecore_wl_input_key_conversion_set(void);

/* static int _ecore_wl_input_keysym_to_string(unsigned int symbol, char *buffer, int len); */

/* wayland interfaces */
static const struct wl_pointer_listener pointer_listener =
{
   _ecore_wl_input_cb_pointer_enter,
   _ecore_wl_input_cb_pointer_leave,
   _ecore_wl_input_cb_pointer_motion,
   _ecore_wl_input_cb_pointer_button,
   _ecore_wl_input_cb_pointer_axis,
};

static const struct wl_keyboard_listener keyboard_listener =
{
   _ecore_wl_input_cb_keyboard_keymap,
   _ecore_wl_input_cb_keyboard_enter,
   _ecore_wl_input_cb_keyboard_leave,
   _ecore_wl_input_cb_keyboard_key,
   _ecore_wl_input_cb_keyboard_modifiers,
   _ecore_wl_input_cb_keyboard_repeat_setup,
};

static const struct wl_touch_listener touch_listener =
{
   _ecore_wl_input_cb_touch_down,
   _ecore_wl_input_cb_touch_up,
   _ecore_wl_input_cb_touch_motion,
   _ecore_wl_input_cb_touch_frame,
   _ecore_wl_input_cb_touch_cancel
};

static const struct wl_seat_listener _ecore_wl_seat_listener =
{
   _ecore_wl_input_seat_handle_capabilities,
   _ecore_wl_input_seat_handle_name
};

static const struct wl_data_device_listener _ecore_wl_data_listener =
{
   _ecore_wl_input_cb_data_offer,
   _ecore_wl_input_cb_data_enter,
   _ecore_wl_input_cb_data_leave,
   _ecore_wl_input_cb_data_motion,
   _ecore_wl_input_cb_data_drop,
   _ecore_wl_input_cb_data_selection
};

static const struct wl_callback_listener _ecore_wl_pointer_surface_listener =
{
   _ecore_wl_input_cb_pointer_frame
};

static const struct tizen_input_device_manager_listener _ecore_tizen_input_device_mgr_listener =
{
   _ecore_wl_input_device_manager_cb_device_add,
   _ecore_wl_input_device_manager_cb_device_remove,
   _ecore_wl_input_device_manager_cb_error,
   _ecore_wl_input_device_manager_cb_block_expired,
};

static const struct tizen_input_device_listener _ecore_tizen_input_device_listener =
{
   _ecore_wl_input_device_cb_device_info,
   _ecore_wl_input_device_cb_event_device,
   _ecore_wl_input_device_cb_axis,
};

/* local variables */
static int _pointer_x, _pointer_y;
static double _tizen_api_version = 0.0;
static int _back_key_lt_24 = 0;
static int _menu_key_lt_24 = 0;
static int _home_key_lt_24 = 0;
static int _num_back_key_latest = 0;
static int _num_menu_key_latest = 0;
static int _num_home_key_latest = 0;
static xkb_keycode_t *_back_key_latest = NULL;
static xkb_keycode_t *_menu_key_latest = NULL;
static xkb_keycode_t *_home_key_latest = NULL;

EAPI void
ecore_wl_input_grab(Ecore_Wl_Input *input, Ecore_Wl_Window *win, unsigned int button)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!input) return;
   input->grab = win;
   input->grab_button = button;
}

EAPI void
ecore_wl_input_ungrab(Ecore_Wl_Input *input)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!input) return;

   if ((input->grab) && (input->grab_button) && (input->grab_count))
     _ecore_wl_input_mouse_up_send(input, input->grab, 0, input->grab_button,
                                   input->grab_timestamp);

   input->grab = NULL;
   input->grab_button = 0;
   input->grab_count = 0;
}

/* NB: This function should be called just before shell move and shell resize
 * functions. Those requests will trigger a mouse/touch implicit grab on the
 * compositor that will prevent the respective mouse/touch up events being
 * released after the end of the operation. This function checks if such grab
 * is in place for those windows and, if so, emit the respective mouse up
 * event. It's a workaround to the fact that wayland doesn't inform the
 * application about this move or resize grab being finished.
 */
void
_ecore_wl_input_grab_release(Ecore_Wl_Input *input, Ecore_Wl_Window *win)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!input) return;
   if (input->grab != win) return;

   ecore_wl_input_ungrab(input);
}

static void
_pointer_update_stop(Ecore_Wl_Input *input)
{
   if (!input->cursor_timer) return;

   ecore_timer_del(input->cursor_timer);
   input->cursor_timer = NULL;
}

EAPI void
ecore_wl_input_pointer_set(Ecore_Wl_Input *input, struct wl_surface *surface, int hot_x, int hot_y)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!input) return;

   _pointer_update_stop(input);
   if (input->pointer)
     wl_pointer_set_cursor(input->pointer, input->pointer_enter_serial,
                           surface, hot_x, hot_y);
}

EAPI void
ecore_wl_input_cursor_size_set(Ecore_Wl_Input *input, const int size)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!input) return;

   input->cursor_size = size;

   EINA_SAFETY_ON_NULL_RETURN(input->display->wl.shm);

   if (input->display->cursor_theme)
     wl_cursor_theme_destroy(input->display->cursor_theme);

   input->display->cursor_theme =
     wl_cursor_theme_load(NULL, input->cursor_size, input->display->wl.shm);
}

EAPI void
ecore_wl_input_cursor_theme_name_set(Ecore_Wl_Input *input, const char *cursor_theme_name)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!input) return;

   eina_stringshare_replace(&input->cursor_theme_name, cursor_theme_name);

   EINA_SAFETY_ON_NULL_RETURN(input->display->wl.shm);

   if (input->display->cursor_theme)
     wl_cursor_theme_destroy(input->display->cursor_theme);
   input->display->cursor_theme =
     wl_cursor_theme_load(input->cursor_theme_name, input->cursor_size,
                          input->display->wl.shm);
}

EAPI struct xkb_keymap *
ecore_wl_input_keymap_get(Ecore_Wl_Input *input)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   EINA_SAFETY_ON_NULL_RETURN_VAL(input, NULL);

   return input->xkb.keymap;
}

static Eina_Bool
_ecore_wl_input_cursor_update(void *data)
{
   struct wl_cursor_image *cursor_image;
   struct wl_buffer *buffer;
   Ecore_Wl_Input *input = data;
   unsigned int delay;

   if ((!input) || (!input->cursor) || (!input->cursor_surface))
     return EINA_FALSE;

   cursor_image = input->cursor->images[input->cursor_current_index];
   if (!cursor_image) return ECORE_CALLBACK_RENEW;

   if ((buffer = wl_cursor_image_get_buffer(cursor_image)))
     {
        ecore_wl_input_pointer_set(input, input->cursor_surface,
                                   cursor_image->hotspot_x,
                                   cursor_image->hotspot_y);
        wl_surface_attach(input->cursor_surface, buffer, 0, 0);
        wl_surface_damage(input->cursor_surface, 0, 0,
                          cursor_image->width, cursor_image->height);
        wl_surface_commit(input->cursor_surface);

        if ((input->cursor->image_count > 1) && (!input->cursor_frame_cb))
          _ecore_wl_input_cb_pointer_frame(input, NULL, 0);
     }

   if (input->cursor->image_count <= 1)
     return ECORE_CALLBACK_CANCEL;

   delay = cursor_image->delay;
   input->cursor_current_index =
      (input->cursor_current_index + 1) % input->cursor->image_count;

   if (!input->cursor_timer)
     input->cursor_timer =
        ecore_timer_loop_add(delay / 1000.0,
                             _ecore_wl_input_cursor_update, input);
   else
     ecore_timer_interval_set(input->cursor_timer, delay / 1000.0);

   return ECORE_CALLBACK_RENEW;
}

EAPI void
ecore_wl_input_cursor_from_name_set(Ecore_Wl_Input *input, const char *cursor_name)
{
   struct wl_cursor *cursor;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!input) return;
   /* No pointer device. Don't need to set cursor and update it */
   if (!input->pointer) return;

   _pointer_update_stop(input);

   eina_stringshare_replace(&input->cursor_name, cursor_name);

   /* No cursor. Set to default Left Pointer */
   if (!cursor_name)
     eina_stringshare_replace(&input->cursor_name, "left_ptr");

   /* try to get this cursor from the theme */
   if (!(cursor = ecore_wl_cursor_get(input->cursor_name)))
     {
        /* if the theme does not have this cursor, default to left pointer */
        if (!(cursor = ecore_wl_cursor_get("left_ptr")))
          return;
     }

   input->cursor = cursor;

   if ((!cursor->images) || (!cursor->images[0]))
     {
        ecore_wl_input_pointer_set(input, NULL, 0, 0);
        return;
     }

   input->cursor_current_index = 0;

   _ecore_wl_input_cursor_update(input);
}

EAPI void
ecore_wl_input_cursor_default_restore(Ecore_Wl_Input *input)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!input) return;

   /* Restore to default wayland cursor */
   ecore_wl_input_cursor_from_name_set(input, "left_ptr");
}

/**
 * @since 1.8
 */
EAPI Ecore_Wl_Input *
ecore_wl_input_get(void)
{
   if (!_ecore_wl_disp)
     {
        ERR("Ecore_Wl_Display doesn't exist");
        return NULL;
     }
   if (!_ecore_wl_disp->input)
     {
        int loop_count = 5;
        while ((!_ecore_wl_disp->input) && (loop_count > 0))
          {
             if (!_ecore_wl_disp->wl.display)
               {
                  ERR("Wayland display doesn't exist");
                  return NULL;
               }
             wl_display_roundtrip(_ecore_wl_disp->wl.display);
             loop_count--;
          }
     }
   return _ecore_wl_disp->input;
}

/**
 * @since 1.8
 */
EAPI struct wl_seat *
ecore_wl_input_seat_get(Ecore_Wl_Input *input)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!input) return NULL;

   return input->seat;
}

/* local functions */
void
_ecore_wl_input_setup(Ecore_Wl_Input *input)
{
   char *temp;
   unsigned int cursor_size;
   char *cursor_theme_name;

   temp = getenv("ECORE_WL_CURSOR_SIZE");
   if (temp)
     cursor_size = atoi(temp);
   else
     cursor_size = ECORE_WL_DEFAULT_CURSOR_SIZE;
   ecore_wl_input_cursor_size_set(input, cursor_size);

   cursor_theme_name = getenv("ECORE_WL_CURSOR_THEME_NAME");
   ecore_wl_input_cursor_theme_name_set(input, cursor_theme_name);
}

void
_ecore_wl_input_add(Ecore_Wl_Display *ewd, unsigned int id)
{
   Ecore_Wl_Input *input;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(input = calloc(1, sizeof(Ecore_Wl_Input)))) return;

   input->display = ewd;
   input->pointer_focus = NULL;
   input->keyboard_focus = NULL;
   input->touch_focus = NULL;

   input->repeat.enabled = EINA_TRUE;
   input->repeat.rate = 0.025;
   input->repeat.delay = 0.4;

   if (ewd->wl.shm)
     _ecore_wl_input_setup(input);

   input->seat_version = 4;
   input->seat =
     wl_registry_bind(ewd->wl.registry, id, &wl_seat_interface, 4);
   ewd->inputs = eina_inlist_append(ewd->inputs, EINA_INLIST_GET(input));

   wl_seat_add_listener(input->seat,
                        &_ecore_wl_seat_listener, input);
   wl_seat_set_user_data(input->seat, input);

   wl_array_init(&input->data_types);

   if (ewd->wl.data_device_manager)
     {
        input->data_device =
          wl_data_device_manager_get_data_device(ewd->wl.data_device_manager,
                                                 input->seat);
        wl_data_device_add_listener(input->data_device,
                                    &_ecore_wl_data_listener, input);
     }

   ewd->input = input;
}

void
_ecore_wl_input_del(Ecore_Wl_Input *input)
{
   Ecore_Wl_Input_Device *dev;

   if (!input) return;

   _pointer_update_stop(input);

   if (input->cursor_name) eina_stringshare_del(input->cursor_name);
   input->cursor_name = NULL;
   eina_stringshare_replace(&input->cursor_theme_name, NULL);

   if (input->touch_focus)
     {
        input->touch_focus = NULL;
     }

   if (input->pointer_focus)
     {
        Ecore_Wl_Window *win = NULL;

        if ((win = input->pointer_focus))
          win->pointer_device = NULL;

        input->pointer_focus = NULL;
     }

   if (input->keyboard_focus)
     {
        Ecore_Wl_Window *win = NULL;

        if ((win = input->keyboard_focus))
          win->keyboard_device = NULL;

        input->keyboard_focus = NULL;
     }

   if (input->data_types.data)
     {
        char **t;

        wl_array_for_each(t, &input->data_types)
          free(*t);
        wl_array_release(&input->data_types);
     }

   if (input->data_source) wl_data_source_destroy(input->data_source);
   input->data_source = NULL;

   if (input->drag_source) _ecore_wl_dnd_del(input->drag_source);
   input->drag_source = NULL;

   if (input->selection_source) _ecore_wl_dnd_del(input->selection_source);
   input->selection_source = NULL;

   if (input->data_device) wl_data_device_destroy(input->data_device);

   if (input->xkb.state)
     xkb_state_unref(input->xkb.state);
   if (input->xkb.keymap)
     xkb_map_unref(input->xkb.keymap);

   if (input->cursor_surface)
     wl_surface_destroy(input->cursor_surface);
   input->cursor_surface = NULL;

   if (input->pointer)
     {
#ifdef WL_POINTER_RELEASE_SINCE_VERSION
        if (input->seat_version >= WL_POINTER_RELEASE_SINCE_VERSION)
          wl_pointer_release(input->pointer);
        else
#endif
        wl_pointer_destroy(input->pointer);
        input->pointer = NULL;
     }

   if (input->keyboard)
     {
#ifdef WL_KEYBOARD_RELEASE_SINCE_VERSION
        if (input->seat_version >= WL_KEYBOARD_RELEASE_SINCE_VERSION)
          wl_keyboard_release(input->keyboard);
        else
#endif
        wl_keyboard_destroy(input->keyboard);
        input->keyboard = NULL;
     }

   if (input->touch)
     {
#ifdef WL_TOUCH_RELEASE_SINCE_VERSION
        if (input->seat_version >= WL_TOUCH_RELEASE_SINCE_VERSION)
          wl_touch_release(input->touch);
        else
#endif
        wl_touch_destroy(input->touch);
        input->touch = NULL;
     }

   _ecore_wl_disp->inputs = eina_inlist_remove
      (_ecore_wl_disp->inputs, EINA_INLIST_GET(input));
   if (input->seat) wl_seat_destroy(input->seat);

   if (input->repeat.tmr) ecore_timer_del(input->repeat.tmr);
   input->repeat.tmr = NULL;

   EINA_LIST_FREE(input->devices, dev)
     {
        if (dev->tz_device) tizen_input_device_destroy(dev->tz_device);
        if (dev->name) eina_stringshare_del(dev->name);
        if (dev->identifier) eina_stringshare_del(dev->identifier);
        dev->seat = NULL;
        free(dev);
     }

   eina_stringshare_replace(&input->last_device_name, NULL);
   eina_stringshare_replace(&input->last_device_name_kbd, NULL);

   free(input);
}

void
_ecore_wl_input_pointer_xy_get(int *x, int *y)
{
   if (x) *x = _pointer_x;
   if (y) *y = _pointer_y;
}

static void
_ecore_wl_input_seat_handle_capabilities(void *data, struct wl_seat *seat, enum wl_seat_capability caps)
{
   Ecore_Wl_Input *input;

   if (!(input = data)) return;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if ((caps & WL_SEAT_CAPABILITY_POINTER) && (!input->pointer))
     {
        input->pointer = wl_seat_get_pointer(seat);
        wl_pointer_set_user_data(input->pointer, input);
        wl_pointer_add_listener(input->pointer, &pointer_listener, input);

        if (!input->cursor_surface)
          {
             input->cursor_surface =
               wl_compositor_create_surface(_ecore_wl_disp->wl.compositor);
          }
     }
   else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && (input->pointer))
     {
        if (input->cursor_surface) wl_surface_destroy(input->cursor_surface);
        input->cursor_surface = NULL;
#ifdef WL_POINTER_RELEASE_SINCE_VERSION
        if (input->seat_version >= WL_POINTER_RELEASE_SINCE_VERSION)
          wl_pointer_release(input->pointer);
        else
#endif
        wl_pointer_destroy(input->pointer);
        input->pointer = NULL;
     }

   if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && (!input->keyboard))
     {
        input->keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_set_user_data(input->keyboard, input);
        wl_keyboard_add_listener(input->keyboard, &keyboard_listener, input);
     }
   else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && (input->keyboard))
     {
#ifdef WL_KEYBOARD_RELEASE_SINCE_VERSION
        if (input->seat_version >= WL_KEYBOARD_RELEASE_SINCE_VERSION)
          wl_keyboard_release(input->keyboard);
        else
#endif
        wl_keyboard_destroy(input->keyboard);
        input->keyboard = NULL;
        _ecore_wl_input_key_conversion_clean_up();
     }

   if ((caps & WL_SEAT_CAPABILITY_TOUCH) && (!input->touch))
     {
        input->touch = wl_seat_get_touch(seat);
        if (input->touch)
          {
             wl_touch_set_user_data(input->touch, input);
             wl_touch_add_listener(input->touch, &touch_listener, input);
          }
     }
   else if (!(caps & WL_SEAT_CAPABILITY_TOUCH) && (input->touch))
     {
#ifdef WL_TOUCH_RELEASE_SINCE_VERSION
        if (input->seat_version >= WL_TOUCH_RELEASE_SINCE_VERSION)
          wl_touch_release(input->touch);
        else
#endif
        wl_touch_destroy(input->touch);
        input->touch = NULL;
     }

   input->caps_update = EINA_TRUE;
}

static void
_ecore_wl_input_seat_handle_name(void *data EINA_UNUSED, struct wl_seat *seat EINA_UNUSED, const char *name EINA_UNUSED)
{
   /* We don't care about the name. */
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
}

static void
_ecore_wl_input_cb_pointer_motion(void *data, struct wl_pointer *pointer EINA_UNUSED, unsigned int timestamp, wl_fixed_t sx, wl_fixed_t sy)
{
   Ecore_Wl_Input *input;

   /* LOGFN(__FILE__, __LINE__, __FUNCTION__); */

   if (!(input = data)) return;

   _pointer_x = input->sx = wl_fixed_to_int(sx);
   _pointer_y = input->sy = wl_fixed_to_int(sy);

   input->timestamp = timestamp;

   if (input->pointer_focus)
     _ecore_wl_input_mouse_move_send(input, input->pointer_focus, timestamp, 0);
}

static void
_ecore_wl_input_cb_pointer_button(void *data, struct wl_pointer *pointer EINA_UNUSED, unsigned int serial, unsigned int timestamp, unsigned int button, unsigned int state)
{
   Ecore_Wl_Input *input;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(input = data)) return;

   input->timestamp = timestamp;
   input->display->serial = serial;

//   _ecore_wl_input_mouse_move_send(input, input->pointer_focus, timestamp);

   if (state)
     {
        if ((input->pointer_focus) && (!input->grab) && (!input->grab_count))
          {
             ecore_wl_input_grab(input, input->pointer_focus, button);
             input->grab_timestamp = timestamp;
          }

        if (input->pointer_focus)
          _ecore_wl_input_mouse_down_send(input, input->pointer_focus,
                                          0, button, timestamp);
        input->grab_count++;
     }
   else
     {
        if (input->pointer_focus)
          _ecore_wl_input_mouse_up_send(input, input->pointer_focus,
                                        0, button, timestamp);

        if (input->grab_count) input->grab_count--;
        if ((input->grab) && (input->grab_button == button) &&
            (!state) && (!input->grab_count))
          ecore_wl_input_ungrab(input);
     }

//   _ecore_wl_input_mouse_move_send(input, timestamp);
}

static void
_ecore_wl_input_cb_pointer_axis(void *data, struct wl_pointer *pointer EINA_UNUSED, unsigned int timestamp, unsigned int axis, wl_fixed_t value)
{
   Ecore_Wl_Input *input;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(input = data)) return;
   _ecore_wl_input_mouse_wheel_send(input, axis, wl_fixed_to_int(value),
                                    timestamp);
}

static void
_ecore_wl_input_cb_pointer_frame(void *data, struct wl_callback *callback, unsigned int timestamp EINA_UNUSED)
{
   Ecore_Wl_Input *input;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(input = data)) return;

   if (callback)
     {
        if (callback != input->cursor_frame_cb) return;
        wl_callback_destroy(callback);
        input->cursor_frame_cb = NULL;
     }

   if (!input->cursor_name)
     {
        ecore_wl_input_pointer_set(input, NULL, 0, 0);
        return;
     }

   if ((input->cursor->image_count > 1) && (!input->cursor_frame_cb))
     {
        input->cursor_frame_cb = wl_surface_frame(input->cursor_surface);
        if (!input->cursor_frame_cb) return;

        wl_callback_add_listener(input->cursor_frame_cb,
                                 &_ecore_wl_pointer_surface_listener, input);
     }
}

static Eina_Bool
_ecore_wl_input_keymap_update_send(Ecore_Wl_Input *input)
{
   Ecore_Wl_Event_Keymap_Update *ev = NULL;

   if (!input || !(input->xkb.keymap)) return EINA_FALSE;

   /* allocate space for event structure */
   ev = calloc(1, sizeof(Ecore_Wl_Event_Keymap_Update));
   if (!ev) return EINA_FALSE;

   ev->input = input;
   ev->keymap = input->xkb.keymap;

   /* raise an event saying the keymap has been updated */
   ecore_event_add(ECORE_WL_EVENT_KEYMAP_UPDATE, ev, NULL, NULL);

   return EINA_TRUE;
}

static void
_ecore_wl_input_cb_keyboard_keymap(void *data, struct wl_keyboard *keyboard EINA_UNUSED, unsigned int format, int fd, unsigned int size)
{
   Ecore_Wl_Input *input;
   char *map = NULL;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(input = data))
     {
        close(fd);
        return;
     }

   if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
     {
        close(fd);
        return;
     }

   map = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
   if (map == MAP_FAILED)
     {
        close(fd);
        return;
     }

   input->xkb.keymap =
     xkb_map_new_from_string(input->display->xkb.context, map,
                             XKB_KEYMAP_FORMAT_TEXT_V1, 0);

   munmap(map, size);
   close(fd);

   if (!(input->xkb.keymap)) return;
   if (!(input->xkb.state = xkb_state_new(input->xkb.keymap)))
     {
        xkb_map_unref(input->xkb.keymap);
        input->xkb.keymap = NULL;
        return;
     }


   // TIZEN ONLY(20160223) : Add back/menu/home key conversion support
   _tizen_api_version = 0.0;

   input->xkb.control_mask =
     1 << xkb_map_mod_get_index(input->xkb.keymap, XKB_MOD_NAME_CTRL);
   input->xkb.alt_mask =
     1 << xkb_map_mod_get_index(input->xkb.keymap, XKB_MOD_NAME_ALT);
   input->xkb.shift_mask =
     1 << xkb_map_mod_get_index(input->xkb.keymap, XKB_MOD_NAME_SHIFT);
   input->xkb.win_mask =
     1 << xkb_map_mod_get_index(input->xkb.keymap, XKB_MOD_NAME_LOGO);
   input->xkb.scroll_mask =
     1 << xkb_map_mod_get_index(input->xkb.keymap, XKB_LED_NAME_SCROLL);
   input->xkb.num_mask =
     1 << xkb_map_mod_get_index(input->xkb.keymap, XKB_LED_NAME_NUM);
   input->xkb.caps_mask =
     1 << xkb_map_mod_get_index(input->xkb.keymap, XKB_MOD_NAME_CAPS);
   input->xkb.altgr_mask =
     1 << xkb_map_mod_get_index(input->xkb.keymap, "ISO_Level3_Shift");

   if (!_ecore_wl_input_keymap_update_send(input))
     {
        xkb_map_unref(input->xkb.keymap);
        input->xkb.keymap = NULL;
        return;
     }
}

static int
_ecore_wl_input_keymap_translate_keysym(xkb_keysym_t keysym, unsigned int modifiers, char *buffer, int bytes)
{
   unsigned long hbytes = 0;
   unsigned char c;

   if (!keysym) return 0;
   hbytes = (keysym >> 8);

   if (!(bytes &&
         ((hbytes == 0) ||
          ((hbytes == 0xFF) &&
           (((keysym >= XKB_KEY_BackSpace) && (keysym <= XKB_KEY_Clear)) ||
            (keysym == XKB_KEY_Return) || (keysym == XKB_KEY_Escape) ||
            (keysym == XKB_KEY_KP_Space) || (keysym == XKB_KEY_KP_Tab) ||
            (keysym == XKB_KEY_KP_Enter) ||
            ((keysym >= XKB_KEY_KP_Multiply) && (keysym <= XKB_KEY_KP_9)) ||
            (keysym == XKB_KEY_KP_Equal) || (keysym == XKB_KEY_Delete))))))
     return 0;

   if (keysym == XKB_KEY_KP_Space)
     c = (XKB_KEY_space & 0x7F);
   else if (hbytes == 0xFF)
     c = (keysym & 0x7F);
   else
     c = (keysym & 0xFF);

   if (modifiers & ECORE_EVENT_MODIFIER_CTRL)
     {
        if (((c >= '@') && (c < '\177')) || c == ' ')
          c &= 0x1F;
        else if (c == '2')
          c = '\000';
        else if ((c >= '3') && (c <= '7'))
          c -= ('3' - '\033');
        else if (c == '8')
          c = '\177';
        else if (c == '/')
          c = '_' & 0x1F;
     }
   buffer[0] = c;
   return 1;
}

static int
_ecore_wl_input_convert_old_keys(unsigned int code)
{
   int i;

   // TIZEN ONLY(20160608) : Add option for key conversion
   const char *tmp;
   tmp = getenv("ECORE_WL_INPUT_KEY_CONVERSION_DISABLE");
   if (tmp && atoi(tmp)) return code;
   //

   for (i = 0; i < _num_back_key_latest; i++)
     {
        if (code == _back_key_latest[i])
          {
             return _back_key_lt_24;
          }
     }

   for (i=0; i<_num_menu_key_latest; i++)
     {
        if (code == _menu_key_latest[i])
          {
             return _menu_key_lt_24;
          }
     }

   for (i=0; i<_num_home_key_latest; i++)
     {
        if (code == _home_key_latest[i])
          {
             return _home_key_lt_24;
          }
     }

   return code;
}

static void
_ecore_wl_input_cb_keyboard_key(void *data, struct wl_keyboard *keyboard, unsigned int serial, unsigned int timestamp, unsigned int keycode, unsigned int state)
{
   Ecore_Wl_Input *input;
   Ecore_Wl_Window *win;
   unsigned int code, nsyms;
   const xkb_keysym_t *syms;
   xkb_keysym_t sym = XKB_KEY_NoSymbol;
   char key[256], keyname[256], compose[256];
   Ecore_Event_Key *e;

   struct wl_surface *surface = NULL;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(input = data)) return;

   win = input->keyboard_focus;
// TIZEN_ONLY(20150911): Deal with key event if window is not exist.
//   if ((!win) || (win->keyboard_device != input) || (!input->xkb.state))
//     return;
//
//   input->display->serial = serial;
//
   /* xkb rules reflect X broken keycodes, so offset by 8 */
   code = keycode + 8;

// TIZEN ONLY(20160223) : Add back/menu/home key conversion support
   if (_tizen_api_version == 0.0) _ecore_wl_input_key_conversion_set();

// TIZEN_ONLY(20160223) : Do keycode conversion for the back/menu/home key(s),
// if it is one of the back/menu/home key and _tizen_api_version is less than 2.4.
   if (0.0 < _tizen_api_version && _tizen_api_version < 2.4)
      code = _ecore_wl_input_convert_old_keys(code);

   INF("Key[%d] event occurs", code);

   if (!win)
     {
        INF("window is not focused");
        surface = (struct wl_surface *) eina_hash_find(_ecore_wl_keygrab_hash_get(), &code);
        if (surface)
          {
             win = ecore_wl_window_surface_find(surface);
             INF("keycode(%d) is grabbed in the window(%p)", code, win);
          }
        else
          {
             //key event callback can be called even though surface is not exist.
             //TODO: Ecore_Event_Key have event_window info, so if (surface == NULL), we should generate proper window info
             INF("surface is not exist");
          }
     }
   else
     {
        if ((win->keyboard_device != input))
          {
             INF("window(%p) is focused, but keyboard device info is wrong", win);
             return;
          }
     }

   if (!input->xkb.state)
     {
        WRN("xkb state is wrong");
        return;
     }

   input->display->serial = serial;

   /* get the keysym for this key code */
   nsyms = xkb_key_get_syms(input->xkb.state, code, &syms);
   /* no valid keysym available: reject */
   if (!nsyms) return;
   if (nsyms == 1) sym = syms[0];

   /* get the name of this keysym */
   memset(key, 0, sizeof(key));
   xkb_keysym_get_name(sym, key, sizeof(key));

   memset(keyname, 0, sizeof(keyname));
   memcpy(keyname, key, sizeof(keyname));

   if (keyname[0] == '\0')
     snprintf(keyname, sizeof(keyname), "Keycode-%u", code);

   /* if shift is active, we need to transform the key to lower */
   if (xkb_state_mod_index_is_active(input->xkb.state,
                                     xkb_map_mod_get_index(input->xkb.keymap,
                                                           XKB_MOD_NAME_SHIFT),
                                     XKB_STATE_MODS_EFFECTIVE))
     {
        if (keyname[0] != '\0')
          keyname[0] = tolower(keyname[0]);
     }

   memset(compose, 0, sizeof(compose));
   _ecore_wl_input_keymap_translate_keysym(sym, input->modifiers,
                                           compose, sizeof(compose));

   e = calloc(1, sizeof(Ecore_Event_Key) + strlen(key) + strlen(keyname) +
              ((compose[0] != '\0') ? strlen(compose) : 0) + 3);
   if (!e) return;

   e->keyname = (char *)(e + 1);
   e->key = e->keyname + strlen(keyname) + 1;
   e->compose = strlen(compose) ? e->key + strlen(key) + 1 : NULL;
   e->string = e->compose;

   strcpy((char *)e->keyname, keyname);
   strcpy((char *)e->key, key);
   if (strlen(compose)) strcpy((char *)e->compose, compose);

   // TIZEN_ONLY(20150911): Deal with key event if window is not exist.
   if (win)
     {
   //
        e->window = win->id;
        e->event_window = win->id;
   // TIZEN_ONLY(20150911): Deal with key event if window is not exist.
     }
   else
     {
        e->window = (uintptr_t)NULL;
        e->event_window = (uintptr_t)NULL;
     }
   //
   e->timestamp = timestamp;
   e->modifiers = input->modifiers;
   e->keycode = code;
   if (keyboard && (input->last_device_class == ECORE_DEVICE_CLASS_KEYBOARD))
     eina_stringshare_replace(&input->last_device_name_kbd, input->last_device_name);

   e->dev = _ecore_wl_input_get_ecore_device(input->last_device_name_kbd, ECORE_DEVICE_CLASS_KEYBOARD);

   if (state)
     ecore_event_add(ECORE_EVENT_KEY_DOWN, e, NULL, NULL);
   else
     ecore_event_add(ECORE_EVENT_KEY_UP, e, NULL, NULL);

   if (!xkb_keymap_key_repeats(input->xkb.keymap, code)) return;

   if ((!state) && (keycode == input->repeat.key))
     {
        input->repeat.sym = 0;
        input->repeat.key = 0;
        input->repeat.time = 0;

        if (input->repeat.tmr) ecore_timer_del(input->repeat.tmr);
        input->repeat.tmr = NULL;
     }
   else if ((state) && (keycode != input->repeat.key))
     {
        if (!input->repeat.enabled) return;

        input->repeat.sym = sym;
        input->repeat.key = keycode;
        input->repeat.time = timestamp;

        if (!input->repeat.tmr)
          {
             input->repeat.tmr =
               ecore_timer_add(input->repeat.rate,
                               _ecore_wl_input_cb_keyboard_repeat, input);
          }
        ecore_timer_delay(input->repeat.tmr, input->repeat.delay);
     }
}

static void
_ecore_wl_input_cb_keyboard_modifiers(void *data, struct wl_keyboard *keyboard EINA_UNUSED, unsigned int serial EINA_UNUSED, unsigned int depressed, unsigned int latched, unsigned int locked, unsigned int group)
{
   Ecore_Wl_Input *input;
   xkb_mod_mask_t mask;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(input = data)) return;

   if (!input->xkb.keymap) return;

   xkb_state_update_mask(input->xkb.state,
                         depressed, latched, locked, 0, 0, group);

   mask = xkb_state_serialize_mods(input->xkb.state,
                                   (XKB_STATE_DEPRESSED | XKB_STATE_LATCHED));

   input->modifiers = 0;
   if (mask & input->xkb.control_mask)
     input->modifiers |= ECORE_EVENT_MODIFIER_CTRL;
   if (mask & input->xkb.alt_mask)
     input->modifiers |= ECORE_EVENT_MODIFIER_ALT;
   if (mask & input->xkb.shift_mask)
     input->modifiers |= ECORE_EVENT_MODIFIER_SHIFT;
   if (mask & input->xkb.win_mask)
     input->modifiers |= ECORE_EVENT_MODIFIER_WIN;
   if (mask & input->xkb.scroll_mask)
     input->modifiers |= ECORE_EVENT_LOCK_SCROLL;
   if (mask & input->xkb.num_mask)
     input->modifiers |= ECORE_EVENT_LOCK_NUM;
   if (mask & input->xkb.caps_mask)
     input->modifiers |= ECORE_EVENT_LOCK_CAPS;
   if (mask & input->xkb.altgr_mask)
     input->modifiers |= ECORE_EVENT_MODIFIER_ALTGR;
}

static void
_ecore_wl_input_cb_keyboard_repeat_setup(void *data, struct wl_keyboard *keyboard EINA_UNUSED, int32_t rate, int32_t delay)
{
   Ecore_Wl_Input *input;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(input = data)) return;

   if (rate == 0)
     {
        input->repeat.enabled = EINA_FALSE;
        return;
     }
   else
     input->repeat.enabled = EINA_TRUE;

   input->repeat.rate = (rate / 1000.0);
   input->repeat.delay = (delay / 1000.0);
}

static Eina_Bool
_ecore_wl_input_cb_keyboard_repeat(void *data)
{
   Ecore_Wl_Input *input;
   Ecore_Wl_Window *win = NULL;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(input = data)) return ECORE_CALLBACK_RENEW;

   // TIZEN_ONLY(20160610): fix key repeat condition. 
   _ecore_wl_input_cb_keyboard_key(input, NULL, input->display->serial,
                                   input->repeat.time,
                                   input->repeat.key, EINA_TRUE);
   //
   return ECORE_CALLBACK_RENEW;
}

static void
_ecore_wl_input_cb_pointer_enter(void *data, struct wl_pointer *pointer EINA_UNUSED, unsigned int serial, struct wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy)
{
   Ecore_Wl_Input *input;
   Ecore_Wl_Window *win = NULL;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!surface) return;
   if (!(input = data)) return;

   if (!input->timestamp)
     {
        struct timeval tv;

        gettimeofday(&tv, NULL);
        input->timestamp = (tv.tv_sec * 1000 + tv.tv_usec / 1000);
     }

   input->sx = wl_fixed_to_double(sx);
   input->sy = wl_fixed_to_double(sy);
   input->display->serial = serial;
   input->pointer_enter_serial = serial;

   /* The cursor on the surface is undefined until we set it */
   ecore_wl_input_cursor_from_name_set(input, input->cursor_name);

   if ((win = ecore_wl_window_surface_find(surface)))
     {
        win->pointer_device = input;
        input->pointer_focus = win;

        if (win->pointer.set)
          {
             ecore_wl_input_pointer_set(input, win->pointer.surface,
                                        win->pointer.hot_x, win->pointer.hot_y);
          }
        /* NB: Commented out for now. Not needed in most circumstances,
         * but left here for any corner-cases */
        /* else */
        /*   { */
        /*      _ecore_wl_input_cursor_update(input); */
        /*   } */

        _ecore_wl_input_mouse_in_send(input, win, input->timestamp);
     }
}

static void
_ecore_wl_input_cb_pointer_leave(void *data, struct wl_pointer *pointer EINA_UNUSED, unsigned int serial, struct wl_surface *surface)
{
   Ecore_Wl_Input *input;
   Ecore_Wl_Window *win;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!surface) return;
   if (!(input = data)) return;

   input->display->serial = serial;
   input->pointer_focus = NULL;

   /* NB: Commented out for now. Not needed in most circumstances, but left
    * here for any corner-cases */
   /* _ecore_wl_input_cursor_update(input); */

   if (!(win = ecore_wl_window_surface_find(surface))) return;

   win->pointer_device = NULL;

   /* _ecore_wl_input_mouse_move_send(input, win, input->timestamp); */
   _ecore_wl_input_mouse_out_send(input, win, input->timestamp);

   if (input->grab)
     {
        /* move or resize started */

        /* printf("Pointer Leave WITH a Grab\n"); */
     }
}

static void
_ecore_wl_input_cb_keyboard_enter(void *data, struct wl_keyboard *keyboard EINA_UNUSED, unsigned int serial, struct wl_surface *surface, struct wl_array *keys EINA_UNUSED)
{
   Ecore_Wl_Input *input;
   Ecore_Wl_Window *win = NULL;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!surface) return;
   if (!(input = data)) return;

   if (!input->timestamp)
     {
        struct timeval tv;

        gettimeofday(&tv, NULL);
        input->timestamp = (tv.tv_sec * 1000 + tv.tv_usec / 1000);
     }

   input->display->serial = serial;

   if (!(win = ecore_wl_window_surface_find(surface))) return;

   win->keyboard_device = input;
   input->keyboard_focus = win;

   _ecore_wl_input_focus_in_send(input, win, input->timestamp);
}

static void
_ecore_wl_input_cb_keyboard_leave(void *data, struct wl_keyboard *keyboard EINA_UNUSED, unsigned int serial, struct wl_surface *surface)
{
   Ecore_Wl_Input *input;
   Ecore_Wl_Window *win;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!surface) return;
   if (!(input = data)) return;
// TIZEN_ONLY(20160615): Fix key repeat logic. 
//   input->repeat.sym = 0;
//   input->repeat.key = 0;
//   input->repeat.time = 0;
//   if (input->repeat.tmr) ecore_timer_del(input->repeat.tmr);
//   input->repeat.tmr = NULL;
//

   input->keyboard_focus = NULL;

   if (!input->timestamp)
     {
        struct timeval tv;

        gettimeofday(&tv, NULL);
        input->timestamp = (tv.tv_sec * 1000 + tv.tv_usec / 1000);
     }

   input->display->serial = serial;

   if (!(win = ecore_wl_window_surface_find(surface))) return;

   win->keyboard_device = NULL;
   _ecore_wl_input_focus_out_send(input, win, input->timestamp);
}

static void
_ecore_wl_input_touch_axis_process(Ecore_Wl_Input *input, int id)
{
   if (id >= ECORE_WL_TOUCH_MAX)
      return;

   if (input->last_radius_x)
     {
        input->touch_axis[id].radius_x = input->last_radius_x;
        input->last_radius_x = 0.0;
     }
   if (input->last_radius_y)
     {
        input->touch_axis[id].radius_y = input->last_radius_y;
        input->last_radius_y = 0.0;
     }
   if (input->last_pressure)
     {
        input->touch_axis[id].pressure = input->last_pressure;
        input->last_pressure = 0.0;
     }
   if (input->last_angle)
     {
        input->touch_axis[id].angle = input->last_angle;
        input->last_angle = 0.0;
     }
}

static double
_ecore_wl_input_touch_radius_calc(double x, double y)
{
#define PI 3.14159265358979323846
   return x*y*PI;
}

static void
_ecore_wl_input_cb_touch_down(void *data, struct wl_touch *touch EINA_UNUSED, unsigned int serial, unsigned int timestamp, struct wl_surface *surface, int id, wl_fixed_t x, wl_fixed_t y)
{
   Ecore_Wl_Input *input;
   Ecore_Wl_Window *win;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!surface) return;
   if (!(input = data)) return;

   if (!(win = ecore_wl_window_surface_find(surface))) return;

   input->timestamp = timestamp;
   input->display->serial = serial;
   input->sx = wl_fixed_to_int(x);
   input->sy = wl_fixed_to_int(y);
   _ecore_wl_input_touch_axis_process(input, id);

   if (input->touch_focus != win)
     {
        input->touch_focus = win;
        _ecore_wl_input_mouse_move_send(input, input->touch_focus, timestamp, id);
     }

   if (!input->grab_count)
     {
      _ecore_wl_input_cb_pointer_enter(data, NULL, serial, surface, x, y);
      if ((input->touch_focus) && (!input->grab))
        {
           ecore_wl_input_grab(input, input->touch_focus, BTN_LEFT);
           input->grab_timestamp = timestamp;
        }
     }

   _ecore_wl_input_mouse_down_send(input, input->touch_focus,
                                   id, BTN_LEFT, timestamp);

   input->grab_count++;
}

static void
_ecore_wl_input_cb_touch_up(void *data, struct wl_touch *touch EINA_UNUSED, unsigned int serial, unsigned int timestamp, int id)
{
   Ecore_Wl_Input *input;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(input = data)) return;
   if (!input->touch_focus) return;

   input->timestamp = timestamp;
   input->display->serial = serial;

   _ecore_wl_input_mouse_up_send(input, input->touch_focus, id, BTN_LEFT, timestamp);
   if (input->grab_count) input->grab_count--;
   if ((input->grab) && (input->grab_button == BTN_LEFT) &&
       (!input->grab_count))
     ecore_wl_input_ungrab(input);
}

static void
_ecore_wl_input_cb_touch_motion(void *data, struct wl_touch *touch EINA_UNUSED, unsigned int timestamp, int id, wl_fixed_t x, wl_fixed_t y)
{
   Ecore_Wl_Input *input;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(input = data)) return;
   if (!input->touch_focus) return;

   input->timestamp = timestamp;
   input->sx = wl_fixed_to_int(x);
   input->sy = wl_fixed_to_int(y);
   _ecore_wl_input_touch_axis_process(input, id);

   _ecore_wl_input_mouse_move_send(input, input->touch_focus, timestamp, id);
}

static void
_ecore_wl_input_cb_touch_frame(void *data EINA_UNUSED, struct wl_touch *touch EINA_UNUSED)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
}

static void
_ecore_wl_input_cb_touch_cancel(void *data EINA_UNUSED, struct wl_touch *touch EINA_UNUSED)
{
   Ecore_Event_Mouse_Button *ev;
   Ecore_Wl_Input *input;
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(input = data)) return;
   if (!input->touch_focus) return;

   if (!(ev = calloc(1, sizeof(Ecore_Event_Mouse_Button)))) return;
   EINA_SAFETY_ON_NULL_RETURN(ev);

   ev->timestamp = (int)(ecore_time_get()*1000);
   ev->same_screen = 1;
   ev->window = input->touch_focus->id;
   ev->event_window = input->touch_focus->id;

   ecore_event_add(ECORE_EVENT_MOUSE_BUTTON_CANCEL, ev, NULL, NULL);
}

static void
_ecore_wl_input_cb_data_offer(void *data, struct wl_data_device *data_device, struct wl_data_offer *offer)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   _ecore_wl_dnd_add(data, data_device, offer);
}

static void
_ecore_wl_input_cb_data_enter(void *data, struct wl_data_device *data_device, unsigned int timestamp, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y, struct wl_data_offer *offer)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!surface) return;

   _ecore_wl_dnd_enter(data, data_device, timestamp, surface, x, y, offer);
}

static void
_ecore_wl_input_cb_data_leave(void *data, struct wl_data_device *data_device)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   _ecore_wl_dnd_leave(data, data_device);
}

static void
_ecore_wl_input_cb_data_motion(void *data, struct wl_data_device *data_device, unsigned int timestamp, wl_fixed_t x, wl_fixed_t y)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   _ecore_wl_dnd_motion(data, data_device, timestamp, x, y);
}

static void
_ecore_wl_input_cb_data_drop(void *data, struct wl_data_device *data_device)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   _ecore_wl_dnd_drop(data, data_device);
}

static void
_ecore_wl_input_cb_data_selection(void *data, struct wl_data_device *data_device, struct wl_data_offer *offer)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   _ecore_wl_dnd_selection(data, data_device, offer);
}

static void
_ecore_wl_input_mouse_move_send(Ecore_Wl_Input *input, Ecore_Wl_Window *win, unsigned int timestamp, int device)
{
   Ecore_Event_Mouse_Move *ev;
   Ecore_Wl_Mouse_Down_Info *down_info;

   /* LOGFN(__FILE__, __LINE__, __FUNCTION__); */

   if (!(ev = calloc(1, sizeof(Ecore_Event_Mouse_Move)))) return;

   ev->timestamp = timestamp;
   ev->x = input->sx;
   ev->y = input->sy;
   ev->root.x = input->sx;
   ev->root.y = input->sy;
   ev->modifiers = input->modifiers;
   ev->multi.device = device;
   if (device >= ECORE_WL_TOUCH_MAX)
     {
        ev->multi.radius = 1.0;
        ev->multi.radius_x = 1.0;
        ev->multi.radius_y = 1.0;
        ev->multi.pressure = 1.0;
        ev->multi.angle = 0.0;
     }
   else
     {
        ev->multi.radius =
           _ecore_wl_input_touch_radius_calc(input->touch_axis[device].radius_x,
                                             input->touch_axis[device].radius_y);
        ev->multi.radius_x = input->touch_axis[device].radius_x;
        ev->multi.radius_y = input->touch_axis[device].radius_y;
        ev->multi.pressure = input->touch_axis[device].pressure;
        ev->multi.angle = input->touch_axis[device].angle;
     }
   ev->multi.x = input->sx;
   ev->multi.y = input->sy;
   ev->multi.root.x = input->sx;
   ev->multi.root.y = input->sy;
   ev->dev = _ecore_wl_input_get_ecore_device(input->last_device_name, input->last_device_class);

   if ((down_info = _ecore_wl_mouse_down_info_get(device)))
     {
        down_info->sx = input->sx;
        down_info->sy = input->sy;
     }

   if (win)
     {
        ev->window = win->id;
        ev->event_window = win->id;
     }

   ecore_event_add(ECORE_EVENT_MOUSE_MOVE, ev, NULL, NULL);
}

static void
_ecore_wl_input_mouse_in_send(Ecore_Wl_Input *input, Ecore_Wl_Window *win, unsigned int timestamp)
{
   Ecore_Event_Mouse_IO *ev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(ev = calloc(1, sizeof(Ecore_Event_Mouse_IO)))) return;

   ev->x = input->sx;
   ev->y = input->sy;
   ev->modifiers = input->modifiers;
   ev->timestamp = timestamp;
   ev->dev = _ecore_wl_input_get_ecore_device(input->last_device_name, input->last_device_class);

   if (win)
     {
        ev->window = win->id;
        ev->event_window = win->id;
     }

   ecore_event_add(ECORE_EVENT_MOUSE_IN, ev, NULL, NULL);
}

static void
_ecore_wl_input_mouse_out_send(Ecore_Wl_Input *input, Ecore_Wl_Window *win, unsigned int timestamp)
{
   Ecore_Event_Mouse_IO *ev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(ev = calloc(1, sizeof(Ecore_Event_Mouse_IO)))) return;

   ev->x = input->sx;
   ev->y = input->sy;
   ev->modifiers = input->modifiers;
   ev->timestamp = timestamp;
   ev->dev = _ecore_wl_input_get_ecore_device(input->last_device_name, input->last_device_class);

   if (win)
     {
        ev->window = win->id;
        ev->event_window = win->id;
     }

   ecore_event_add(ECORE_EVENT_MOUSE_OUT, ev, NULL, NULL);
}

static void
_ecore_wl_input_focus_in_send(Ecore_Wl_Input *input EINA_UNUSED, Ecore_Wl_Window *win, unsigned int timestamp)
{
   Ecore_Wl_Event_Focus_In *ev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(ev = calloc(1, sizeof(Ecore_Wl_Event_Focus_In)))) return;
   ev->timestamp = timestamp;
   if (win) ev->win = win->id;
   ecore_event_add(ECORE_WL_EVENT_FOCUS_IN, ev, NULL, NULL);
}

static void
_ecore_wl_input_focus_out_send(Ecore_Wl_Input *input EINA_UNUSED, Ecore_Wl_Window *win, unsigned int timestamp)
{
   Ecore_Wl_Event_Focus_Out *ev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(ev = calloc(1, sizeof(Ecore_Wl_Event_Focus_Out)))) return;
   ev->timestamp = timestamp;
   if (win) ev->win = win->id;
   ecore_event_add(ECORE_WL_EVENT_FOCUS_OUT, ev, NULL, NULL);
}

static void
_ecore_wl_input_mouse_down_send(Ecore_Wl_Input *input, Ecore_Wl_Window *win, int device, unsigned int button, unsigned int timestamp)
{
   Ecore_Event_Mouse_Button *ev;
   Ecore_Wl_Mouse_Down_Info *down_info;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(ev = calloc(1, sizeof(Ecore_Event_Mouse_Button)))) return;

   if (button == BTN_LEFT)
     ev->buttons = 1;
   else if (button == BTN_MIDDLE)
     ev->buttons = 2;
   else if (button == BTN_RIGHT)
     ev->buttons = 3;
   else
     ev->buttons = button;

   ev->timestamp = timestamp;
   ev->x = input->sx;
   ev->y = input->sy;
   ev->root.x = input->sx;
   ev->root.y = input->sy;
   ev->modifiers = input->modifiers;

   ev->double_click = 0;
   ev->triple_click = 0;

   /* handling double and triple click, taking into account multiple input
    * devices */
   if ((down_info = _ecore_wl_mouse_down_info_get(device)))
     {
        down_info->sx = input->sx;
        down_info->sy = input->sy;
        if (down_info->did_triple)
          {
             down_info->last_win = 0;
             down_info->last_last_win = 0;
             down_info->last_event_win = 0;
             down_info->last_last_event_win = 0;
             down_info->last_time = 0;
             down_info->last_last_time = 0;
          }
        //Check Double Clicked
        if (((int)(timestamp - down_info->last_time) <=
             (int)(1000 * _ecore_wl_double_click_time)) &&
            ((win) &&
                (win->id == down_info->last_win) &&
                (win->id == down_info->last_event_win)))
          {
             ev->double_click = 1;
             down_info->did_double = EINA_TRUE;
          }
        else
          {
             down_info->did_double = EINA_FALSE;
             down_info->did_triple = EINA_FALSE;
          }

        //Check Triple Clicked
        if (((int)(timestamp - down_info->last_last_time) <=
             (int)(2 * 1000 * _ecore_wl_double_click_time)) &&
            ((win) &&
                (win->id == down_info->last_win) &&
                (win->id == down_info->last_last_win) &&
                (win->id == down_info->last_event_win) &&
                (win->id == down_info->last_last_event_win)))
          {
             ev->triple_click = 1;
             down_info->did_triple = EINA_TRUE;
          }
        else
          {
             down_info->did_triple = EINA_FALSE;
          }
     }

   ev->multi.device = device;
   if (device >= ECORE_WL_TOUCH_MAX)
     {
        ev->multi.radius = 1.0;
        ev->multi.radius_x = 1.0;
        ev->multi.radius_y = 1.0;
        ev->multi.pressure = 1.0;
        ev->multi.angle = 0.0;
     }
   else
     {
        ev->multi.radius =
           _ecore_wl_input_touch_radius_calc(input->touch_axis[device].radius_x,
                                             input->touch_axis[device].radius_y);
        ev->multi.radius_x = input->touch_axis[device].radius_x;
        ev->multi.radius_y = input->touch_axis[device].radius_y;
        ev->multi.pressure = input->touch_axis[device].pressure;
        ev->multi.angle = input->touch_axis[device].angle;
     }
   ev->multi.x = input->sx;
   ev->multi.y = input->sy;
   ev->multi.root.x = input->sx;
   ev->multi.root.y = input->sy;
   ev->dev = _ecore_wl_input_get_ecore_device(input->last_device_name, input->last_device_class);

   if (win)
     {
        ev->window = win->id;
        ev->event_window = win->id;
     }

   ecore_event_add(ECORE_EVENT_MOUSE_BUTTON_DOWN, ev, NULL, NULL);

   if ((down_info) &&
       (!down_info->did_triple))
     {
        down_info->last_last_win = down_info->last_win;
        down_info->last_win = ev->window;
        down_info->last_last_event_win = down_info->last_event_win;
        down_info->last_event_win = ev->window;
        down_info->last_last_time = down_info->last_time;
        down_info->last_time = timestamp;
     }
}

static void
_ecore_wl_input_mouse_up_send(Ecore_Wl_Input *input, Ecore_Wl_Window *win, int device, unsigned int button, unsigned int timestamp)
{
   Ecore_Event_Mouse_Button *ev;
   Ecore_Wl_Mouse_Down_Info *down_info;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(ev = calloc(1, sizeof(Ecore_Event_Mouse_Button)))) return;

   if (button == BTN_LEFT)
     ev->buttons = 1;
   else if (button == BTN_MIDDLE)
     ev->buttons = 2;
   else if (button == BTN_RIGHT)
     ev->buttons = 3;
   else
     ev->buttons = button;

   ev->timestamp = timestamp;
   ev->root.x = input->sx;
   ev->root.y = input->sy;
   ev->modifiers = input->modifiers;

   ev->double_click = 0;
   ev->triple_click = 0;

   if ((down_info = _ecore_wl_mouse_down_info_get(device)))
     {
        if (down_info->did_double)
          ev->double_click = 1;
        if (down_info->did_triple)
          ev->triple_click = 1;
        ev->x = down_info->sx;
        ev->y = down_info->sy;
        ev->multi.x = down_info->sx;
        ev->multi.y = down_info->sy;
     }
   else
     {
        ev->x = input->sx;
        ev->y = input->sy;
        ev->multi.x = input->sx;
        ev->multi.y = input->sy;
     }

   ev->multi.device = device;
   ev->multi.radius = 1;
   ev->multi.radius_x = 1;
   ev->multi.radius_y = 1;
   ev->multi.pressure = 1.0;
   ev->multi.angle = 0.0;
   ev->multi.root.x = input->sx;
   ev->multi.root.y = input->sy;
   ev->dev = _ecore_wl_input_get_ecore_device(input->last_device_name,  input->last_device_class);

   if (device < ECORE_WL_TOUCH_MAX)
     {
        input->touch_axis[device].radius_x = 1.0;
        input->touch_axis[device].radius_y = 1.0;
        input->touch_axis[device].pressure = 1.0;
        input->touch_axis[device].angle = 0;
     }

   if (win)
     {
        ev->window = win->id;
        ev->event_window = win->id;
     }

   ecore_event_add(ECORE_EVENT_MOUSE_BUTTON_UP, ev, NULL, NULL);
}

static void
_ecore_wl_input_mouse_wheel_send(Ecore_Wl_Input *input, unsigned int axis, int value, unsigned int timestamp)
{
   Ecore_Event_Mouse_Wheel *ev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(ev = calloc(1, sizeof(Ecore_Event_Mouse_Wheel)))) return;

   ev->timestamp = timestamp;
   ev->modifiers = input->modifiers;
   ev->x = input->sx;
   ev->y = input->sy;
   /* ev->root.x = input->sx; */
   /* ev->root.y = input->sy; */
   ev->dev = _ecore_wl_input_get_ecore_device(input->last_device_name,  input->last_device_class);

   if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
     {
        ev->direction = 0;
        ev->z = value;
     }
   else if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
     {
        ev->direction = 1;
        ev->z = value;
     }

   if (input->grab)
     {
        ev->window = input->grab->id;
        ev->event_window = input->grab->id;
     }
   else if (input->pointer_focus)
     {
        ev->window = input->pointer_focus->id;
        ev->event_window = input->pointer_focus->id;
     }

   ecore_event_add(ECORE_EVENT_MOUSE_WHEEL, ev, NULL, NULL);
}

static void
_ecore_wl_mouse_down_info_clear(void)
{
   Eina_Inlist *l;
   Ecore_Wl_Mouse_Down_Info *info = NULL;

   l = _ecore_wl_mouse_down_info_list;
   while (l)
     {
        info = EINA_INLIST_CONTAINER_GET(l, Ecore_Wl_Mouse_Down_Info);
        l = eina_inlist_remove(l, l);
        free(info);
     }
   _ecore_wl_mouse_down_info_list = NULL;
}

static Ecore_Wl_Mouse_Down_Info *
_ecore_wl_mouse_down_info_get(int dev)
{
   Eina_Inlist *l = NULL;
   Ecore_Wl_Mouse_Down_Info *info = NULL;

   // Return the existing info
   l = _ecore_wl_mouse_down_info_list;
   EINA_INLIST_FOREACH(l, info)
     if (info->dev == dev) return info;

   // New Device. Add it.
   info = calloc(1, sizeof(Ecore_Wl_Mouse_Down_Info));
   if (!info) return NULL;

   info->dev = dev;
   l = eina_inlist_append(l, (Eina_Inlist *)info);
   _ecore_wl_mouse_down_info_list = l;
   return info;
}

void
_ecore_wl_events_init(void)
{
}

void
_ecore_wl_events_shutdown(void)
{
   _ecore_wl_mouse_down_info_clear();
}

static void
_ecore_wl_input_device_info_free(void *data EINA_UNUSED, void *ev)
{
   Ecore_Event_Device_Info *e;

   e = ev;
   eina_stringshare_del(e->name);
   eina_stringshare_del(e->identifier);
   eina_stringshare_del(e->seatname);

   free(e);
}

static Ecore_Device_Class
_ecore_wl_input_cap_to_ecore_device_class(unsigned int cap)
{
   switch(cap)
     {
      case ECORE_DEVICE_POINTER:
         return ECORE_DEVICE_CLASS_MOUSE;
      case ECORE_DEVICE_KEYBOARD:
         return ECORE_DEVICE_CLASS_KEYBOARD;
      case ECORE_DEVICE_TOUCH:
         return ECORE_DEVICE_CLASS_TOUCH;
      default:
         return ECORE_DEVICE_CLASS_NONE;
     }
   return ECORE_DEVICE_CLASS_NONE;
}

void
_ecore_wl_input_device_info_send(int win_id, const char *name,  const char *identifier, Ecore_Device_Class clas, Eina_Bool flag)
{
   Ecore_Event_Device_Info *e;

   if (!(e = calloc(1, sizeof(Ecore_Event_Device_Info)))) return;

   e->name = eina_stringshare_add(name);
   e->identifier = eina_stringshare_add(identifier);
   e->seatname = eina_stringshare_add(name);
   e->clas = clas;
   e->window = win_id;

   if (flag)
     ecore_event_add(ECORE_EVENT_DEVICE_ADD, e, _ecore_wl_input_device_info_free, NULL);
   else
     ecore_event_add(ECORE_EVENT_DEVICE_DEL, e, _ecore_wl_input_device_info_free, NULL);
}

static Ecore_Device *
_ecore_wl_input_get_ecore_device(const char *name, Ecore_Device_Class clas)
{
   const Eina_List *dev_list = NULL;
   const Eina_List *l;
   Ecore_Device *dev = NULL;
   const char *identifier;

   if (!name) return NULL;

   dev_list = ecore_device_list();
   if (!dev_list) return NULL;
   EINA_LIST_FOREACH(dev_list, l, dev)
     {
        if (!dev) continue;
        identifier = ecore_device_identifier_get(dev);
        if (!identifier) continue;
        if ((ecore_device_class_get(dev) == clas) && (!strcmp(identifier, name)))
          return dev;
     }
   return NULL;
}

static Eina_Bool
_ecore_wl_input_add_ecore_device(const char *name, const char *identifier, Ecore_Device_Class clas)
{
   const Eina_List *dev_list = NULL;
   const Eina_List *l;
   Ecore_Device *dev;
   const char *ecdev_name;

   if (!identifier) return EINA_FALSE;

   dev_list = ecore_device_list();
   if (dev_list)
     {
        EINA_LIST_FOREACH(dev_list, l, dev)
          {
             if (!dev) continue;
             ecdev_name = ecore_device_identifier_get(dev);
             if (!ecdev_name) continue;
             if ((ecore_device_class_get(dev) == clas) && (!strcmp(ecdev_name, identifier)))
                return EINA_FALSE;
          }
     }

   if(!(dev = ecore_device_add())) return EINA_FALSE;

   ecore_device_name_set(dev, name);
   ecore_device_description_set(dev, name);
   ecore_device_identifier_set(dev, identifier);
   ecore_device_class_set(dev, clas);
   return EINA_TRUE;
}

static Eina_Bool
_ecore_wl_input_del_ecore_device(const char *name EINA_UNUSED, const char *identifier, Ecore_Device_Class clas)
{
   const Eina_List *dev_list = NULL;
   const Eina_List *l;
   Ecore_Device *dev = NULL;
   const char *ecdev_name;

   if (!identifier) return EINA_FALSE;

   dev_list = ecore_device_list();
   if (!dev_list) return EINA_FALSE;
   EINA_LIST_FOREACH(dev_list, l, dev)
      {
         if (!dev) continue;
         ecdev_name = ecore_device_identifier_get(dev);
         if (!ecdev_name) continue;
         if ((ecore_device_class_get(dev) == clas) && (!strcmp(ecdev_name, identifier)))
           {
              ecore_device_del(dev);
              return EINA_TRUE;
           }
      }
   return EINA_FALSE;
}

void
_ecore_wl_input_device_info_broadcast(const char *name, const char *identifier, Ecore_Device_Class clas, Eina_Bool flag)
{
   Eina_Hash *windows = NULL;
   Eina_Iterator *itr;
   Ecore_Wl_Window *win = NULL;
   void *data;
   Eina_Bool ret = EINA_FALSE;

   windows = _ecore_wl_window_hash_get();
   if (!windows) return;
   if (!name) return;
   itr = eina_hash_iterator_data_new(windows);
   while (eina_iterator_next(itr, &data))
     {
        win = data;
        if (flag)
          ret = _ecore_wl_input_add_ecore_device(name, identifier, clas);
        else
          ret = _ecore_wl_input_del_ecore_device(name, identifier, clas);
        if (ret)
          _ecore_wl_input_device_info_send(win->id, name, identifier, clas, flag);
     }

   eina_iterator_free(itr);
}

void
_ecore_wl_input_device_manager_setup(unsigned int id)
{
   _ecore_wl_disp->wl.tz_input_device_manager =
   wl_registry_bind(_ecore_wl_disp->wl.registry, id, &tizen_input_device_manager_interface, 1);

   tizen_input_device_manager_add_listener(_ecore_wl_disp->wl.tz_input_device_manager,
                                       &_ecore_tizen_input_device_mgr_listener, _ecore_wl_disp->wl.display);
}

static void
_ecore_wl_input_device_manager_cb_device_add(void *data EINA_UNUSED, struct tizen_input_device_manager *tizen_input_device_manager EINA_UNUSED,
                          unsigned int serial EINA_UNUSED, const char *identifier, struct tizen_input_device *device, struct wl_seat *seat)
{
   Ecore_Wl_Input *input = _ecore_wl_disp->input;
   Ecore_Wl_Input_Device *dev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!input) return;
   if ((!identifier) || (!device) || (!seat)) return;

   if (!(dev = calloc(1, sizeof(Ecore_Wl_Input_Device)))) return;

   dev->tz_device = device;
   tizen_input_device_add_listener(dev->tz_device, &_ecore_tizen_input_device_listener, dev);
   dev->identifier = eina_stringshare_add(identifier);
   dev->seat = seat;

   input->devices = eina_list_append(input->devices, dev);
}

static void
_ecore_wl_input_device_manager_cb_device_remove(void *data EINA_UNUSED, struct tizen_input_device_manager *tizen_input_device_manager EINA_UNUSED,
                            unsigned int serial EINA_UNUSED, const char *identifier, struct tizen_input_device *device, struct wl_seat *seat)
{
   Ecore_Wl_Input *input = _ecore_wl_disp->input;
   Eina_List *l, *ll;
   Ecore_Wl_Input_Device *dev;
   Ecore_Device_Class clas;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   if (!input) return;
   if ((!identifier) || (!device) || (!seat)) return;

   EINA_LIST_FOREACH_SAFE(input->devices, l, ll, dev)
     {
        if (!dev->identifier) continue;
        if ((!strcmp(dev->identifier, identifier)) && (seat == dev->seat) && (device == dev->tz_device))
          {
             clas = _ecore_wl_input_cap_to_ecore_device_class(dev->clas);
             _ecore_wl_input_device_info_broadcast(dev->name, dev->identifier, clas, EINA_FALSE);

             if (dev->tz_device) tizen_input_device_release(dev->tz_device);
             if (dev->name) eina_stringshare_del(dev->name);
             if (dev->identifier) eina_stringshare_del(dev->identifier);
             dev->seat = NULL;

             input->devices = eina_list_remove_list(input->devices, l);

             free(dev);

             break;
          }
     }
}

static void
_ecore_wl_input_device_manager_cb_error(void *data EINA_UNUSED, struct tizen_input_device_manager *tizen_input_device_manager EINA_UNUSED, uint32_t errorcode EINA_UNUSED)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
}

static void
_ecore_wl_input_device_manager_cb_block_expired(void *data EINA_UNUSED, struct tizen_input_device_manager *tizen_input_device_manager EINA_UNUSED)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
}

static void
_ecore_wl_input_device_cb_device_info(void *data, struct tizen_input_device *tizen_input_device EINA_UNUSED, const char *name, uint32_t clas, uint32_t subclas, struct wl_array *axes EINA_UNUSED)
{
   Ecore_Wl_Input_Device *dev;
   Ecore_Device_Class e_clas;

   if (!(dev = data)) return;
   dev->clas = clas;
   dev->subclas = subclas;
   dev->name = eina_stringshare_add(name);
   e_clas = _ecore_wl_input_cap_to_ecore_device_class(clas);

   _ecore_wl_input_device_info_broadcast(dev->name, dev->identifier, e_clas, EINA_TRUE);
}

static void
_ecore_wl_input_device_cb_event_device(void *data, struct tizen_input_device *tizen_input_device EINA_UNUSED, unsigned int serial EINA_UNUSED, const char *name EINA_UNUSED, uint32_t time EINA_UNUSED)
{
   Ecore_Wl_Input *input = _ecore_wl_disp->input;
   Ecore_Wl_Input_Device *dev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   if (!input) return;

   if (!(dev = data)) return;
   if (!dev->identifier) return;
   eina_stringshare_replace(&input->last_device_name, dev->identifier);
   input->last_device_class = _ecore_wl_input_cap_to_ecore_device_class(dev->clas);

   return;
}

static void
_ecore_wl_input_detent_rotate_free(void *data EINA_UNUSED, void *ev)
{
   Ecore_Event_Detent_Rotate *e = ev;
   free(e);
}

static void
_ecore_wl_input_device_cb_axis(void *data EINA_UNUSED, struct tizen_input_device *tizen_input_device EINA_UNUSED, uint32_t axis_type, wl_fixed_t value)
{
   Ecore_Wl_Input *input = _ecore_wl_disp->input;
   double dvalue = wl_fixed_to_double(value);
   Ecore_Event_Detent_Rotate *e;

   switch (axis_type)
     {
        case TIZEN_INPUT_DEVICE_AXIS_TYPE_RADIUS_X:
           input->last_radius_x = dvalue;
           break;
        case TIZEN_INPUT_DEVICE_AXIS_TYPE_RADIUS_Y:
           input->last_radius_y = dvalue;
           break;
        case TIZEN_INPUT_DEVICE_AXIS_TYPE_PRESSURE:
           input->last_pressure = dvalue;
           break;
        case TIZEN_INPUT_DEVICE_AXIS_TYPE_ANGLE:
           input->last_angle = dvalue;
           break;
        case TIZEN_INPUT_DEVICE_AXIS_TYPE_DETENT:
           /* Do something after get detent event.
            * value 1 is clockwise,
            * value -1 is counterclockwise,
            */
           if (!(e = calloc(1, sizeof(Ecore_Event_Detent_Rotate))))
             {
                ERR("detent: cannot allocate memory");
                return;
             }
           if (dvalue == 1)
             e->direction = ECORE_DETENT_DIRECTION_CLOCKWISE;
           else
             e->direction = ECORE_DETENT_DIRECTION_COUNTER_CLOCKWISE;
           e->timestamp = (unsigned int)ecore_time_get();
           DBG("detent: dir: %d, time: %d", e->direction, e->timestamp);
           ecore_event_add(ECORE_EVENT_DETENT_ROTATE, e, _ecore_wl_input_detent_rotate_free, NULL);
           break;
        default:
           WRN("Invalid type(%d) is ignored.\n", axis_type);
           break;
     }
   return;
}

static void
_ecore_wl_input_key_conversion_clean_up(void)
{
   _back_key_lt_24 = 0;
   _menu_key_lt_24 = 0;
   _home_key_lt_24 = 0;

   _num_back_key_latest = 0;
   _num_menu_key_latest = 0;
   _num_home_key_latest = 0;

   if (_back_key_latest)
     {
        free(_back_key_latest);
        _back_key_latest = NULL;
     }
   if (_menu_key_latest)
     {
        free(_menu_key_latest);
        _menu_key_latest = NULL;
     }
   if (_home_key_latest)
     {
        free(_home_key_latest);
        _home_key_latest = NULL;
     }
}

static void
_ecore_wl_input_key_conversion_set(void)
{
   char *temp;
   xkb_keycode_t *keycodes = NULL;
   static int retry_cnt = 0;

   if ((_tizen_api_version < 0.0) || (_tizen_api_version > 0.0)) return;
   EINA_SAFETY_ON_NULL_RETURN(_ecore_wl_disp->input);
   EINA_SAFETY_ON_NULL_RETURN(_ecore_wl_disp->input->xkb.keymap);

   temp = getenv("TIZEN_API_VERSION");

   if (!temp)
     {
        _tizen_api_version = 0.0;
        retry_cnt++;
        if (retry_cnt > 20)
          {
             INF("No tizen api version.\n");
             _tizen_api_version = -1.0;
          }
     }
   else
     {
        _tizen_api_version = atof(temp);
        INF("TIZEN_API_VERSION: %lf, Environment variable: %s\n", _tizen_api_version, temp);
        if (_tizen_api_version < 2.4)
          {
             _ecore_wl_input_key_conversion_clean_up();

             ecore_wl_keycode_from_keysym(_ecore_wl_disp->input->xkb.keymap,
                xkb_keysym_from_name("XF86Stop", XKB_KEYSYM_NO_FLAGS), &keycodes);
             if (!keycodes)
               {
                  ERR("There is no entry available for the old name of back key. No conversion will be done for back key.\n");
               }
             else
               {
                  _back_key_lt_24 = (int)keycodes[0];
                  free(keycodes);
                  keycodes = NULL;

                  _num_back_key_latest  = ecore_wl_keycode_from_keysym(_ecore_wl_disp->input->xkb.keymap,
                    xkb_keysym_from_name("XF86Back", XKB_KEYSYM_NO_FLAGS), &_back_key_latest);
               }

             ecore_wl_keycode_from_keysym(_ecore_wl_disp->input->xkb.keymap,
                xkb_keysym_from_name("XF86Send", XKB_KEYSYM_NO_FLAGS), &keycodes);
             if (!keycodes)
               {
                  ERR("There is no entry available for the old name of back key. No conversion will be done for menu key.\n");
               }
             else
               {
                  _menu_key_lt_24 = (int)keycodes[0];
                  free(keycodes);
                  keycodes = NULL;

                  _num_menu_key_latest  = ecore_wl_keycode_from_keysym(_ecore_wl_disp->input->xkb.keymap,
                    xkb_keysym_from_name("XF86Menu", XKB_KEYSYM_NO_FLAGS), &_menu_key_latest);
               }

             ecore_wl_keycode_from_keysym(_ecore_wl_disp->input->xkb.keymap,
                xkb_keysym_from_name("XF86Phone", XKB_KEYSYM_NO_FLAGS), &keycodes);
             if (!keycodes)
               {
                  ERR("There is no entry available for the old name of back key. No conversion will be done for home key.\n");
               }
             else
               {
                  _home_key_lt_24 = (int)keycodes[0];
                  free(keycodes);
                  keycodes = NULL;

                  _num_home_key_latest  = ecore_wl_keycode_from_keysym(_ecore_wl_disp->input->xkb.keymap,
                    xkb_keysym_from_name("XF86Home", XKB_KEYSYM_NO_FLAGS), &_home_key_latest);
               }

             if ((!_back_key_lt_24) && (!_menu_key_lt_24) && (!_home_key_lt_24)) _tizen_api_version = -1.0;
          }
        else
          {
             _ecore_wl_input_key_conversion_clean_up();
          }
     }
}
