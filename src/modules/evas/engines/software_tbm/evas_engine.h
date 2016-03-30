#ifndef EVAS_ENGINE_H
# define EVAS_ENGINE_H

# ifdef LOGFNS
#  include <stdio.h>
#  define LOGFN(fl, ln, fn) printf("-EVAS-TBM: %25s: %5i - %s\n", fl, ln, fn);
# else
#  define LOGFN(fl, ln, fn)
# endif

extern int _evas_engine_software_tbm_log_dom;

# ifdef ERR
#  undef ERR
# endif
# define ERR(...) EINA_LOG_DOM_ERR(_evas_engine_software_tbm_log_dom, __VA_ARGS__)

# ifdef DBG
#  undef DBG
# endif
# define DBG(...) EINA_LOG_DOM_DBG(_evas_engine_software_tbm_log_dom, __VA_ARGS__)

# ifdef INF
#  undef INF
# endif
# define INF(...) EINA_LOG_DOM_INFO(_evas_engine_software_tbm_log_dom, __VA_ARGS__)

# ifdef WRN
#  undef WRN
# endif
# define WRN(...) EINA_LOG_DOM_WARN(_evas_engine_software_tbm_log_dom, __VA_ARGS__)

# ifdef CRI
#  undef CRI
# endif
# define CRI(...) EINA_LOG_DOM_CRIT(_evas_engine_software_tbm_log_dom, __VA_ARGS__)

# include <wayland-client.h>
# include "../software_generic/Evas_Engine_Software_Generic.h"
# include "Evas_Engine_Software_Tbm.h"

#define TBM_SURF_PLANE_MAX 4 /**< maximum number of the planes  */
/* option to map the tbm_surface */
#define TBM_SURF_OPTION_READ      (1 << 0) /**< access option to read  */
#define TBM_SURF_OPTION_WRITE     (1 << 1) /**< access option to write */

typedef struct _tbm_surface * tbm_surface_h;
typedef uint32_t tbm_format;

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

/* returns 0 on success */

struct _Outbuf
{
   int w, h;
   int rotation, alpha;
   Outbuf_Depth depth;

   Evas *evas;
   Evas_Engine_Info_Software_Tbm *info;

   void *tbm_queue;
   Eina_Bool ext_tbm_queue;

   tbm_surface_h surface;

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


Outbuf *_evas_software_tbm_outbuf_setup(int w, int h, int rot, Outbuf_Depth depth, Eina_Bool alpha, void *tbm_queue);
void _evas_software_tbm_outbuf_free(Outbuf *ob);
void _evas_software_tbm_outbuf_flush(Outbuf *ob, Tilebuf_Rect *rects, Evas_Render_Mode render_mode);
void _evas_software_tbm_outbuf_idle_flush(Outbuf *ob);

Render_Engine_Swap_Mode _evas_software_tbm_outbuf_swap_mode_get(Outbuf *ob);
int _evas_software_tbm_outbuf_rotation_get(Outbuf *ob);
void _evas_software_tbm_outbuf_reconfigure(Outbuf *ob, int x, int y, int w, int h, int rot, Outbuf_Depth depth, Eina_Bool alpha, Eina_Bool resize);
void *_evas_software_tbm_outbuf_update_region_new(Outbuf *ob, int x, int y, int w, int h, int *cx, int *cy, int *cw, int *ch);
void _evas_software_tbm_outbuf_update_region_push(Outbuf *ob, RGBA_Image *update, int x, int y, int w, int h);
void _evas_software_tbm_outbuf_update_region_free(Outbuf *ob, RGBA_Image *update);



#endif
