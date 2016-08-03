#include "evas_vg_cache.h"
#include "evas_vg_common.h"

static Evas_Cache_Svg* svg_cache = NULL;

static void
_evas_cache_vg_data_free_cb(void *data)
{
   Vg_Data *val = data;

   eo_del(val->vg);
   free(val);
}

static void
_evas_cache_svg_entry_free_cb(void *data)
{
   Svg_Entry *entry = data;

   eina_stringshare_del(entry->file);
   free(entry->key);
   eo_del(entry->root);
   free(entry);
}

void
evas_cache_svg_init(void)
{
   if (svg_cache)
     {
        svg_cache->ref++;
        return;
     }

   svg_cache =  calloc(1, sizeof(Evas_Cache_Svg));
   svg_cache->vg_hash = eina_hash_string_superfast_new(_evas_cache_vg_data_free_cb);
   svg_cache->active = eina_hash_string_superfast_new(_evas_cache_svg_entry_free_cb);
   svg_cache->ref++;
}

void
evas_cache_svg_shutdown(void)
{
   if (!svg_cache) return;
   svg_cache->ref--;
   if (svg_cache->ref) return;
   _evas_vg_svg_node_eet_destroy();
   eina_hash_free(svg_cache->vg_hash);
   eina_hash_free(svg_cache->active);
   svg_cache = NULL;
}

static Vg_Data *
_evas_cache_vg_data_find(const char *path, int vg_id)
{
   Vg_Data *vd;
   Eina_Strbuf *key;

   key = eina_strbuf_new();
   eina_strbuf_append_printf(key, "%s/%d", path, vg_id);
   vd = eina_hash_find(svg_cache->vg_hash, eina_strbuf_string_get(key));
   if (!vd)
     {
        vd = _evas_vg_load_vg_data(path, vg_id);
        eina_hash_add(svg_cache->vg_hash, eina_strbuf_string_get(key), vd);
     }
   eina_strbuf_free(key);
   return vd;
}

static void
_evas_cache_svg_vg_tree_update(Svg_Entry *entry)
{
   Vg_Data *src_vg = NULL, *dst_vg = NULL;
   if(!entry) return;

   if (!entry->src_vg)
     {
        entry->root = NULL;
        return;
     } 

   if (!entry->dest_vg)
     {
        src_vg = _evas_cache_vg_data_find(entry->file, entry->src_vg);
     }
   else
     {
        dst_vg = _evas_cache_vg_data_find(entry->file, entry->dest_vg);
     }
   entry->root = _evas_vg_dup_vg_tree(src_vg, dst_vg, entry->key_frame, entry->w, entry->h);
   eina_stringshare_del(entry->file);
   entry->file = NULL;
}

Svg_Entry*
evas_cache_svg_find(const char *path,
                    int src_vg, int dest_vg,
                    float key_frame, int w, int h)
{
   Svg_Entry* se;
   Eina_Strbuf *key;

   if (!svg_cache) return NULL;

   key = eina_strbuf_new();
   eina_strbuf_append_printf(key, "%s/%d/%d/%.2f/%d/%d",
                             path, src_vg, dest_vg, key_frame, w, h);
   se = eina_hash_find(svg_cache->active, eina_strbuf_string_get(key));
   if (!se)
     {
        se = calloc(1, sizeof(Svg_Entry));
        se->file = eina_stringshare_add(path);
        se->key_frame = key_frame;
        se->src_vg = src_vg;
        se->dest_vg = dest_vg;
        se->w = w;
        se->h = h;
        se->key = eina_strbuf_string_steal(key);
        eina_hash_direct_add(svg_cache->active, se->key, se);
     }
   eina_strbuf_free(key);
   se->ref++;
   return se;
}

Efl_VG*
evas_cache_svg_vg_tree_get(Svg_Entry *entry)
{
   if (entry->root) return entry->root;

   if (entry->file)
     {
        _evas_cache_svg_vg_tree_update(entry);
     }
   return entry->root;
}

void
evas_cache_svg_entry_del(Svg_Entry *svg_entry)
{
   if (!svg_entry) return;

   svg_entry->ref--;
}

