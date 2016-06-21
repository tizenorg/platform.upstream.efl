#ifndef _ECORE_WAYLAND_H_
# define _ECORE_WAYLAND_H_

/*
 * Wayland supoprt is considered experimental as wayland itself is still
 * unstable and liable to change core protocol. If you use this api, it is
 * possible it will break in future, until this notice is removed.
 */

# include <Eina.h>
# include <Ecore.h>
# include <wayland-client.h>
# include <wayland-cursor.h>
# include <xkbcommon/xkbcommon.h>
# include "xdg-shell-client-protocol.h"

# ifdef EAPI
#  undef EAPI
# endif

# ifdef __GNUC__
#  if __GNUC__ >= 4
#   define EAPI __attribute__ ((visibility("default")))
#  else
#   define EAPI
#  endif
# else
#  define EAPI
# endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Ecore_Wl_Output Ecore_Wl_Output;
typedef struct _Ecore_Wl_Input Ecore_Wl_Input;
typedef struct _Ecore_Wl_Global Ecore_Wl_Global; /** @since 1.7.6 */
typedef struct _Ecore_Wl_Subsurf Ecore_Wl_Subsurf; /** @since 1.8 */

# ifndef _ECORE_WAYLAND_WINDOW_PREDEF
typedef struct _Ecore_Wl_Window Ecore_Wl_Window;
# endif

typedef struct _Ecore_Wl_Input_Device Ecore_Wl_Input_Device;

/**
 * @deprecated Do Not Use
 * @since 1.7
 */
typedef struct _Ecore_Wl_Dnd Ecore_Wl_Dnd;

typedef struct _Ecore_Wl_Dnd_Source Ecore_Wl_Dnd_Source;
typedef struct _Ecore_Wl_Dnd_Target Ecore_Wl_Dnd_Target;
typedef struct _Ecore_Wl_Aux_Hint Ecore_Wl_Aux_Hint;

typedef struct _Ecore_Wl_Event_Mouse_In Ecore_Wl_Event_Mouse_In;
typedef struct _Ecore_Wl_Event_Mouse_Out Ecore_Wl_Event_Mouse_Out;
typedef struct _Ecore_Wl_Event_Focus_In Ecore_Wl_Event_Focus_In;
typedef struct _Ecore_Wl_Event_Focus_Out Ecore_Wl_Event_Focus_Out;
typedef struct _Ecore_Wl_Event_Window_Configure Ecore_Wl_Event_Window_Configure;
typedef struct _Ecore_Wl_Event_Window_Activate Ecore_Wl_Event_Window_Activate;
typedef struct _Ecore_Wl_Event_Window_Deactivate Ecore_Wl_Event_Window_Deactivate;
typedef struct _Ecore_Wl_Event_Window_Visibility_Change Ecore_Wl_Event_Window_Visibility_Change;
typedef struct _Ecore_Wl_Event_Window_Rotate Ecore_Wl_Event_Window_Rotate;
typedef struct _Ecore_Wl_Event_Window_Show Ecore_Wl_Event_Window_Show;
typedef struct _Ecore_Wl_Event_Window_Hide Ecore_Wl_Event_Window_Hide;
typedef struct _Ecore_Wl_Event_Window_Lower Ecore_Wl_Event_Window_Lower;
typedef struct _Ecore_Wl_Event_Dnd_Enter Ecore_Wl_Event_Dnd_Enter;
typedef struct _Ecore_Wl_Event_Dnd_Position Ecore_Wl_Event_Dnd_Position;
typedef struct _Ecore_Wl_Event_Dnd_Leave Ecore_Wl_Event_Dnd_Leave;
typedef struct _Ecore_Wl_Event_Dnd_Drop Ecore_Wl_Event_Dnd_Drop;
typedef struct _Ecore_Wl_Event_Dnd_End Ecore_Wl_Event_Dnd_End;
typedef struct _Ecore_Wl_Event_Data_Source_Cancelled Ecore_Wl_Event_Data_Source_Cancelled; /** @since 1.12 */
typedef struct _Ecore_Wl_Event_Data_Source_Send Ecore_Wl_Event_Data_Source_Send; /** @since 1.7 */
typedef struct _Ecore_Wl_Event_Data_Source_Target  Ecore_Wl_Event_Data_Source_Target; /** @since 1.12 */
typedef struct _Ecore_Wl_Event_Selection_Data_Ready Ecore_Wl_Event_Selection_Data_Ready; /** @since 1.7 */
typedef struct _Ecore_Wl_Event_Interfaces_Bound Ecore_Wl_Event_Interfaces_Bound;
typedef struct _Ecore_Wl_Event_Conformant_Change Ecore_Wl_Event_Conformant_Change;
typedef struct _Ecore_Wl_Event_Aux_Hint_Allowed Ecore_Wl_Event_Aux_Hint_Allowed;
typedef struct _Ecore_Wl_Event_Window_Iconify_State_Change Ecore_Wl_Event_Window_Iconify_State_Change;
typedef struct _Ecore_Wl_Event_Effect Ecore_Wl_Event_Effect_Start;
typedef struct _Ecore_Wl_Event_Effect Ecore_Wl_Event_Effect_End;
typedef struct _Ecore_Wl_Display Ecore_Wl_Display;
typedef struct _Ecore_Wl_Event_Global Ecore_Wl_Event_Global;
typedef struct _Ecore_Wl_Event_Keymap_Update Ecore_Wl_Event_Keymap_Update;

struct _Ecore_Wl_Event_Global
{
   Ecore_Wl_Display *display;
   Eina_Stringshare *interface;
   unsigned int id, version;
};

struct _Ecore_Wl_Event_Keymap_Update
{
   Ecore_Wl_Input *input;
   struct xkb_keymap *keymap;
};

enum _Ecore_Wl_Window_Type
{
   ECORE_WL_WINDOW_TYPE_NONE,
   ECORE_WL_WINDOW_TYPE_TOPLEVEL,
   ECORE_WL_WINDOW_TYPE_FULLSCREEN,
   ECORE_WL_WINDOW_TYPE_MAXIMIZED,
   ECORE_WL_WINDOW_TYPE_TRANSIENT,
   ECORE_WL_WINDOW_TYPE_MENU,
   ECORE_WL_WINDOW_TYPE_DND,
   ECORE_WL_WINDOW_TYPE_CUSTOM,
   ECORE_WL_WINDOW_TYPE_NOTIFICATION,
   ECORE_WL_WINDOW_TYPE_UTILITY
};

enum _Ecore_Wl_Window_Buffer_Type
{
   ECORE_WL_WINDOW_BUFFER_TYPE_EGL_WINDOW,
   ECORE_WL_WINDOW_BUFFER_TYPE_EGL_IMAGE,
   ECORE_WL_WINDOW_BUFFER_TYPE_SHM
};

//this enum for keyrouter and client window side
//keycode (8~255)
enum _Ecore_Wl_Window_Keygrab_Mode
{
   ECORE_WL_WINDOW_KEYGRAB_UNKNOWN = 0, /**< Unknown keygrab mode */
   ECORE_WL_WINDOW_KEYGRAB_SHARED = 1, /**< Getting the grabbed-key together with the other client windows */
   ECORE_WL_WINDOW_KEYGRAB_TOPMOST = 2, /**< Getting the grabbed-key only when window is top of the stack */
   ECORE_WL_WINDOW_KEYGRAB_OVERRIDE_EXCLUSIVE = 3, /**< Getting the grabbed-key exclusively regardless of window's position. Being overrided the grab by the other client window  */
   ECORE_WL_WINDOW_KEYGRAB_EXCLUSIVE = 4 /**< Getting the grabbed-key exclusively regardless of window's position */
};

enum _Ecore_Wl_Conformant_Part_Type
{
   ECORE_WL_INDICATOR_PART = 0,
   ECORE_WL_KEYBOARD_PART = 1,
   ECORE_WL_CLIPBOARD_PART = 2
};

enum _Ecore_Wl_Virtual_Keyboard_State
{
   ECORE_WL_VIRTUAL_KEYBOARD_STATE_UNKNOWN = 0,
   ECORE_WL_VIRTUAL_KEYBOARD_STATE_OFF,
   ECORE_WL_VIRTUAL_KEYBOARD_STATE_ON,
};

enum _Ecore_Wl_Indicator_State
{
   ECORE_WL_INDICATOR_STATE_UNKNOWN = 0,
   ECORE_WL_INDICATOR_STATE_OFF,
   ECORE_WL_INDICATOR_STATE_ON
};

enum _Ecore_Wl_Clipboard_State
{
   ECORE_WL_CLIPBOARD_STATE_UNKNOWN = 0,
   ECORE_WL_CLIPBOARD_STATE_OFF,
   ECORE_WL_CLIPBOARD_STATE_ON
};

enum _Ecore_Wl_Indicator_Opacity_Mode
{
   ECORE_WL_INDICATOR_OPACITY_UNKNOWN = 0,
   ECORE_WL_INDICATOR_OPAQUE,
   ECORE_WL_INDICATOR_TRANSLUCENT,
   ECORE_WL_INDICATOR_TRANSPARENT
};

enum _Ecore_Wl_Window_Stack_Mode
{
   ECORE_WL_WINDOW_STACK_NONE  = 0,
   ECORE_WL_WINDOW_STACK_ABOVE = 1,
   ECORE_WL_WINDOW_STACK_BELOW = 2,
};

typedef enum _Ecore_Wl_Window_Type Ecore_Wl_Window_Type;
typedef enum _Ecore_Wl_Window_Buffer_Type Ecore_Wl_Window_Buffer_Type;
typedef enum _Ecore_Wl_Window_Keygrab_Mode Ecore_Wl_Window_Keygrab_Mode;
typedef enum _Ecore_Wl_Conformant_Part_Type Ecore_Wl_Conformant_Part_Type;
typedef enum _Ecore_Wl_Virtual_Keyboard_State Ecore_Wl_Virtual_Keyboard_State;
typedef enum _Ecore_Wl_Indicator_State Ecore_Wl_Indicator_State;
typedef enum _Ecore_Wl_Clipboard_State Ecore_Wl_Clipboard_State;
typedef enum _Ecore_Wl_Indicator_Opacity_Mode Ecore_Wl_Indicator_Opacity_Mode;
typedef enum _Ecore_Wl_Window_Stack_Mode Ecore_Wl_Window_Stack_Mode;

/** @since 1.7.6 */
struct _Ecore_Wl_Global
{
   EINA_INLIST;
   unsigned int id;
   char *interface;
   unsigned int version;
};

struct _Ecore_Wl_Event_Mouse_In
{
   int modifiers;
   int x, y;
   struct
     {
        int x, y;
     } root;
   unsigned int window;
   unsigned int event_window;
   unsigned int root_window;
   unsigned int timestamp;
};

struct _Ecore_Wl_Event_Mouse_Out
{
   int modifiers;
   int x, y;
   struct
     {
        int x, y;
     } root;
   unsigned int window;
   unsigned int event_window;
   unsigned int root_window;
   unsigned int timestamp;
};

struct _Ecore_Wl_Event_Focus_In
{
   unsigned int win;
   unsigned int timestamp;
};

struct _Ecore_Wl_Event_Focus_Out
{
   unsigned int win;
   unsigned int timestamp;
};

struct _Ecore_Wl_Event_Window_Configure
{
   unsigned int win;
   unsigned int event_win;
   int x, y, w, h;
   int edges;
};

struct _Ecore_Wl_Event_Window_Activate
{
   unsigned int win;
   unsigned int parent_win;
   unsigned int event_win;
   Eina_Bool fobscured;
};

struct _Ecore_Wl_Event_Window_Deactivate
{
   unsigned int win;
   unsigned int parent_win;
   unsigned int event_win;
   Eina_Bool fobscured;
};

struct _Ecore_Wl_Event_Window_Visibility_Change
{
   unsigned int win;
   int          fully_obscured;
};

struct _Ecore_Wl_Event_Window_Rotate
{
   unsigned int win;
   unsigned int angle;
   int w, h;
};

struct _Ecore_Wl_Event_Window_Show
{
   unsigned int win;
   unsigned int parent_win;
   unsigned int event_win;
   unsigned int data[4];
};

struct _Ecore_Wl_Event_Window_Hide
{
   unsigned int win;
   unsigned int parent_win;
   unsigned int event_win;
};

struct _Ecore_Wl_Event_Window_Lower
{
   unsigned int win;
   unsigned int timestamp;
};

struct _Ecore_Wl_Event_Dnd_Enter
{
   unsigned int win, source;
   char **types;
   int num_types;
   unsigned int serial;
   struct wl_data_offer *offer;
   struct
     {
        int x, y;
     } position;
};

struct _Ecore_Wl_Event_Dnd_Position
{
   unsigned int win, source;
   struct
     {
        int x, y;
     } position;
};

struct _Ecore_Wl_Event_Dnd_Leave
{
   unsigned int win, source;
};

struct _Ecore_Wl_Event_Dnd_Drop
{
   unsigned int win, source;
   struct
     {
        int x, y;
     } position;
};

struct _Ecore_Wl_Event_Dnd_End
{
   unsigned int win, source;
};

/** @since 1.12 */
struct _Ecore_Wl_Event_Data_Source_Cancelled
{
   unsigned int win, source;
};

/** @since 1.7 */
struct _Ecore_Wl_Event_Data_Source_Send
{
   char *type;
   int fd;
};

/** @since 1.12 */
struct _Ecore_Wl_Event_Data_Source_Target
{
   char *type;
};

/** @since 1.7 */
struct _Ecore_Wl_Event_Selection_Data_Ready
{
   char *data;
   int len;
   Eina_Bool done;
};

struct _Ecore_Wl_Event_Interfaces_Bound
{
   Eina_Bool compositor : 1;
   Eina_Bool shm : 1;
   Eina_Bool shell : 1;
   Eina_Bool output : 1;
   Eina_Bool seat : 1;
   Eina_Bool data_device_manager : 1;
   Eina_Bool subcompositor : 1;
   Eina_Bool policy : 1;
   Eina_Bool policy_ext : 1;
};

struct _Ecore_Wl_Event_Conformant_Change
{
   unsigned int win;
   Ecore_Wl_Conformant_Part_Type part_type;
   Eina_Bool state;
};

struct _Ecore_Wl_Event_Aux_Hint_Allowed
{
   unsigned int win;
   int id;
};

struct _Ecore_Wl_Event_Window_Iconify_State_Change
{
   unsigned int win;
   int iconified;
   int force;
};

struct _Ecore_Wl_Event_Effect
{
   int win;
   unsigned int type;
};

/**
 * @file
 * @brief Ecore functions for dealing with the Wayland window system
 *
 * @defgroup Ecore_Wl_Group Ecore_Wayland - Wayland integration
 * @ingroup Ecore

 * Ecore_Wl provides a wrapper and convenience functions for using the
 * Wayland protocol in implementing a window system. Function groups for
 * this part of the library include the following:
 *
 * @li @ref Ecore_Wl_Init_Group
 * @li @ref Ecore_Wl_Display_Group
 * @li @ref Ecore_Wl_Flush_Group
 * @li @ref Ecore_Wl_Window_Group
 * @li @ref Ecore_Wl_Input_Group
 * @li @ref Ecore_Wl_Dnd_Group
 * @li @ref Ecore_Wl_Subsurf
 */

EAPI extern int ECORE_WL_EVENT_MOUSE_IN;
EAPI extern int ECORE_WL_EVENT_MOUSE_OUT;
EAPI extern int ECORE_WL_EVENT_FOCUS_IN;
EAPI extern int ECORE_WL_EVENT_FOCUS_OUT;
EAPI extern int ECORE_WL_EVENT_WINDOW_CONFIGURE;
EAPI extern int ECORE_WL_EVENT_WINDOW_ACTIVATE;
EAPI extern int ECORE_WL_EVENT_WINDOW_DEACTIVATE;
EAPI extern int ECORE_WL_EVENT_WINDOW_VISIBILITY_CHANGE;
EAPI extern int ECORE_WL_EVENT_WINDOW_SHOW;
EAPI extern int ECORE_WL_EVENT_WINDOW_HIDE;
EAPI extern int ECORE_WL_EVENT_WINDOW_LOWER;
EAPI extern int ECORE_WL_EVENT_WINDOW_ROTATE;
EAPI extern int ECORE_WL_EVENT_DND_ENTER;
EAPI extern int ECORE_WL_EVENT_DND_POSITION;
EAPI extern int ECORE_WL_EVENT_DND_LEAVE;
EAPI extern int ECORE_WL_EVENT_DND_DROP;
EAPI extern int ECORE_WL_EVENT_DND_OFFER; /** @since 1.8 */
EAPI extern int ECORE_WL_EVENT_DND_END; /** @since 1.8 */
EAPI extern int ECORE_WL_EVENT_DATA_SOURCE_TARGET; /** @since 1.7 */
EAPI extern int ECORE_WL_EVENT_DATA_SOURCE_SEND; /** @since 1.7 */
EAPI extern int ECORE_WL_EVENT_DATA_SOURCE_CANCELLED; /** @since 1.7 */
EAPI extern int ECORE_WL_EVENT_SELECTION_DATA_READY; /** @since 1.7 */
EAPI extern int ECORE_WL_EVENT_INTERFACES_BOUND;
EAPI extern int ECORE_WL_EVENT_CONFORMANT_CHANGE;
EAPI extern int ECORE_WL_EVENT_AUX_HINT_ALLOWED;
EAPI extern int ECORE_WL_EVENT_WINDOW_ICONIFY_STATE_CHANGE;
EAPI extern int ECORE_WL_EVENT_EFFECT_START;
EAPI extern int ECORE_WL_EVENT_EFFECT_END;
EAPI extern int ECORE_WL_EVENT_GLOBAL_ADDED;
EAPI extern int ECORE_WL_EVENT_GLOBAL_REMOVED;
EAPI extern int ECORE_WL_EVENT_KEYMAP_UPDATE;

/**
 * @defgroup Ecore_Wl_Init_Group Wayland Library Init and Shutdown Functions
 * @ingroup Ecore_Wl_Group
 *
 * Functions that start and shutdown the Ecore Wayland Library.
 */

/**
 * Initialize the Wayland display connection to the given display.
 *
 * @param   name Display target name. if @c NULL, the default display is
 *          assumed.
 * @return  The number of times the library has been initialized without being
 *          shut down. 0 is returned if an error occurs.
 *
 * @ingroup Ecore_Wl_Init_Group
 */
EAPI int ecore_wl_init(const char *name);

/**
 * Shuts down the Ecore Wayland Library
 *
 * In shutting down the library, the Wayland display connection is terminated
 * and any event handlers for it are removed.
 *
 * @return  The number of times the library has been initialized without
 *          being shut down.
 *
 * @ingroup Ecore_Wl_Init_Group
 */
EAPI int ecore_wl_shutdown(void);

/**
 * @defgroup Ecore_Wl_Flush_Group Wayland Synchronization Functions
 * @ingroup Ecore_Wl_Group
 *
 * Functions that ensure that all commands which have been issued by the
 * Ecore Wayland library have been sent to the server.
 */

/**
 * Sends all Wayland commands to the Wayland Display.
 *
 * @ingroup Ecore_Wl_Flush_Group
 * @since 1.2
 */
EAPI void ecore_wl_flush(void);

/**
 * Flushes the command buffer and waits until all requests have been
 * processed by the server.
 *
 * @ingroup Ecore_Wl_Flush_Group
 * @since 1.2
 */
EAPI void ecore_wl_sync(void);

/**
 * @defgroup Ecore_Wl_Display_Group Wayland Display Functions
 * @ingroup Ecore_Wl_Group
 *
 * Functions that set and retrieve various information about the Wayland Display.
 */

/**
 * Retrieves the Wayland Shm Interface used for the current Wayland connection.
 *
 * @return The current wayland shm interface, or NULL on error
 *
 * @ingroup Ecore_Wl_Display_Group
 * @since 1.2
 */
EAPI struct wl_shm *ecore_wl_shm_get(void);

/**
 * Retrieves the Wayland Display Interface used for the current Wayland connection.
 *
 * @return The current wayland display interface, or NULL on error
 *
 * @ingroup Ecore_Wl_Display_Group
 * @since 1.2
 */
EAPI struct wl_display *ecore_wl_display_get(void);

/**
 * Retrieves the Compositor interface.
 *
 * This interface is used by clients to request the creation of surfaces and
 * regions.
 *
 * @return The current wayland compositor interface, or NULL on error
 * @since 1.8
 */

/**
 * Retrieves the size of the current screen.
 *
 * @param w where to return the width. May be NULL. Returns 0 on error.
 * @param h where to return the height. May be NULL. Returns 0 on error.
 *
 * @ingroup Ecore_Wl_Display_Group
 * @since 1.2
 */
EAPI void ecore_wl_screen_size_get(int *w, int *h);

/**
 * Retrieves the position of the current screen.
 *
 * @param x where to return the horizontal position. May be NULL. Returns 0 on error.
 * @param y where to return the vertical position. May be NULL. Returns 0 on error.
 *
 * @ingroup Ecore_Wl_Display_Group
 * @since 1.2
 */
EAPI void ecore_wl_pointer_xy_get(int *x, int *y);

/**
 * Return the screen DPI
 *
 * This is a simplistic call to get DPI. It does not account for differing
 * DPI in the x and y axes nor does it account for multihead or xinerama and
 * xrandr where different parts of the screen may have different DPI etc.
 *
 * @return the general screen DPI (dots/pixels per inch).
 *
 * @since 1.2
 */
EAPI int ecore_wl_dpi_get(void);

/**
 * Dispatch Wayland events
 *
 * Dispatches any events queued on the default queue.  Wayland will attempt
 * to read from the display fd and queue things up according to the proxy
 * assignment.
 */
EAPI void ecore_wl_display_iterate(void);

/*
 * @ingroup Ecore_Wl_Display_Group
 * @since 1.8
 */
EAPI Eina_Bool ecore_wl_animator_source_set(Ecore_Animator_Source source);

/**
 * Retrieves the requested cursor from the cursor theme
 *
 * @param cursor_name The desired cursor name to be looked up in the theme
 * @return the cursor or NULL if the cursor cannot be found
 *
 * @since 1.2
 */
EAPI struct wl_cursor *ecore_wl_cursor_get(const char *cursor_name);

/**
 * @defgroup Ecore_Wl_Input_Group Functions dealing with Wayland Input
 * @ingroup Ecore_Wl_Group
 *
 * Functions to interface with Wayland Input
 */

/**
 * @ingroup Ecore_Wl_Input_Group
 * @since 1.8
 */
EAPI Ecore_Wl_Input *ecore_wl_input_get(void);
EAPI void ecore_wl_input_grab(Ecore_Wl_Input *input, Ecore_Wl_Window *win, unsigned int button);
EAPI void ecore_wl_input_ungrab(Ecore_Wl_Input *input);
EAPI void ecore_wl_input_pointer_set(Ecore_Wl_Input *input, struct wl_surface *surface, int hot_x, int hot_y);
EAPI void ecore_wl_input_cursor_from_name_set(Ecore_Wl_Input *input, const char *cursor_name);
EAPI void ecore_wl_input_cursor_default_restore(Ecore_Wl_Input *input);
EAPI struct wl_seat *ecore_wl_input_seat_get(Ecore_Wl_Input *input);

EAPI Eina_Inlist *ecore_wl_outputs_get(void);

/**
 * @ingroup Ecore_Wl_Input_Group
 * @since 1.12
 */
EAPI void ecore_wl_input_cursor_size_set(Ecore_Wl_Input *input, const int size);
EAPI void ecore_wl_input_cursor_theme_name_set(Ecore_Wl_Input *input, const char *cursor_theme_name);

/**
 * @ingroup Ecore_Wl_Input_Group
 * @since 1.17
 */
EAPI struct xkb_keymap *ecore_wl_input_keymap_get(Ecore_Wl_Input *input);


/**
 * Retrieves the Wayland Globals Interface list used for the current Wayland connection.
 *
 * This call, if done after the ECORE_WL_EVENT_INTERFACES_BOUND event was
 * received already, won't block the mainloop or trigger a dispatch. It will
 * return the current globals immediately. However, if done before this event,
 * it will probably block the mainloop waiting for the sync "done" event to be
 * received (by using one or more wl_display_dispatch call), and then finally
 * return the wl globals list.
 *
 * There's no need to call dispatch manually, since this call will do it if necessary.
 *
 * @return The current wayland globals interface list, or NULL on error
 *
 * @ingroup Ecore_Wl_Display_Group
 * @since 1.7.6
 */
EAPI Eina_Inlist *ecore_wl_globals_get(void);

/**
 * Retrieves the Wayland Registry used for the current Wayland connection.
 *
 * @return The current wayland registry, or NULL on error
 *
 * @ingroup Ecore_Wl_Display_Group
 * @since 1.7.6
 */
EAPI struct wl_registry *ecore_wl_registry_get(void);

/**
 * @defgroup Ecore_Wl_Window_Group Wayland Window functions.
 * @ingroup Ecore_Wl_Group
 *
 * Functions that can be used to create a Wayland window.
 */

/**
 * Creates a new window
 *
 * @param parent The parent window to use. If @p parent is @c 0, the root window
 *               of the default display is used.
 * @param x      X Position
 * @param y      Y position
 * @param w      Width
 * @param h      Height
 * @param buffer_type The type of the buffer to be used to create a new Ecore_Wl_Window.
 *
 * @return The new window, or NULL on failure
 *
 * @ingroup Ecore_Wl_Window_Group
 * @since 1.2
 */
EAPI Ecore_Wl_Window *ecore_wl_window_new(Ecore_Wl_Window *parent, int x, int y, int w, int h, int buffer_type);

/**
 * Deletes the given window
 *
 * @param win The given window
 *
 * @ingroup Ecore_Wl_Window_Group
 * @since 1.2
 */
EAPI void ecore_wl_window_free(Ecore_Wl_Window *win);

/**
 * Signals for Wayland to initiate a window move.
 *
 * The position requested (@p x, @p y) is not honored by Wayland because
 * Wayland does not allow specific window placement to be set.
 *
 * @param win The window to move.
 * @param x   X Position
 * @param y   Y Position
 *
 * @ingroup Ecore_Wl_Window_Group
 * @since 1.2
 */
EAPI void ecore_wl_window_move(Ecore_Wl_Window *win, int x, int y);

/**
 * Signals for Wayland to initiate a window resize.
 *
 * The size requested (@p w, @p h) is not honored by Wayland because
 * Wayland does not allow specific window sizes to be set.
 *
 * @param win      The window to resize.
 * @param w        Width
 * @param h        Height
 * @param location The edge of the window from where the resize should start.
 *
 * @ingroup Ecore_Wl_Window_Group
 * @since 1.2
 */
EAPI void ecore_wl_window_resize(Ecore_Wl_Window *win, int w, int h, int location);
EAPI void ecore_wl_window_damage(Ecore_Wl_Window *win, int x, int y, int w, int h);
EAPI void ecore_wl_window_buffer_attach(Ecore_Wl_Window *win, struct wl_buffer *buffer, int x, int y);
EAPI struct wl_compositor *ecore_wl_compositor_get(void);

/* @since 1.8 */
EAPI void ecore_wl_window_commit(Ecore_Wl_Window *win);

/**
 * Shows a window
 *
 * Synonymous to "mapping" a window in Wayland System terminology.
 *
 * @param win The window to show.
 *
 * @ingroup Ecore_Wl_Window_Group
 * @since 1.2
 */
EAPI void ecore_wl_window_show(Ecore_Wl_Window *win);

/**
 * Hides a window
 *
 * Synonymous to "unmapping" a window in Wayland System terminology.
 *
 * @param win The window to hide.
 *
 * @ingroup Ecore_Wl_Window_Group
 * @since 1.2
 */
EAPI void ecore_wl_window_hide(Ecore_Wl_Window *win);

/**
 * Raises a window
 *
 * @param win The window to raise.
 *
 * @ingroup Ecore_Wl_Window_Group
 * @since 1.2
 */
EAPI void ecore_wl_window_raise(Ecore_Wl_Window *win);
EAPI void ecore_wl_window_lower(Ecore_Wl_Window *win);
EAPI void ecore_wl_window_activate(Ecore_Wl_Window *win);
EAPI void ecore_wl_window_maximized_set(Ecore_Wl_Window *win, Eina_Bool maximized);
EAPI Eina_Bool ecore_wl_window_maximized_get(Ecore_Wl_Window *win);
EAPI void ecore_wl_window_fullscreen_set(Ecore_Wl_Window *win, Eina_Bool fullscreen);
EAPI Eina_Bool ecore_wl_window_fullscreen_get(Ecore_Wl_Window *win);
EAPI void ecore_wl_window_transparent_set(Ecore_Wl_Window *win, Eina_Bool transparent);
EAPI Eina_Bool ecore_wl_window_transparent_get(Ecore_Wl_Window *win);
EAPI void ecore_wl_window_update_size(Ecore_Wl_Window *win, int w, int h);
EAPI void ecore_wl_window_update_location(Ecore_Wl_Window *win, int x, int y);
EAPI struct wl_surface *ecore_wl_window_surface_get(Ecore_Wl_Window *win);
EAPI struct wl_shell_surface *ecore_wl_window_shell_surface_get(Ecore_Wl_Window *win);
EAPI struct xdg_surface *ecore_wl_window_xdg_surface_get(Ecore_Wl_Window *win); /** @since 1.12 */
EAPI Ecore_Wl_Window *ecore_wl_window_find(unsigned int id);
EAPI Ecore_Wl_Window_Type ecore_wl_window_type_get(Ecore_Wl_Window *win);
EAPI void ecore_wl_window_type_set(Ecore_Wl_Window *win, Ecore_Wl_Window_Type type);
EAPI void ecore_wl_window_pointer_set(Ecore_Wl_Window *win, struct wl_surface *surface, int hot_x, int hot_y);
EAPI Eina_Bool ecore_wl_window_pointer_warp(Ecore_Wl_Window *win, int x, int y);
EAPI void ecore_wl_window_cursor_from_name_set(Ecore_Wl_Window *win, const char *cursor_name);
EAPI void ecore_wl_window_cursor_default_restore(Ecore_Wl_Window *win);
EAPI void ecore_wl_window_parent_set(Ecore_Wl_Window *win, Ecore_Wl_Window *parent);
EAPI void ecore_wl_window_position_set(Ecore_Wl_Window *win, int x, int y);
EAPI void ecore_wl_window_focus_skip_set(Ecore_Wl_Window *win, Eina_Bool focus_skip);
EAPI void ecore_wl_window_role_set(Ecore_Wl_Window *win, const char *role);
EAPI int ecore_wl_window_active_angle_get(Ecore_Wl_Window *win);

/**
 * Iconify a window
 *
 * @param win The window to iconifiy
 *
 * @ingroup Ecore_Wl_Window_Group
 * @since 1.12
 */
EAPI void ecore_wl_window_iconified_set(Ecore_Wl_Window *win, Eina_Bool iconified);
EAPI Eina_Bool ecore_wl_window_iconified_get(Ecore_Wl_Window *win);

EAPI void ecore_wl_window_iconify_state_update(Ecore_Wl_Window *win, Eina_Bool iconified, Eina_Bool send_event);

EAPI int ecore_wl_window_id_get(Ecore_Wl_Window *win);
EAPI void ecore_wl_window_title_set(Ecore_Wl_Window *win, const char *title);
EAPI void ecore_wl_window_class_name_set(Ecore_Wl_Window *win, const char *class_name);
EAPI int ecore_wl_window_surface_id_get(Ecore_Wl_Window *win);

EAPI Ecore_Wl_Input *ecore_wl_window_keyboard_get(Ecore_Wl_Window *win);

EAPI void ecore_wl_window_stack_mode_set(Ecore_Wl_Window *win, Ecore_Wl_Window_Stack_Mode mode);

/**
 * Returns a wl_surface with no association to any wl_shell_surface.
 *
 * @param win The window which will use this newly created surface.
 * @return wl_surface or NULL on error
 *
 * @ingroup Ecore_Wl_Window_Group
 * @since 1.8
 */
EAPI struct wl_surface *ecore_wl_window_surface_create(Ecore_Wl_Window *win);

EAPI void ecore_wl_window_alpha_set(Ecore_Wl_Window *win, Eina_Bool alpha);
EAPI Eina_Bool ecore_wl_window_alpha_get(Ecore_Wl_Window *win);

/**
 * Returns the Ecore_Wl_Window from a wl_surface
 *
 * @param surface The surface for which to find the Ecore_Wl_Window from
 * @return the Ecore_Wl_Window associated with this surface, or NULL if not found
 *
 * @ingroup Ecore_Wl_Window_Group
 * @since 1.8
 */
EAPI Ecore_Wl_Window *ecore_wl_window_surface_find(struct wl_surface *surface);

/**
 * Set the input rect of the Ecore_Wl_Window.
 *
 * To set an empty rect, pass x and y and width and height as -1, -1, 1, 1.
 *
 * @param win The window
 * @param input_rect The rectangle to be set as input
 *
 * @ingroup Ecore_Wl_Window_Group
 * @since 1.8
 */
EAPI void ecore_wl_window_input_rect_set(Ecore_Wl_Window *win, Eina_Rectangle *input_rect);

/**
 * Add the input rect of the Ecore_Wl_Window.
 *
 * @param win The window
 * @param input_rect The rectangle to be added as input
 *
 * @ingroup Ecore_Wl_Window_Group
 * @since 1.8
 */
EAPI void ecore_wl_window_input_rect_add(Ecore_Wl_Window *win, Eina_Rectangle *input_rect);

/**
 * Subtract the input rect of the Ecore_Wl_Window.
 *
 * @param win The window
 * @param input_rect The rectangle to be subtracted as input
 *
 * @ingroup Ecore_Wl_Window_Group
 * @since 1.8
 */
EAPI void ecore_wl_window_input_rect_subtract(Ecore_Wl_Window *win, Eina_Rectangle *input_rect);

/**
 * Set the input region of the Ecore_Wl_Window.
 *
 * To set an empty region, pass width and height as 0.
 *
 * An empty input region means the entire window surface will accept input.
 *
 * @param win The window
 * @param x The left point of the region.
 * @param y The top point of the region.
 * @param w The width of the region.
 * @param h The height of the region.
 *
 * @ingroup Ecore_Wl_Window_Group
 * @since 1.8
 */
EAPI void ecore_wl_window_input_region_set(Ecore_Wl_Window *win, int x, int y, int w, int h);

/**
 * Set the opaque region of the Ecore_Wl_Window
 *
 * @param win The window
 * @param x The left point of the region.
 * @param y The top point of the region.
 * @param w The width of the region.
 * @param h The height of the region.
 *
 * @ingroup Ecore_Wl_Window_Group
 * @since 1.8
 */
EAPI void ecore_wl_window_opaque_region_set(Ecore_Wl_Window *win, int x, int y, int w, int h);

/**
 * Set the rotation of the Ecore_Wl_Window
 *
 * @param win The window
 * @param rotation The degree of rotation for this window
 *
 * @ingroup Ecore_Wl_Window_Group
 * @since 1.8
 */
EAPI void ecore_wl_window_rotation_set(Ecore_Wl_Window *win, int rotation);

/**
 * Get the rotation of the Ecore_Wl_Window
 *
 * @param win The window
 * @return The degree of rotation for this window
 *
 * @ingroup Ecore_Wl_Window_Group
 * @since 1.8
 */
EAPI int ecore_wl_window_rotation_get(Ecore_Wl_Window *win);

/**
 * @defgroup Ecore_Wl_Dnd_Group Functions dealing with Wayland Drag-n-Drop
 * @ingroup Ecore_Wl_Group
 *
 * Functions to interface with Wayland Drag-n-Drop
 */

/**
 * @deprecated use ecore_wl_dnd_selection_set
 * @since 1.7
*/
EINA_DEPRECATED EAPI Eina_Bool ecore_wl_dnd_set_selection(Ecore_Wl_Dnd *dnd, const char **types_offered);

/**
 * @deprecated use ecore_wl_dnd_selection_get
 * @since 1.7
*/
EINA_DEPRECATED EAPI Eina_Bool ecore_wl_dnd_get_selection(Ecore_Wl_Dnd *dnd, const char *type);

/**
 * @deprecated Do Not Use
 * @since 1.7
 */
EINA_DEPRECATED EAPI Ecore_Wl_Dnd *ecore_wl_dnd_get(void);

/**
 * @deprecated use ecore_wl_dnd_drag_start
 * @since 1.7
 */
EINA_DEPRECATED EAPI Eina_Bool ecore_wl_dnd_start_drag(Ecore_Wl_Dnd *dnd);

/**
 * @deprecated use ecore_wl_dnd_selection_owner_has
 * @since 1.7
 */
EINA_DEPRECATED EAPI Eina_Bool ecore_wl_dnd_selection_has_owner(Ecore_Wl_Dnd *dnd);

/**
 * @return false on error, true otherwise
 *
 * @ingroup Ecore_Wl_Dnd_Group
 * @since 1.8
 */
EAPI Eina_Bool ecore_wl_dnd_selection_set(Ecore_Wl_Input *input, const char **types_offered);

/**
 * @return false if type not found or on error, true otherwise
 *
 * @ingroup Ecore_Wl_Dnd_Group
 * @since 1.8
 */
EAPI Eina_Bool ecore_wl_dnd_selection_get(Ecore_Wl_Input *input, const char *type);

/**
 * @return true if input has a selection source, false otherwise or on error
 *
 * @ingroup Ecore_Wl_Dnd_Group
 * @since 1.8
 */
EAPI Eina_Bool ecore_wl_dnd_selection_owner_has(Ecore_Wl_Input *input);

/**
 * @return false on error, true otherwise
 *
 * @ingroup Ecore_Wl_Dnd_Group
 * @since 1.8
 */
EAPI Eina_Bool ecore_wl_dnd_selection_clear(Ecore_Wl_Input *input);

/**
 * @ingroup Ecore_Wl_Dnd_Group
 * @since 1.8
 */
EAPI void ecore_wl_dnd_drag_start(Ecore_Wl_Input *input, Ecore_Wl_Window *win, Ecore_Wl_Window *dragwin, int x, int y, int w, int h);

/**
 * @ingroup Ecore_Wl_Dnd_Group
 * @since 1.8
 */
EAPI void ecore_wl_dnd_drag_end(Ecore_Wl_Input *input);

/**
 * @return false if type not found or on error, true otherwise
 *
 * @ingroup Ecore_Wl_Dnd_Group
 * @since 1.8
 */
EAPI Eina_Bool ecore_wl_dnd_drag_get(Ecore_Wl_Input *input, const char *type);

/**
 * @ingroup Ecore_Wl_Dnd_Group
 * @since 1.8
 */
EAPI void ecore_wl_dnd_drag_types_set(Ecore_Wl_Input *input, const char **types_offered);

/**
 * @return the data types being offered for drag-and-drop, or NULL on error
 *
 * @ingroup Ecore_Wl_Dnd_Group
 * @since 1.8
 */
EAPI struct wl_array *ecore_wl_dnd_drag_types_get(Ecore_Wl_Input *input);

/**
 * @ingroup Ecore_Wl_Dnd_Group
 * @since 1.8
 */
/* EAPI Ecore_Wl_Dnd_Source *ecore_wl_dnd_drag_source_create(Ecore_Wl_Dnd *dnd); */

/**
 * Enable/disable server mode.
 *
 * With this enabled, the compositor acts as a Wayland server, iterating
 * the display and dispatching events.  With it disabled, the process is
 * handled as a client.
 */
EAPI void ecore_wl_server_mode_set(Eina_Bool on);

EAPI Eina_Bool ecore_wl_server_mode_get(void);

/**
 * @defgroup Ecore_Wl_Subsurf Functions to manipulate subsurfaces.
 * @ingroup Ecore_Wl_Group
 *
 * Functions to manipulate wayland subsurfaces, using Ecore_Wl_Subsurf.
 *
 * This API is intended to expose Wayland subsurface functionality, although it
 * should not be necessary for most applications to use it, as soon as we have
 * means to make Evas automatically switch Evas images to use subsurfaces.
 *
 * It can/should be used, for instance, when subsurfaces are needed to be not
 * in sync with the main window surface.
 */

/**
 * Create and return a new subsurface.
 *
 * Create a new surface (and subsurface interface), with the parent surface
 * being the one associated with the given @param win.
 *
 * The @param win must be visible, otherwise there will be no surface created
 * for it yet.
 *
 * @return the allocated and initialized Ecore_Wl_Subsurf object, or
 * NULL on failure
 *
 * @ingroup Ecore_Wl_Subsurf
 * @since 1.8
 */
EAPI Ecore_Wl_Subsurf *ecore_wl_subsurf_create(Ecore_Wl_Window *win);

/**
 * Destroy the given subsurface, as well as the surface associated with it.
 *
 * @param ess the subsurface
 *
 * @ingroup Ecore_Wl_Subsurf
 * @since 1.8
 */
EAPI void ecore_wl_subsurf_del(Ecore_Wl_Subsurf *ess);

/**
 * Get the wl_surface for this subsurface
 *
 * @param ess the subsurface
 * @return the wl_surface associated with this subsurface, or NULL on failure
 *
 * @ingroup Ecore_Wl_Subsurf
 * @since 1.8
 */
EAPI struct wl_surface *ecore_wl_subsurf_surface_get(Ecore_Wl_Subsurf *ess);

/**
 * Set the position of this subsurface, relative to its parent surface.
 * If ess is defined and the x, y coordinates differ from the currently
 * tracked position, this also schedules a sub-surface position change.
 *
 * @param ess the subsurface
 * @param x coordinate in the parent surface
 * @param y coordinate in the parent surface
 *
 * @ingroup Ecore_Wl_Subsurf
 * @since 1.8
 */
EAPI void ecore_wl_subsurf_position_set(Ecore_Wl_Subsurf *ess, int x, int y);

/**
 * Get the position of this subsurface, relative to its parent surface.
 * Coordinates will be returned in x and y if non-NULL.
 *
 * @param ess the subsurface
 * @param x coordinate in the parent surface, or NULL to ignore
 * @param y coordinate in the parent surface, or NULL to ignore
 *
 * @ingroup Ecore_Wl_Subsurf
 * @since 1.8
 */
EAPI void ecore_wl_subsurf_position_get(Ecore_Wl_Subsurf *ess, int *x, int *y);

/**
 * Place subsurface on layer above a reference surface
 *
 * Moves the @param ess subsurface to just above the reference @param
 * surface, changing the z-order.  The reference @param surface must
 * be either a sibling or parent surface, else a protocol error will
 * be generated.
 *
 * @param ess the subsurface
 * @param surface the sibling reference surface
 *
 * @ingroup Ecore_Wl_Subsurf
 * @since 1.8
 */
EAPI void ecore_wl_subsurf_place_above(Ecore_Wl_Subsurf *ess, struct wl_surface *surface);

/**
 * Place subsurface on layer below a reference surface
 *
 * See ecore_wl_subsurf_place_above.
 *
 * @param ess the subsurface
 * @param surface the sibling reference surface
 *
 * @ingroup Ecore_Wl_Subsurf
 * @since 1.8
 */
EAPI void ecore_wl_subsurf_place_below(Ecore_Wl_Subsurf *ess, struct wl_surface *surface);

/**
 * Enables or disables sub-surface synchronization
 *
 * When synchronization is enabled, surface commits on the subsurface
 * will be cached and only applied when the parent surface's state is
 * applied.  This ensures atomic updates of the parent and all of its
 * synchronized sub-surfaces.
 *
 * When synchronization is disabled, commits will apply to the pending
 * state directly without caching, just like a normal wl_surface.  If
 * there are already cached events when this is set, those events are
 * applied simultaneously with the desync event.
 *
 * Attempting to enable synchronization when the subsurface already
 * thinks it's sync'd, or desync when it believes its desync'd, will
 * be trivially ignored and will not generate a Wayland event.
 *
 * See Wayland's set_desync documentation for further details and
 * exceptional cases.
 *
 * @param ess the subsurface
 * @param val true to enable synchronization, false to desynchronize
 *
 * @ingroup Ecore_Wl_Subsurf
 * @since 1.8
 */
EAPI void ecore_wl_subsurf_sync_set(Ecore_Wl_Subsurf *ess, Eina_Bool val);

/**
 * Set an opaque region for the given subsurface.
 *
 * This is an optimization hint to the compositor to allow it avoid
 * redrawing content unnecessarily.  Note that marking transparent
 * content as opaque will cause repaint artifacts.
 *
 * Use a 0x0 region size to unset the opaque region.
 *
 * @param ess the subsurface
 * @param x coordinate in the parent surface
 * @param y coordinate in the parent surface
 * @param w width to set as opaque
 * @param h height to set as opaque
 *
 * @ingroup Ecore_Wl_Subsurf
 * @since 1.8
 */
EAPI void ecore_wl_subsurf_opaque_region_set(Ecore_Wl_Subsurf *ess, int x, int y, int w, int h);

/**
 * Set keygrab value of the window.
 *
 * @ingroup Ecore_Wl_Window
 */
EAPI Eina_Bool ecore_wl_window_keygrab_set(Ecore_Wl_Window *win, const char *key, int mod, int not_mod, int priority, Ecore_Wl_Window_Keygrab_Mode grab_mode);

/**
 * Unset keygrab value of the window.
 *
 * @ingroup Ecore_Wl_Window
 */
EAPI Eina_Bool ecore_wl_window_keygrab_unset(Ecore_Wl_Window *win, const char *key, int mod, int any_mod);

EAPI void ecore_wl_window_conformant_set(Ecore_Wl_Window *win, unsigned int is_conformant);
EAPI Eina_Bool ecore_wl_window_conformant_get(Ecore_Wl_Window *win);

EAPI void ecore_wl_window_indicator_geometry_set(Ecore_Wl_Window *win, int x, int y, int w, int h);
EAPI Eina_Bool ecore_wl_window_indicator_geometry_get(Ecore_Wl_Window *win, int *x, int *y, int *w, int *h);
EAPI void ecore_wl_window_indicator_state_set(Ecore_Wl_Window *win, Ecore_Wl_Indicator_State state);
EAPI Ecore_Wl_Indicator_State ecore_wl_window_indicator_state_get(Ecore_Wl_Window *win);
EAPI void ecore_wl_window_indicator_opacity_set(Ecore_Wl_Window *win, Ecore_Wl_Indicator_Opacity_Mode mode);
EAPI Ecore_Wl_Indicator_Opacity_Mode ecore_wl_window_indicator_opacity_get(Ecore_Wl_Window *win);

EAPI void ecore_wl_window_clipboard_geometry_set(Ecore_Wl_Window *win, int x, int y, int w, int h);
EAPI Eina_Bool ecore_wl_window_clipboard_geometry_get(Ecore_Wl_Window *win, int *x, int *y, int *w, int *h);
EAPI void ecore_wl_window_clipboard_state_set(Ecore_Wl_Window *win, Ecore_Wl_Clipboard_State state);
EAPI Ecore_Wl_Clipboard_State ecore_wl_window_clipboard_state_get(Ecore_Wl_Window *win);

EAPI void ecore_wl_window_keyboard_geometry_set(Ecore_Wl_Window *win, int x, int y, int w, int h);
EAPI Eina_Bool ecore_wl_window_keyboard_geometry_get(Ecore_Wl_Window *win, int *x, int *y, int *w, int *h);
EAPI void ecore_wl_window_keyboard_state_set(Ecore_Wl_Window *win, Ecore_Wl_Virtual_Keyboard_State state);
EAPI Ecore_Wl_Virtual_Keyboard_State ecore_wl_window_keyboard_state_get(Ecore_Wl_Window *win);

EAPI void ecore_wl_window_rotation_preferred_rotation_set(Ecore_Wl_Window *win, int rot);
EAPI void ecore_wl_window_rotation_available_rotations_set(Ecore_Wl_Window *win, const int *rots, unsigned int count);
EAPI void ecore_wl_window_rotation_change_done_send(Ecore_Wl_Window *win);
EAPI void ecore_wl_window_rotation_geometry_set(Ecore_Wl_Window *win, int rot, int x, int y, int w, int h);

EAPI Eina_List * ecore_wl_window_aux_hints_supported_get(Ecore_Wl_Window *win);
EAPI void ecore_wl_window_aux_hint_add(Ecore_Wl_Window *win, int id, const char *hint, const char *val);
EAPI void ecore_wl_window_aux_hint_change(Ecore_Wl_Window *win, int id, const char *val);
EAPI void ecore_wl_window_aux_hint_del(Ecore_Wl_Window *win, int id);

EAPI void ecore_wl_window_floating_mode_set(Ecore_Wl_Window *win, Eina_Bool floating);

#ifdef __cplusplus
}
#endif

#undef EAPI
#define EAPI

#endif
