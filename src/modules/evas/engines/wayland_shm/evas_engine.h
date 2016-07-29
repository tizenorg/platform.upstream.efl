#ifndef _EVAS_ENGINE_H
# define _EVAS_ENGINE_H

//# define LOGFNS 1

# ifdef LOGFNS
#  include <stdio.h>
#  define LOGFN(fl, ln, fn) printf("-EVAS-WL: %25s: %5i - %s\n", fl, ln, fn);
# else
#  define LOGFN(fl, ln, fn)
# endif

extern int _evas_engine_way_shm_log_dom;

# ifdef ERR
#  undef ERR
# endif
# define ERR(...) EINA_LOG_DOM_ERR(_evas_engine_way_shm_log_dom, __VA_ARGS__)

# ifdef DBG
#  undef DBG
# endif
# define DBG(...) EINA_LOG_DOM_DBG(_evas_engine_way_shm_log_dom, __VA_ARGS__)

# ifdef INF
#  undef INF
# endif
# define INF(...) EINA_LOG_DOM_INFO(_evas_engine_way_shm_log_dom, __VA_ARGS__)

# ifdef WRN
#  undef WRN
# endif
# define WRN(...) EINA_LOG_DOM_WARN(_evas_engine_way_shm_log_dom, __VA_ARGS__)

# ifdef CRI
#  undef CRI
# endif
# define CRI(...) EINA_LOG_DOM_CRIT(_evas_engine_way_shm_log_dom, __VA_ARGS__)

# include <wayland-client.h>
# include "../software_generic/Evas_Engine_Software_Generic.h"
# include "Evas_Engine_Wayland_Shm.h"

# define MAX_BUFFERS 4
# define SURFACE_HINT_RESIZING 0x10


typedef struct _Shm_Surface Shm_Surface;
typedef struct _Tbmbuf_Surface TBM_Surface;

typedef enum _Surface_Type Surface_Type;
enum _Surface_Type {
   SURFACE_EMPTY,
   SURFACE_SHM,
   SURFACE_TBM
};

typedef struct _Surface Surface;
struct _Surface
{
   Surface_Type type;
   union {
      Shm_Surface *shm;
      TBM_Surface *tbm;
   } surf;
   Evas_Engine_Info_Wayland_Shm *info;
   struct
{
        void (*destroy)(Surface *surface);
        void (*reconfigure)(Surface *surface, int x, int y, int w, int h, uint32_t flags);
        void *(*data_get)(Surface *surface, int *w, int *h);
        int  (*assign)(Surface *surface);
        void (*post)(Surface *surface, Eina_Rectangle *rects, unsigned int count);
     } funcs;
};

struct _Outbuf
{
   int w, h;
   int rotation;
   int onebuf;
   int num_buff;
   Outbuf_Depth depth;

   Evas_Engine_Info_Wayland_Shm *info;

   Surface *surface;

   struct 
     {
        /* one big buffer for updates. flushed on idle_flush */
        RGBA_Image *onebuf;
        Eina_Array onebuf_regions;

        /* a list of pending regions to write out */
        Eina_List *pending_writes;

        /* list of previous frame pending regions to write out */
        Eina_List *prev_pending_writes;

        /* Eina_Bool redraw : 1; */
        Eina_Bool destination_alpha : 1;
     } priv;
};

Eina_Bool _evas_tbmbuf_surface_create(Surface *s, int w, int h, int num_buff);
Eina_Bool _evas_shm_surface_create(Surface *s, int w, int h, int num_buff);

Outbuf *_evas_outbuf_setup(int w, int h, Evas_Engine_Info_Wayland_Shm *info);
void _evas_outbuf_free(Outbuf *ob);
void _evas_outbuf_flush(Outbuf *ob, Tilebuf_Rect *rects, Evas_Render_Mode render_mode);
void _evas_outbuf_idle_flush(Outbuf *ob);

Render_Engine_Swap_Mode _evas_outbuf_swap_mode_get(Outbuf *ob);
int _evas_outbuf_rotation_get(Outbuf *ob);
void _evas_outbuf_reconfigure(Outbuf *ob, int x, int y, int w, int h, int rot, Outbuf_Depth depth, Eina_Bool alpha, Eina_Bool resize);
void *_evas_outbuf_update_region_new(Outbuf *ob, int x, int y, int w, int h, int *cx, int *cy, int *cw, int *ch);
void _evas_outbuf_update_region_push(Outbuf *ob, RGBA_Image *update, int x, int y, int w, int h);
void _evas_outbuf_update_region_free(Outbuf *ob, RGBA_Image *update);
void _evas_surface_damage(struct wl_surface *s, int compositor_version, int w, int h, Eina_Rectangle *rects, unsigned int count);

Eina_Bool _evas_surface_init(Surface *s, int w, int h, int num_buf);

#endif
