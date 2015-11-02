#include "evas_common_private.h"
#include "evas_private.h"
#ifdef EVAS_CSERVE2
# include "evas_cs2_private.h"
#endif

#include "evas_engine.h"

/* logging domain variable */
int _evas_engine_way_shm_log_dom = -1;

/* evas function tables - filled in later (func and parent func) */
static Evas_Func func, pfunc;

/* engine structure data */
typedef struct _Render_Engine Render_Engine;
struct _Render_Engine
{
   Render_Engine_Software_Generic generic;

   void (*outbuf_reconfigure)(Outbuf *ob, int x, int y, int w, int h, int rot, Outbuf_Depth depth, Eina_Bool alpha, Eina_Bool resize);
};

/* LOCAL FUNCTIONS */
<<<<<<< HEAD
Eina_Bool
_render_engine_swapbuf_busy_check(void *data)
{
   Evas *evas = data;
   Evas_Public_Data *epd;
   Render_Engine *re = NULL;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get evas public data */
   if (!(epd = eo_data_scope_get(evas, EVAS_CANVAS_CLASS)))
     return 0;

   if (!(re = epd->engine.data.output))
     return 0;

   return _evas_outbuf_buffer_busy_check(re->generic.ob);
}

Render_Engine *
_render_engine_swapbuf_setup(int w, int h, unsigned int rotation, unsigned int depth, Eina_Bool alpha, struct wl_shm *shm, struct wl_surface *surface)
=======
Render_Engine *
_render_engine_swapbuf_setup(int w, int h, unsigned int rotation, unsigned int depth, Eina_Bool alpha, struct wl_shm *shm, struct wl_surface *surface, struct wl_display *disp)
>>>>>>> opensource/master
{
   Render_Engine *re;
   Outbuf *ob;
   Render_Engine_Merge_Mode merge_mode = MERGE_SMART;
   const char *s;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to allocate space for new render engine */
   if (!(re = calloc(1, sizeof(Render_Engine)))) return NULL;

<<<<<<< HEAD
   ob = _evas_outbuf_setup(w, h, rotation, depth, alpha, shm, surface);
   if (!ob) goto err;

   if (!evas_render_engine_software_generic_init(&re->generic, ob,
                                                 _evas_outbuf_swapmode_get,
                                                 _evas_outbuf_rotation_get,
                                                 NULL,
                                                 NULL,
=======
   ob = _evas_outbuf_setup(w, h, rotation, depth, alpha, shm, surface, disp);
   if (!ob) goto err;

   if (!evas_render_engine_software_generic_init(&re->generic, ob, 
                                                 _evas_outbuf_swap_mode_get,
                                                 _evas_outbuf_rotation_get,
                                                 NULL,
                                                 NULL, 
>>>>>>> opensource/master
                                                 _evas_outbuf_update_region_new,
                                                 _evas_outbuf_update_region_push,
                                                 _evas_outbuf_update_region_free,
                                                 _evas_outbuf_idle_flush,
                                                 _evas_outbuf_flush,
                                                 _evas_outbuf_free,
                                                 w, h))
     goto err;
<<<<<<< HEAD

   re->outbuf_reconfigure = _evas_outbuf_reconfigure;
=======

   re->outbuf_reconfigure = _evas_outbuf_reconfigure;

   s = getenv("EVAS_WAYLAND_PARTIAL_MERGE");
   if (s)
     {
        if ((!strcmp(s, "bounding")) || (!strcmp(s, "b")))
          merge_mode = MERGE_BOUNDING;
        else if ((!strcmp(s, "full")) || (!strcmp(s, "f")))
          merge_mode = MERGE_FULL;
        else if ((!strcmp(s, "smart")) || (!strcmp(s, "s")))
          merge_mode = MERGE_SMART;
     }

   evas_render_engine_software_generic_merge_mode_set(&re->generic, merge_mode);
>>>>>>> opensource/master

   /* return allocated render engine */
   return re;

err:
   if (ob) _evas_outbuf_free(ob);
   free(re);
   return NULL;
}

/* ENGINE API FUNCTIONS WE PROVIDE */
static void *
eng_info(Evas *eo_evas EINA_UNUSED)
{
   Evas_Engine_Info_Wayland_Shm *einfo;
<<<<<<< HEAD

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

=======

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

>>>>>>> opensource/master
   /* try to allocate space for new engine info */
   if (!(einfo = calloc(1, sizeof(Evas_Engine_Info_Wayland_Shm))))
     return NULL;

   /* fill in engine info */
   einfo->magic.magic = rand();
   einfo->render_mode = EVAS_RENDER_MODE_BLOCKING;
<<<<<<< HEAD
   einfo->func.busy_check = _render_engine_swapbuf_busy_check;
=======
>>>>>>> opensource/master

   /* return allocated engine info */
   return einfo;
}

static void 
eng_info_free(Evas *eo_evas EINA_UNUSED, void *info)
{
   Evas_Engine_Info_Wayland_Shm *einfo;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to free previously allocated engine info */
   if ((einfo = (Evas_Engine_Info_Wayland_Shm *)info))
     free(einfo);
}

static int 
eng_setup(Evas *eo_evas, void *info)
{
   Evas_Engine_Info_Wayland_Shm *einfo;
   Evas_Public_Data *epd;
   Render_Engine *re = NULL;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to cast to our engine info */
   if (!(einfo = (Evas_Engine_Info_Wayland_Shm *)info))
     return 0;

   /* try to get evas public data */
   if (!(epd = eo_data_scope_get(eo_evas, EVAS_CANVAS_CLASS)))
     return 0;

   /* test for valid engine output */
   if (!(re = epd->engine.data.output))
     {
        /* if we have no engine data, assume we have not initialized yet */
<<<<<<< HEAD
        evas_common_cpu_init();
        evas_common_blend_init();
        evas_common_image_init();
        evas_common_convert_init();
        evas_common_scale_init();
        evas_common_rectangle_init();
        evas_common_polygon_init();
        evas_common_line_init();
        evas_common_font_init();
        evas_common_draw_init();
        evas_common_tilebuf_init();

        re = _render_engine_swapbuf_setup(epd->output.w, epd->output.h,
                                          einfo->info.rotation,
                                          einfo->info.depth,
                                          einfo->info.destination_alpha,
                                          einfo->info.wl_shm,
                                          einfo->info.wl_surface);

        if (re)
          {
             re->generic.ob->info = einfo;
             if (re->generic.ob->surface)
               {
                  re->generic.ob->surface->callback.released = einfo->callback.released;
                  re->generic.ob->surface->callback.data = einfo->callback.data;
               }
          }
=======
        evas_common_init();

        re = _render_engine_swapbuf_setup(epd->output.w, epd->output.h,
                                          einfo->info.rotation, 
                                          einfo->info.depth, 
                                          einfo->info.destination_alpha,
                                          einfo->info.wl_shm, 
                                          einfo->info.wl_surface,
                                          einfo->info.wl_disp);

        if (re) 
          re->generic.ob->info = einfo;
>>>>>>> opensource/master
        else
          goto err;
     }
   else
     {
        Outbuf *ob;

<<<<<<< HEAD
        ob = _evas_outbuf_setup(epd->output.w, epd->output.h,
                                einfo->info.rotation, einfo->info.depth,
                                einfo->info.destination_alpha,
                                einfo->info.wl_shm,
                                einfo->info.wl_surface);
        if (ob)
          {
             ob->info = einfo;
             if (ob->surface)
               {
                  ob->surface->callback.released = einfo->callback.released;
                  ob->surface->callback.data = einfo->callback.data;
               }
             evas_render_engine_software_generic_update(&re->generic, ob,
                                                        epd->output.w,
=======
        ob = _evas_outbuf_setup(epd->output.w, epd->output.h, 
                                einfo->info.rotation, einfo->info.depth, 
                                einfo->info.destination_alpha, 
                                einfo->info.wl_shm, einfo->info.wl_surface,
                                einfo->info.wl_disp);
        if (ob)
          {
             ob->info = einfo;
             evas_render_engine_software_generic_update(&re->generic, ob, 
                                                        epd->output.w, 
>>>>>>> opensource/master
                                                        epd->output.h);
          }
     }

   epd->engine.data.output = re;
   if (!epd->engine.data.output)
     {
        ERR("Failed to create Render Engine");
        goto err;
     }

   if (!epd->engine.data.context)
     {
        epd->engine.data.context = 
          epd->engine.func->context_new(epd->engine.data.output);
     }

   return 1;

err:
<<<<<<< HEAD
   evas_common_font_shutdown();
   evas_common_image_shutdown();
=======
   evas_common_shutdown();
>>>>>>> opensource/master
   return 0;
}

static void 
eng_output_free(void *data)
{
   Render_Engine *re;

   if ((re = (Render_Engine *)data))
     {
        evas_render_engine_software_generic_clean(&re->generic);
        free(re);
     }

   evas_common_shutdown();
}

static void 
eng_output_resize(void *data, int w, int h)
{
   Render_Engine *re;
   Evas_Engine_Info_Wayland_Shm *einfo;
   int dx = 0, dy = 0;
   Eina_Bool resize = EINA_FALSE;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(re = (Render_Engine *)data)) return;
   if (!(einfo = re->generic.ob->info)) return;

   if (einfo->info.edges & 4) // resize from left
     {
        if ((einfo->info.rotation == 90) || (einfo->info.rotation == 270))
          dx = re->generic.ob->h - h;
        else
          dx = re->generic.ob->w - w;
     }

   if (einfo->info.edges & 1) // resize from top
     {
        if ((einfo->info.rotation == 90) || (einfo->info.rotation == 270))
          dy = re->generic.ob->w - w;
        else
          dy = re->generic.ob->h - h;
     }

   if (einfo->info.edges) resize = EINA_TRUE;

<<<<<<< HEAD
   re->outbuf_reconfigure(re->generic.ob, dx, dy, w, h,
                          einfo->info.rotation, einfo->info.depth,
=======
   re->outbuf_reconfigure(re->generic.ob, dx, dy, w, h, 
                          einfo->info.rotation, einfo->info.depth, 
>>>>>>> opensource/master
                          einfo->info.destination_alpha, resize);

   evas_common_tilebuf_free(re->generic.tb);
   if ((re->generic.tb = evas_common_tilebuf_new(w, h)))
     evas_common_tilebuf_set_tile_size(re->generic.tb, TILESIZE, TILESIZE);

   re->generic.w = w;
   re->generic.h = h;
}

/* EVAS MODULE FUNCTIONS */
static int 
module_open(Evas_Module *em)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* check for valid evas module */
   if (!em) return 0;

   /* try to get functions from whatever engine module we inherit from */
   if (!_evas_module_engine_inherit(&pfunc, "software_generic")) return 0;

   /* try to create our logging domain */
   _evas_engine_way_shm_log_dom = 
     eina_log_domain_register("evas-wayland_shm", EVAS_DEFAULT_LOG_COLOR);
   if (_evas_engine_way_shm_log_dom < 0)
     {
        EINA_LOG_ERR("Cannot create a module logging domain");
        return 0;
     }

   /* copy parent functions */
   func = pfunc;

   /* override engine specific functions */
#define ORD(f) EVAS_API_OVERRIDE(f, &func, eng_)
   ORD(info);
   ORD(info_free);
   ORD(setup);
   ORD(output_free);
   ORD(output_resize);

   /* advertise our own engine functions */
   em->functions = (void *)(&func);

   return 1;
}

static void 
module_close(Evas_Module *em EINA_UNUSED)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* unregister logging domain */
   if (_evas_engine_way_shm_log_dom > -1)
     eina_log_domain_unregister(_evas_engine_way_shm_log_dom);

   /* reset logging domain variable */
   _evas_engine_way_shm_log_dom = -1;
}

static Evas_Module_Api evas_modapi = 
{
   EVAS_MODULE_API_VERSION, "wayland_shm", "none", {module_open, module_close}
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_ENGINE, engine, wayland_shm);

#ifndef EVAS_STATIC_BUILD_WAYLAND_SHM
EVAS_EINA_MODULE_DEFINE(engine, wayland_shm);
#endif
