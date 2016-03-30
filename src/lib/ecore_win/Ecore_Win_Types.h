#ifndef _ECORE_WIN_TYPES_H_
#define _ECORE_WIN_TYPES_H_

#ifndef _ECORE_X_H
#define _ECORE_X_WINDOW_PREDEF
typedef unsigned int Ecore_X_Window;
typedef unsigned int Ecore_X_Pixmap;
typedef unsigned int Ecore_X_Atom;
typedef struct _Ecore_X_Icon 
{
   unsigned int width, height;
   unsigned int *data;
} Ecore_X_Icon;
#endif

#ifndef _ECORE_WIN_PRIVATE_H
/* basic data types */
typedef struct _Ecore_Win Ecore_Win;
typedef void   (*Ecore_Win_Event_Cb) (Ecore_Win *ewin); /**< Callback used for several ecore evas events @since 1.2 */
#endif

#ifndef _ECORE_WAYLAND_H_
#define _ECORE_WAYLAND_WINDOW_PREDEF
typedef struct _Ecore_Wl_Window Ecore_Wl_Window;
#endif

typedef uintptr_t Ecore_Window;
typedef void* Ecore_Display;
typedef void* Ecore_Surface;
#define _ECORE_WINDOW_PREDEF 1

#endif /* _ECORE_WIN_TYPES_H_ */
