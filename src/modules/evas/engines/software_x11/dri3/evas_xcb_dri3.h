/*
 * Copyright (c) 2015 Samsung, Inc
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlib-xcb.h>
#include <X11/xshmfence.h>
#include <xcb/xcb.h>
#include <xcb/dri3.h>
#include <xcb/present.h>

typedef struct _tbm_bufmgr *tbm_bufmgr;
typedef struct _tbm_bo *tbm_bo;

typedef union _tbm_bo_handle
{
    void     *ptr;
    int32_t  s32;
    uint32_t u32;
    int64_t  s64;
    uint64_t u64;
} tbm_bo_handle;


#define DRI3_MAX_BACK   3
#define DRI3_BACK_ID(i) (i)
#define DRI3_FRONT_ID   (DRI3_MAX_BACK)

#define DRI3_NUM_BUFFERS        (1 + DRI3_MAX_BACK)

typedef struct _dri3_info {
   xcb_connection_t    *conn;     /* connection to xserver */
   Display             *disp;
   int         	      drm_fd;
   tbm_bufmgr           bufmgr;
} dri3_info;

typedef struct _dri3_buffer {
   int                  size;
   int                  w;
   int                  h;
   int                  stride;
   int                  depth;
   int                  bpp;
   int                  tbm_fd;
   int                  buffer_fd;
   tbm_bo               bo;
   Pixmap               pixmap;

   struct xshmfence     *shm_fence;
   int                  fence_fd;
   xcb_sync_fence_t     sync_fence;

   struct xshmfence     *wait_shm_fence;
   int                  wait_fence_fd;
   xcb_sync_fence_t     wait_sync_fence;

   int                  busy;
   uint64_t             last_swap;
   Eina_Bool            own_pixmap;
} dri3_buffer;

typedef struct _dri3_drawable {
   Drawable             window;
   int                  width;
   int                  height;
   int                  depth;
   int                  swap_interval; /* swap interval */

   /* SBC numbers are tracked by using the serial numbers
    * in the present request and complete events
    */
   uint64_t             send_sbc;
   uint64_t             recv_sbc;

   /* Last received UST/MSC values */
   uint64_t             ust, msc;

   /* Serial numbers for tracking wait_for_msc events */
   uint32_t             send_msc_serial;
   uint32_t             recv_msc_serial;

   dri3_buffer          *buffers[DRI3_NUM_BUFFERS];
   dri3_buffer          *back_buffer;
   int                  cur_back_id;
   int                  num_back;
   int                  buffer_age;

   uint32_t             *stamp;

   xcb_present_event_t  eid;
   xcb_gcontext_t       gc;
   xcb_special_event_t  *special_event;

   uint8_t              flipping;

} dri3_drawable;

typedef struct _dri3_fence {
   struct               xshmfence *shm_fence;
   int                  fence_fd;
   xcb_sync_fence_t     sync_fence;
} dri3_fence;

dri3_info info;

/* dri3 initialization */
int   dri3_init_dri3(Display *dpy);
void  dri3_deinit_dri3();

int   dri3_query_dri3();
int   dri3_query_present();
int   dri3_XFixes_query(Display *disp);


/* dri3 buffer */
dri3_buffer  *dri3_get_pixmap_buffer(Pixmap pixmap);
void          dri3_destroy_buffer(dri3_buffer *buffer);

/* dri3 fence */
void  dri3_fence_set (dri3_buffer *buffer);
void  dri3_fence_reset (dri3_buffer *buffer);
void  dri3_fence_trigger (dri3_buffer *buffer);
int   dri3_fence_triggered (dri3_buffer *buffer);
void  dri3_fence_await (dri3_buffer *buffer);

void  dri3_wait_fence_set (dri3_buffer *buffer);
void  dri3_wait_fence_reset (dri3_buffer *buffer);
void  dri3_wait_fence_trigger (dri3_buffer *buffer);
int   dri3_wait_fence_triggered (dri3_buffer *buffer);
void  dri3_wait_fence_await (dri3_buffer *buffer);



void  dri3_tbm_import(dri3_buffer *buffer);
void  *dri3_tbm_bo_handle_map(dri3_buffer *buffer);
void  dri3_tbm_bo_handle_unmap(dri3_buffer *buffer);
void  dri3_tbm_bo_handle_unref(dri3_buffer *buffer);

/*dri3 drawables */
dri3_drawable *dri3_create_drawable (XID window, int w, int h, int depth);
void  dri3_destroy_drawable (dri3_drawable *drawable);

XID   dri3_XFixes_create_region(Display *disp,XRectangle *xrects, int nrects);
void  dri3_XFixes_destory_region(Display *disp, XID region);
void  dri3_get_data (dri3_buffer *buffer, RGBA_Image *im);
dri3_buffer *dri3_get_backbuffers (dri3_drawable *drawable, uint32_t *stamp);
int64_t dri3_swap_buffers (dri3_drawable *drawable, int64_t target_msc, int64_t divisor, int64_t remainder, XID region);
int   dri3_get_buffer_age(dri3_drawable *drawable);

int   dri3_drawable_get_msc (dri3_drawable *drawable, int64_t *ust, int64_t *msc, int64_t *sbc);
int   dri3_wait_for_msc (dri3_drawable *drawable, int64_t target_msc, int64_t divisor, int64_t remainder, int64_t *ust, int64_t *msc, int64_t *sbc);
int   dri3_wait_for_sbc (dri3_drawable *drawable, uint64_t target_sbc, uint64_t *ust, uint64_t *msc, uint64_t *sbc);
void  dri3_set_swap_interval (dri3_drawable *drawable, int interval);
int   dri3_get_swap_interval (dri3_drawable *drawable);


