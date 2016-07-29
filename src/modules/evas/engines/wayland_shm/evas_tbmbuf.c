#include "evas_common_private.h"
#include "evas_private.h"
#include "evas_engine.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <wayland-client.h>

#define KEY_WINDOW (unsigned long)(&key_window)
#define KEY_WL_BUFFER (unsigned long)(&key_wl_buffer)

static int key_window;
static int key_wl_buffer;



#define __tbm_fourcc_code(a,b,c,d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | \
                        ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

#define TBM_FORMAT_XRGB8888 __tbm_fourcc_code('X', 'R', '2', '4')
#define TBM_FORMAT_ARGB8888 __tbm_fourcc_code('A', 'R', '2', '4')
#define TBM_SURF_PLANE_MAX 4 /**< maximum number of the planes  */
/* option to map the tbm_surface */
#define TBM_SURF_OPTION_READ      (1 << 0) /**< access option to read  */
#define TBM_SURF_OPTION_WRITE     (1 << 1) /**< access option to write */

typedef struct _tbm_surface * tbm_surface_h;
//typedef struct _tbm_surface_queue *tbm_surface_queue_h;
typedef uint32_t tbm_format;
typedef void (*tbm_data_free) (void *user_data);

typedef struct _tbm_surface_plane
{
    unsigned char *ptr;   /**< Plane pointer */
    uint32_t size;        /**< Plane size */
    uint32_t offset;      /**< Plane offset */
    uint32_t stride;      /**< Plane stride */

    void *reserved1;      /**< Reserved pointer1 */
    void *reserved2;      /**< Reserved pointer2 */
    void *reserved3;      /**< Reserved pointer3 */
} tbm_surface_plane_s;

typedef struct _tbm_surface_info
{
    uint32_t width;      /**< TBM surface width */
    uint32_t height;     /**< TBM surface height */
    tbm_format format;   /**< TBM surface format*/
    uint32_t bpp;        /**< TBM surface bbp */
    uint32_t size;       /**< TBM surface size */

    uint32_t num_planes;                            /**< The number of planes */
    tbm_surface_plane_s planes[TBM_SURF_PLANE_MAX]; /**< Array of planes */

    void *reserved4;   /**< Reserved pointer4 */
    void *reserved5;   /**< Reserved pointer5 */
    void *reserved6;   /**< Reserved pointer6 */
} tbm_surface_info_s;


typedef enum _queue_node_type {
    QUEUE_NODE_TYPE_NONE,
    QUEUE_NODE_TYPE_DEQUEUE,
    QUEUE_NODE_TYPE_ENQUEUE,
    QUEUE_NODE_TYPE_ACQUIRE,
    QUEUE_NODE_TYPE_RELEASE
} Queue_Node_Type;

typedef enum {
    TBM_SURFACE_QUEUE_ERROR_NONE = 0,                             /**< Successful */
    TBM_SURFACE_QUEUE_ERROR_INVALID_SURFACE = -1,
    TBM_SURFACE_QUEUE_ERROR_INVALID_QUEUE = -2,
    TBM_SURFACE_QUEUE_ERROR_EMPTY = -3,
    TBM_SURFACE_QUEUE_ERROR_INVALID_PARAMETER = -4,
    TBM_SURFACE_QUEUE_ERROR_SURFACE_ALLOC_FAILED = -5,
} tbm_surface_queue_error_e;

typedef struct _Tbmbuf_Surface Tbmbuf_Surface;

struct _Tbmbuf_Surface
{
   struct wl_display *wl_display;
   struct wl_surface *wl_surface;
   struct wl_buffer *wl_buffer;
   struct wl_registry *registry;
   struct wayland_tbm_client *tbm_client;
   tbm_surface_h tbm_surface;
   void *tbm_queue;
   int wait_release;
   int mapping;
   int dequeue;

   int compositor_version;

   int w, h;
   int dx, dy;
   int stride;

   Eina_Bool alpha : 1;
};

static void *tbm_lib = NULL;
static void *tbm_client_lib = NULL;
static int   tbm_ref = 0;

static int (*sym_tbm_surface_map) (tbm_surface_h surface, int opt, tbm_surface_info_s *info) = NULL;
static int (*sym_tbm_surface_unmap) (tbm_surface_h surface) = NULL;
static int (*sym_tbm_surface_queue_can_dequeue) (void *tbm_queue, int value) = NULL;
static int (*sym_tbm_surface_queue_dequeue) (void *tbm_queue, tbm_surface_h *surface) = NULL;
static int (*sym_tbm_surface_queue_enqueue) (void *tbm_queue, tbm_surface_h surface) = NULL;
static tbm_surface_queue_error_e (*sym_tbm_surface_queue_acquire) (void *tbm_queue, tbm_surface_h *surface) = NULL;
static int (*sym_tbm_surface_get_width) (tbm_surface_h surface) = NULL;
static int (*sym_tbm_surface_get_height) (tbm_surface_h surface) = NULL;
static tbm_surface_queue_error_e (*sym_tbm_surface_queue_get_surfaces) (
                                                            void *surface_queue,
                                                            tbm_surface_h *surfaces, int *num) = NULL;
static tbm_surface_queue_error_e (*sym_tbm_surface_queue_release) (void *surface_queue, tbm_surface_h surface) = NULL;
static void (*sym_tbm_surface_queue_destroy) (void *surface_queue) = NULL;
static void (*sym_tbm_surface_internal_unref) (tbm_surface_h surface) = NULL;
static void (*sym_tbm_surface_internal_ref) (tbm_surface_h surface) = NULL;
static int (*sym_tbm_surface_internal_get_user_data) (tbm_surface_h surface, unsigned long key, void **data) = NULL;
static int (*sym_tbm_surface_internal_add_user_data) (tbm_surface_h surface, unsigned long key, tbm_data_free data_free_func) = NULL;
static int (*sym_tbm_surface_internal_set_user_data) (tbm_surface_h surface, unsigned long key, void *data) = NULL;


static struct wayland_tbm_client * (*sym_wayland_tbm_client_init) (struct wl_display *display) = NULL;
static void (*sym_wayland_tbm_client_deinit) (struct wayland_tbm_client *tbm_client) = NULL;
static struct wl_buffer * (*sym_wayland_tbm_client_create_buffer) (struct wayland_tbm_client *tbm_client, tbm_surface_h surface) = NULL;
static struct wl_buffer * (*sym_wayland_tbm_client_destroy_buffer) (struct wayland_tbm_client *tbm_client, struct wl_buffer *buffer) = NULL;
static void *(*sym_wayland_tbm_client_create_surface_queue) (struct wayland_tbm_client *tbm_client,
                                                         struct wl_surface *surface,
                                                         int queue_size,
                                                         int width, int height, tbm_format format) = NULL;
static struct wl_tbm_queue * (*sym_wayland_tbm_client_get_wl_tbm_queue) (struct wayland_tbm_client *tbm_client, struct wl_surface *surface) = NULL;

static void _evas_tbmbuf_surface_destroy(Surface *s);

static Eina_Bool
tbm_init(void)
{
   if (tbm_lib)
     {
        tbm_ref++;
        return EINA_TRUE;
     }

   const char *tbm_libs[] =
   {
      "libtbm.so.1",
      "libtbm.so.0",
      NULL,
   };
   const char *tbm_clients[] =
   {
      "libwayland-tbm-client.so.0",
      NULL,
   };
   int i, fail;
#define SYM(lib, xx)                            \
  do {                                          \
       sym_ ## xx = dlsym(lib, #xx);            \
       if (!(sym_ ## xx)) {                     \
            ERR("%s", dlerror());               \
            fail = 1;                           \
         }                                      \
    } while (0)

   for (i = 0; tbm_libs[i]; i++)
     {
        tbm_lib = dlopen(tbm_libs[i], RTLD_LOCAL | RTLD_LAZY);
        if (tbm_lib)
          {
             fail = 0;
             SYM(tbm_lib, tbm_surface_map);
             SYM(tbm_lib, tbm_surface_unmap);
             SYM(tbm_lib, tbm_surface_queue_can_dequeue);
             SYM(tbm_lib, tbm_surface_queue_dequeue);
             SYM(tbm_lib, tbm_surface_queue_enqueue);
             SYM(tbm_lib, tbm_surface_queue_acquire);
             SYM(tbm_lib, tbm_surface_get_width);
             SYM(tbm_lib, tbm_surface_get_height);
             SYM(tbm_lib, tbm_surface_queue_get_surfaces);
             SYM(tbm_lib, tbm_surface_queue_release);
             SYM(tbm_lib, tbm_surface_queue_destroy);
             SYM(tbm_lib, tbm_surface_internal_ref);
             SYM(tbm_lib, tbm_surface_internal_unref);
             SYM(tbm_lib, tbm_surface_internal_get_user_data);
             SYM(tbm_lib, tbm_surface_internal_add_user_data);
             SYM(tbm_lib, tbm_surface_internal_set_user_data);
             if (fail)
               {
                  dlclose(tbm_lib);
                  tbm_lib = NULL;
               }
             else break;
          }
     }
   if (!tbm_lib) return EINA_FALSE;

   for (i = 0; tbm_clients[i]; i++)
     {
        tbm_client_lib = dlopen(tbm_clients[i], RTLD_LOCAL | RTLD_LAZY);
        if (tbm_client_lib)
          {
             fail = 0;
             SYM(tbm_client_lib, wayland_tbm_client_init);
             SYM(tbm_client_lib, wayland_tbm_client_deinit);
             SYM(tbm_client_lib, wayland_tbm_client_get_wl_tbm_queue);
             SYM(tbm_client_lib, wayland_tbm_client_create_buffer);
             SYM(tbm_client_lib, wayland_tbm_client_destroy_buffer);
             SYM(tbm_client_lib, wayland_tbm_client_create_surface_queue);
             if (fail)
               {
                  dlclose(tbm_client_lib);
                  tbm_client_lib = NULL;
               }
             else break;
          }
     }
   if (!tbm_client_lib) return EINA_FALSE;

   tbm_ref++;
   return EINA_TRUE;
}

static void
tbm_shutdown(void)
{
   if (tbm_ref > 0)
     {
        tbm_ref--;

        if (tbm_ref == 0)
          {
             if (tbm_lib)
               {
                  dlclose(tbm_lib);
                  tbm_lib = NULL;
               }
             if (tbm_client_lib)
               {
                  dlclose(tbm_client_lib);
                  tbm_client_lib = NULL;
               }
          }
     }
}


static void
_evas_tbmbuf_buffer_unmap(Tbmbuf_Surface *surface)
{
   sym_tbm_surface_unmap(surface->tbm_surface);
}


static void
_evas_tbmbuf_surface_reconfigure(Surface *s, int dx EINA_UNUSED, int dy EINA_UNUSED, int w, int h, uint32_t flags EINA_UNUSED)
{
   Tbmbuf_Surface *surface;

   if (!s) return;

   surface = s->surf.tbm;

   if (!surface) return;

   if ((w >= surface->w) && (w <= surface->stride / 4) && (h == surface->h))
      {
         surface->w = w;
         return;
      }

   if (surface->mapping)
      {
         _evas_tbmbuf_buffer_unmap(surface);
         sym_tbm_surface_internal_unref(surface->tbm_surface);
      }


   if (surface->dequeue)
      sym_tbm_surface_queue_enqueue(surface->tbm_queue, surface->tbm_surface);


   _evas_tbmbuf_surface_destroy(s);
   _evas_tbmbuf_surface_create(s, w, h, 0);
}

static void *
_evas_tbmbuf_surface_data_get(Surface *s, int *w, int *h)
{
   Tbmbuf_Surface *surface;
   void *image;

   if (!s) return NULL;
   surface = s->surf.tbm;

   if (!surface) return NULL;

   tbm_surface_info_s info;
   sym_tbm_surface_map(surface->tbm_surface, TBM_SURF_OPTION_READ|TBM_SURF_OPTION_WRITE, &info);
   sym_tbm_surface_internal_ref(surface->tbm_surface);
   surface->mapping = 1;

   image = info.planes[0].ptr;

   surface->stride = info.planes[0].stride;
   if (w) *w = surface->stride / 4;
   if (h) *h = info.height;

   return image;
}


static void
_wait_free_buffer(Tbmbuf_Surface *surface)
{
   if (!surface) return;

   struct wl_tbm_queue *wl_queue;
   int i, num_surface;
   tbm_surface_h surfaces[5];
   struct wl_event_queue *queue = NULL;
   struct wl_buffer *buffer;

   wl_display_dispatch_pending(surface->wl_display);
   if (sym_tbm_surface_queue_can_dequeue(surface->tbm_queue, 0))
      return;

   DBG("WAIT free buffer");
   wl_queue = sym_wayland_tbm_client_get_wl_tbm_queue(surface->tbm_client, surface->wl_surface);
   if (!wl_queue) {
         ERR(" wayland_tbm_client_get_wl_tbm_queue() wl_queue == NULL");
         return;
   }

   queue = wl_display_create_queue((struct wl_display *)surface->wl_display);
   if (!queue) {
         ERR("wl_display_create_queue() queue == NULL");
         return;
   }

   sym_tbm_surface_queue_get_surfaces(surface->tbm_queue, surfaces, &num_surface);
   for(i=0; i<num_surface; i++) {
         sym_tbm_surface_internal_get_user_data(surfaces[i], KEY_WL_BUFFER, (void **)&buffer);
         if (buffer) {
               wl_proxy_set_queue((struct wl_proxy*)buffer, queue);
         }
   }
   wl_proxy_set_queue((struct wl_proxy*)wl_queue, queue);

   while (!sym_tbm_surface_queue_can_dequeue(surface->tbm_queue, 0)) {
         wl_display_dispatch_queue(surface->wl_display, queue);
   }

   for(i=0; i<num_surface; i++) {
         sym_tbm_surface_internal_get_user_data(surfaces[i], KEY_WL_BUFFER, (void **)&buffer);
         if (buffer) {
               wl_proxy_set_queue((struct wl_proxy*)buffer, NULL);
         }
   }

   wl_proxy_set_queue((struct wl_proxy*)wl_queue, NULL);

   wl_event_queue_destroy(queue);
}

static void
buffer_release(void *data, struct wl_buffer *buffer EINA_UNUSED)
{
   void *tbm_queue = NULL;
   tbm_surface_h tbm_buffer = data;
   sym_tbm_surface_internal_get_user_data(tbm_buffer, KEY_WINDOW, (void **)&tbm_queue);
   if (tbm_queue)
      {
         sym_tbm_surface_queue_release(tbm_queue, tbm_buffer);
      }
}

static void
buffer_destroy(void *data)
{
   if (!data) return;
   struct wl_buffer* buffer = data;
   wl_buffer_destroy(buffer);
}

static const struct wl_buffer_listener buffer_listener = {
      buffer_release
};

static int
_evas_tbmbuf_surface_assign(Surface *s)
{
   Tbmbuf_Surface *surface;

   surface = s->surf.tbm;

   if (!surface)
      {
         ERR("surface is NULL");
         return 0;
      }

   surface->tbm_surface = NULL;
   tbm_surface_queue_error_e ret = TBM_SURFACE_QUEUE_ERROR_NONE;
   struct wl_buffer *buffer;

   _wait_free_buffer(surface);

   ret = sym_tbm_surface_queue_dequeue(surface->tbm_queue, &surface->tbm_surface);
   surface->dequeue = 1;

   if (ret != TBM_SURFACE_QUEUE_ERROR_NONE ||
         surface->tbm_surface == NULL) {
         ERR("dequeue:%p from queue:%p err:%d\n", surface->tbm_surface, surface->tbm_queue, ret);
         surface->wait_release = 1;
         return 0;
   }

   surface->wait_release = 0;
   if(!sym_tbm_surface_internal_get_user_data(surface->tbm_surface, KEY_WL_BUFFER, (void **)&buffer)) {
         buffer = sym_wayland_tbm_client_create_buffer(surface->tbm_client, surface->tbm_surface);

         sym_tbm_surface_internal_add_user_data(surface->tbm_surface, KEY_WL_BUFFER, (tbm_data_free)buffer_destroy);
         sym_tbm_surface_internal_set_user_data(surface->tbm_surface, KEY_WL_BUFFER, buffer);

         sym_tbm_surface_internal_add_user_data(surface->tbm_surface, KEY_WINDOW, NULL);
         sym_tbm_surface_internal_set_user_data(surface->tbm_surface, KEY_WINDOW, surface->tbm_queue);

         wl_buffer_add_listener(buffer, &buffer_listener, surface->tbm_surface);
   }

   int num_surface;
   tbm_surface_h surfaces[5];
   sym_tbm_surface_queue_get_surfaces(surface->tbm_queue, surfaces, &num_surface);
   return num_surface;
}

static void
_evas_tbmbuf_surface_post(Surface *s, Eina_Rectangle *rects, unsigned int count)
{
   Tbmbuf_Surface *surface;
   if (!s) return;

   surface = s->surf.tbm;
   if (!surface->wl_surface) return;

   struct wl_buffer *buffer = NULL;

   _evas_tbmbuf_buffer_unmap(surface);
   surface->mapping = 0;

   sym_tbm_surface_internal_get_user_data(surface->tbm_surface, KEY_WL_BUFFER, (void **)&buffer);
   if (!buffer) {
         ERR("Enqueue:%p from queue:%p", surface->tbm_surface, surface->tbm_queue);
         return;
   }

   wl_surface_attach(surface->wl_surface, buffer, 0, 0);
   _evas_surface_damage(surface->wl_surface, surface->compositor_version,
                        surface->w, surface->h, rects, count);
   wl_surface_commit(surface->wl_surface);

   sym_tbm_surface_internal_unref(surface->tbm_surface);

   sym_tbm_surface_queue_enqueue(surface->tbm_queue, surface->tbm_surface);
   sym_tbm_surface_queue_acquire(surface->tbm_queue, &surface->tbm_surface);
   surface->dequeue = 0;

   return;
}

static void
_evas_tbmbuf_surface_destroy(Surface *s)
{
   Tbmbuf_Surface *surf = NULL;
   if (!s) return;

   surf = s->surf.tbm;
   if (surf)
      {
         if (surf->tbm_queue)
            sym_tbm_surface_queue_destroy(surf->tbm_queue);

         if (surf->tbm_client)
            sym_wayland_tbm_client_deinit(surf->tbm_client);

         free(surf);
         s->surf.tbm = NULL;
      }
   tbm_shutdown();
}

Eina_Bool
_evas_tbmbuf_surface_create(Surface *s, int w, int h, int num_buff EINA_UNUSED)
{
   Tbmbuf_Surface *surf = NULL;

   if (!tbm_init())
     {
        ERR("Could not initialize TBM!");
        goto err;
     }

   if (!(s->surf.tbm = calloc(1, sizeof(Tbmbuf_Surface)))) goto err;
   surf = s->surf.tbm;

   surf->dx = 0;
   surf->dy = 0;
   surf->w = w;
   surf->h = h;
   surf->wl_display = s->info->info.wl_disp;
   surf->wl_surface = s->info->info.wl_surface;
   if (!surf->wl_surface)  goto err;
   surf->alpha = s->info->info.destination_alpha;
   surf->compositor_version = 3;

   /* create tbm_client */
   surf->tbm_client = sym_wayland_tbm_client_init(surf->wl_display);
   if (surf->tbm_client == NULL) {
         ERR("No wayland_tbm global");
         goto err;
   }

   /* create surface buffers */
   surf->tbm_queue = sym_wayland_tbm_client_create_surface_queue(surf->tbm_client,
                                                             surf->wl_surface,
                                                             3,
                                                             w, h,
                                                             TBM_FORMAT_ARGB8888);
   surf->tbm_surface = NULL;

   s->type = SURFACE_TBM;
   s->funcs.destroy = _evas_tbmbuf_surface_destroy;
   s->funcs.reconfigure = _evas_tbmbuf_surface_reconfigure;
   s->funcs.data_get = _evas_tbmbuf_surface_data_get;
   s->funcs.assign = _evas_tbmbuf_surface_assign;
   s->funcs.post = _evas_tbmbuf_surface_post;

   return EINA_TRUE;

   err:
   if (surf)
      {
         free(surf);
         s->surf.tbm = NULL;
      }
   return EINA_FALSE;
}
