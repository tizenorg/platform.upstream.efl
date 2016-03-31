#ifndef _ECORE_WIN_PRIVATE_H
#define _ECORE_WIN_PRIVATE_H

#ifdef EAPI
# undef EAPI
#endif

#ifdef _WIN32
# ifdef EFL_WIN_EVAS_BUILD
#  ifdef DLL_EXPORT
#   define EAPI __declspec(dllexport)
#  else
#   define EAPI
#  endif /* ! DLL_EXPORT */
# else
#  define EAPI __declspec(dllimport)
# endif /* ! EFL_WIN_BUILD */
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

#define ECORE_MAGIC_WIN 0x76543212

/** Log domain macros and variables **/

EAPI extern int _ecore_win_log_dom;

#ifdef ECORE_WIN_DEFAULT_LOG_COLOR
# undef ECORE_WIN_DEFAULT_LOG_COLOR
#endif
#define ECORE_WIN_DEFAULT_LOG_COLOR EINA_COLOR_BLUE

#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_ecore_win_log_dom, __VA_ARGS__)
#ifdef DBG
# undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(_ecore_win_log_dom, __VA_ARGS__)
#ifdef INF
# undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_ecore_win_log_dom, __VA_ARGS__)
#ifdef WRN
# undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(_ecore_win_log_dom, __VA_ARGS__)
#ifdef CRI
# undef CRI
#endif
#define CRI(...) EINA_LOG_DOM_CRIT(_ecore_win_log_dom, __VA_ARGS__)

# undef TRACE_EFL_BEGIN
# undef TRACE_EFL_END
# ifdef ENABLE_TTRACE
#  include <ttrace.h>
#  define TRACE_EFL_BEGIN(NAME) traceBegin(TTRACE_TAG_EFL, "EFL:ECORE_WIN:"#NAME)
#  define TRACE_EFL_END() traceEnd(TTRACE_TAG_EFL)
# else
#  define TRACE_EFL_BEGIN(NAME)
#  define TRACE_EFL_END()
# endif

#define PORTRAIT_CHECK(r) \
  ((r == 0) || (r == 180))

#define ECORE_WIN_PORTRAIT(ewin) \
  (PORTRAIT_CHECK(ewin->rotation))


#define IDLE_FLUSH_TIME 0.5
#ifndef _ECORE_WIN_H
typedef struct _Ecore_Win Ecore_Win;
typedef void   (*Ecore_Win_Event_Cb) (Ecore_Win *ewin);
#endif

typedef struct _Ecore_Win_Engine Ecore_Win_Engine;
typedef struct _Ecore_Win_Engine_Func Ecore_Win_Engine_Func;
typedef struct _Ecore_Win_Interface Ecore_Win_Interface;

/* Engines interfaces */
struct _Ecore_Win_Engine_Func
{
   void (*fn_free) (Ecore_Win *ewin);
   void (*fn_callback_resize_set) (Ecore_Win *ewin, Ecore_Win_Event_Cb func);
   void (*fn_callback_move_set) (Ecore_Win *ewin, Ecore_Win_Event_Cb func);
   void (*fn_callback_show_set) (Ecore_Win *ewin, Ecore_Win_Event_Cb func);
   void (*fn_callback_hide_set) (Ecore_Win *ewin, Ecore_Win_Event_Cb func);
   void (*fn_callback_delete_request_set) (Ecore_Win *ewin, Ecore_Win_Event_Cb func);
   void (*fn_callback_destroy_set) (Ecore_Win *ewin, Ecore_Win_Event_Cb func);
   void (*fn_callback_focus_in_set) (Ecore_Win *ewin, Ecore_Win_Event_Cb func);
   void (*fn_callback_focus_out_set) (Ecore_Win *ewin, Ecore_Win_Event_Cb func);
   void (*fn_callback_mouse_in_set) (Ecore_Win *ewin, Ecore_Win_Event_Cb func);
   void (*fn_callback_mouse_out_set) (Ecore_Win *ewin, Ecore_Win_Event_Cb func);
   void (*fn_callback_sticky_set) (Ecore_Win *ewin, Ecore_Win_Event_Cb func);
   void (*fn_callback_unsticky_set) (Ecore_Win *ewin, Ecore_Win_Event_Cb func);
   void (*fn_callback_pre_render_set) (Ecore_Win *ewin, Ecore_Win_Event_Cb func);
   void (*fn_callback_post_render_set) (Ecore_Win *ewin, Ecore_Win_Event_Cb func);
   void (*fn_move) (Ecore_Win *ewin, int x, int y);
   void (*fn_managed_move) (Ecore_Win *ewin, int x, int y);
   void (*fn_resize) (Ecore_Win *ewin, int w, int h);
   void (*fn_move_resize) (Ecore_Win *ewin, int x, int y, int w, int h);
   void (*fn_rotation_set) (Ecore_Win *ewin, int rot, int resize);
   void (*fn_shaped_set) (Ecore_Win *ewin, int shaped);
   void (*fn_show) (Ecore_Win *ewin);
   void (*fn_hide) (Ecore_Win *ewin);
   void (*fn_raise) (Ecore_Win *ewin);
   void (*fn_lower) (Ecore_Win *ewin);
   void (*fn_activate) (Ecore_Win *ewin);
   void (*fn_title_set) (Ecore_Win *ewin, const char *t);
   void (*fn_name_class_set) (Ecore_Win *ewin, const char *n, const char *c);
   void (*fn_size_min_set) (Ecore_Win *ewin, int w, int h);
   void (*fn_size_max_set) (Ecore_Win *ewin, int w, int h);
   void (*fn_size_base_set) (Ecore_Win *ewin, int w, int h);
   void (*fn_size_step_set) (Ecore_Win *ewin, int w, int h);
//   void (*fn_object_cursor_set) (Ecore_Win *ewin, Evas_Object *obj, int layer, int hot_x, int hot_y);
   void (*fn_object_cursor_unset) (Ecore_Win *ewin);
   void (*fn_layer_set) (Ecore_Win *ewin, int layer);
   void (*fn_focus_set) (Ecore_Win *ewin, Eina_Bool on);
   void (*fn_iconified_set) (Ecore_Win *ewin, Eina_Bool on);
   void (*fn_borderless_set) (Ecore_Win *ewin, Eina_Bool on);
   void (*fn_override_set) (Ecore_Win *ewin, Eina_Bool on);
   void (*fn_maximized_set) (Ecore_Win *ewin, Eina_Bool on);
   void (*fn_fullscreen_set) (Ecore_Win *ewin, Eina_Bool on);
   void (*fn_avoid_damage_set) (Ecore_Win *ewin, int on);
   void (*fn_withdrawn_set) (Ecore_Win *ewin, Eina_Bool on);
   void (*fn_sticky_set) (Ecore_Win *ewin, Eina_Bool on);
   void (*fn_ignore_events_set) (Ecore_Win *ewin, int ignore);
   void (*fn_alpha_set) (Ecore_Win *ewin, int alpha);
   void (*fn_transparent_set) (Ecore_Win *ewin, int transparent);
   void (*fn_profiles_set) (Ecore_Win *ewin, const char **profiles, int count);
   void (*fn_profile_set) (Ecore_Win *ewin, const char *profile);

   void (*fn_window_group_set) (Ecore_Win *ewin, const Ecore_Win *ee_group);
   void (*fn_aspect_set) (Ecore_Win *ewin, double aspect);
   void (*fn_urgent_set) (Ecore_Win *ewin, Eina_Bool on);
   void (*fn_modal_set) (Ecore_Win *ewin, Eina_Bool on);
   void (*fn_demands_attention_set) (Ecore_Win *ewin, Eina_Bool on);
   void (*fn_focus_skip_set) (Ecore_Win *ewin, Eina_Bool on);

   int (*fn_render) (Ecore_Win *ewin);
   void (*fn_screen_geometry_get) (const Ecore_Win *ewin, int *x, int *y, int *w, int *h);
   void (*fn_screen_dpi_get) (const Ecore_Win *ewin, int *xdpi, int *ydpi);
   void (*fn_msg_parent_send) (Ecore_Win *ewin, int maj, int min, void *data, int size);
   void (*fn_msg_send) (Ecore_Win *ewin, int maj, int min, void *data, int size);

   /* 1.8 abstractions */
//   void (*fn_pointer_xy_get) (const Ecore_Win *ewin, Evas_Coord *x, Evas_Coord *y);
//   Eina_Bool (*fn_pointer_warp) (const Ecore_Win *ewin, Evas_Coord x, Evas_Coord y);

   void (*fn_wm_rot_preferred_rotation_set) (Ecore_Win *ewin, int rot);
   void (*fn_wm_rot_available_rotations_set) (Ecore_Win *ewin, const int *rots, unsigned int count);
   void (*fn_wm_rot_manual_rotation_done_set) (Ecore_Win *ewin, Eina_Bool set);
   void (*fn_wm_rot_manual_rotation_done) (Ecore_Win *ewin);

   void (*fn_aux_hints_set) (Ecore_Win *ewin, const char *hints);
};

struct _Ecore_Win_Interface
{
    const char *name;
    unsigned int version;
};

struct _Ecore_Win_Engine
{
   Ecore_Win_Engine_Func *func;
   void *data;
   Eina_List *ifaces;
};

struct _Ecore_Win
{
   EINA_INLIST;
   ECORE_MAGIC;
   const char *driver;
   char       *name;
   int         x, y, w, h;
   short       rotation;
   Eina_Bool   shaped  : 1;
   Eina_Bool   visible : 1;
   Eina_Bool   draw_ok : 1;
   Eina_Bool   should_be_visible : 1;
   Eina_Bool   alpha  : 1;
   Eina_Bool   transparent  : 1;
   Eina_Bool   in  : 1;
   Eina_Bool   events_block  : 1; /* @since 1.14 */

   Eina_Hash  *data;

   struct {
      int      x, y, w, h;
   } req;

   struct {
      int      x, y;
   } mouse;

   struct {
      int      w, h;
   } expecting_resize;

   struct {
      char           *title;
      char           *name;
      char           *clas;
      struct {
         char        *name;
         char       **available_list;
         int          count;
      } profile;
      struct {
         int          w, h;
      } min, max, base, step;
      struct {
//         Evas_Object *object;
         int          layer;
         struct {
            int       x, y;
         } hot;
      } cursor;
      struct {
         Eina_Bool       supported;      // indicate that the underlying window system supports window manager rotation protocol
         Eina_Bool       app_set;        // indicate that the ee supports window manager rotation protocol
         Eina_Bool       win_resize;     // indicate that the ee will be resized by the WM
         int             angle;          // rotation value which is decided by the WM 
         int             w, h;           // window size to rotate
         int             preferred_rot;  // preferred rotation hint
         int            *available_rots; // array of avaialable rotation values
         unsigned int    count;          // number of elements of available_rots
         struct {
            Eina_Bool    set;
            Eina_Bool    wait_for_done;
            Ecore_Timer *timer;
         } manual_mode;
      } wm_rot;
      struct {
         Eina_List      *supported_list;
         Eina_List      *hints;
         int             id;
      } aux_hint;
      int             layer;
      Ecore_Window    window;
      struct wl_surface *wl_surface;
      struct wl_display *wl_disp;
	  
      unsigned char   avoid_damage;
      Ecore_Win     *group_ee;
      Ecore_Window    group_ee_win;
      double          aspect;
      Eina_Bool       focused      : 1;
      Eina_Bool       iconified    : 1;
      Eina_Bool       borderless   : 1;
      Eina_Bool       override     : 1;
      Eina_Bool       maximized    : 1;
      Eina_Bool       fullscreen   : 1;
      Eina_Bool       withdrawn    : 1;
      Eina_Bool       sticky       : 1;
      Eina_Bool       request_pos  : 1;
      Eina_Bool       draw_frame   : 1;
      Eina_Bool       hwsurface    : 1;
      Eina_Bool       urgent           : 1;
      Eina_Bool       modal            : 1;
      Eina_Bool       demand_attention : 1;
      Eina_Bool       focus_skip       : 1;
      Eina_Bool       obscured         : 1;
  } prop;

   struct {
      void          (*fn_resize) (Ecore_Win *ewin);
      void          (*fn_move) (Ecore_Win *ewin);
      void          (*fn_show) (Ecore_Win *ewin);
      void          (*fn_hide) (Ecore_Win *ewin);
      void          (*fn_delete_request) (Ecore_Win *ewin);
      void          (*fn_destroy) (Ecore_Win *ewin);
      void          (*fn_focus_in) (Ecore_Win *ewin);
      void          (*fn_focus_out) (Ecore_Win *ewin);
      void          (*fn_sticky) (Ecore_Win *ewin);
      void          (*fn_unsticky) (Ecore_Win *ewin);
      void          (*fn_mouse_in) (Ecore_Win *ewin);
      void          (*fn_mouse_out) (Ecore_Win *ewin);
      void          (*fn_pre_render) (Ecore_Win *ewin);
      void          (*fn_post_render) (Ecore_Win *ewin);
      void          (*fn_pre_free) (Ecore_Win *ewin);
      void          (*fn_state_change) (Ecore_Win *ewin);
      void          (*fn_msg_parent_handle) (Ecore_Win *ewin, int maj, int min, void *data, int size);
      void          (*fn_msg_handle) (Ecore_Win *ewin, int maj, int min, void *data, int size);
//      void          (*fn_pointer_xy_get) (const Ecore_Win *ewin, Evas_Coord *x, Evas_Coord *y);
//      Eina_Bool     (*fn_pointer_warp) (const Ecore_Win *ewin, Evas_Coord x, Evas_Coord y);
   } func;

   Ecore_Win_Engine engine;
   Eina_List *sub_ecore_win;

   struct {
      unsigned char avoid_damage;
      unsigned char resize_shape : 1;
      unsigned char shaped : 1;
      unsigned char shaped_changed : 1;
      unsigned char alpha : 1;
      unsigned char alpha_changed : 1;
      unsigned char transparent : 1;
      unsigned char transparent_changed : 1;
      int           rotation;
      int           rotation_resize;
      unsigned char rotation_changed : 1;
   } delayed;

   int refcount;

   unsigned char ignore_events : 1;
   unsigned char manual_render : 1;
   unsigned char registered : 1;
   unsigned char no_comp_sync  : 1;
   unsigned char semi_sync  : 1;
   unsigned char deleted : 1;
   unsigned char profile_supported : 1;
   unsigned char in_async_render : 1;

   Eina_Bool indicator_state : 1;
   Eina_Bool keyboard_state : 1;
   Eina_Bool clipboard_state : 1;
};

EAPI void _ecore_win_ref(Ecore_Win *ewin);
EAPI void _ecore_win_unref(Ecore_Win *ewin);

EAPI void _ecore_win_register(Ecore_Win *ewin);
EAPI void _ecore_win_free(Ecore_Win *ewin);
EAPI void _ecore_win_idle_timeout_update(Ecore_Win *ewin);

EAPI extern Eina_Bool _ecore_win_app_comp_sync;


EAPI Ecore_Win_Interface *_ecore_win_interface_get(const Ecore_Win *ewin, const char *iname);


Eina_Module *_ecore_winengine_load(const char *engine);
void _ecore_win_engine_init(void);
void _ecore_win_engine_shutdown(void);

#undef EAPI
#define EAPI

#endif
