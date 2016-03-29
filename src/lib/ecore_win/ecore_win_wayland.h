#ifndef ECORE_WIN_WAYLAND_H_
# define ECORE_WIN_WAYLAND_H_

typedef struct _Ecore_Win_Interface_Wayland Ecore_Win_Interface_Wayland;

struct _Ecore_Win_Interface_Wayland 
{
   Ecore_Win_Interface base;

   void (*resize)(Ecore_Win *ewin, int location);
   void (*move)(Ecore_Win *ewin, int x, int y);
   void (*pointer_set)(Ecore_Win *ewin, int hot_x, int hot_y);
   void (*type_set)(Ecore_Win *ewin, int type);
   Ecore_Wl_Window* (*window_get)(const Ecore_Win *ewin);
};

#endif
