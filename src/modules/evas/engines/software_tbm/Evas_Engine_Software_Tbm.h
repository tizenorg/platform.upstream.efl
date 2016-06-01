#ifndef _EVAS_ENGINE_SOFTWARE_TBM_H
# define _EVAS_ENGINE_SOFTWARE_TBM_H

typedef struct _Evas_Engine_Info_Software_Tbm Evas_Engine_Info_Software_Tbm;

struct _Evas_Engine_Info_Software_Tbm
{
   /* PRIVATE - don't mess with this baby or evas will poke its tongue out
    * at you and make nasty noises */
   Evas_Engine_Info magic;

   /* engine specific data & parameters it needs to set up */
   struct
     {
      int depth, rotation, edges;
      void *tbm_queue;
      Eina_Bool ext_tbm_queue;
      unsigned int destination_alpha : 1;
     } info;

   /* non-blocking or blocking mode */
   Evas_Engine_Render_Mode render_mode;

};

#endif
