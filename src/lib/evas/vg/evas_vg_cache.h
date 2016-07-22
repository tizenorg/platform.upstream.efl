#ifndef _EVAS_CACHE_SVG_H
#define _EVAS_CACHE_SVG_H

#include <Evas.h>

typedef struct _Svg_Entry             Svg_Entry;
typedef struct _Evas_Cache_Svg        Evas_Cache_Svg;

struct _Evas_Cache_Svg
{
   Eina_Hash             *vg_hash;
   Eina_Hash             *active;
   int                    ref;
};

struct _Svg_Entry
{
   Eina_Stringshare     *file;
   char                 *key;
   int                   src_vg;
   int                   dest_vg;
   float                 key_frame;
   int                   w;
   int                   h;
   Efl_VG               *root;
   int                   ref;
};

void              evas_cache_svg_init(void);
void              evas_cache_svg_shutdown(void);
Svg_Entry*        evas_cache_svg_find(const char *path,
                                      int src_svg, int dest_svg,
                                      float key_frame, int w, int h);
Efl_VG*           evas_cache_svg_vg_tree_get(Svg_Entry *svg_entry);
void              evas_cache_svg_entry_del(Svg_Entry *svg_entry);

#endif /* _EVAS_CACHE_SVG_H */