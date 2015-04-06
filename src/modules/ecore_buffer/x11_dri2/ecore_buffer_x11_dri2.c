#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <Eina.h>
#include <Ecore.h>
#include <Ecore_X.h>

#include <tbm_bufmgr.h>
#include <tbm_surface.h>
#include <tbm_surface_internal.h>

#include <xf86drm.h>
#include <X11/Xmd.h>
#include <dri2/dri2.h>

#include "Ecore_Buffer.h"
#include "ecore_buffer_private.h"

#define LOG(f, x...) printf("[X11_DRI2|%30.30s|%04d] " f "\n", __func__, __LINE__, ##x)

typedef struct _Buffer_Module_Data Buffer_Module_Data;
typedef struct _Buffer_Data Buffer_Data;

struct _Buffer_Module_Data {
     int dummy;
     tbm_bufmgr tbm_mgr;
};

struct _Buffer_Data {
     Ecore_X_Pixmap pixmap;

     int w;
     int h;
     int stride;
     Ecore_Buffer_Format format;

     Eina_Bool is_imported;

     struct
     {
        void *surface;
        Eina_Bool owned;
     } tbm;
};

static int
__buf_get_num_planes (Ecore_Buffer_Format format)
{
   int num_planes = 0;

   switch (format)
     {
      case ECORE_BUFFER_FORMAT_C8:
      case ECORE_BUFFER_FORMAT_RGB332:
      case ECORE_BUFFER_FORMAT_BGR233:
      case ECORE_BUFFER_FORMAT_XRGB4444:
      case ECORE_BUFFER_FORMAT_XBGR4444:
      case ECORE_BUFFER_FORMAT_RGBX4444:
      case ECORE_BUFFER_FORMAT_BGRX4444:
      case ECORE_BUFFER_FORMAT_ARGB4444:
      case ECORE_BUFFER_FORMAT_ABGR4444:
      case ECORE_BUFFER_FORMAT_RGBA4444:
      case ECORE_BUFFER_FORMAT_BGRA4444:
      case ECORE_BUFFER_FORMAT_XRGB1555:
      case ECORE_BUFFER_FORMAT_XBGR1555:
      case ECORE_BUFFER_FORMAT_RGBX5551:
      case ECORE_BUFFER_FORMAT_BGRX5551:
      case ECORE_BUFFER_FORMAT_ARGB1555:
      case ECORE_BUFFER_FORMAT_ABGR1555:
      case ECORE_BUFFER_FORMAT_RGBA5551:
      case ECORE_BUFFER_FORMAT_BGRA5551:
      case ECORE_BUFFER_FORMAT_RGB565:
      case ECORE_BUFFER_FORMAT_BGR565:
      case ECORE_BUFFER_FORMAT_RGB888:
      case ECORE_BUFFER_FORMAT_BGR888:
      case ECORE_BUFFER_FORMAT_XRGB8888:
      case ECORE_BUFFER_FORMAT_XBGR8888:
      case ECORE_BUFFER_FORMAT_RGBX8888:
      case ECORE_BUFFER_FORMAT_BGRX8888:
      case ECORE_BUFFER_FORMAT_ARGB8888:
      case ECORE_BUFFER_FORMAT_ABGR8888:
      case ECORE_BUFFER_FORMAT_RGBA8888:
      case ECORE_BUFFER_FORMAT_BGRA8888:
      case ECORE_BUFFER_FORMAT_XRGB2101010:
      case ECORE_BUFFER_FORMAT_XBGR2101010:
      case ECORE_BUFFER_FORMAT_RGBX1010102:
      case ECORE_BUFFER_FORMAT_BGRX1010102:
      case ECORE_BUFFER_FORMAT_ARGB2101010:
      case ECORE_BUFFER_FORMAT_ABGR2101010:
      case ECORE_BUFFER_FORMAT_RGBA1010102:
      case ECORE_BUFFER_FORMAT_BGRA1010102:
      case ECORE_BUFFER_FORMAT_YUYV:
      case ECORE_BUFFER_FORMAT_YVYU:
      case ECORE_BUFFER_FORMAT_UYVY:
      case ECORE_BUFFER_FORMAT_VYUY:
      case ECORE_BUFFER_FORMAT_AYUV:
         num_planes = 1;
         break;
      case ECORE_BUFFER_FORMAT_NV12:
      case ECORE_BUFFER_FORMAT_NV21:
      case ECORE_BUFFER_FORMAT_NV16:
      case ECORE_BUFFER_FORMAT_NV61:
         num_planes = 2;
         break;
      case ECORE_BUFFER_FORMAT_YUV410:
      case ECORE_BUFFER_FORMAT_YVU410:
      case ECORE_BUFFER_FORMAT_YUV411:
      case ECORE_BUFFER_FORMAT_YVU411:
      case ECORE_BUFFER_FORMAT_YUV420:
      case ECORE_BUFFER_FORMAT_YVU420:
      case ECORE_BUFFER_FORMAT_YUV422:
      case ECORE_BUFFER_FORMAT_YVU422:
      case ECORE_BUFFER_FORMAT_YUV444:
      case ECORE_BUFFER_FORMAT_YVU444:
         num_planes = 3;
         break;

      default :
         break;
     }

   return num_planes;
}

static int
__buf_get_bpp (Ecore_Buffer_Format format)
{
   int bpp = 0;

   switch (format)
     {
      case ECORE_BUFFER_FORMAT_C8:
      case ECORE_BUFFER_FORMAT_RGB332:
      case ECORE_BUFFER_FORMAT_BGR233:
         bpp = 8;
         break;
      case ECORE_BUFFER_FORMAT_XRGB4444:
      case ECORE_BUFFER_FORMAT_XBGR4444:
      case ECORE_BUFFER_FORMAT_RGBX4444:
      case ECORE_BUFFER_FORMAT_BGRX4444:
      case ECORE_BUFFER_FORMAT_ARGB4444:
      case ECORE_BUFFER_FORMAT_ABGR4444:
      case ECORE_BUFFER_FORMAT_RGBA4444:
      case ECORE_BUFFER_FORMAT_BGRA4444:
      case ECORE_BUFFER_FORMAT_XRGB1555:
      case ECORE_BUFFER_FORMAT_XBGR1555:
      case ECORE_BUFFER_FORMAT_RGBX5551:
      case ECORE_BUFFER_FORMAT_BGRX5551:
      case ECORE_BUFFER_FORMAT_ARGB1555:
      case ECORE_BUFFER_FORMAT_ABGR1555:
      case ECORE_BUFFER_FORMAT_RGBA5551:
      case ECORE_BUFFER_FORMAT_BGRA5551:
      case ECORE_BUFFER_FORMAT_RGB565:
      case ECORE_BUFFER_FORMAT_BGR565:
         bpp = 16;
         break;
      case ECORE_BUFFER_FORMAT_RGB888:
      case ECORE_BUFFER_FORMAT_BGR888:
         bpp = 24;
         break;
      case ECORE_BUFFER_FORMAT_XRGB8888:
      case ECORE_BUFFER_FORMAT_XBGR8888:
      case ECORE_BUFFER_FORMAT_RGBX8888:
      case ECORE_BUFFER_FORMAT_BGRX8888:
      case ECORE_BUFFER_FORMAT_ARGB8888:
      case ECORE_BUFFER_FORMAT_ABGR8888:
      case ECORE_BUFFER_FORMAT_RGBA8888:
      case ECORE_BUFFER_FORMAT_BGRA8888:
      case ECORE_BUFFER_FORMAT_XRGB2101010:
      case ECORE_BUFFER_FORMAT_XBGR2101010:
      case ECORE_BUFFER_FORMAT_RGBX1010102:
      case ECORE_BUFFER_FORMAT_BGRX1010102:
      case ECORE_BUFFER_FORMAT_ARGB2101010:
      case ECORE_BUFFER_FORMAT_ABGR2101010:
      case ECORE_BUFFER_FORMAT_RGBA1010102:
      case ECORE_BUFFER_FORMAT_BGRA1010102:
      case ECORE_BUFFER_FORMAT_YUYV:
      case ECORE_BUFFER_FORMAT_YVYU:
      case ECORE_BUFFER_FORMAT_UYVY:
      case ECORE_BUFFER_FORMAT_VYUY:
      case ECORE_BUFFER_FORMAT_AYUV:
         bpp = 32;
         break;
      case ECORE_BUFFER_FORMAT_NV12:
      case ECORE_BUFFER_FORMAT_NV21:
         bpp = 12;
         break;
      case ECORE_BUFFER_FORMAT_NV16:
      case ECORE_BUFFER_FORMAT_NV61:
         bpp = 16;
         break;
      case ECORE_BUFFER_FORMAT_YUV410:
      case ECORE_BUFFER_FORMAT_YVU410:
         bpp = 9;
         break;
      case ECORE_BUFFER_FORMAT_YUV411:
      case ECORE_BUFFER_FORMAT_YVU411:
      case ECORE_BUFFER_FORMAT_YUV420:
      case ECORE_BUFFER_FORMAT_YVU420:
         bpp = 12;
         break;
      case ECORE_BUFFER_FORMAT_YUV422:
      case ECORE_BUFFER_FORMAT_YVU422:
         bpp = 16;
         break;
      case ECORE_BUFFER_FORMAT_YUV444:
      case ECORE_BUFFER_FORMAT_YVU444:
         bpp = 24;
         break;
      default :
         break;
     }

   return bpp;
}

static Ecore_Buffer_Module_Data
_ecore_buffer_x11_dri2_init(const char *context EINA_UNUSED, const char *options EINA_UNUSED)
{
   Ecore_X_Display *xdpy;
   Ecore_X_Window root;
   int eb, ee;
   int major, minor;
   char *driver_name;
   char *device_name;
   int fd = 0;
   drm_magic_t magic;
   Buffer_Module_Data* mdata = NULL;

   ecore_x_init(NULL);
   xdpy = ecore_x_display_get();
   EINA_SAFETY_ON_NULL_GOTO(xdpy, on_error);
   root = ecore_x_window_root_first_get();
   EINA_SAFETY_ON_FALSE_GOTO(root, on_error);

   mdata = calloc(1, sizeof(Buffer_Module_Data));

   //Init DRI2 and TBM
   DRI2QueryExtension(xdpy,&eb, &ee);
   DRI2QueryVersion(xdpy, &major, &minor);
   DRI2Connect(xdpy, ecore_x_window_root_first_get(),
               &driver_name,
               &device_name);

   fd = open (device_name, O_RDWR);
   EINA_SAFETY_ON_TRUE_GOTO(fd<0, on_error);
   EINA_SAFETY_ON_TRUE_GOTO(drmGetMagic(fd, &magic), on_error);
   EINA_SAFETY_ON_FALSE_GOTO(DRI2Authenticate(xdpy, root, magic), on_error);

   mdata->tbm_mgr = tbm_bufmgr_init(fd);
   EINA_SAFETY_ON_NULL_GOTO(mdata->tbm_mgr, on_error);

   free(driver_name);
   free(device_name);

   return mdata;

on_error:
   if (fd) close(fd);
   if (driver_name) free(driver_name);
   if (device_name) free(device_name);
   if (mdata)
     {
        if (mdata->tbm_mgr)
          tbm_bufmgr_deinit(mdata->tbm_mgr);
        free(mdata);
     }
   ecore_shutdown();
   return NULL;
}

static void
_ecore_buffer_x11_dri2_shutdown(Ecore_Buffer_Module_Data bmPriv)
{
   Buffer_Module_Data *bm = (Buffer_Module_Data*)bmPriv;

   if (bm->tbm_mgr)
     tbm_bufmgr_deinit(bm->tbm_mgr);

   ecore_x_shutdown();
}

static Ecore_Buffer_Data
_ecore_buffer_x11_dri2_buffer_alloc(Ecore_Buffer_Module_Data bmPriv,
                                    int width, int height,
                                    Ecore_Buffer_Format format,
                                    unsigned int flags EINA_UNUSED)
{
   Ecore_X_Display* xdpy;
   int bpp;
   int num_plane;
   Ecore_X_Pixmap pixmap;
   Buffer_Data* buf;
   DRI2Buffer* bufs = NULL;
   tbm_bo bo = NULL;
   Buffer_Module_Data *bm = (Buffer_Module_Data*)bmPriv;

   int rw, rh, rcount;
   unsigned int attachment = DRI2BufferFrontLeft;
   tbm_surface_info_s info;
   int i;

   bpp = __buf_get_bpp(format);
   EINA_SAFETY_ON_TRUE_RETURN_VAL((bpp != 32), NULL);
   num_plane = __buf_get_num_planes(format);
   EINA_SAFETY_ON_TRUE_RETURN_VAL((num_plane != 1), NULL);

   xdpy = ecore_x_display_get();
   pixmap = ecore_x_pixmap_new(0, width, height, bpp);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(pixmap, NULL);

   buf = calloc(1, sizeof(Buffer_Data));
   buf->w = width;
   buf->h = height;
   buf->format = format;
   buf->pixmap = pixmap;
   buf->is_imported = EINA_FALSE;

   //Get DRI2Buffer
   DRI2CreateDrawable(xdpy, buf->pixmap);
   bufs = DRI2GetBuffers(xdpy, buf->pixmap, &rw, &rh, &attachment, 1, &rcount);
   EINA_SAFETY_ON_NULL_GOTO(bufs, on_error);
   EINA_SAFETY_ON_FALSE_GOTO((buf->w == rw)&&(buf->h == rh),on_error);
   buf->stride = bufs->pitch;

   //Import tbm_surface
   bo = tbm_bo_import(bm->tbm_mgr, bufs->name);

   info.width = width;
   info.height =  height;
   info.format = format;
   info.bpp = bpp;
   info.size = width * bufs->pitch;
   for ( i = 0 ; i < num_plane ; i++)
   {
      info.planes[i].size = width * bufs->pitch;
      info.planes[i].stride = bufs->pitch;
      info.planes[i].offset = 0;
   }

   buf->tbm.surface = tbm_surface_internal_create_with_bos(&info, &bo, 1);
   buf->tbm.owned = EINA_TRUE;
   EINA_SAFETY_ON_NULL_GOTO(buf->tbm.surface, on_error);
   tbm_bo_unref(bo);

   free(bufs);
   return buf;

on_error:
   if (bo)
     tbm_bo_unref(bo);

   if (buf)
     {
        if (buf->pixmap)
          {
             ecore_x_pixmap_free(buf->pixmap);
             DRI2DestroyDrawable(xdpy, buf->pixmap);
          }

        free(buf);
     }

   if (bufs)
     free(bufs);
   return NULL;
}

static Ecore_Buffer_Data
_ecore_buffer_x11_dri2_buffer_alloc_with_tbm_surface(Ecore_Buffer_Module_Data bmPriv,
                                                     void *tbm_surface,
                                                     int *ret_w, int *ret_h,
                                                     Ecore_Buffer_Format *ret_format,
                                                     unsigned int flags EINA_UNUSED)
{
   Buffer_Data* buf;
   int width, height;
   Ecore_Buffer_Format format;

   EINA_SAFETY_ON_NULL_RETURN_VAL(tbm_surface, NULL);

   width = tbm_surface_get_width(tbm_surface);
   height = tbm_surface_get_height(tbm_surface);
   format = tbm_surface_get_format(tbm_surface);

   buf = calloc(1, sizeof(Buffer_Data));
   buf->w = width;
   buf->h = height;
   buf->format = format;
   buf->pixmap = 0;
   buf->tbm.surface = tbm_surface;
   buf->tbm.owned = EINA_FALSE;
   buf->is_imported = EINA_FALSE;

   if (ret_w) *ret_w = width;
   if (ret_h) *ret_h = height;
   if (ret_format) *ret_format = format;

   return buf;
}

static void
_ecore_buffer_x11_dri2_buffer_free(Ecore_Buffer_Module_Data bmPriv EINA_UNUSED,
                                   Ecore_Buffer_Data priv)
{
   Buffer_Data* buf = (Buffer_Data*)priv;

   if (buf->pixmap)
     {
        DRI2DestroyDrawable(ecore_x_display_get(), buf->pixmap);

        if (!buf->is_imported)
          {
             ecore_x_pixmap_free(buf->pixmap);
             buf->pixmap = 0;
          }
     }

   if (buf->tbm.surface)
     {
        tbm_surface_destroy(buf->tbm.surface);
        buf->tbm.surface = NULL;
     }

   free(buf);
   return;
}

static Ecore_Export_Type
_ecore_buffer_x11_dri2_buffer_export(Ecore_Buffer_Module_Data bmPriv EINA_UNUSED,
                                     Ecore_Buffer_Data priv, int *id)
{
   Buffer_Data* buf = (Buffer_Data*)priv;

   if (id) *id = buf->pixmap;

   return EXPORT_TYPE_ID;
}

static void *
_ecore_buffer_x11_dri2_buffer_import(Ecore_Buffer_Module_Data bmPriv EINA_UNUSED,
                                     int w, int h,
                                     Ecore_Buffer_Format format,
                                     Ecore_Export_Type type,
                                     int export_id,
                                     unsigned int flags EINA_UNUSED)
{
   Buffer_Module_Data *bm = (Buffer_Module_Data*)bmPriv;
   Ecore_X_Display* xdpy;
   Ecore_X_Pixmap pixmap = (Ecore_X_Pixmap)export_id;
   Buffer_Data* buf;
   int rw, rh, rx, ry;
   DRI2Buffer* bufs = NULL;
   tbm_bo bo = NULL;
   int rcount;
   unsigned int attachment = DRI2BufferFrontLeft;
   tbm_surface_info_s info;
   int num_plane,i;

   EINA_SAFETY_ON_FALSE_RETURN_VAL(type == EXPORT_TYPE_ID, NULL);
   xdpy = ecore_x_display_get();

   //Check valid pixmap
   ecore_x_pixmap_geometry_get(pixmap, &rx, &ry, &rw, &rh);
   EINA_SAFETY_ON_FALSE_RETURN_VAL( (rw==w) && (rh==h), NULL);

   buf = calloc(1, sizeof(Buffer_Data));
   EINA_SAFETY_ON_NULL_RETURN_VAL(buf, NULL);

   buf->w = w;
   buf->h = h;
   buf->format = format;
   buf->pixmap = pixmap;
   buf->is_imported = EINA_TRUE;

   //Get DRI2Buffer
   DRI2CreateDrawable(xdpy, buf->pixmap);
   bufs = DRI2GetBuffers(xdpy, buf->pixmap, &rw, &rh, &attachment, 1, &rcount);
   EINA_SAFETY_ON_NULL_GOTO(bufs, on_error);
   EINA_SAFETY_ON_FALSE_GOTO((buf->w == rw)&&(buf->h == rh),on_error);
   buf->stride = bufs->pitch;

   //Import tbm_surface
   bo = tbm_bo_import(bm->tbm_mgr, bufs->name);

   num_plane = __buf_get_num_planes(format);
   info.width = w;
   info.height = h;
   info.format = format;
   info.bpp = __buf_get_bpp(format);
   info.size = w * bufs->pitch;
   for ( i = 0 ; i < num_plane ; i++)
   {
      info.planes[i].size = w * bufs->pitch;
      info.planes[i].stride = bufs->pitch;
      info.planes[i].offset = 0;
   }

   buf->tbm.surface = tbm_surface_internal_create_with_bos(&info, &bo, 1);
   buf->tbm.owned = EINA_TRUE;
   EINA_SAFETY_ON_NULL_GOTO(buf->tbm.surface, on_error);
   tbm_bo_unref(bo);
   free(bufs);

   return buf;
on_error:

   if (bo)
     tbm_bo_unref(bo);

   if (buf)
     {
        if (buf->pixmap)
          {
             DRI2DestroyDrawable(xdpy, buf->pixmap);
          }

        if (buf->tbm.surface)
          tbm_surface_destroy(buf->tbm.surface);

        free(buf);
     }

   if (bufs)
     free(bufs);

   return NULL;
}

static Ecore_Pixmap
_ecore_buffer_x11_dri2_pixmap_get(Ecore_Buffer_Module_Data bmPriv EINA_UNUSED,
                                  Ecore_Buffer_Data priv)
{
   Buffer_Data* buf = (Buffer_Data*)priv;
   EINA_SAFETY_ON_NULL_RETURN_VAL(buf, 0);

   if (!buf->tbm.owned)
     {
        LOG("Not Supported to get pixmap\n");
        return 0;
     }
   return buf->pixmap;
}

static void *
_ecore_buffer_x11_dri2_tbm_bo_get(Ecore_Buffer_Module_Data bmPriv EINA_UNUSED,
                                  Ecore_Buffer_Data priv)
{
   Buffer_Data* buf = (Buffer_Data*)priv;
   EINA_SAFETY_ON_NULL_RETURN_VAL(buf, NULL);
   return buf->tbm.surface;
}

static Ecore_Buffer_Backend _ecore_buffer_x11_dri2_backend = {
     "x11_dri2",
     &_ecore_buffer_x11_dri2_init,
     &_ecore_buffer_x11_dri2_shutdown,
     &_ecore_buffer_x11_dri2_buffer_alloc,
     &_ecore_buffer_x11_dri2_buffer_alloc_with_tbm_surface,
     &_ecore_buffer_x11_dri2_buffer_free,
     &_ecore_buffer_x11_dri2_buffer_export,
     &_ecore_buffer_x11_dri2_buffer_import,
     &_ecore_buffer_x11_dri2_pixmap_get,
     &_ecore_buffer_x11_dri2_tbm_bo_get,
};

Eina_Bool x11_dri2_init(void)
{
   return ecore_buffer_register(&_ecore_buffer_x11_dri2_backend);
}

void x11_dri2_shutdown(void)
{
   ecore_buffer_unregister(&_ecore_buffer_x11_dri2_backend);
}

EINA_MODULE_INIT(x11_dri2_init);
EINA_MODULE_SHUTDOWN(x11_dri2_shutdown);
