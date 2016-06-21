#ifndef TIZEN_POLICY_EXT_CLIENT_PROTOCOL_H
#define TIZEN_POLICY_EXT_CLIENT_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

struct wl_client;
struct wl_resource;

struct tizen_policy_ext;
struct tizen_rotation;

extern const struct wl_interface tizen_policy_ext_interface;
extern const struct wl_interface tizen_rotation_interface;

struct tizen_policy_ext_listener {
	/**
	 * active_angle - (none)
	 * @angle: (none)
	 */
	void (*active_angle)(void *data,
			     struct tizen_policy_ext *tizen_policy_ext,
			     uint32_t angle);
};

static inline int
tizen_policy_ext_add_listener(struct tizen_policy_ext *tizen_policy_ext,
			      const struct tizen_policy_ext_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) tizen_policy_ext,
				     (void (**)(void)) listener, data);
}

#define TIZEN_POLICY_EXT_GET_ROTATION	0
#define TIZEN_POLICY_EXT_GET_ACTIVE_ANGLE	1

static inline void
tizen_policy_ext_set_user_data(struct tizen_policy_ext *tizen_policy_ext, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) tizen_policy_ext, user_data);
}

static inline void *
tizen_policy_ext_get_user_data(struct tizen_policy_ext *tizen_policy_ext)
{
	return wl_proxy_get_user_data((struct wl_proxy *) tizen_policy_ext);
}

static inline void
tizen_policy_ext_destroy(struct tizen_policy_ext *tizen_policy_ext)
{
	wl_proxy_destroy((struct wl_proxy *) tizen_policy_ext);
}

static inline struct tizen_rotation *
tizen_policy_ext_get_rotation(struct tizen_policy_ext *tizen_policy_ext, struct wl_surface *surface)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor((struct wl_proxy *) tizen_policy_ext,
			 TIZEN_POLICY_EXT_GET_ROTATION, &tizen_rotation_interface, NULL, surface);

	return (struct tizen_rotation *) id;
}

static inline void
tizen_policy_ext_get_active_angle(struct tizen_policy_ext *tizen_policy_ext, struct wl_surface *surface)
{
	wl_proxy_marshal((struct wl_proxy *) tizen_policy_ext,
			 TIZEN_POLICY_EXT_GET_ACTIVE_ANGLE, surface);
}

#ifndef TIZEN_ROTATION_ANGLE_ENUM
#define TIZEN_ROTATION_ANGLE_ENUM
enum tizen_rotation_angle {
	TIZEN_ROTATION_ANGLE_NONE = 0,
	TIZEN_ROTATION_ANGLE_0 = 1,
	TIZEN_ROTATION_ANGLE_90 = 2,
	TIZEN_ROTATION_ANGLE_180 = 4,
	TIZEN_ROTATION_ANGLE_270 = 8,
};
#endif /* TIZEN_ROTATION_ANGLE_ENUM */

struct tizen_rotation_listener {
	/**
	 * available_angles_done - (none)
	 * @angles: (none)
	 */
	void (*available_angles_done)(void *data,
				      struct tizen_rotation *tizen_rotation,
				      uint32_t angles);
	/**
	 * preferred_angle_done - (none)
	 * @angle: (none)
	 */
	void (*preferred_angle_done)(void *data,
				     struct tizen_rotation *tizen_rotation,
				     uint32_t angle);
	/**
	 * angle_change - suggest a angle_change
	 * @angle: (none)
	 * @serial: (none)
	 *
	 * 
	 */
	void (*angle_change)(void *data,
			     struct tizen_rotation *tizen_rotation,
			     uint32_t angle,
			     uint32_t serial);
};

static inline int
tizen_rotation_add_listener(struct tizen_rotation *tizen_rotation,
			    const struct tizen_rotation_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) tizen_rotation,
				     (void (**)(void)) listener, data);
}

#define TIZEN_ROTATION_DESTROY	0
#define TIZEN_ROTATION_SET_AVAILABLE_ANGLES	1
#define TIZEN_ROTATION_SET_PREFERRED_ANGLE	2
#define TIZEN_ROTATION_ACK_ANGLE_CHANGE	3

static inline void
tizen_rotation_set_user_data(struct tizen_rotation *tizen_rotation, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) tizen_rotation, user_data);
}

static inline void *
tizen_rotation_get_user_data(struct tizen_rotation *tizen_rotation)
{
	return wl_proxy_get_user_data((struct wl_proxy *) tizen_rotation);
}

static inline uint32_t
tizen_rotation_get_version(struct tizen_rotation *tizen_rotation)
{
	return wl_proxy_get_version((struct wl_proxy *) tizen_rotation);
}

static inline void
tizen_rotation_destroy(struct tizen_rotation *tizen_rotation)
{
	wl_proxy_marshal((struct wl_proxy *) tizen_rotation,
			 TIZEN_ROTATION_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) tizen_rotation);
}

static inline void
tizen_rotation_set_available_angles(struct tizen_rotation *tizen_rotation, uint32_t angles)
{
	wl_proxy_marshal((struct wl_proxy *) tizen_rotation,
			 TIZEN_ROTATION_SET_AVAILABLE_ANGLES, angles);
}

static inline void
tizen_rotation_set_preferred_angle(struct tizen_rotation *tizen_rotation, uint32_t angle)
{
	wl_proxy_marshal((struct wl_proxy *) tizen_rotation,
			 TIZEN_ROTATION_SET_PREFERRED_ANGLE, angle);
}

static inline void
tizen_rotation_ack_angle_change(struct tizen_rotation *tizen_rotation, uint32_t serial)
{
	wl_proxy_marshal((struct wl_proxy *) tizen_rotation,
			 TIZEN_ROTATION_ACK_ANGLE_CHANGE, serial);
}

#ifdef  __cplusplus
}
#endif

#endif
