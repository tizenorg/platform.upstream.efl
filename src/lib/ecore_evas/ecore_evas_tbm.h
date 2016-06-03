#ifndef ECORE_EVAS_TBM_H
#define ECORE_EVAS_TBM_H

typedef struct _Ecore_Evas_Engine_Tbm_Data Ecore_Evas_Engine_Tbm_Data;

struct _Ecore_Evas_Engine_Tbm_Data {
   void* tbm_queue;
   tbm_surface_h tbm_surf;
   Eina_Bool ext_tbm_queue;
   void  (*free_func) (void *data, void *tbm_queue);
   void *(*alloc_func) (void *data, int w, int h);
   Evas_Object *image;
   void *data;
};

#endif // ECORE_EVAS_TBM_H
