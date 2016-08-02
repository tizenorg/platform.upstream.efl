#include "evas_engine.h"

#include "../software_generic/evas_native_common.h"

#ifdef HAVE_DLSYM
# include <dlfcn.h>
#endif

/* local structures */
typedef struct _Render_Engine Render_Engine;

struct _Render_Engine
{
   Render_Engine_Software_Generic generic;
};

Evas_Native_Tbm_Surface_Image_Set_Call  glsym_evas_native_tbm_surface_image_set = NULL;

/* For wl_buffer's native set */
static void *tbm_server_lib = NULL;
typedef struct _tbm_surface * tbm_surface_h;
static tbm_surface_h (*glsym_wayland_tbm_server_get_surface) (struct wayland_tbm_server *tbm_srv, struct wl_resource *wl_buffer) = NULL;

/* function tables - filled in later (func and parent func) */
static Evas_Func func, pfunc;

/* external variables */
int _evas_engine_drm_log_dom;

/* local inline functions */
static inline Outbuf *
eng_get_ob(Render_Engine *re)
{
   return re->generic.ob;
}

/* local functions */
static void *
_output_setup(Evas_Engine_Info_Drm *info, int w, int h)
{
   Render_Engine *re = NULL;
   Outbuf *ob;

   /* try to allocate space for our render engine structure */
   if (!(re = calloc(1, sizeof(Render_Engine))))
     goto on_error;

   /* try to create new outbuf */
   if (!(ob = evas_outbuf_setup(info, w, h)))
     goto on_error;

   if (!evas_render_engine_software_generic_init(&re->generic, ob,
                                                 evas_outbuf_buffer_state_get,
                                                 evas_outbuf_rot_get,
                                                 evas_outbuf_reconfigure, NULL,
                                                 evas_outbuf_update_region_new,
                                                 evas_outbuf_update_region_push,
                                                 evas_outbuf_update_region_free,
                                                 NULL, evas_outbuf_flush,
                                                 evas_outbuf_free, 
                                                 ob->w, ob->h))
     goto on_error;

   /* return the allocated render_engine structure */
   return re;

 on_error:
   if (re) evas_render_engine_software_generic_clean(&re->generic);

   free(re);
   return NULL;
}

/* engine api functions */
static void *
eng_info(Evas *evas EINA_UNUSED)
{
   Evas_Engine_Info_Drm *info;

   /* try to allocate space for our engine info structure */
   if (!(info = calloc(1, sizeof(Evas_Engine_Info_Drm))))
     return NULL;

   /* set some engine default properties */
   info->magic.magic = rand();
   info->render_mode = EVAS_RENDER_MODE_BLOCKING;

   return info;
}

static void
eng_info_free(Evas *evas EINA_UNUSED, void *einfo)
{
   Evas_Engine_Info_Drm *info;

   /* free the engine info */
   if ((info = (Evas_Engine_Info_Drm *)einfo))
     free(info);
}

static void
_symbols(void)
{
   static int done = 0;
   int fail = 0;
   const char *wayland_tbm_server_lib = "libwayland-tbm-server.so.0";

   if (done) return;

#define LINK2GENERIC(sym)                   \
   glsym_##sym = dlsym(RTLD_DEFAULT, #sym); \
   if (!(glsym_##sym))                      \
     {                                      \
       ERR("%s", dlerror());                \
       fail = 1;                            \
     }

   // Get function pointer to native_common that is now provided through the link of SW_Generic.
   LINK2GENERIC(evas_native_tbm_surface_image_set);
   if (fail == 1)
     {
       ERR("fail to dlsym about evas_native_tbm_surface_image_set symbol");
       return;
     }
   tbm_server_lib = dlopen(wayland_tbm_server_lib, RTLD_LOCAL | RTLD_LAZY);
   if (tbm_server_lib)
     {
        LINK2GENERIC(wayland_tbm_server_get_surface);
        if (fail == 1)
          {
             ERR("fail to dlsym about wayland_tbm_server_get_surface symbol");
             dlclose(tbm_server_lib);
             tbm_server_lib = NULL;
             return;
          }
     }
   else
     return;

   done = 1;
}

static int
eng_setup(Evas *evas, void *einfo)
{
   Evas_Engine_Info_Drm *info;
   Evas_Public_Data *epd;
   Render_Engine *re;

   /* try to cast to our engine info structure */
   if (!(info = (Evas_Engine_Info_Drm *)einfo)) return 0;

   /* try to get the evas public data */
   if (!(epd = eo_data_scope_get(evas, EVAS_CANVAS_CLASS))) return 0;

   /* check for valid engine output */
   if (!(re = epd->engine.data.output))
     {
        /* NB: If we have no valid output then assume we have not been
         * initialized yet and call any needed common init routines */
        evas_common_init();

        /* try to create a new render_engine */
        if (!(re = _output_setup(info, epd->output.w, epd->output.h)))
          return 0;
     }
   else
     {
        Outbuf *ob;

        /* try to create a new outbuf */
        ob = evas_outbuf_setup(info, epd->output.w, epd->output.h);
        if (!ob) return 0;

        /* if we have an existing outbuf, free it */
        evas_render_engine_software_generic_update(&re->generic, ob, 
                                                   ob->w, ob->h);
     }

   /* reassign engine output */
   epd->engine.data.output = re;
   if (!epd->engine.data.output) return 0;

   /* check for valid engine context */
   if (!epd->engine.data.context)
     {
        /* create a context if needed */
        epd->engine.data.context =
          epd->engine.func->context_new(epd->engine.data.output);
     }

   return 1;
}

static void
eng_output_free(void *data)
{
   Render_Engine *re;

   if ((re = data))
     {
        evas_render_engine_software_generic_clean(&re->generic);
        free(re);
     }

   if (tbm_server_lib)
     {
       dlclose(tbm_server_lib);
       tbm_server_lib = NULL;
     }

   evas_common_shutdown();
}

static void
eng_output_copy(void *data, void *buffer, int stride, int width, int height, uint format, int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh)
{
   Render_Engine *re = (Render_Engine *)data;
   Outbuf *ob;

   EINA_SAFETY_ON_NULL_RETURN(re);

   ob = eng_get_ob(re);
   EINA_SAFETY_ON_NULL_RETURN(ob);

   evas_outbuf_copy(ob, buffer, stride, width, height, format, sx, sy, sw, sh, dx, dy, dw, dh);
}

static void *
eng_image_native_set(void *data EINA_UNUSED, void *image, void *native)
{
   Evas_Native_Surface *ns = native;
   Image_Entry *ie = image;
   RGBA_Image *im = image, *im2;
   void *wl_buf = NULL;

   if (!im || !ns) return im;

   if (ns)
     {
        if (ns->type == EVAS_NATIVE_SURFACE_TBM)
          {
             if (im->native.data)
               {
                  //image have native surface already
                  Evas_Native_Surface *ens = im->native.data;

                  if ((ens->type == ns->type) &&
                      (ens->data.tbm.buffer == ns->data.tbm.buffer))
                    return im;
               }
          }
        else if (ns->type == EVAS_NATIVE_SURFACE_WL)
          {
             wl_buf = ns->data.wl.legacy_buffer;
             if (im->native.data)
               {
                  Evas_Native_Surface *ens;

                  ens = im->native.data;
                  if (ens->data.wl.legacy_buffer == wl_buf)
                    return im;
               }
          }
     }

   if ((ns->type == EVAS_NATIVE_SURFACE_OPENGL) &&
       (ns->version == EVAS_NATIVE_SURFACE_VERSION))
     im2 = evas_cache_image_data(evas_common_image_cache_get(),
                                 ie->w, ie->h,
                                 ns->data.x11.visual, 1,
                                 EVAS_COLORSPACE_ARGB8888);
   else
     im2 = evas_cache_image_data(evas_common_image_cache_get(),
                                 ie->w, ie->h,
                                 NULL, 1,
                                 EVAS_COLORSPACE_ARGB8888);

   if (im->native.data)
      {
         if (im->native.func.free)
            im->native.func.free(im->native.func.data, im);
      }

#ifdef EVAS_CSERVE2
   if (evas_cserve2_use_get() && evas_cache2_image_cached(ie))
     evas_cache2_image_close(ie);
   else
#endif
   evas_cache_image_drop(ie);
   im = im2;

   if (ns->type == EVAS_NATIVE_SURFACE_TBM)
     {
        if (glsym_evas_native_tbm_surface_image_set)
          return glsym_evas_native_tbm_surface_image_set(NULL, im, ns);
        else
          return NULL;
     }
   else if (ns->type == EVAS_NATIVE_SURFACE_WL)
     {
       // TODO  : need the code for all wl_buffer type
       // For TBM surface
       if (glsym_wayland_tbm_server_get_surface && glsym_evas_native_tbm_surface_image_set)
         {
            tbm_surface_h _tbm_surface;

            _tbm_surface = glsym_wayland_tbm_server_get_surface(NULL,ns->data.wl.legacy_buffer);
            return glsym_evas_native_tbm_surface_image_set(_tbm_surface, im, ns);
         }
       else
         {
            return NULL;
         }
     }

   return im;
}

static void *
eng_image_native_get(void *data EINA_UNUSED, void *image)
{
   RGBA_Image *im = image;
   Native *n;
   if (!im) return NULL;
   n = im->native.data;
   if (!n) return NULL;
   return &(n->ns);
}

/* module api functions */
static int
module_open(Evas_Module *em)
{
   /* check for valid evas module */
   if (!em) return 0;

   /* try to inherit functions from software_generic engine */
   if (!_evas_module_engine_inherit(&pfunc, "software_generic")) return 0;

   /* try to create eina logging domain */
   _evas_engine_drm_log_dom = 
     eina_log_domain_register("evas-drm", EVAS_DEFAULT_LOG_COLOR);

   /* if we could not create a logging domain, error out */
   if (_evas_engine_drm_log_dom < 0)
     {
        EINA_LOG_ERR("Can not create a module log domain.");
        return 0;
     }

   /* store parent functions */
   func = pfunc;

   /* override the methods we provide */
   EVAS_API_OVERRIDE(info, &func, eng_);
   EVAS_API_OVERRIDE(info_free, &func, eng_);
   EVAS_API_OVERRIDE(setup, &func, eng_);
   EVAS_API_OVERRIDE(output_free, &func, eng_);
   EVAS_API_OVERRIDE(output_copy, &func, eng_);
   EVAS_API_OVERRIDE(image_native_set, &func, eng_);
   EVAS_API_OVERRIDE(image_native_get, &func, eng_);

   _symbols();

   /* advertise our engine functions */
   em->functions = (void *)(&func);

   return 1;
}

static void 
module_close(Evas_Module *em EINA_UNUSED)
{
   /* unregister the eina log domain for this engine */
   eina_log_domain_unregister(_evas_engine_drm_log_dom);
}

static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION, "drm", "none", { module_open, module_close }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_ENGINE, engine, drm);

#ifndef EVAS_STATIC_BUILD_DRM
EVAS_EINA_MODULE_DEFINE(engine, drm);
#endif
