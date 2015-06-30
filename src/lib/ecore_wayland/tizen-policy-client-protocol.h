#ifndef TIZEN_POLICY_CLIENT_PROTOCOL_H
#define TIZEN_POLICY_CLIENT_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

struct wl_client;
struct wl_resource;

struct tizen_policy;
struct tizen_position;
struct tizen_visibility;
struct wl_surface;

extern const struct wl_interface tizen_policy_interface;
extern const struct wl_interface tizen_visibility_interface;
extern const struct wl_interface tizen_position_interface;

#define TIZEN_POLICY_GET_VISIBILITY	0
#define TIZEN_POLICY_GET_POSITION	1
#define TIZEN_POLICY_ACTIVATE	2
#define TIZEN_POLICY_LOWER	3
#define TIZEN_POLICY_FOCUS_SKIP_SET	4
#define TIZEN_POLICY_FOCUS_SKIP_UNSET	5
#define TIZEN_POLICY_ROLE_SET	6

static inline void
tizen_policy_set_user_data(struct tizen_policy *tizen_policy, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) tizen_policy, user_data);
}

static inline void *
tizen_policy_get_user_data(struct tizen_policy *tizen_policy)
{
	return wl_proxy_get_user_data((struct wl_proxy *) tizen_policy);
}

static inline void
tizen_policy_destroy(struct tizen_policy *tizen_policy)
{
	wl_proxy_destroy((struct wl_proxy *) tizen_policy);
}

static inline struct tizen_visibility *
tizen_policy_get_visibility(struct tizen_policy *tizen_policy, struct wl_surface *surface)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor((struct wl_proxy *) tizen_policy,
			 TIZEN_POLICY_GET_VISIBILITY, &tizen_visibility_interface, NULL, surface);

	return (struct tizen_visibility *) id;
}

static inline struct tizen_position *
tizen_policy_get_position(struct tizen_policy *tizen_policy, struct wl_surface *surface)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor((struct wl_proxy *) tizen_policy,
			 TIZEN_POLICY_GET_POSITION, &tizen_position_interface, NULL, surface);

	return (struct tizen_position *) id;
}

static inline void
tizen_policy_activate(struct tizen_policy *tizen_policy, struct wl_surface *surface)
{
	wl_proxy_marshal((struct wl_proxy *) tizen_policy,
			 TIZEN_POLICY_ACTIVATE, surface);
}

static inline void
tizen_policy_lower(struct tizen_policy *tizen_policy, struct wl_surface *surface)
{
	wl_proxy_marshal((struct wl_proxy *) tizen_policy,
			 TIZEN_POLICY_LOWER, surface);
}

static inline void
tizen_policy_focus_skip_set(struct tizen_policy *tizen_policy, struct wl_surface *surface)
{
	wl_proxy_marshal((struct wl_proxy *) tizen_policy,
			 TIZEN_POLICY_FOCUS_SKIP_SET, surface);
}

static inline void
tizen_policy_focus_skip_unset(struct tizen_policy *tizen_policy, struct wl_surface *surface)
{
	wl_proxy_marshal((struct wl_proxy *) tizen_policy,
			 TIZEN_POLICY_FOCUS_SKIP_UNSET, surface);
}

static inline void
tizen_policy_role_set(struct tizen_policy *tizen_policy, struct wl_surface *surface, const char *role)
{
	wl_proxy_marshal((struct wl_proxy *) tizen_policy,
			 TIZEN_POLICY_ROLE_SET, surface, role);
}

#ifndef TIZEN_VISIBILITY_VISIBILITY_ENUM
#define TIZEN_VISIBILITY_VISIBILITY_ENUM
enum tizen_visibility_visibility {
	TIZEN_VISIBILITY_VISIBILITY_UNOBSCURED = 0,
	TIZEN_VISIBILITY_VISIBILITY_PARTIALLY_OBSCURED = 1,
	TIZEN_VISIBILITY_VISIBILITY_FULLY_OBSCURED = 2,
};
#endif /* TIZEN_VISIBILITY_VISIBILITY_ENUM */

struct tizen_visibility_listener {
	/**
	 * notify - (none)
	 * @visibility: (none)
	 */
	void (*notify)(void *data,
		       struct tizen_visibility *tizen_visibility,
		       uint32_t visibility);
};

static inline int
tizen_visibility_add_listener(struct tizen_visibility *tizen_visibility,
			      const struct tizen_visibility_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) tizen_visibility,
				     (void (**)(void)) listener, data);
}

#define TIZEN_VISIBILITY_DESTROY	0

static inline void
tizen_visibility_set_user_data(struct tizen_visibility *tizen_visibility, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) tizen_visibility, user_data);
}

static inline void *
tizen_visibility_get_user_data(struct tizen_visibility *tizen_visibility)
{
	return wl_proxy_get_user_data((struct wl_proxy *) tizen_visibility);
}

static inline void
tizen_visibility_destroy(struct tizen_visibility *tizen_visibility)
{
	wl_proxy_marshal((struct wl_proxy *) tizen_visibility,
			 TIZEN_VISIBILITY_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) tizen_visibility);
}

struct tizen_position_listener {
	/**
	 * changed - (none)
	 * @x: (none)
	 * @y: (none)
	 */
	void (*changed)(void *data,
			struct tizen_position *tizen_position,
			int32_t x,
			int32_t y);
};

static inline int
tizen_position_add_listener(struct tizen_position *tizen_position,
			    const struct tizen_position_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) tizen_position,
				     (void (**)(void)) listener, data);
}

#define TIZEN_POSITION_DESTROY	0
#define TIZEN_POSITION_SET	1

static inline void
tizen_position_set_user_data(struct tizen_position *tizen_position, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) tizen_position, user_data);
}

static inline void *
tizen_position_get_user_data(struct tizen_position *tizen_position)
{
	return wl_proxy_get_user_data((struct wl_proxy *) tizen_position);
}

static inline void
tizen_position_destroy(struct tizen_position *tizen_position)
{
	wl_proxy_marshal((struct wl_proxy *) tizen_position,
			 TIZEN_POSITION_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) tizen_position);
}

static inline void
tizen_position_set(struct tizen_position *tizen_position, int32_t x, int32_t y)
{
	wl_proxy_marshal((struct wl_proxy *) tizen_position,
			 TIZEN_POSITION_SET, x, y);
}

#ifdef  __cplusplus
}
#endif

#endif
