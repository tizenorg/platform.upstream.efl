#include "shared_buffer.h"

struct _Shared_Buffer
{
   Ecore_Buffer *buffer;
<<<<<<< HEAD
   Shared_Buffer_State state;
   struct bq_buffer *proxy;
   int w, h;
   int format;
   unsigned int flags;
   const char *backend;
};

Shared_Buffer *
_shared_buffer_new(struct bq_buffer *proxy, int w, int h, int format, unsigned int flags)
{
   Shared_Buffer *sb;

   sb = calloc(sizeof(Shared_Buffer), 1);
   if (!sb)
     return NULL;

   sb->proxy = proxy;
=======
   struct bq_buffer *resource;
   const char *engine;
   int w, h;
   int format;
   unsigned int flags;
   Shared_Buffer_State state;
};

Shared_Buffer *
_shared_buffer_new(const char *engine, struct bq_buffer *resource, int w, int h, int format, unsigned int flags)
{
   Shared_Buffer *sb;

   sb = calloc(1, sizeof(Shared_Buffer));
   if (!sb)
     return NULL;

   sb->engine = eina_stringshare_add(engine);
   sb->resource = resource;
>>>>>>> opensource/master
   sb->w = w;
   sb->h = h;
   sb->format = format;
   sb->flags = flags;
<<<<<<< HEAD
=======
   sb->state = SHARED_BUFFER_STATE_NEW;
>>>>>>> opensource/master

   return sb;
}

void
_shared_buffer_free(Shared_Buffer *sb)
{
<<<<<<< HEAD
   if (!sb)
     return;

=======
   EINA_SAFETY_ON_NULL_RETURN(sb);

   if (sb->engine) eina_stringshare_del(sb->engine);
>>>>>>> opensource/master
   free(sb);
}

Eina_Bool
<<<<<<< HEAD
_shared_buffer_info_get(Shared_Buffer *sb, int *w, int *h, int *format, unsigned int *flags)
{
   if (!sb)
     return EINA_FALSE;

=======
_shared_buffer_info_get(Shared_Buffer *sb, const char **engine, int *w, int *h, int *format, unsigned int *flags)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(sb, EINA_FALSE);

   if (engine) *engine = sb->engine;
>>>>>>> opensource/master
   if (w) *w = sb->w;
   if (h) *h = sb->h;
   if (format) *format = sb->format;
   if (flags) *flags = sb->flags;

   return EINA_TRUE;
}

Eina_Bool
_shared_buffer_buffer_set(Shared_Buffer *sb, Ecore_Buffer *buffer)
{
<<<<<<< HEAD
   if (!sb)
     return EINA_FALSE;
=======
   EINA_SAFETY_ON_NULL_RETURN_VAL(sb, EINA_FALSE);
>>>>>>> opensource/master

   if (sb->buffer)
     {
        ERR("Already exist buffer");
        return EINA_FALSE;
     }

   sb->buffer = buffer;

   return EINA_TRUE;
}

Ecore_Buffer *
_shared_buffer_buffer_get(Shared_Buffer *sb)
{
<<<<<<< HEAD
   if (!sb)
     return NULL;
=======
   EINA_SAFETY_ON_NULL_RETURN_VAL(sb, NULL);
>>>>>>> opensource/master

   return sb->buffer;
}

Eina_Bool
<<<<<<< HEAD
_shared_buffer_proxy_set(Shared_Buffer *sb, struct bq_buffer *proxy)
{
   if (!sb)
     return EINA_FALSE;

   if (sb->proxy)
     {
        ERR("Already exist proxy");
        return EINA_FALSE;
     }

   sb->proxy = proxy;
=======
_shared_buffer_resource_set(Shared_Buffer *sb, struct bq_buffer *resource)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(sb, EINA_FALSE);

   if (sb->resource)
     {
        ERR("Already exist resource");
        return EINA_FALSE;
     }

   sb->resource = resource;
>>>>>>> opensource/master

   return EINA_TRUE;
}

struct bq_buffer *
<<<<<<< HEAD
_shared_buffer_proxy_get(Shared_Buffer *sb)
{
   if (!sb)
     return NULL;

   return sb->proxy;
=======
_shared_buffer_resource_get(Shared_Buffer *sb)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(sb, NULL);

   return sb->resource;
>>>>>>> opensource/master
}

void
_shared_buffer_state_set(Shared_Buffer *sb, Shared_Buffer_State state)
{
<<<<<<< HEAD
   if (!sb)
     return;
=======
   EINA_SAFETY_ON_NULL_RETURN(sb);
>>>>>>> opensource/master

   sb->state = state;
}

Shared_Buffer_State
_shared_buffer_state_get(Shared_Buffer *sb)
{
<<<<<<< HEAD
   if (!sb)
     return SHARED_BUFFER_STATE_UNKNOWN;
=======
   EINA_SAFETY_ON_NULL_RETURN_VAL(sb, SHARED_BUFFER_STATE_UNKNOWN);
>>>>>>> opensource/master

   return sb->state;
}

const char *
_shared_buffer_state_string_get(Shared_Buffer *sb)
{
<<<<<<< HEAD
   if (!sb)
     return "INVALID_OBJECT";
=======
   EINA_SAFETY_ON_NULL_RETURN_VAL(sb, "INVAILD OBJECT");
>>>>>>> opensource/master

   switch (sb->state)
     {
      case SHARED_BUFFER_STATE_ENQUEUE:
         return "SHARED_BUFFER_STATE_ENQUEUE";
      case SHARED_BUFFER_STATE_SUBMIT:
         return "SHARED_BUFFER_STATE_SUBMIT";
      case SHARED_BUFFER_STATE_DEQUEUE:
<<<<<<< HEAD
         return "SHARED_BUFFER_STATE_SUBMIT";
=======
         return "SHARED_BUFFER_STATE_DEQUEUE";
>>>>>>> opensource/master
      case SHARED_BUFFER_STATE_ATTACH:
         return "SHARED_BUFFER_STATE_ATTACH";
      case SHARED_BUFFER_STATE_IMPORT:
         return "SHARED_BUFFER_STATE_IMPORT";
      case SHARED_BUFFER_STATE_DETACH:
         return "SHARED_BUFFER_STATE_DETACH";
      case SHARED_BUFFER_STATE_ACQUIRE:
         return "SHARED_BUFFER_STATE_ACQUIRE";
      case SHARED_BUFFER_STATE_RELEASE:
         return "SHARED_BUFFER_STATE_RELEASE";
      default:
      case SHARED_BUFFER_STATE_UNKNOWN:
         return "SHARED_BUFFER_STATE_UNKNOWN";
     }
}
