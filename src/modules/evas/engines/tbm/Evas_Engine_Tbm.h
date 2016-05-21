#ifndef _EVAS_ENGINE_TBM_H
# define _EVAS_ENGINE_TBM_H

# define EVAS_ENGINE_TBM_SWAP_MODE_EXISTS 1

typedef struct _Evas_Engine_Info_Tbm Evas_Engine_Info_Tbm;

typedef enum _Evas_Engine_Info_Tbm_Swap_Mode
{
   EVAS_ENGINE_TBM_SWAP_MODE_AUTO = 0,
   EVAS_ENGINE_TBM_SWAP_MODE_FULL = 1,
   EVAS_ENGINE_TBM_SWAP_MODE_COPY = 2,
   EVAS_ENGINE_TBM_SWAP_MODE_DOUBLE = 3,
   EVAS_ENGINE_TBM_SWAP_MODE_TRIPLE = 4,
   EVAS_ENGINE_TBM_SWAP_MODE_QUADRUPLE = 5
} Evas_Engine_Info_Tbm_Swap_Mode;

struct _Evas_Engine_Info_Tbm
{
   /* PRIVATE - don't mess with this baby or evas will poke its tongue out
    * at you and make nasty noises */
   Evas_Engine_Info magic;

   /* engine specific data & parameters it needs to set up */
   struct 
     {
        int depth, screen, rotation, edges;
        void *bufmgr;
        void *tbm_queue;
        Eina_Bool ext_tbm_queue;
        unsigned int destination_alpha : 1;
     } info;

   struct 
     {
        void (*pre_swap) (void *data, Evas *evas);
        void (*post_swap) (void *data, Evas *evas);
        void *data;
     } callback;

   /* non-blocking or blocking mode */
   Evas_Engine_Render_Mode render_mode;

   Eina_Bool vsync : 1;
   Eina_Bool indirect : 1;
   unsigned char swap_mode : 4;

   /* window surface should be made with these config */
   int           depth_bits;
   int           stencil_bits;
   int           msaa_bits;
};

#endif
