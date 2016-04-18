#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "ecore_drm_private.h"

EAPI int ECORE_DRM_EVENT_SEAT_ADD = -1;
static Eina_Hash *_fd_hash = NULL;

/* local functions */
static int 
_cb_open_restricted(const char *path, int flags, void *data)
{
   Ecore_Drm_Input *input;
   int fd = -1;

   if (!(input = data)) return -1;

   /* try to open the device */
   fd = _ecore_drm_launcher_device_open_no_pending(path, flags);
   if (fd < 0) ERR("Could not open device");
   if (_fd_hash)
     eina_hash_add(_fd_hash, path, (void *)(intptr_t)fd);

   return fd;
}

static void 
_cb_close_restricted(int fd, void *data)
{
   Ecore_Drm_Input *input;
   Ecore_Drm_Seat *seat;
   Ecore_Drm_Evdev *edev;
   Eina_List *l, *ll;

   if (!(input = data)) return;

   EINA_LIST_FOREACH(input->dev->seats, l, seat)
     {
        EINA_LIST_FOREACH(seat->devices, ll, edev)
          {
             if (edev->fd == fd)
               {
                  _ecore_drm_launcher_device_close(edev->path, fd);

                  /* re-initialize fd after closing */
                  edev->fd = -1;
                  return;
               }
          }
     }
}

static Ecore_Drm_Seat *
_seat_create(Ecore_Drm_Input *input, const char *seat)
{
   Ecore_Drm_Seat *s;

   /* try to allocate space for new seat */
   if (!(s = calloc(1, sizeof(Ecore_Drm_Seat))))
     return NULL;

   s->input = input;
   s->name = eina_stringshare_add(seat);

   /* add this new seat to list */
   input->dev->seats = eina_list_append(input->dev->seats, s);

   ecore_event_add(ECORE_DRM_EVENT_SEAT_ADD, NULL, NULL, NULL);

   return s;
}

static void
_ecore_drm_event_input_device_add_free(void *data EINA_UNUSED, void *ev)
{
   Ecore_Drm_Event_Input_Device_Add *e;

   e = ev;
   eina_stringshare_del(e->name);
   eina_stringshare_del(e->sysname);
   eina_stringshare_del(e->seatname);

   free(e);
}

static void
_ecore_drm_event_input_device_del_free(void *data EINA_UNUSED, void *ev)
{
   Ecore_Drm_Event_Input_Device_Del *e;

   e = ev;
   eina_stringshare_del(e->name);
   eina_stringshare_del(e->sysname);
   eina_stringshare_del(e->seatname);

   free(e);
}

static Ecore_Drm_Seat *
_seat_get(Ecore_Drm_Input *input, const char *seat)
{
   Ecore_Drm_Seat *s;
   Eina_List *l;

   /* search for this name in existing seats */
   EINA_LIST_FOREACH(input->dev->seats, l, s)
     if (!strcmp(s->name, seat)) return s;

   return _seat_create(input, seat);
}

static void
_ecore_event_device_info_free(void *data EINA_UNUSED, void *ev)
{
   Ecore_Event_Device_Info *e;

   e = ev;
   eina_stringshare_del(e->name);
   eina_stringshare_del(e->identifier);
   eina_stringshare_del(e->seatname);

   free(e);
}

static Ecore_Device_Class
_ecore_drm_seat_cap_to_ecore_device_class(unsigned int cap)
{
   switch(cap)
     {
      case EVDEV_SEAT_POINTER:
         return ECORE_DEVICE_CLASS_MOUSE;
      case EVDEV_SEAT_KEYBOARD:
         return ECORE_DEVICE_CLASS_KEYBOARD;
      case EVDEV_SEAT_TOUCH:
         return ECORE_DEVICE_CLASS_TOUCH;
      default:
         return ECORE_DEVICE_CLASS_NONE;
     }
   return ECORE_DEVICE_CLASS_NONE;
}

void
_ecore_drm_device_info_send(unsigned int window, Ecore_Drm_Evdev *edev, Ecore_Device_Class clas, Eina_Bool flag)
{
   Ecore_Event_Device_Info *e;

   if (!(e = calloc(1, sizeof(Ecore_Event_Device_Info)))) return;

   e->name = eina_stringshare_add(libinput_device_get_name(edev->device));
   e->identifier = eina_stringshare_add(edev->path);
   e->seatname = eina_stringshare_add(edev->seat->name);
   e->clas = clas;
   e->window = window;

   if (flag)
     ecore_event_add(ECORE_EVENT_DEVICE_ADD, e, _ecore_event_device_info_free, NULL);
   else
     ecore_event_add(ECORE_EVENT_DEVICE_DEL, e, _ecore_event_device_info_free, NULL);
}

static Eina_Bool
_ecore_drm_device_add_ecore_device(Ecore_Drm_Evdev *edev, Ecore_Device_Class clas)
{
   const Eina_List *dev_list = NULL;
   const Eina_List *l;
   Ecore_Device *dev = NULL;
   const char *identifier;

   if (!edev->path) return EINA_FALSE;

   dev_list = ecore_device_list();
   if (dev_list)
     {
        EINA_LIST_FOREACH(dev_list, l, dev)
          {
             if (!dev) continue;
             identifier = ecore_device_identifier_get(dev);
             if (!identifier) continue;
             if ((ecore_device_class_get(dev) == clas) && (!strcmp(identifier, edev->path)))
                return EINA_FALSE;
          }
     }

   if(!(dev = ecore_device_add())) return EINA_FALSE;

   ecore_device_name_set(dev, libinput_device_get_name(edev->device));
   ecore_device_description_set(dev, libinput_device_get_name(edev->device));
   ecore_device_identifier_set(dev, edev->path);
   ecore_device_class_set(dev, clas);
   return EINA_TRUE;
}

static Eina_Bool
_ecore_drm_device_del_ecore_device(Ecore_Drm_Evdev *edev, Ecore_Device_Class clas)
{
   const Eina_List *dev_list = NULL;
   const Eina_List *l;
   Ecore_Device *dev = NULL;
   const char *identifier;

   if (!edev->path) return EINA_FALSE;

   dev_list = ecore_device_list();
   if (!dev_list) return EINA_FALSE;
   EINA_LIST_FOREACH(dev_list, l, dev)
      {
         if (!dev) continue;
         identifier = ecore_device_identifier_get(dev);
         if (!identifier) continue;
         if ((ecore_device_class_get(dev) == clas) && (!strcmp(identifier, edev->path)))
           {
              ecore_device_del(dev);
              return EINA_TRUE;
           }
      }
   return EINA_FALSE;
}

void
_ecore_drm_device_add(unsigned int window, Ecore_Drm_Evdev *edev)
{
   Eina_Bool ret = EINA_FALSE;
   Ecore_Device_Class clas;

   if (edev->seat_caps & EVDEV_SEAT_POINTER)
     {
        clas = _ecore_drm_seat_cap_to_ecore_device_class(EVDEV_SEAT_POINTER);
        ret = _ecore_drm_device_add_ecore_device(edev, clas);
        if (ret) _ecore_drm_device_info_send(window, edev, clas, 1);
     }
   if (edev->seat_caps & EVDEV_SEAT_KEYBOARD)
     {
        clas = _ecore_drm_seat_cap_to_ecore_device_class(EVDEV_SEAT_KEYBOARD);
        ret = _ecore_drm_device_add_ecore_device(edev, clas);
        if (ret) _ecore_drm_device_info_send(window, edev, clas, 1);
     }
   if (edev->seat_caps & EVDEV_SEAT_TOUCH)
     {
        clas = _ecore_drm_seat_cap_to_ecore_device_class(EVDEV_SEAT_TOUCH);
        ret = _ecore_drm_device_add_ecore_device(edev, clas);
        if (ret) _ecore_drm_device_info_send(window, edev, clas, 1);
     }
}

void
_ecore_drm_device_remove(unsigned int window, Ecore_Drm_Evdev *edev)
{
   Eina_Bool ret = EINA_FALSE;
   Ecore_Device_Class clas;

   if (edev->seat_caps & EVDEV_SEAT_POINTER)
     {
        clas = _ecore_drm_seat_cap_to_ecore_device_class(EVDEV_SEAT_POINTER);
        ret = _ecore_drm_device_del_ecore_device(edev, clas);
        if (ret) _ecore_drm_device_info_send(window, edev, clas, 0);
     }
   if (edev->seat_caps & EVDEV_SEAT_KEYBOARD)
     {
        clas = _ecore_drm_seat_cap_to_ecore_device_class(EVDEV_SEAT_KEYBOARD);
        ret = _ecore_drm_device_del_ecore_device(edev, clas);
        if (ret) _ecore_drm_device_info_send(window, edev, clas, 0);
     }
   if (edev->seat_caps & EVDEV_SEAT_TOUCH)
     {
        clas = _ecore_drm_seat_cap_to_ecore_device_class(EVDEV_SEAT_TOUCH);
        ret = _ecore_drm_device_del_ecore_device(edev, clas);
        if (ret) _ecore_drm_device_info_send(window, edev, clas, 0);
     }
}

static void 
_device_added(Ecore_Drm_Input *input, struct libinput_device *device)
{
   struct libinput_seat *libinput_seat;
   const char *seat_name;
   Ecore_Drm_Seat *seat;
   Ecore_Drm_Evdev *edev;
   Ecore_Drm_Event_Input_Device_Add *ev;

   TRACE_INPUT_BEGIN(_device_added);

   libinput_seat = libinput_device_get_seat(device);
   seat_name = libinput_seat_get_logical_name(libinput_seat);

   /* try to get a seat */
   if (!(seat = _seat_get(input, seat_name)))
     {
        ERR("Could not get matching seat: %s", seat_name);
        TRACE_INPUT_END();
        return;
     }

   /* try to create a new evdev device */
   if (!(edev = _ecore_drm_evdev_device_create(seat, device)))
     {
        ERR("Failed to create new evdev device");
        TRACE_INPUT_END();
        return;
     }

   edev->fd = (int)(intptr_t)eina_hash_find(_fd_hash, edev->path);

   /* append this device to the seat */
   seat->devices = eina_list_append(seat->devices, edev);

   ev = calloc(1, sizeof(Ecore_Drm_Event_Input_Device_Add));
   if (!ev)
     {
        TRACE_INPUT_END();
        return;
     }

   ev->name = eina_stringshare_add(libinput_device_get_name(device));
   ev->sysname = eina_stringshare_add(edev->path);
   ev->seatname = eina_stringshare_add(edev->seat->name);
   ev->caps = edev->seat_caps;

   ecore_event_add(ECORE_DRM_EVENT_INPUT_DEVICE_ADD,
                   ev,
                   _ecore_drm_event_input_device_add_free,
                   NULL);

   if (input->dev->window != -1) // window id is valid
     _ecore_drm_device_add(input->dev->window, edev);

   TRACE_INPUT_END();
}

static void 
_device_removed(Ecore_Drm_Input *input, struct libinput_device *device)
{
   Ecore_Drm_Evdev *edev;
   Ecore_Drm_Event_Input_Device_Del *ev;

   TRACE_INPUT_BEGIN(_device_removed);

   /* try to get the evdev structure */
   if (!(edev = libinput_device_get_user_data(device)))
     {
        TRACE_INPUT_END();
        return;
     }

   ev = calloc(1, sizeof(Ecore_Drm_Event_Input_Device_Del));
   if (!ev)
     {
        TRACE_INPUT_END();
        return;
     }

   ev->name = eina_stringshare_add(libinput_device_get_name(device));
   ev->sysname = eina_stringshare_add(edev->path);
   ev->seatname = eina_stringshare_add(edev->seat->name);
   ev->caps = edev->seat_caps;

   ecore_event_add(ECORE_DRM_EVENT_INPUT_DEVICE_DEL,
                   ev,
                   _ecore_drm_event_input_device_del_free,
                   NULL);

   if (input->dev->window != -1) // window id is valid
     _ecore_drm_device_remove(input->dev->window, edev);

   /* remove this evdev from the seat's list of devices */
   edev->seat->devices = eina_list_remove(edev->seat->devices, edev);

   if (_fd_hash)
     eina_hash_del_by_key(_fd_hash, edev->path);

   /* tell launcher to release device */
   if (edev->fd >= 0)
     _ecore_drm_launcher_device_close(edev->path, edev->fd);

   /* destroy this evdev */
   _ecore_drm_evdev_device_destroy(edev);

   TRACE_INPUT_END();
}

static int 
_udev_event_process(struct libinput_event *event)
{
   struct libinput *libinput;
   struct libinput_device *device;
   Ecore_Drm_Input *input;
   Eina_Bool ret = EINA_TRUE;

   libinput = libinput_event_get_context(event);
   input = libinput_get_user_data(libinput);
   device = libinput_event_get_device(event);

   switch (libinput_event_get_type(event))
     {
      case LIBINPUT_EVENT_DEVICE_ADDED:
        _device_added(input, device);
        break;
      case LIBINPUT_EVENT_DEVICE_REMOVED:
        _device_removed(input, device);
        break;
      default:
        ret = EINA_FALSE;
     }

   return ret;
}

static void 
_input_event_process(struct libinput_event *event)
{
   if (_udev_event_process(event)) return;
   if (_ecore_drm_evdev_event_process(event)) return;
}

static void 
_input_events_process(Ecore_Drm_Input *input)
{
   struct libinput_event *event;

   while ((event = libinput_get_event(input->libinput)))
     {
        _input_event_process(event);
        libinput_event_destroy(event);
     }
}

static Eina_Bool 
_cb_input_dispatch(void *data, Ecore_Fd_Handler *hdlr EINA_UNUSED)
{
   Ecore_Drm_Input *input;

   if (!(input = data)) return EINA_TRUE;

   if (libinput_dispatch(input->libinput) != 0)
     ERR("Failed to dispatch libinput events: %m");

   /* process pending events */
   _input_events_process(input);

   return EINA_TRUE;
}

const struct libinput_interface _input_interface = 
{
   _cb_open_restricted,
   _cb_close_restricted,
};

/* public functions */
EAPI Eina_Bool 
ecore_drm_inputs_create(Ecore_Drm_Device *dev)
{
   Ecore_Drm_Input *input;

   /* check for valid device */
   EINA_SAFETY_ON_NULL_RETURN_VAL(dev, EINA_FALSE);

   TRACE_INPUT_BEGIN(ecore_drm_inputs_create);
   TRACE_EFL_BEGIN(DRM INPUTS CREATE);

   /* try to allocate space for new input structure */
   if (!(input = calloc(1, sizeof(Ecore_Drm_Input))))
     {
        TRACE_INPUT_END();
        TRACE_EFL_END();
        return EINA_FALSE;
     }

   /* set reference for parent device */
   input->dev = dev;

   /* try to create libinput context */
   input->libinput = 
     libinput_udev_create_context(&_input_interface, input, eeze_udev_get());
   if (!input->libinput)
     {
        ERR("Could not create libinput context: %m");
        goto err;
     }

   /* set libinput log priority */
   libinput_log_set_priority(input->libinput, LIBINPUT_LOG_PRIORITY_INFO);

   /* assign udev seat */
   if (libinput_udev_assign_seat(input->libinput, dev->seat) != 0)
     {
        ERR("Failed to assign seat: %m");
        goto err;
     }

   /* process pending events */
   _input_events_process(input);

   /* enable this input */
   if (!ecore_drm_inputs_enable(input))
     {
        ERR("Failed to enable input");
        goto err;
     }

   /* append this input */
   dev->inputs = eina_list_append(dev->inputs, input);

   TRACE_EFL_END();
   TRACE_INPUT_END();
   return EINA_TRUE;

err:
   if (input->libinput) libinput_unref(input->libinput);
   free(input);
   TRACE_EFL_END();
   TRACE_INPUT_END();
   return EINA_FALSE;
}

EAPI Eina_Bool
ecore_drm_inputs_devices_create(Ecore_Drm_Device *dev)
{
   Ecore_Drm_Input *input;
   struct libinput_device *device;
   int devices_num;
   char *env;
   Eina_Stringshare *path;

   /* check for valid device */
   EINA_SAFETY_ON_NULL_RETURN_VAL(dev, EINA_FALSE);

   TRACE_INPUT_BEGIN(ecore_drm_inputs_devices_create);
   TRACE_EFL_BEGIN(DRM INPUTS DEVICES CREATE);

   if ((env = getenv("PATH_DEVICES_NUM")))
     devices_num = atoi(env);
   if (!env || devices_num == 0)
     {
        TRACE_INPUT_END();
        TRACE_EFL_END();
        return EINA_TRUE;
     }

   INF("PATH_DEVICES_NUM : %d", devices_num);

   /* try to allocate space for new input structure */
   if (!(input = calloc(1, sizeof(Ecore_Drm_Input))))
     {
        TRACE_INPUT_END();
        TRACE_EFL_END();
        return EINA_FALSE;
     }

   /* set reference for parent device */
   input->dev = dev;

   /* try to create libinput context */
   input->libinput =
     libinput_path_create_context(&_input_interface, input);
   if (!input->libinput)
     {
        ERR("Could not create libinput path context: %m");
        goto err;
     }

   /* set libinput log priority */
   libinput_log_set_priority(input->libinput, LIBINPUT_LOG_PRIORITY_INFO);

   for (int i = 0; i < devices_num; i++)
     {
        char buf[1024] = "PATH_DEVICE_";
        eina_convert_itoa(i + 1, buf + 12);
        env = getenv(buf);
        if (env)
          {
             path = eina_stringshare_add(env);
             device = libinput_path_add_device(input->libinput, path);
             if (!device)
               ERR("Failed to initialized device %s", path);
             else
               INF("libinput_path created input device %s", path);
          }
     }

   /* process pending events */
   _input_events_process(input);

   /* enable this input */
   if (!ecore_drm_inputs_enable(input))
     {
        ERR("Failed to enable input");
        goto err;
     }

   /* append this input */
   dev->inputs = eina_list_append(dev->inputs, input);

   TRACE_EFL_END();
   TRACE_INPUT_END();
   return EINA_TRUE;

err:
   if (input->libinput) libinput_unref(input->libinput);
   free(input);
   TRACE_EFL_END();
   TRACE_INPUT_END();
   return EINA_FALSE;
}

EAPI void 
ecore_drm_inputs_destroy(Ecore_Drm_Device *dev)
{
   Ecore_Drm_Input *input;
   Ecore_Drm_Seat *seat;
   Ecore_Drm_Evdev *edev;

   EINA_SAFETY_ON_NULL_RETURN(dev);
   EINA_LIST_FREE(dev->seats, seat)
     {
        EINA_LIST_FREE(seat->devices, edev)
          {
             _ecore_drm_launcher_device_close(edev->path, edev->fd);
             _ecore_drm_evdev_device_destroy(edev);
          }

        if (seat->name) eina_stringshare_del(seat->name);
        free(seat);
     }

   EINA_LIST_FREE(dev->inputs, input)
     {
        if (input->hdlr) ecore_main_fd_handler_del(input->hdlr);
        if (input->libinput) libinput_unref(input->libinput);
        free(input);
     }
}

EAPI Eina_Bool 
ecore_drm_inputs_enable(Ecore_Drm_Input *input)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(input, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(input->libinput, EINA_FALSE);

   input->fd = libinput_get_fd(input->libinput);

   if (!input->hdlr)
     {
        input->hdlr = 
          ecore_main_fd_handler_add(input->fd, ECORE_FD_READ, 
                                    _cb_input_dispatch, input, NULL, NULL);
     }

   if (input->suspended)
     {
        if (libinput_resume(input->libinput) != 0) 
          goto err;

        input->suspended = EINA_FALSE;

        /* process pending events */
        _input_events_process(input);
     }

   input->enabled = EINA_TRUE;
   input->suspended = EINA_FALSE;

   return EINA_TRUE;

err:
   input->enabled = EINA_FALSE;
   if (input->hdlr) ecore_main_fd_handler_del(input->hdlr);
   input->hdlr = NULL;
   return EINA_FALSE;
}

EAPI void 
ecore_drm_inputs_disable(Ecore_Drm_Input *input)
{
   EINA_SAFETY_ON_NULL_RETURN(input);
   EINA_SAFETY_ON_TRUE_RETURN(input->suspended);

   /* suspend this input */
   libinput_suspend(input->libinput);

   /* process pending events */
   _input_events_process(input);

   input->suspended = EINA_TRUE;
}

EAPI Eina_List *
ecore_drm_seat_evdev_list_get(Ecore_Drm_Seat *seat)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(seat, NULL);
   return seat->devices;
}

void
_ecore_drm_inputs_init(void)
{
  _fd_hash = eina_hash_string_superfast_new(NULL);
}

void
_ecore_drm_inputs_shutdown(void)
{
   eina_hash_free(_fd_hash);
   _fd_hash = NULL;
}
