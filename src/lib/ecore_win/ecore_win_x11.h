#ifndef ECORE_WIN_X11_H_
# define ECORE_WIN_X11_H_

typedef struct _Ecore_Win_Interface_X11 Ecore_Win_Interface_X11;
typedef struct _Ecore_Win_Interface_Software_X11 Ecore_Win_Interface_Software_X11;
typedef struct _Ecore_Win_Interface_Gl_X11 Ecore_Win_Interface_Gl_X11;

struct _Ecore_Win_Interface_X11 {
   Ecore_Win_Interface base;

   void           (*leader_set)(Ecore_Win *ee, Ecore_X_Window win);
   Ecore_X_Window (*leader_get)(Ecore_Win *ee);
   void           (*leader_default_set)(Ecore_Win *ee);
   void           (*shape_input_rectangle_set)(Ecore_Win *ee, int x, int y, int w, int h);
   void           (*shape_input_rectangle_add)(Ecore_Win *ee, int x, int y, int w, int h);
   void           (*shape_input_rectangle_subtract)(Ecore_Win *ee, int x, int y, int w, int h);
   void           (*shape_input_empty)(Ecore_Win *ee);
   void           (*shape_input_reset)(Ecore_Win *ee);
   void           (*shape_input_apply)(Ecore_Win *ee);
};

struct _Ecore_Win_Interface_Software_X11 {
   Ecore_Win_Interface base;

   Ecore_X_Window (*window_get)(const Ecore_Win *ee);
   Ecore_X_Pixmap (*pixmap_get)(const Ecore_Win *ee);
   void           (*resize_set)(Ecore_Win *ee, Eina_Bool on);
   Eina_Bool      (*resize_get)(const Ecore_Win *ee);
   void           (*extra_event_window_add)(Ecore_Win *ee, Ecore_X_Window win);

   void          *(*pixmap_visual_get)(const Ecore_Win *ee);
   unsigned long  (*pixmap_colormap_get)(const Ecore_Win *ee);
   int            (*pixmap_depth_get)(const Ecore_Win *ee);
};

struct _Ecore_Win_Interface_Gl_X11 {
   Ecore_Win_Interface base;

   Ecore_X_Window  (*window_get)(const Ecore_Win *ee);
   Ecore_X_Pixmap  (*pixmap_get)(const Ecore_Win *ee);
   void            (*resize_set)(Ecore_Win *ee, Eina_Bool on);
   Eina_Bool       (*resize_get)(const Ecore_Win *ee);
   void            (*extra_event_window_add)(Ecore_Win *ee, Ecore_X_Window win);
   void            (*pre_post_swap_callback_set)(const Ecore_Win *ee, void *data, void (*pre_cb) (void *data, Evas *e), void (*post_cb) (void *data, Evas *e));

   void          *(*pixmap_visual_get)(const Ecore_Win *ee);
   unsigned long  (*pixmap_colormap_get)(const Ecore_Win *ee);
   int            (*pixmap_depth_get)(const Ecore_Win *ee);
};

#endif
