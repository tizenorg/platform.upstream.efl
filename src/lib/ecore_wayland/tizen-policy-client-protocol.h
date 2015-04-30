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
struct tizen_visibility;

extern const struct wl_interface tizen_policy_interface;
extern const struct wl_interface tizen_visibility_interface;

#define TIZEN_POLICY_GET_VISIBILITY	0

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

#ifdef  __cplusplus
}
#endif

#endif