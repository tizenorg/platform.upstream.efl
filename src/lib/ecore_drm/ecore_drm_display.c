#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef HAVE_TDM
# include <tdm.h>

#include "ecore_drm_private.h"

typedef struct _Ecore_Drm_Hal_Display
{
   tdm_display *display;

   int fd;
   Ecore_Fd_Handler *hdlr;
} Ecore_Drm_Hal_Display;

typedef struct _Ecore_Drm_Hal_Output
{
   tdm_output *output;
   tdm_layer *primary_layer;
} Ecore_Drm_Hal_Output;

static const char *conn_types[] =
{
   "None", "VGA", "DVI-I", "DVI-D", "DVI-A",
   "Composite", "S-Video", "LVDS", "Component", "DIN",
   "DisplayPort", "HDMI-A", "HDMI-B", "TV", "eDP", "Virtual",
   "DSI",
};

static int fb_data_key = 0;
static Eina_List *fb_list = NULL;

#define ECORE_DRM_FAKE_OUTPUT 1
#if ECORE_DRM_FAKE_OUTPUT
static Eina_Bool ecore_drm_fake_output;
#endif

static Eina_Bool
_ecore_drm_display_output_mode_set_with_fb(Ecore_Drm_Output *output, Ecore_Drm_Output_Mode *mode, Ecore_Drm_Fb *fb, int x, int y);

static void
_ecore_drm_display_output_cb_commit(tdm_output *output EINA_UNUSED, unsigned int sequence EINA_UNUSED,
                                    unsigned int tv_sec EINA_UNUSED, unsigned int tv_usec EINA_UNUSED,
                                    void *user_data)
{
   Ecore_Drm_Pageflip_Callback *cb;
   static int flip_count = 0;

   if (!(cb = user_data)) return;

   flip_count++;
   if (flip_count < cb->count) return;

   cb->dev->current = cb->dev->next;
   cb->dev->next = NULL;

   flip_count = 0;
   if (cb->func) cb->func(cb->data);
   free(cb);
}

static void
_ecore_drm_display_output_cb_vblank(tdm_output *output EINA_UNUSED, unsigned int sequence EINA_UNUSED,
                                    unsigned int tv_sec EINA_UNUSED, unsigned int tv_usec EINA_UNUSED,
                                    void *user_data)
{
   Ecore_Drm_VBlank_Callback *cb;

   if (!(cb = user_data)) return;
   if (cb->func) cb->func(cb->data);
   free(cb);
}

static Eina_Bool
_ecore_drm_display_cb_event(void *data, Ecore_Fd_Handler *hdlr EINA_UNUSED)
{
   Ecore_Drm_Hal_Display *hal_display;
   tdm_error ret;

   if (!(hal_display = data)) return ECORE_CALLBACK_RENEW;

   ret = tdm_display_handle_events(hal_display->display);
   if (ret != TDM_ERROR_NONE)
     ERR("tdm_display_handle_events failed");

   return ECORE_CALLBACK_RENEW;
}

Eina_Bool
_ecore_drm_display_init(Ecore_Drm_Device *dev)
{
   Ecore_Drm_Hal_Display *hal_display;
   int fd;

   EINA_SAFETY_ON_NULL_RETURN_VAL(dev, EINA_FALSE);

   if (dev->hal_display)
      return EINA_TRUE;

   hal_display = calloc(1, sizeof(Ecore_Drm_Hal_Display));
   EINA_SAFETY_ON_NULL_RETURN_VAL(hal_display, EINA_FALSE);

   hal_display->display = tdm_display_init(NULL);
   EINA_SAFETY_ON_NULL_GOTO(hal_display->display, fail_init);

   hal_display->fd = -1;
   tdm_display_get_fd(hal_display->display, &fd);
   EINA_SAFETY_ON_FALSE_GOTO(fd >= 0, fail_fd);

   hal_display->fd = dup(fd);

   hal_display->hdlr =
     ecore_main_fd_handler_add(hal_display->fd, ECORE_FD_READ,
                               _ecore_drm_display_cb_event, hal_display, NULL, NULL);
   EINA_SAFETY_ON_NULL_GOTO(hal_display->hdlr, fail_hdlr);

   dev->hal_display = hal_display;

   return EINA_TRUE;

fail_hdlr:
   if (hal_display->fd >= 0)
     close(hal_display->fd);

   hal_display->fd = -1;
fail_fd:
   tdm_display_deinit(hal_display->display);
fail_init:
   free(hal_display);
   return EINA_FALSE;
}

void
_ecore_drm_display_destroy(Ecore_Drm_Device *dev)
{
   Ecore_Drm_Hal_Display *hal_display;

   if (!dev || !(hal_display = dev->hal_display)) return;

   if (hal_display->hdlr) ecore_main_fd_handler_del(hal_display->hdlr);
   tdm_display_deinit(hal_display->display);

   if (hal_display->fd >= 0)
     close(hal_display->fd);

   free(hal_display);
   dev->hal_display = NULL;
}

int
_ecore_drm_display_get_fd(Ecore_Drm_Device *dev)
{
   Ecore_Drm_Hal_Display *hal_display;

   if (!dev || !(hal_display = dev->hal_display)) return -1;

   return hal_display->fd;
}

Ecore_Drm_Fb*
_ecore_drm_display_fb_find(void *hal_buffer)
{
   Ecore_Drm_Fb *fb;
   Eina_List *l;

   EINA_LIST_FOREACH(fb_list, l, fb)
     {
        if (fb->hal_buffer == hal_buffer)
          return fb;
     }

   return NULL;
}

Ecore_Drm_Fb*
_ecore_drm_display_fb_find_with_id(unsigned int id)
{
   Ecore_Drm_Fb *fb;
   Eina_List *l;

   EINA_LIST_FOREACH(fb_list, l, fb)
     {
        if (fb->id == id)
          return fb;
     }

   return NULL;
}

Ecore_Drm_Fb*
_ecore_drm_display_fb_create(Ecore_Drm_Device *dev, int width, int height)
{
   Ecore_Drm_Fb *fb;
   tbm_bo bo;
   tbm_surface_info_s info;
   static unsigned int id = 0;

   EINA_SAFETY_ON_NULL_RETURN_VAL(dev, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL((width < 1) || (height < 1), NULL);
   if (!(fb = calloc(1, sizeof(Ecore_Drm_Fb)))) return NULL;

   fb->dev = dev;
   fb->hal_buffer =
      tbm_surface_internal_create_with_flags(width, height, TBM_FORMAT_ARGB8888, TBM_BO_SCANOUT);
   if (!fb->hal_buffer)
     {
        ERR("Could not create hal buffer");
        goto create_err;
     }

   bo = tbm_surface_internal_get_bo(fb->hal_buffer, 0);
   if (!bo)
     {
        ERR("Could not get tbm bo");
        goto get_bo;
     }

   tbm_bo_add_user_data(bo, (unsigned long)&fb_data_key, NULL);
   tbm_bo_set_user_data(bo, (unsigned long)&fb_data_key, fb);

   tbm_surface_get_info(fb->hal_buffer, &info);

   fb->from_client = EINA_TRUE;
   fb->id = ++id;
   fb->hdl = tbm_bo_get_handle(bo, TBM_DEVICE_DEFAULT).u32;
   fb->stride = info.planes[0].stride;
   fb->size = info.size;
   fb->w = info.width;
   fb->h = info.height;
   fb->fd = ecore_drm_device_fd_get(dev);
   fb->mmap = info.planes[0].ptr;

   memset(fb->mmap, 0, fb->size);

   ecore_drm_display_fb_add(fb);

   return fb;

get_bo:
   tbm_surface_destroy(fb->hal_buffer);
create_err:
   free(fb);
   return NULL;
}

void
_ecore_drm_display_fb_destroy(Ecore_Drm_Fb *fb)
{
   Ecore_Drm_Device *dev;

   if ((!fb) || (!fb->mmap)) return;

   dev = fb->dev;
   if (dev->next == fb)
      dev->next = NULL;

   ecore_drm_display_fb_remove(fb);

   if (fb->hal_buffer) tbm_surface_destroy(fb->hal_buffer);

   free(fb);
}

EAPI Eina_Bool
ecore_drm_display_fb_hal_buffer_create(Ecore_Drm_Fb *fb)
{
   Ecore_Drm_Device *dev;
   struct drm_gem_flink arg = {0, };
   tbm_bufmgr bufmgr;
   tbm_bo bo;
   tbm_surface_info_s info = {0,};

   EINA_SAFETY_ON_NULL_RETURN_VAL(fb, EINA_FALSE);

   dev = fb->dev;
   EINA_SAFETY_ON_NULL_RETURN_VAL(dev, EINA_FALSE);

   arg.handle = fb->hdl;
   if (drmIoctl(dev->drm.fd, DRM_IOCTL_GEM_FLINK, &arg))
     {
        ERR("Cannot convert to flink: %m");
        return EINA_FALSE;
     }

   bufmgr = tbm_bufmgr_init(dev->drm.fd);
   EINA_SAFETY_ON_NULL_RETURN_VAL(bufmgr, EINA_FALSE);
   bo = tbm_bo_import(bufmgr, arg.name);
   if (!bo)
     {
        tbm_bufmgr_deinit(bufmgr);
        ERR("Cannot import (%d)", arg.name);
        return EINA_FALSE;
     }

   info.width = fb->w;
   info.height = fb->h;
   info.format = TBM_FORMAT_ARGB8888;
   info.bpp = tbm_surface_internal_get_bpp(info.format);
   info.num_planes = 1;
   info.planes[0].stride = fb->stride;
   fb->hal_buffer = tbm_surface_internal_create_with_bos(&info, &bo, 1);
   if (!fb->hal_buffer)
     {
        ERR("Cannot create hal_buffer");
        tbm_bo_unref(bo);
        tbm_bufmgr_deinit(bufmgr);
        return EINA_FALSE;
     }

   tbm_bo_unref(bo);
   tbm_bufmgr_deinit(bufmgr);
   return EINA_TRUE;
}

EAPI void
ecore_drm_display_fb_hal_buffer_destroy(Ecore_Drm_Fb *fb)
{
   EINA_SAFETY_ON_NULL_RETURN(fb);

   if (!fb->hal_buffer) return;

   tbm_surface_destroy(fb->hal_buffer);
   fb->hal_buffer = NULL;
}

EAPI void
ecore_drm_display_fb_add(Ecore_Drm_Fb *fb)
{
   fb_list = eina_list_append(fb_list, fb);
}

EAPI void
ecore_drm_display_fb_remove(Ecore_Drm_Fb *fb)
{
   fb_list = eina_list_remove(fb_list, fb);
}

void
_ecore_drm_display_fb_set(Ecore_Drm_Device *dev, Ecore_Drm_Fb *fb)
{
   Ecore_Drm_Output *output;
   Eina_List *l;

   EINA_SAFETY_ON_NULL_RETURN(dev);
   EINA_SAFETY_ON_NULL_RETURN(fb);

   if (dev->dumb[0])
     {
        if ((fb->w != dev->dumb[0]->w) || (fb->h != dev->dumb[0]->h))
          {
             /* we need to copy from fb to dev->dumb */
             WRN("Trying to set a Framebuffer of improper size !!");
             return;
          }
     }

   if (!dev->next) dev->next = fb;
   if (!dev->next) return;

   EINA_LIST_FOREACH(dev->outputs, l, output)
     {
        int x = 0, y = 0;

        if ((!output->enabled) || (!output->current_mode)) continue;

        if (!output->cloned)
          {
             x = output->x;
             y = output->y;
          }

        if ((!dev->current) ||
            (dev->current->stride != dev->next->stride))
          {
             if (!_ecore_drm_display_output_mode_set_with_fb(output, output->current_mode, dev->next, x, y))
               {
                  ERR("Failed to set Mode %dx%d for Output %s: %m",
                      output->current_mode->width, output->current_mode->height,
                      output->name);
               }

             /* TODO: set dpms on ?? */
          }
     }
}

void
_ecore_drm_display_fb_send(Ecore_Drm_Device *dev, Ecore_Drm_Fb *fb, Ecore_Drm_Pageflip_Cb func, void *data)
{
   Ecore_Drm_Output *output;
   Eina_List *l;
   Ecore_Drm_Pageflip_Callback *cb;
   Ecore_Drm_Hal_Display *hal_display;
   tdm_error ret;

   EINA_SAFETY_ON_NULL_RETURN(dev);
   EINA_SAFETY_ON_NULL_RETURN(fb);
   EINA_SAFETY_ON_NULL_RETURN(func);

#if ECORE_DRM_FAKE_OUTPUT
   if (ecore_drm_fake_output)
     return;
#endif

   if (eina_list_count(dev->outputs) < 1) return;

   if (fb->pending_flip) return;

   hal_display = dev->hal_display;
   EINA_SAFETY_ON_NULL_RETURN(hal_display);

   if (!(cb = calloc(1, sizeof(Ecore_Drm_Pageflip_Callback))))
     return;

   cb->dev = dev;
   cb->func = func;
   cb->data = data;

   EINA_LIST_FOREACH(dev->outputs, l, output)
     if (output->enabled) cb->count++;

   EINA_LIST_FOREACH(dev->outputs, l, output)
     {
        Ecore_Drm_Hal_Output *hal_output;
        tbm_surface_h tdm_buffer;

        if ((!output->enabled) || (!output->current_mode)) continue;

        hal_output = output->hal_output;
        if (!hal_output || !hal_output->output) continue;;

        tdm_buffer = fb->hal_buffer;

        ret = tdm_layer_set_buffer(hal_output->primary_layer, tdm_buffer);
        if (ret != TDM_ERROR_NONE)
          {
             ERR("Cannot set buffer: err(%d)", ret);
             continue;
          }

        ret = tdm_output_commit(hal_output->output, 0, _ecore_drm_display_output_cb_commit, cb);
        if (ret != TDM_ERROR_NONE)
          {
             ERR("Cannot commit crtc %u: err(%d)", output->crtc_id, ret);
             continue;
          }

        fb->pending_flip = EINA_TRUE;
     }

   while (fb->pending_flip)
     {
        ret = tdm_display_handle_events(hal_display->display);
        if (ret != TDM_ERROR_NONE)
          {
             ERR("tdm_display_handle_events failed: err(%d)", ret);
                free(cb);
                break;
          }
     }
}

static Ecore_Drm_Output_Mode*
_ecore_drm_display_output_mode_add(Ecore_Drm_Output *output, const tdm_output_mode *tdm_mode)
{
   Ecore_Drm_Output_Mode *mode;
   uint64_t refresh;

   EINA_SAFETY_ON_NULL_RETURN_VAL(tdm_mode, NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL((tdm_mode->htotal > 0), NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL((tdm_mode->vtotal > 0), NULL);

   /* try to allocate space for mode */
   if (!(mode = calloc(1, sizeof(Ecore_Drm_Output_Mode))))
     {
        ERR("Could not allocate space for mode");
        return NULL;
     }

   mode->flags = 0;
   mode->width = tdm_mode->hdisplay;
   mode->height = tdm_mode->vdisplay;

   refresh = (tdm_mode->clock * 1000000LL / tdm_mode->htotal + tdm_mode->vtotal / 2) / tdm_mode->vtotal;
   if (tdm_mode->flags & TDM_OUTPUT_MODE_FLAG_INTERLACE)
     refresh *= 2;
   if (tdm_mode->flags & TDM_OUTPUT_MODE_FLAG_DBLSCAN)
     refresh /= 2;
   if (tdm_mode->vscan > 1)
     refresh /= tdm_mode->vscan;

   mode->refresh = refresh;
   mode->hal_mode = tdm_mode;

   if (tdm_mode->type & TDM_OUTPUT_MODE_TYPE_PREFERRED)
     mode->flags |= TDM_OUTPUT_MODE_TYPE_PREFERRED;

   output->modes = eina_list_append(output->modes, mode);

   return mode;
}

static Ecore_Drm_Output*
_ecore_drm_display_output_find(Ecore_Drm_Device *dev, unsigned int crtc_id)
{
   Ecore_Drm_Output *output;
   Eina_List *l;

   EINA_LIST_FOREACH(dev->outputs, l, output)
     {
        if (output->crtc_id == crtc_id)
          return output;
     }

   return NULL;
}

static Ecore_Drm_Output*
_ecore_drm_display_output_create(Ecore_Drm_Device *dev, unsigned int crtc_id, int x, int y, Eina_Bool cloned)
{
   Ecore_Drm_Output *output = NULL;
   Ecore_Drm_Hal_Output *hal_output = NULL;
   Ecore_Drm_Hal_Display *hal_display = dev->hal_display;
   unsigned int mmWidth = 0, mmHeight = 0, subpixel = 0;
   tdm_output *tdm_output_obj;
   tdm_output_conn_status status = TDM_OUTPUT_CONN_STATUS_DISCONNECTED;
   const char *type;
   const tdm_output_mode *tdm_modes = NULL;
   const tdm_output_mode *tdm_mode = NULL;
   int i, count;
   Ecore_Drm_Output_Mode *mode, *current = NULL, *preferred = NULL, *best = NULL;
   Eina_List *l;
   char temp[TDM_NAME_LEN];
   const char *maker = NULL, *model = NULL, *name = NULL;
   tdm_error err = TDM_ERROR_NONE;

   tdm_output_obj = tdm_display_get_output(hal_display->display, crtc_id, &err);
   if (!tdm_output_obj)
     {
        ERR("failed to get output: err(%d)", err);
        return NULL;
     }

   err = tdm_output_get_conn_status(tdm_output_obj, &status);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(err == TDM_ERROR_NONE, NULL);

   if (status == TDM_OUTPUT_CONN_STATUS_DISCONNECTED) return NULL;

   if (!(output = calloc(1, sizeof(Ecore_Drm_Output)))) return NULL;
   if (!(hal_output = calloc(1, sizeof(Ecore_Drm_Hal_Output))))
     goto fail_hal_output;

   output->hal_output = hal_output;
   hal_output->output = tdm_output_obj;

   err = tdm_output_get_layer_count(hal_output->output, &count);
   EINA_SAFETY_ON_FALSE_GOTO(err == TDM_ERROR_NONE, fail_layer_count);

   for (i = 0; i < count; i++)
   {
      tdm_layer *layer = tdm_output_get_layer(hal_output->output, i, NULL);
      tdm_layer_capability capabilities = 0;
      tdm_layer_get_capabilities(layer, &capabilities);
      if (capabilities & TDM_LAYER_CAPABILITY_PRIMARY)
      {
         hal_output->primary_layer = layer;
         break;
      }
   }

   if (!hal_output->primary_layer)
     {
        ERR("couldn't find primary layer");
        goto fail_primary_layer;
     }

   output->x = x;
   output->y = y;
   output->dev = dev;
   output->cloned = cloned;

   tdm_output_get_physical_size(hal_output->output, &mmWidth, &mmHeight);
   output->phys_width = mmWidth;
   output->phys_height = mmHeight;
   tdm_output_get_subpixel(hal_output->output, &subpixel);
   output->subpixel = subpixel;

   tdm_output_get_model_info(hal_output->output, &maker, &model, &name);

   output->make = eina_stringshare_add(maker);
   output->model = eina_stringshare_add(model);
   output->name = eina_stringshare_add(name);

   output->connected = (status > 0);
   output->enabled = output->connected;

   tdm_output_get_output_type(hal_output->output, &output->conn_type);
   if (output->conn_type < ALEN(conn_types))
     type = conn_types[output->conn_type];
   else
     type = "UNKNOWN";

   snprintf(temp, sizeof(temp), "%s-%d", type, 0);
   eina_stringshare_replace(&output->name, temp);

   output->crtc_id = crtc_id;
   output->pipe = crtc_id;
   dev->crtc_allocator |= (1 << output->crtc_id);
   output->conn_id = crtc_id;
   dev->conn_allocator |= (1 << output->conn_id);

   output->crtc = NULL;
   output->dpms = NULL;

   err = tdm_output_get_mode(hal_output->output, &tdm_mode);
   EINA_SAFETY_ON_FALSE_GOTO(err == TDM_ERROR_NONE, fail_get_mode);

   tdm_output_get_available_modes(hal_output->output, &tdm_modes, &count);
   for (i = 0; i < count; i++)
     if (!_ecore_drm_display_output_mode_add(output, &tdm_modes[i]))
       goto fail_add_mode;

   EINA_LIST_REVERSE_FOREACH(output->modes, l, mode)
     {
        if (mode->hal_mode == tdm_mode)
          current = mode;
        if (mode->flags & TDM_OUTPUT_MODE_TYPE_PREFERRED)
          preferred = mode;
        best = mode;
     }

   if (current) output->current_mode = current;
   else if (preferred) output->current_mode = preferred;
   else if (best) output->current_mode = best;

   if (!output->current_mode) goto fail_current;

   output->current_mode->flags |= TDM_OUTPUT_MODE_TYPE_DEFAULT;

   dev->outputs = eina_list_append(dev->outputs, output);

   /* NB: 'primary' output property is not supported in HW, so we need to
    * implement it via software. As such, the First output which gets
    * listed via libdrm will be assigned 'primary' until user changes
    * it via config */
   if (eina_list_count(dev->outputs) == 1)
     output->primary = EINA_TRUE;

   DBG("Created New Output At %d,%d", output->x, output->y);
   DBG("\tCrtc Pos: %d %d", output->x, output->y);
   DBG("\tCrtc: %d", output->crtc_id);
   DBG("\tConn: %d", output->conn_id);
   DBG("\tMake: %s", output->make);
   DBG("\tModel: %s", output->model);
   DBG("\tName: %s", output->name);
   DBG("\tCloned: %d", output->cloned);
   DBG("\tPrimary: %d", output->primary);

   EINA_LIST_FOREACH(output->modes, l, mode)
     {
        DBG("\tAdded Mode: %dx%d@%.1f%s%s%s",
            mode->width, mode->height, (mode->refresh / 1000.0),
            (mode->flags & DRM_MODE_TYPE_PREFERRED) ? ", preferred" : "",
            (mode->flags & DRM_MODE_TYPE_DEFAULT) ? ", current" : "",
            (count == 0) ? ", built-in" : "");
     }

   _ecore_drm_output_event_send(output, EINA_TRUE);

   return output;

fail_current:
fail_get_mode:
fail_add_mode:
   EINA_LIST_FREE(output->modes, mode)
     free(mode);
   dev->crtc_allocator &= ~(1 << output->crtc_id);
   dev->conn_allocator &= ~(1 << output->conn_id);
   eina_stringshare_del(output->name);
   eina_stringshare_del(output->model);
   eina_stringshare_del(output->make);
fail_layer_count:
fail_primary_layer:
   free(hal_output);
fail_hal_output:
   free(output);
   return NULL;
}

void
_ecore_drm_display_output_free(Ecore_Drm_Output *output)
{
   Ecore_Drm_Output_Mode *mode;

   /* check for valid output */
   if (!output) return;

   if (output->pending_flip)
     {
        output->pending_destroy = EINA_TRUE;
        return;
     }

   _ecore_drm_output_event_send(output, EINA_FALSE);

   output->dev->outputs = eina_list_remove(output->dev->outputs, output);

   /* free modes */
   EINA_LIST_FREE(output->modes, mode)
     free(mode);

   /* free strings */
   if (output->name) eina_stringshare_del(output->name);
   if (output->model) eina_stringshare_del(output->model);
   if (output->make) eina_stringshare_del(output->make);

   if (output->hal_output) free(output->hal_output);
   free(output);
}

#if ECORE_DRM_FAKE_OUTPUT
Eina_Bool
_ecore_drm_display_fake_output_make(Ecore_Drm_Device *dev)
{
   Ecore_Drm_Output *output = NULL;
   Ecore_Drm_Hal_Output *hal_output = NULL;
   Ecore_Drm_Hal_Display *hal_display = dev->hal_display;
   tdm_output *tdm_output_obj;
   tdm_output_conn_status status = TDM_OUTPUT_CONN_STATUS_DISCONNECTED;
   const char *type;
   tdm_output_mode *tdm_mode_test = NULL;
   int i, count;
   Ecore_Drm_Output_Mode *mode, *current = NULL;
   Eina_List *l;
   char temp[TDM_NAME_LEN];
   const char *maker = NULL, *model = NULL, *name = NULL;
   unsigned int crtc_id = 0;

   tdm_output_obj = tdm_display_get_output(hal_display->display, crtc_id, NULL);
   if (!tdm_output_obj) return EINA_FALSE;

   status = TDM_OUTPUT_CONN_STATUS_CONNECTED;
   if (!(output = calloc(1, sizeof(Ecore_Drm_Output)))) return EINA_FALSE;
   if (!(hal_output = calloc(1, sizeof(Ecore_Drm_Hal_Output))))
     goto fail_hal_output;

   output->hal_output = hal_output;
   hal_output->output = tdm_output_obj;

   tdm_output_get_layer_count(hal_output->output, &count);
   for (i = 0; i < count; i++)
   {
      tdm_layer *layer = tdm_output_get_layer(hal_output->output, i, NULL);
      tdm_layer_capability capabilities = 0;
      tdm_layer_get_capabilities(layer, &capabilities);
      if (capabilities & TDM_LAYER_CAPABILITY_PRIMARY)
      {
         hal_output->primary_layer = layer;
         break;
      }
   }

   if (!hal_output->primary_layer)
     {
        ERR("couldn't find primary layer");
        goto fail_primary_layer;
     }

   output->x = 0;
   output->y = 0;
   output->dev = dev;
   output->cloned = EINA_FALSE;
   output->phys_width = 700;
   output->phys_height = 390;
   output->subpixel = 1;

   tdm_output_get_model_info(hal_output->output, &maker, &model, &name);
   output->make = eina_stringshare_add(maker);
   output->model = eina_stringshare_add(model);
   output->name = eina_stringshare_add(name);

   output->connected = (status > 0);
   output->enabled = output->connected;

   tdm_output_get_output_type(hal_output->output, &output->conn_type);
   if (output->conn_type < ALEN(conn_types))
     type = conn_types[output->conn_type];
   else
     type = "UNKNOWN";

   snprintf(temp, sizeof(temp), "%s-%d", type, 0);
   eina_stringshare_replace(&output->name, temp);

   output->crtc_id = crtc_id;
   output->pipe = crtc_id;
   dev->crtc_allocator |= (1 << output->crtc_id);
   output->conn_id = crtc_id;
   dev->conn_allocator |= (1 << output->conn_id);

   output->crtc = NULL;
   output->dpms = NULL;

   tdm_mode_test = calloc(1, sizeof(tdm_output_mode));
   if (!tdm_mode_test) goto fail_alloc;

   tdm_mode_test->clock = 148500;
   tdm_mode_test->hdisplay = 1920;
   tdm_mode_test->hsync_start = 2008;
   tdm_mode_test->hsync_end = 2052;
   tdm_mode_test->htotal = 2200;
   tdm_mode_test->hskew = 1;
   tdm_mode_test->vdisplay = 1080;
   tdm_mode_test->vsync_start = 1084;
   tdm_mode_test->vsync_end = 1089;
   tdm_mode_test->vtotal = 1125;
   tdm_mode_test->vscan = 1;
   tdm_mode_test->vrefresh = 60;
   tdm_mode_test->flags = TDM_OUTPUT_MODE_FLAG_PHSYNC | TDM_OUTPUT_MODE_FLAG_PVSYNC;
   tdm_mode_test->type = TDM_OUTPUT_MODE_TYPE_PREFERRED | TDM_OUTPUT_MODE_TYPE_DRIVER;
   strncpy(tdm_mode_test->name, "1920x1080", TDM_NAME_LEN);

   if (!_ecore_drm_display_output_mode_add(output, tdm_mode_test))
     goto fail_add_mode;

   EINA_LIST_REVERSE_FOREACH(output->modes, l, mode)
    {
       current = mode;
    }

   output->current_mode = current;
   if (!output->current_mode) goto fail_current;

   output->current_mode->flags |= TDM_OUTPUT_MODE_TYPE_DEFAULT;

   dev->outputs = eina_list_append(dev->outputs, output);

   /* use one fake output */
   output->primary = EINA_TRUE;

   DBG("Created New Output At %d,%d", output->x, output->y);
   DBG("\tCrtc Pos: %d %d", output->x, output->y);
   DBG("\tCrtc: %d", output->crtc_id);
   DBG("\tConn: %d", output->conn_id);
   DBG("\tMake: %s", output->make);
   DBG("\tModel: %s", output->model);
   DBG("\tName: %s", output->name);
   DBG("\tCloned: %d", output->cloned);
   DBG("\tPrimary: %d", output->primary);

   EINA_LIST_FOREACH(output->modes, l, mode)
     {
        DBG("\tAdded Mode: %dx%d@%.1f%s%s%s",
            mode->width, mode->height, (mode->refresh / 1000.0),
            (mode->flags & DRM_MODE_TYPE_PREFERRED) ? ", preferred" : "",
            (mode->flags & DRM_MODE_TYPE_DEFAULT) ? ", current" : "",
            (count == 0) ? ", built-in" : "");
     }

   _ecore_drm_output_event_send(output, EINA_TRUE);

   ecore_drm_fake_output = EINA_TRUE;

   free(tdm_mode_test);

   DBG("%s excuted", __func__);

   return EINA_TRUE;

fail_current:
fail_add_mode:
   EINA_LIST_FREE(output->modes, mode)
     free(mode);
   dev->crtc_allocator &= ~(1 << output->crtc_id);
   dev->conn_allocator &= ~(1 << output->conn_id);
   eina_stringshare_del(output->name);
   eina_stringshare_del(output->model);
   eina_stringshare_del(output->make);
   free(tdm_mode_test);
fail_alloc:
fail_primary_layer:
   free(hal_output);
fail_hal_output:
   free(output);
   return EINA_FALSE;

}
#endif

Eina_Bool
_ecore_drm_display_outputs_create(Ecore_Drm_Device *dev)
{
   Eina_Bool ret = EINA_TRUE;
   Ecore_Drm_Output *output = NULL;
   int i = 0, x = 0, y = 0, count = 0;
   Ecore_Drm_Hal_Display *hal_display;

   EINA_SAFETY_ON_NULL_RETURN_VAL(dev, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(dev->hal_display, EINA_FALSE);

   hal_display = dev->hal_display;

   tdm_display_get_output_count(hal_display->display, &count);
   if (!(dev->crtcs = calloc(count, sizeof(unsigned int))))
     {
        ERR("Could not allocate space for crtcs");
        return EINA_FALSE;
     }

   dev->crtc_count = count;
   for (i = 0; i < count; i++)
      dev->crtcs[i] = i;

   dev->min_width = 0;
   dev->min_height = 0;
   dev->max_width = UINT32_MAX;
   dev->max_height = UINT32_MAX;

   for (i = 0; i < count; i++)
     {
        if (!(output = _ecore_drm_display_output_create(dev, i, x, y, EINA_FALSE)))
          continue;

        x += output->current_mode->width;
     }

   ret = EINA_TRUE;
   if (eina_list_count(dev->outputs) < 1)
     ret = EINA_FALSE;

#if ECORE_DRM_FAKE_OUTPUT
   if (ret == EINA_FALSE)
     ret = _ecore_drm_display_fake_output_make(dev);
#endif

   return ret;
}

void
_ecore_drm_display_outputs_update(Ecore_Drm_Device *dev)
{
   Ecore_Drm_Hal_Display *hal_display;
   int i, count = 0, x = 0, y = 0;
   Ecore_Drm_Output *output;
   tdm_error err;

   EINA_SAFETY_ON_NULL_RETURN(dev);
   EINA_SAFETY_ON_NULL_RETURN(dev->hal_display);

   hal_display = dev->hal_display;

   err = tdm_display_get_output_count(hal_display->display, &count);
   EINA_SAFETY_ON_FALSE_RETURN(err == TDM_ERROR_NONE);

   /* find any new connects */
   for (i = 0; i < count; i++)
     {
        tdm_output_conn_status status = TDM_OUTPUT_CONN_STATUS_DISCONNECTED;
        tdm_output *tdm_output_obj;

        tdm_output_obj = tdm_display_get_output(hal_display->display, i, &err);
        EINA_SAFETY_ON_FALSE_RETURN(err == TDM_ERROR_NONE);

        output = _ecore_drm_display_output_find(dev, i);

        err = tdm_output_get_conn_status(tdm_output_obj, &status);
        EINA_SAFETY_ON_FALSE_RETURN(err == TDM_ERROR_NONE);

        if (status == TDM_OUTPUT_CONN_STATUS_DISCONNECTED)
          {
             if (output)
                _ecore_drm_display_output_free(output);

             continue;
          }

        /* already created */
        if (output) continue;

        if (eina_list_count(dev->outputs) > 0)
          {
             Ecore_Drm_Output *last;

             if ((last = eina_list_last_data_get(dev->outputs)))
               x = last->x + last->current_mode->width;
             else
               x = 0;
          }
        else
          x = 0;

        /* try to create a new output */
        /* NB: hotplugged outputs will be set to cloned by default */
        _ecore_drm_display_output_create(dev, i, x, y, EINA_TRUE);
     }
}

void
_ecore_drm_display_output_render_enable(Ecore_Drm_Output *output)
{
   Ecore_Drm_Device *dev;

   EINA_SAFETY_ON_NULL_RETURN(output);
   EINA_SAFETY_ON_NULL_RETURN(output->dev);
   EINA_SAFETY_ON_NULL_RETURN(output->current_mode);

   if (!output->enabled) return;

   dev = output->dev;

   if (!dev->current)
     {
        /* schedule repaint */
        /* NB: this will trigger a redraw at next idle */
        output->need_repaint = EINA_TRUE;
        return;
     }

   ecore_drm_output_dpms_set(output, DRM_MODE_DPMS_ON);

   _ecore_drm_display_output_mode_set_with_fb(output, output->current_mode,
                                              dev->current, output->x, output->y);
}

void
_ecore_drm_display_output_render_disable(Ecore_Drm_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN(output);

   output->need_repaint = EINA_FALSE;
   if (!output->enabled) return;

   ecore_drm_output_dpms_set(output, DRM_MODE_DPMS_OFF);
}

void
_ecore_drm_display_output_dpms_set(Ecore_Drm_Output *output, int level)
{
   Ecore_Drm_Hal_Output *hal_output;
   tdm_error err;

   EINA_SAFETY_ON_NULL_RETURN(output);
   EINA_SAFETY_ON_NULL_RETURN(output->dev);

   hal_output = output->hal_output;
   EINA_SAFETY_ON_NULL_RETURN(hal_output->output);

   err = tdm_output_set_dpms(hal_output->output, level);
   EINA_SAFETY_ON_FALSE_RETURN(err == TDM_ERROR_NONE);
}

unsigned int
_ecore_drm_display_output_crtc_buffer_get(Ecore_Drm_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, 0);
   EINA_SAFETY_ON_NULL_RETURN_VAL(output->dev, 0);
   EINA_SAFETY_ON_NULL_RETURN_VAL(output->dev->current, 0);

   return output->dev->current->id;
}

void
_ecore_drm_display_output_size_get(Ecore_Drm_Device *dev EINA_UNUSED, int id, int *w, int *h)
{
   Ecore_Drm_Fb *fb;

   fb = _ecore_drm_display_fb_find_with_id(id);

   if (w) *w = fb->w;
   if (h) *h = fb->h;
}

void
_ecore_drm_display_output_crtc_size_get(Ecore_Drm_Output *output, int *width, int *height)
{
   if (width) *width = 0;
   if (height) *height = 0;

   EINA_SAFETY_ON_NULL_RETURN(output);
   EINA_SAFETY_ON_NULL_RETURN(output->current_mode);

   if (width) *width = output->current_mode->width;
   if (height) *height = output->current_mode->height;
}

static Eina_Bool
_ecore_drm_display_output_mode_set_with_fb(Ecore_Drm_Output *output, Ecore_Drm_Output_Mode *mode,
                                           Ecore_Drm_Fb *fb, int x, int y)
{
   Ecore_Drm_Hal_Output *hal_output;
   Eina_Bool ret = EINA_TRUE;

#if ECORE_DRM_FAKE_OUTPUT
   if (ecore_drm_fake_output)
     return EINA_TRUE;
#endif
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(output->dev, EINA_FALSE);

   hal_output = output->hal_output;
   EINA_SAFETY_ON_NULL_RETURN_VAL(hal_output->output, EINA_FALSE);

   output->x = x;
   output->y = y;
   output->current_mode = mode;

   if (mode && fb)
     {
        tdm_info_layer info;
        tbm_surface_h tdm_buffer = NULL;
        tbm_surface_info_s buffer_info;
        tdm_error err;

        EINA_SAFETY_ON_NULL_RETURN_VAL(mode->hal_mode, EINA_FALSE);

        tdm_buffer = fb->hal_buffer;

        tbm_surface_get_info(tdm_buffer, &buffer_info);
        info.src_config.size.h = buffer_info.planes[0].stride >> 2;
        info.src_config.size.v = tbm_surface_get_height(tdm_buffer);
        info.src_config.pos.x = output->x;
        info.src_config.pos.y = output->y;
        info.src_config.pos.w = mode->width;
        info.src_config.pos.h = mode->height;
        info.src_config.format = tbm_surface_get_format(tdm_buffer);
        info.dst_pos.x = 0;
        info.dst_pos.y = 0;
        info.dst_pos.w = info.src_config.pos.w;
        info.dst_pos.h = info.src_config.pos.h;
        info.transform = 0;

        /* do nothing if size is invalid */
        if (info.src_config.size.h < info.src_config.pos.w ||
            info.src_config.size.v < info.src_config.pos.h)
          {
             WRN("size(%dx%d) less than pos_size(%dx%d)",
                 info.src_config.size.h, info.src_config.size.v,
                 info.src_config.pos.w, info.src_config.pos.h);
             return EINA_TRUE;
          }

        err = tdm_output_set_mode(hal_output->output, mode->hal_mode);
        EINA_SAFETY_ON_FALSE_GOTO(err == TDM_ERROR_NONE, fail_set);

        err = tdm_layer_set_info(hal_output->primary_layer, &info);
        EINA_SAFETY_ON_FALSE_GOTO(err == TDM_ERROR_NONE, fail_set);

        err = tdm_layer_set_buffer(hal_output->primary_layer, tdm_buffer);
        EINA_SAFETY_ON_FALSE_GOTO(err == TDM_ERROR_NONE, fail_set);

        TRACE_EFL_BEGIN(Mode_Set);
        err = tdm_output_commit(hal_output->output, 0, NULL, NULL);
        if (err != TDM_ERROR_NONE)
          {
             TRACE_EFL_END();
             ERR("Cannot commit crtc %u: err(%d)", output->crtc_id, err);
             goto fail_set;
          }
        TRACE_EFL_END();
     }
   else
     {
        tdm_layer_set_buffer(hal_output->primary_layer, NULL);
        tdm_output_commit(hal_output->output, 0, NULL, NULL);
     }

   return ret;
fail_set:
   tdm_layer_unset_buffer(hal_output->primary_layer);
   return EINA_FALSE;
}

Eina_Bool
_ecore_drm_display_output_mode_set(Ecore_Drm_Output *output, Ecore_Drm_Output_Mode *mode, int x, int y)
{
   Ecore_Drm_Device *dev;
   Ecore_Drm_Fb *fb = NULL;
   Ecore_Drm_Hal_Output *hal_output;
   tdm_error err;

   EINA_SAFETY_ON_NULL_RETURN_VAL(output, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(output->dev, EINA_FALSE);

   dev = output->dev;

   hal_output = output->hal_output;
   EINA_SAFETY_ON_NULL_RETURN_VAL(hal_output->output, EINA_FALSE);

   err = tdm_output_set_mode(hal_output->output, mode->hal_mode);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(err == TDM_ERROR_NONE, EINA_FALSE);

   if (dev->current) fb = dev->current;
   else if (dev->next) fb = dev->next;
   EINA_SAFETY_ON_NULL_RETURN_VAL(fb, EINA_FALSE);

   return _ecore_drm_display_output_mode_set_with_fb(output, mode, fb, x, y);
}

Eina_Bool
_ecore_drm_display_output_possible_crtc_get(Ecore_Drm_Output *output EINA_UNUSED, unsigned int crtc EINA_UNUSED)
{
   return EINA_TRUE;
}

Eina_Bool
_ecore_drm_display_output_wait_vblank(Ecore_Drm_Output *output, int interval, Ecore_Drm_VBlank_Cb func, void *data)
{
   Ecore_Drm_Hal_Output *hal_output;
   Ecore_Drm_VBlank_Callback *cb;
   tdm_error ret;

   EINA_SAFETY_ON_NULL_RETURN_VAL(output, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(func, EINA_FALSE);

   hal_output = output->hal_output;
   EINA_SAFETY_ON_NULL_RETURN_VAL(hal_output->output, EINA_FALSE);

   if (!(cb = calloc(1, sizeof(Ecore_Drm_VBlank_Callback))))
     return EINA_FALSE;

   cb->output = output;
   cb->func = func;
   cb->data = data;

   ret = tdm_output_wait_vblank(hal_output->output, interval, 0, _ecore_drm_display_output_cb_vblank, cb);
   if (ret != TDM_ERROR_NONE)
     {
        ERR("Error Wait VBlank: %m");
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

void*
_ecore_drm_display_output_hal_private_get(Ecore_Drm_Output *output)
{
   Ecore_Drm_Hal_Output *hal_output;

   EINA_SAFETY_ON_NULL_RETURN_VAL(output, NULL);

   hal_output = output->hal_output;
   EINA_SAFETY_ON_NULL_RETURN_VAL(hal_output, NULL);

   return hal_output->output;
}

EAPI Ecore_Drm_Fb*
ecore_drm_display_output_primary_layer_fb_get(Ecore_Drm_Output *output)
{
   Ecore_Drm_Device *dev;

   EINA_SAFETY_ON_NULL_RETURN_VAL(output, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(output->dev, NULL);

   dev = output->dev;

   if (dev->current) return dev->current;
   else if (dev->next) return dev->next;

   return NULL;
}

#endif
