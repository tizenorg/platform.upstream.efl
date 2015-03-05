#ifndef _ECORE_BUFFER_QUEUE_H_
#define _ECORE_BUFFER_QUEUE_H_

#include <Eina.h>
#include "Ecore_Buffer.h"

#ifdef EAPI
# undef EAPI
#endif

#ifdef _WIN32
# ifdef EFL_ECORE_EVAS_BUILD
#  ifdef DLL_EXPORT
#   define EAPI __declspec(dllexport)
#  else
#   define EAPI
#  endif /* ! DLL_EXPORT */
# else
#  define EAPI __declspec(dllimport)
# endif /* ! EFL_ECORE_EVAS_BUILD */
#else
# ifdef __GNUC__
#  if __GNUC__ >= 4
#   define EAPI __attribute__ ((visibility("default")))
#  else
#   define EAPI
#  endif
# else
#  define EAPI
# endif
#endif /* ! _WIN32 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _Ecore_Buffer_Return
{
   ECORE_BUFFER_RETURN_ERROR,
   ECORE_BUFFER_RETURN_SUCCESS,
   ECORE_BUFFER_RETURN_EMPTY,
   ECORE_BUFFER_RETURN_NOT_EMPTY,
   ECORE_BUFFER_RETURN_NEED_ALLOC,
} Ecore_Buffer_Return;

typedef struct _Ecore_Buffer_Consumer Ecore_Buffer_Consumer;
typedef struct _Ecore_Buffer_Provider Ecore_Buffer_Provider;

typedef void (*Ecore_Buffer_Consumer_Provider_Add_Cb) (Ecore_Buffer_Consumer *consumer, void *data);
typedef void (*Ecore_Buffer_Consumer_Provider_Del_Cb) (Ecore_Buffer_Consumer *consumer, void *data);
typedef void (*Ecore_Buffer_Consumer_Enqueue_Cb) (Ecore_Buffer_Consumer *consumer, void *data);

typedef void (*Ecore_Buffer_Provider_Consumer_Add_Cb) (Ecore_Buffer_Provider *provider, int queue_size, int w, int h, void *data);
typedef void (*Ecore_Buffer_Provider_Consumer_Del_Cb) (Ecore_Buffer_Provider *provider, void *data);
typedef void (*Ecore_Buffer_Provider_Enqueue_Cb) (Ecore_Buffer_Provider *provider, void *data);

/**
 * @brief Init the Ecore_Buffer_Queue system.
 *
 * @return How many times the lib has been initialized, 0 indicates failure.
 *
 * Set up the connection of Buffer Queue deamon, and Init Ecore_Buffer_Queue libraries.
 *
 * @see ecore_buffer_queue_shutdown()
 */
EAPI int   ecore_buffer_queue_init(void);
/**
 * @brief Shut down the Ecore_Buffer_Queue system.
 *
 * this closes the connection of Buffer Queue deamon, and Shut down Ecore_Buffer_Queue libraries.
 *
 * @see ecore_buffer_queue_init()
 */
EAPI void  ecore_buffer_queue_shutdown(void);
/**
 * @brief Creates a new Buffer Consumer based on name and common parameters.
 *
 * @param[in] name the name of Buffer_Queue, this is needed by Consumer and Provider to connect each other.
 * @param[in] queue_size size of Queue (If you pass this 0, then default size two(2) is appied)
 * @param[in] w width of buffer recommeneded to provider.
 * @param[in] h height of buffer recommended to provider.
 *
 * @return Ecore_Buffer_Consumer instance or @c NULL if creation failed.
 */
EAPI Ecore_Buffer_Consumer    *ecore_buffer_consumer_new(const char *name, int32_t queue_size, int32_t w, int32_t h);
/**
 * @brief Free an Ecore_Buffer_Consumer
 *
 * @param[in] consumer The Ecore_Buffer_Consumer to free
 *
 * This frees up any memory used by the Ecore_Buffer_Consumer.
 */
EAPI void                      ecore_buffer_consumer_free(Ecore_Buffer_Consumer *consumer);
/**
 * @brief Return the latest Ecore_Buffer submitted by provider.
 *
 * @param[in] consumer The Ecore_Buffer_Consumer to request for buffer
 *
 * @return Ecore_Buffer handle or NULL if acquirement failed.
 *
 * @see ecore_buffer_consumer_buffer_release()
 *
 * Consumer can store Ecore_Buffer submitted by Provider as much as size of queue
 * which is passed as a argument of ecore_buffer_consumer_new().
 */
EAPI Ecore_Buffer             *ecore_buffer_consumer_buffer_dequeue(Ecore_Buffer_Consumer *consumer);
/**
 * @brief Release the acquired Ecore_Buffer.
 *
 * @param[in] consumer The Ecore_Buffer_Consumer to request release buffer
 * @param[in] buffer The Ecore_Buffer to release
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 *
 * @see ecore_buffer_consumer_buffer_dequeue()
 *
 * Consumer should release the Ecore_Buffer after acquiring and using it.
 * By doing release, Ecore_Buffer will be used by provider again,
 * or freed internally if Ecore_Buffer is not necessary anymore.
 * If not, the resource of Ecore_Buffer is continually owned by consumer until released.
 */
EAPI Eina_Bool                 ecore_buffer_consumer_buffer_release(Ecore_Buffer_Consumer *consumer, Ecore_Buffer *buffer);
/**
 * @brief Check if Queue of Ecore_Buffer is empty.
 *
 * @param[in] consumer The Ecore_Buffer_Consumer to query
 *
 * @return @c EINA_TRUE means queue is empty, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool                 ecore_buffer_consumer_queue_is_empty(Ecore_Buffer_Consumer *consumer);
/**
 * @brief Set a callback for provider connection events.
 * @param[in] consumer The Ecore_Buffer_Consumer to set callbacks on
 * @param[in] func The function to call
 * @param[in] data A pointer to the user data to store.
 *
 * A call to this function will set a callback on an Ecore_Buffer_Consumer, causing
 * @p func to be called whenever @p consumer is connected with provider.
 */
EAPI void                      ecore_buffer_consumer_provider_add_cb_set(Ecore_Buffer_Consumer *consumer, Ecore_Buffer_Consumer_Provider_Add_Cb func, void *data);
/**
 * @brief Set a callback for provider disconnection events.
 * @param[in] consumer The Ecore_Buffer_Consumer to set callbacks on
 * @param[in] func The function to call
 * @param[in] data A pointer to the user data to store.
 *
 * A call to this function will set a callback on an Ecore_Buffer_Consumer, causing
 * @p func to be called whenever @p consumer is disconnected with provider.
 */
EAPI void                      ecore_buffer_consumer_provider_del_cb_set(Ecore_Buffer_Consumer *consumer, Ecore_Buffer_Consumer_Provider_Del_Cb func, void *data);
/**
 * @brief Set a callback for enqueued buffer events.
 * @param[in] consumer The Ecore_Buffer_Consumer to set callbacks on
 * @param[in] func The function to call
 * @param[in] data A pointer to the user data to store.
 *
 * A call to this function will set a callback on an Ecore_Buffer_Consumer, causing
 * @p func to be called whenever @p consumer has received buffer submitted from provider.
 *
 * You may success acuiqre Ecore_Buffer after this callback called.
 */
EAPI void                      ecore_buffer_consumer_buffer_enqueued_cb_set(Ecore_Buffer_Consumer *consumer, Ecore_Buffer_Consumer_Enqueue_Cb func, void *data);
/**
 * @brief Creates a new Buffer Provider based on name.
 *
 * @param[in] name the name of Buffer_Queue.
 *
 * @return Ecore_Buffer_Provider instance or @c NULL if creation failed.
 */
EAPI Ecore_Buffer_Provider    *ecore_buffer_provider_new(const char *name);
/**
 * @brief Free an Ecore_Buffer_Provider
 *
 * @param[in] provider The Ecore_Buffer_Provider to free
 *
 * This frees up any memory used by the Ecore_Buffer_Provider.
 */
EAPI void                      ecore_buffer_provider_free(Ecore_Buffer_Provider *provider);
/**
 * @brief Return the Ecore_Buffer released by consumer or State of Queue.
 *
 * @param[in] provider The Ecore_Buffer_Provider to request for buffer
 * @param[out] ret_buf A Pointer to the Ecore_Buffer
 *
 * @return The enumeration of Ecore_Buffer_Return to indicate result of Dequeueing.
 *
 * This function gives you drawable buffer and inform you the state of Queue.
 * Each return value of enumeration has meaning as below.
 * @ECORE_BUFFER_RETURN_ERROR means error occured.
 * @ECORE_BUFFER_RETURN_SUCCESS means success to dequeue, therefore ret_buf is valid.
 * @ECORE_BUFFER_RETURN_EMPTY means queue is empty, not available slot in Queue.
 *  in other words, there is no free drawable buffer in Queue.
 * @ECORE_BUFFER_RETURN_NEED_ALLOC means that there is available slot, but not allocated.
 *  so, You may create new Ecore_Buffer, and then just submit the Ecore_Buffer.
 *
 * @see ecore_buffer_new(), ecore_buffer_provider_buffer_enqueue()
 */
EAPI Ecore_Buffer_Return       ecore_buffer_provider_buffer_acquire(Ecore_Buffer_Provider *provider, Ecore_Buffer **ret_buf);
/**
 * @brief Submit the Ecore_Buffer to Consumer to request compositing.
 *
 * @param[in] provider The Ecore_Buffer_Provider connected with consumer.
 * @param[in] buffer The Ecore_Buffer to submit
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 *
 * This function allow you to submit the Ecore_Buffer to consumer to request compositing.
 * And this will be success, in case only you submit dequeued Ecore_Buffer,
 * and new Ecore_Buffer after received return value of ECORE_BUFFER_RETURN_NEED_ALLOC by ecore_buffer_provider_buffer_acquire().
 *
 * @see ecore_buffer_new(), ecore_buffer_provider_buffer_dequeue()
 */
EAPI Eina_Bool                 ecore_buffer_provider_buffer_enqueue(Ecore_Buffer_Provider *provider, Ecore_Buffer *buffer);
/**
 * @brief Check if state of queue.
 *
 * @param[in] provider The Ecore_Buffer_Provider to query
 *
 * @ECORE_BUFFER_RETURN_NOT_EMPTY: means there is a dequeueable Ecore_Buffer at least one.
 * @ECORE_BUFFER_RETURN_EMPTY means queue is empty, not available slot in Queue.
 *  in other words, there is no free drawable buffer in Queue.
 * @ECORE_BUFFER_RETURN_NEED_ALLOC means that there is available slot, but not allocated.
 *  so, You may create new Ecore_Buffer, and then just submit the Ecore_Buffer.
 *
 * @return @c EINA_TRUE means queue is empty, @c EINA_FALSE otherwise.
 */
EAPI Ecore_Buffer_Return       ecore_buffer_provider_buffer_acquirable_check(Ecore_Buffer_Provider *provider);
/**
 * @brief Set a callback for consumer connection events.
 * @param[in] consumer The Ecore_Buffer_Provider to set callbacks on
 * @param[in] func The function to call
 * @param[in] data A pointer to the user data to store.
 *
 * A call to this function will set a callback on an Ecore_Buffer_Provider, causing
 * @p func to be called whenever @p provider is connected with consumer.
 */
EAPI void                      ecore_buffer_provider_consumer_add_cb_set(Ecore_Buffer_Provider *provider, Ecore_Buffer_Provider_Consumer_Add_Cb func, void *data);
/**
 * @brief Set a callback for consumer disconnection events.
 * @param[in] consumer The Ecore_Buffer_Provider to set callbacks on
 * @param[in] func The function to call
 * @param[in] data A pointer to the user data to store.
 *
 * A call to this function will set a callback on an Ecore_Buffer_Provider, causing
 * @p func to be called whenever @p provider is disconnected with consumer.
 */
EAPI void                      ecore_buffer_provider_consumer_del_cb_set(Ecore_Buffer_Provider *provider, Ecore_Buffer_Provider_Consumer_Del_Cb func, void *data);
/**
 * @brief Set a callback for released buffer events.
 * @param[in] consumer The Ecore_Buffer_Provider to set callbacks on
 * @param[in] func The function to call
 * @param[in] data A pointer to the user data to store.
 *
 * A call to this function will set a callback on an Ecore_Buffer_Provider, causing
 * @p func to be called whenever @p provider has received Ecore_Buffer released from provider.
 *
 * You may success dequeue the Ecore_Buffer after this callback called.
 */
EAPI void                      ecore_buffer_provider_buffer_released_cb_set(Ecore_Buffer_Provider *provider, Ecore_Buffer_Provider_Enqueue_Cb func, void *data);

#endif /* _ECORE_BUFFER_QUEUE_H_ */
