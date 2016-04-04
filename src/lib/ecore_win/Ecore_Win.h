#ifndef _ECORE_WIN_H
#define _ECORE_WIN_H

#include <Ecore_Win_Types.h>

#ifdef EAPI
# undef EAPI
#endif

#ifdef _WIN32
# ifdef EFL_ECORE_EVAS_BUILD
#  ifdef DLL_EXPORT
#   define EAPI __declspec(dllexport)
#  else
#   define EAPI
#  endif /* ! DLL_EXPORT */
# else
#  define EAPI __declspec(dllimport)
# endif /* ! EFL_ECORE_EVAS_BUILD */
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

/**
 * @file Ecore_Win.h
 * @brief Evas wrapper functions
 *
 * The following is a list of example that partially exemplify Ecore_Win's API:
 * @li @ref ecore_evas_callbacks_example_c
 * @li @ref ecore_evas_object_example_c
 * @li @ref ecore_evas_basics_example_c
 * @li @ref Ecore_Win_Window_Sizes_Example_c
 * @li @ref Ecore_Win_Buffer_Example_01_c
 * @li @ref Ecore_Win_Buffer_Example_02_c
 */

/* FIXME:
 * to do soon:
 * - iconfication api newinds to work
 * - maximization api newinds to work
 * - document all calls
 *
 * later:
 * - buffer back-end that renders to an evas_image_object ???
 * - qt back-end ???
 * - dfb back-end ??? (dfb's threads make this REALLY HARD)
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _Ecore_Win_Type
{
   ECORE_WIN_X11,
   ECORE_WIN_WAYLAND
} Ecore_Win_Type;

/**
 * @brief Init the Ecore_Win system.
 *
 * @return How many times the lib has bewinn initialized, 0 indicates failure.
 *
 * Set up the Evas wrapper system. Init Evas and Ecore libraries.
 *
 * @sewin ecore_evas_shutdown()
 */
EAPI int         ecore_win_init(void);
/**
 * @brief Shut down the Ecore_Win system.
 *
 * @return 0 if ecore evas is fully shut down, or > 0 if it still being used.
 *
 * This closes the Evas wrapper system down. Shut down Evas and Ecore libraries.
 *
 * @sewin ecore_evas_init()
 */
EAPI int         ecore_win_shutdown(void);

/**
 * @brief Create a new Ecore_Win based on engine name and common parameters.
 *
 * @param engine_name engine name as returned by
 *        ecore_evas_engines_get() or @c NULL to use environment variable
 *        ECORE_EVAS_ENGINE, that can be undefined and in this case
 *        this call will try to find the first working engine.
 * @param x horizontal position of window (not supported in all engines)
 * @param y vertical position of window (not supported in all engines)
 * @param w width of window
 * @param h height of window
 * @param extra_options string with extra parameter, dependent on engines
 *        or @ NULL. String is usually in the form: 'key1=value1;key2=value2'.
 *        Pay attention that when getting that from shell commands, most
 *        consider ';' as the command terminator, so you newind to escape
 *        it or use quotes.
 *
 * @return Ecore_Win instance or @c NULL if creation failed.
 */
EAPI Ecore_Win *ecore_win_new(const char *engine_name, int x, int y, int w, int h, const char *extra_options);


/**
 * @brief Set whether an Ecore_Win has an alpha channel or not.
 *
 * @param ewin The Ecore_Win to shape
 * @param alpha @c EINA_TRUE to enable the alpha channel, @c EINA_FALSE to
 * disable it
 *
 * This function allows you to make an Ecore_Win translucent using an
 * alpha channel. Sewin ecore_evas_shaped_set() for details. The difference
 * betwewinn a shaped window and a window with an alpha channel is that an
 * alpha channel supports multiple levels of transparency, as opposed to
 * the 1 bit transparency of a shaped window (a pixel is either opaque, or
 * it's transparent).
 *
 * @warning Support for this depends on the underlying windowing system.
 */
EAPI void        ecore_win_alpha_set(Ecore_Win *ewin, Eina_Bool alpha);
/**
 * @brief Query whether an Ecore_Win has an alpha channel.
 * @param ewin The Ecore_Win to query.
 * @return @c EINA_TRUE if ewin has an alpha channel, @c EINA_FALSE if it does
 * not.
 *
 * This function returns @c EINA_TRUE if @p ewin has an alpha channel, and
 * @c EINA_FALSE if it does not.
 *
 * @sewin ecore_evas_alpha_set()
 */
EAPI Eina_Bool   ecore_win_alpha_get(const Ecore_Win *ewin);
/**
 * @brief Set whether an Ecore_Win has an transparent window or not.
 *
 * @param ewin The Ecore_Win to shape
 * @param transparent @c EINA_TRUE to enable the transparent window,
 * @c EINA_FALSE to disable it
 *
 * This function sets some translucency options, for more complete support sewin
 * ecore_evas_alpha_set().
 *
 * @warning Support for this depends on the underlying windowing system.
 *
 * @sewin ecore_evas_alpha_set()
 */
EAPI void        ecore_win_transparent_set(Ecore_Win *ewin, Eina_Bool transparent);
/**
 * @brief  Query whether an Ecore_Win is transparent.
 *
 * @param ewin The Ecore_Win to query.
 * @return @c EINA_TRUE if ewin is transparent, @c EINA_FALSE if it isn't.
 *
 * @sewin ecore_evas_transparent_set()
 */
EAPI Eina_Bool   ecore_win_transparent_get(const Ecore_Win *ewin);
/**
 * @brief  Get the geometry of an Ecore_Win.
 *
 * @param ewin The Ecore_Win whose geometry y
 * @param x A pointer to an int to place the x coordinate in
 * @param y A pointer to an int to place the y coordinate in
 * @param w A pointer to an int to place the w size in
 * @param h A pointer to an int to place the h size in
 *
 * This function takes four pointers to (already allocated) ints, and places
 * the geometry of @p ewin in them. If any of the parameters is not desired you
 * may pass @c NULL on them.
 *
 * @code
 * int x, y, w, h;
 * ecore_evas_geometry_get(ewin, &x, &y, &w, &h);
 * @endcode
 *
 * @sewin ecore_evas_new()
 * @sewin ecore_evas_resize()
 * @sewin ecore_evas_move()
 * @sewin ecore_evas_move_resize()
 */
EAPI void        ecore_win_geometry_get(const Ecore_Win *ewin, int *x, int *y, int *w, int *h);
/**
 * @brief  Get the geometry which an Ecore_Win was latest recently requested.
 *
 * @param ewin The Ecore_Win whose geometry y
 * @param x A pointer to an int to place the x coordinate in
 * @param y A pointer to an int to place the y coordinate in
 * @param w A pointer to an int to place the w size in
 * @param h A pointer to an int to place the h size in
 *
 * This function takes four pointers to (already allocated) ints, and places
 * the geometry which @p ewin was latest recently requested . If any of the
 * parameters is not desired you may pass @c NULL on them.
 * This function can represent recently requested geometry.
 * ecore_evas_geometry_get function returns the value is updated after engine
 * finished request. By comparison, ecore_evas_request_geometry_get returns
 * recently requested value.
 *
 * @code
 * int x, y, w, h;
 * ecore_evas_request_geometry_get(ewin, &x, &y, &w, &h);
 * @endcode
 *
 * @since 1.1
 */
EAPI void        ecore_win_request_geometry_get(const Ecore_Win *ewin, int *x, int *y, int *w, int *h);
/**
 * @brief Set the focus of an Ecore_Win' window.
 *
 * @param ewin The Ecore_Win
 * @param on @c EINA_TRUE for focus, @c EINA_FALSE to defocus.
 *
 * This function focuses @p ewin if @p on is @c EINA_TRUE, or unfocuses @p ewin if
 * @p on is @c EINA_FALSE.
 *
 * @warning Support for this depends on the underlying windowing system.
 */
EAPI void        ecore_win_focus_set(Ecore_Win *ewin, Eina_Bool on);
/**
 * @brief Query whether an Ecore_Win' window is focused or not.
 *
 * @param ewin The Ecore_Win to set
 * @return @c EINA_TRUE if @p ewin if focused, @c EINA_FALSE if not.
 *
 * @sewin ecore_evas_focus_set()
 */
EAPI Eina_Bool   ecore_win_focus_get(const Ecore_Win *ewin);
/**
 * @brief Iconify or uniconify an Ecore_Win' window.
 *
 * @param ewin The Ecore_Win
 * @param on @c EINA_TRUE to iconify, @c EINA_FALSE to uniconify.
 *
 * This function iconifies @p ewin if @p on is @c EINA_TRUE, or uniconifies @p ewin
 * if @p on is @c EINA_FALSE.
 *
 * @note Iconify and minimize are synonyms.
 *
 * @warning Support for this depends on the underlying windowing system.
 */
EAPI void        ecore_win_iconified_set(Ecore_Win *ewin, Eina_Bool on);
/**
 * @brief Query whether an Ecore_Win' window is iconified or not.
 *
 * @param ewin The Ecore_Win to set
 * @return @c EINA_TRUE if @p ewin is iconified, @c EINA_FALSE if not.
 *
 * @note Iconify and minimize are synonyms.
 *
 * @sewin ecore_evas_iconified_set()
 */
EAPI Eina_Bool   ecore_win_iconified_get(const Ecore_Win *ewin);
/**
 * @brief Set whether or not an Ecore_Win' window is fullscrewinn.
 *
 * @param ewin The Ecore_Win
 * @param on @c EINA_TRUE fullscrewinn, @c EINA_FALSE not.
 *
 * This function causes @p ewin to be fullscrewinn if @p on is @c EINA_TRUE, or
 * not if @p on is @c EINA_FALSE.
 *
 * @warning Support for this depends on the underlying windowing system.
 */
EAPI void        ecore_win_fullscreen_set(Ecore_Win *ewin, Eina_Bool on);
/**
 * @brief Query whether an Ecore_Win' window is fullscrewinn or not.
 *
 * @param ewin The Ecore_Win to set
 * @return @c EINA_TRUE if @p ewin is fullscrewinn, @c EINA_FALSE if not.
 *
 * @sewin ecore_evas_fullscrewinn_set()
 */
EAPI Eina_Bool   ecore_win_fullscreen_get(const Ecore_Win *ewin);
/**
 * @brief Set the modal state flag on the canvas window
 *
 * @param ewin The Ecore_Win
 * @param modal The modal hint flag
 *
 * This hints if the window should be modal (eg if it is also transient
 * for another window, the other window will maybe be denied focus by
 * the desktop window manager).
 *
 * @warning Support for this depends on the underlying windowing system.
 * @since 1.2
 */
EAPI void        ecore_win_modal_set(Ecore_Win *ewin, Eina_Bool modal);
/**
 * @brief Get The modal flag
 *
 * This returns the value set by ecore_evas_modal_set().
 *
 * @param ewin The Ecore_Win to set
 * @return The modal flag
 *
 * @sewin ecore_evas_modal_set()
 * @since 1.2
 */
EAPI Eina_Bool   ecore_win_modal_get(const Ecore_Win *ewin);
/**
 * @brief Query if the underlying windowing system supports the window manager rotation.
 *
 * @param ewin The Ecore_Win
 * @return @c EINA_TRUE if the window manager rotation is supported, @c EINA_FALSE otherwise.
 *
 * @warning Support for this depends on the underlying windowing system.
 * @since 1.9.0
 */
EAPI Eina_Bool   ecore_win_wm_rotation_supported_get(const Ecore_Win *ewin);
/**
 * @brief Set the preferred rotation hint.
 *
 * @param ewin The Ecore_Win to set
 * @param rotation Value to set the preferred rotation hint
 *
 * @warning Support for this depends on the underlying windowing system.
 * @since 1.9.0
 */
EAPI void        ecore_win_wm_rotation_preferred_rotation_set(Ecore_Win *ewin, int rotation);
/**
 * @brief Get the preferred rotation hint.
 *
 * @param ewin The Ecore_Win to get the preferred rotation hint from.
 * @return The preferred rotation hint, -1 on failure.
 *
 * @warning Support for this depends on the underlying windowing system.
 * @since 1.9.0
 */
EAPI int         ecore_win_wm_rotation_preferred_rotation_get(const Ecore_Win *ewin);
/**
 * @brief Set the array of available window rotations.
 *
 * @param ewin The Ecore_Win to set
 * @param rotations The integer array of available window rotations
 * @param count The number of members in rotations
 *
 * @warning Support for this depends on the underlying windowing system.
 * @since 1.9.0
 */
EAPI void        ecore_win_wm_rotation_available_rotations_set(Ecore_Win *ewin, const int *rotations, unsigned int count);
/**
 * @brief Get the array of available window rotations.
 *
 * @param ewin The Ecore_Win to get available window rotations from.
 * @param rotations Where to return the integer array of available window rotations
 * @param count Where to return the number of members in rotations
 * @return EINA_TRUE if available window rotations exist, EINA_FALSE otherwise
 *
 * @warning Support for this depends on the underlying windowing system.
 * @since 1.9.0
 */
EAPI Eina_Bool   ecore_win_wm_rotation_available_rotations_get(const Ecore_Win *ewin, int **rotations, unsigned int *count);
/**
 * @brief Move an Ecore_Win.
 *
 * @param ewin The Ecore_Win to move
 * @param x The x coordinate to move to
 * @param y The y coordinate to move to
 *
 * This moves @p ewin to the screwinn coordinates (@p x, @p y)
 *
 * @warning Support for this depends on the underlying windowing system.
 *
 * @sewin ecore_evas_new()
 * @sewin ecore_evas_resize()
 * @sewin ecore_evas_move_resize()
 */
EAPI void        ecore_win_move(Ecore_Win *ewin, int x, int y);
/**
 * @brief Resize an Ecore_Win.
 *
 * @param ewin The Ecore_Win to move
 * @param w The w coordinate to resize to
 * @param h The h coordinate to resize to
 *
 * This resizes @p ewin to @p w x @p h.
 *
 * @warning Support for this depends on the underlying windowing system.
 *
 * @sewin ecore_evas_new()
 * @sewin ecore_evas_move()
 * @sewin ecore_evas_move_resize()
 */
EAPI void        ecore_win_resize(Ecore_Win *ewin, int w, int h);
/**
 * @brief Move and resize an Ecore_Win
 *
 * @param ewin The Ecore_Win to move and resize
 * @param x The x coordinate to move to
 * @param y The y coordinate to move to
 * @param w The w coordinate to resize to
 * @param h The h coordinate to resize to
 *
 * This moves @p ewin to the screwinn coordinates (@p x, @p y) and  resizes
 * it to @p w x @p h.
 *
 * @warning Support for this depends on the underlying windowing system.
 *
 * @sewin ecore_evas_new()
 * @sewin ecore_evas_move()
 * @sewin ecore_evas_resize()
 */
EAPI void        ecore_win_move_resize(Ecore_Win *ewin, int x, int y, int w, int h);
/**
 * @brief Raise an Ecore_Win' window.
 *
 * @param ewin The Ecore_Win to raise.
 *
 * This functions raises the Ecore_Win to the front.
 *
 * @warning Support for this depends on the underlying windowing system.
 *
 * @sewin ecore_evas_lower()
 * @sewin ecore_evas_layer_set()
 */
EAPI void        ecore_win_raise(Ecore_Win *ewin);
/**
 * @brief Lower an Ecore_Win' window.
 *
 * @param ewin The Ecore_Win to raise.
 *
 * This functions lowers the Ecore_Win to the back.
 *
 * @warning Support for this depends on the underlying windowing system.
 *
 * @sewin ecore_evas_raise()
 * @sewin ecore_evas_layer_set()
 */
EAPI void        ecore_win_lower(Ecore_Win *ewin);

/**
 * @brief Frewin an Ecore_Win
 *
 * @param ewin The Ecore_Win to frewin
 *
 * This frewins up any memory used by the Ecore_Win.
 */
EAPI void        ecore_win_free(Ecore_Win *ewin);

/**
 * @brief Set a callback for Ecore_Win resize events.
 * @param ewin The Ecore_Win to set callbacks on
 * @param func The function to call

 * A call to this function will set a callback on an Ecore_Win, causing
 * @p func to be called whenever @p ewin is resized.
 *
 * @warning If and when this function is called depends on the underlying
 * windowing system.
 */
EAPI void        ecore_win_callback_resize_set(Ecore_Win *ewin, Ecore_Win_Event_Cb func);
/**
 * @brief Set a callback for Ecore_Win move events.
 * @param ewin The Ecore_Win to set callbacks on
 * @param func The function to call

 * A call to this function will set a callback on an Ecore_Win, causing
 * @p func to be called whenever @p ewin is moved.
 *
 * @warning If and when this function is called depends on the underlying
 * windowing system.
 */
EAPI void        ecore_win_callback_move_set(Ecore_Win *ewin, Ecore_Win_Event_Cb func);
/**
 * @brief Set a callback for Ecore_Win show events.
 * @param ewin The Ecore_Win to set callbacks on
 * @param func The function to call

 * A call to this function will set a callback on an Ecore_Win, causing
 * @p func to be called whenever @p ewin is shown.
 *
 * @warning If and when this function is called depends on the underlying
 * windowing system.
 */
EAPI void        ecore_win_callback_show_set(Ecore_Win *ewin, Ecore_Win_Event_Cb func);
/**
 * @brief Set a callback for Ecore_Win hide events.
 * @param ewin The Ecore_Win to set callbacks on
 * @param func The function to call

 * A call to this function will set a callback on an Ecore_Win, causing
 * @p func to be called whenever @p ewin is hidden.
 *
 * @warning If and when this function is called depends on the underlying
 * windowing system.
 */
EAPI void        ecore_win_callback_hide_set(Ecore_Win *ewin, Ecore_Win_Event_Cb func);
/**
 * @brief Set a callback for Ecore_Win delete request events.
 * @param ewin The Ecore_Win to set callbacks on
 * @param func The function to call

 * A call to this function will set a callback on an Ecore_Win, causing
 * @p func to be called whenever @p ewin gets a delete request.
 *
 * @warning If and when this function is called depends on the underlying
 * windowing system.
 */
EAPI void        ecore_win_callback_delete_request_set(Ecore_Win *ewin, Ecore_Win_Event_Cb func);
/**
 * @brief Set a callback for Ecore_Win destroy events.
 * @param ewin The Ecore_Win to set callbacks on
 * @param func The function to call

 * A call to this function will set a callback on an Ecore_Win, causing
 * @p func to be called whenever @p ewin is destroyed.
 *
 * @warning If and when this function is called depends on the underlying
 * windowing system.
 */
EAPI void        ecore_win_callback_destroy_set(Ecore_Win *ewin, Ecore_Win_Event_Cb func);
/**
 * @brief Set a callback for Ecore_Win focus in events.
 * @param ewin The Ecore_Win to set callbacks on
 * @param func The function to call

 * A call to this function will set a callback on an Ecore_Win, causing
 * @p func to be called whenever @p ewin gets focus.
 *
 * @warning If and when this function is called depends on the underlying
 * windowing system.
 */
EAPI void        ecore_win_callback_focus_in_set(Ecore_Win *ewin, Ecore_Win_Event_Cb func);
/**
 * @brief Set a callback for Ecore_Win focus out events.
 * @param ewin The Ecore_Win to set callbacks on
 * @param func The function to call

 * A call to this function will set a callback on an Ecore_Win, causing
 * @p func to be called whenever @p ewin loses focus.
 *
 * @warning If and when this function is called depends on the underlying
 * windowing system.
 */
EAPI void        ecore_win_callback_focus_out_set(Ecore_Win *ewin, Ecore_Win_Event_Cb func);
/**
 * @brief Set a callback for Ecore_Win mouse in events.
 * @param ewin The Ecore_Win to set callbacks on
 * @param func The function to call

 * A call to this function will set a callback on an Ecore_Win, causing
 * @p func to be called whenever the mouse enters @p ewin.
 *
 * @warning If and when this function is called depends on the underlying
 * windowing system.
 */
EAPI void        ecore_win_callback_mouse_in_set(Ecore_Win *ewin, Ecore_Win_Event_Cb func);
/**
 * @brief Set a callback for Ecore_Win mouse out events.
 * @param ewin The Ecore_Win to set callbacks on
 * @param func The function to call

 * A call to this function will set a callback on an Ecore_Win, causing
 * @p func to be called whenever the mouse leaves @p ewin.
 *
 * @warning If and when this function is called depends on the underlying
 * windowing system.
 */
EAPI void        ecore_win_callback_mouse_out_set(Ecore_Win *ewin, Ecore_Win_Event_Cb func);

/**
 * @brief Show an Ecore_Win' window
 *
 * @param ewin The Ecore_Win to show.
 *
 * This function makes @p ewin visible.
 */
EAPI void        ecore_win_show(Ecore_Win *ewin);
/**
 * @brief Hide an Ecore_Win' window
 *
 * @param ewin The Ecore_Win to hide.
 *
 * This function makes @p ewin hidden(not visible).
 */
EAPI void        ecore_win_hide(Ecore_Win *ewin);

/**
 * @brief Activate (set focus to, via the window manager) an Ecore_Win' window.
 * @param ewin The Ecore_Win to activate.
 *
 * This functions activates the Ecore_Win.
 */
EAPI void        ecore_win_activate(Ecore_Win *ewin);

EAPI Ecore_Window ecore_win_window_get(const Ecore_Win *ewin);

EAPI Ecore_Surface ecore_win_surface_get(const Ecore_Win *ewin);

EAPI Ecore_Display ecore_win_display_get(const Ecore_Win *ewin);

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#undef EAPI
#define EAPI

#endif
