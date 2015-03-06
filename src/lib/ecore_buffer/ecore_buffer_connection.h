#ifndef _ECORE_BUFFER_CONNECTION_H_
#define _ECORE_BUFFER_CONNECTION_H_

#include <stdio.h>
#include <wayland-client.h>

#include <Eina.h>
#include <Ecore.h>
#include <Ecore_Buffer_Queue.h>

#include "es-share-client-protocol.h"
#include "ecore_buffer_private.h"


typedef struct _Ecore_Buffer_Connection Ecore_Buffer_Connection;

extern Ecore_Buffer_Connection *_ecore_buffer_connection;

void                  _ecore_buffer_connection_init_wait(void);
struct es_provider   *_ecore_buffer_connection_provider_create(const char *name);
struct es_consumer   *_ecore_buffer_connection_consumer_create(const char *name, int queue_size, int w, int h);

#endif
