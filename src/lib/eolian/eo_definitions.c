#include <stdio.h>
#include <stdlib.h>

#include "eo_definitions.h"

void
eo_definitions_temps_free(Eo_Lexer_Temps *tmp)
{
   Eina_Strbuf *buf;
   Eolian_Function_Parameter *par;
   Eolian_Type *tp;
   Eolian_Variable *var;
   Eolian_Class *cl;
   const char *s;

   EINA_LIST_FREE(tmp->str_bufs, buf)
     eina_strbuf_free(buf);

   EINA_LIST_FREE(tmp->params, par)
     database_parameter_del(par);

   if (tmp->legacy_def)
     eina_stringshare_del(tmp->legacy_def);

   if (tmp->kls)
     database_class_del(tmp->kls);

   EINA_LIST_FREE(tmp->type_defs, tp)
     database_type_del(tp);

   EINA_LIST_FREE(tmp->var_defs, var)
     database_var_del(var);

   if (tmp->param)
     database_parameter_del(tmp->param);

   EINA_LIST_FREE(tmp->str_items, s)
     if (s) eina_stringshare_del(s);

   EINA_LIST_FREE(tmp->strs, s)
     if (s) eina_stringshare_del(s);

   EINA_LIST_FREE(tmp->classes, cl)
     database_class_del(cl);
}
