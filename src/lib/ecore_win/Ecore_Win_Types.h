#ifndef _ECORE_WIN_TYPES_H_
#define _ECORE_WIN_TYPES_H_


#ifndef _ECORE_WIN_PRIVATE_H
/* basic data types */
typedef struct _Ecore_Win Ecore_Win;
typedef void   (*Ecore_Win_Event_Cb) (Ecore_Win *ewin); /**< Callback used for several ecore evas events @since 1.2 */
#endif

typedef uintptr_t Ecore_Window;
typedef void* Ecore_Display;
#define _ECORE_WINDOW_PREDEF 1

#endif /* _ECORE_WIN_TYPES_H_ */
