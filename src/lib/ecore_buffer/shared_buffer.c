#include "shared_buffer.h"

struct _Shared_Buffer
{
   Ecore_Buffer *buffer;
   struct es_buffer *proxy;

   const char *backend;
   int w, h;
   int format;
   unsigned int flags;

   Shared_Buffer_State state;
};

Shared_Buffer *
_shared_buffer_new(struct es_buffer *proxy, int w, int h, int format, unsigned int flags)
{
   Shared_Buffer *sb;

   sb = ZALLOC(Shared_Buffer, 1);
   if (!sb)
     return NULL;

   sb->proxy = proxy;
   sb->w = w;
   sb->h = h;
   sb->format = format;
   sb->flags = flags;

   return sb;
}

void
_shared_buffer_free(Shared_Buffer *sb)
{
   EINA_SAFETY_ON_NULL_RETURN(sb);

   free(sb);
}

Eina_Bool
_shared_buffer_info_get(Shared_Buffer *sb, int *w, int *h, int *format, unsigned int *flags)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(sb, EINA_FALSE);

   if (w) *w = sb->w;
   if (h) *h = sb->h;
   if (format) *format = sb->format;
   if (flags) *flags = sb->flags;

   return EINA_TRUE;
}

Eina_Bool
_shared_buffer_buffer_set(Shared_Buffer *sb, Ecore_Buffer *buffer)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(sb, EINA_FALSE);

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
   EINA_SAFETY_ON_NULL_RETURN_VAL(sb, NULL);

   return sb->buffer;
}

Eina_Bool
_shared_buffer_proxy_set(Shared_Buffer *sb, struct es_buffer *proxy)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(sb, EINA_FALSE);

   if (sb->proxy)
     {
        ERR("Already exist proxy");
        return EINA_FALSE;
     }

   sb->proxy = proxy;

   return EINA_TRUE;
}

struct es_buffer *
_shared_buffer_proxy_get(Shared_Buffer *sb)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(sb, NULL);

   return sb->proxy;
}

void
_shared_buffer_state_set(Shared_Buffer *sb, Shared_Buffer_State state)
{
   EINA_SAFETY_ON_NULL_RETURN(sb);

   sb->state = state;
}

Shared_Buffer_State
_shared_buffer_state_get(Shared_Buffer *sb)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(sb, SHARED_BUFFER_STATE_UNKNOWN);

   return sb->state;
}

const char *
_shared_buffer_state_string_get(Shared_Buffer *sb)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(sb, "INVAILD OBJECT");

   switch (sb->state)
     {
      case SHARED_BUFFER_STATE_ENQUEUE:
         return "SHARED_BUFFER_STATE_ENQUEUE";
      case SHARED_BUFFER_STATE_SUBMIT:
         return "SHARED_BUFFER_STATE_SUBMIT";
      case SHARED_BUFFER_STATE_DEQUEUE:
         return "SHARED_BUFFER_STATE_SUBMIT";
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
