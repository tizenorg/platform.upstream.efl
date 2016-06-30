#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <math.h>
#include <float.h>
#include <ctype.h>
// TIZEN_ONLY(20160420): efl/interface: update locale before parsing svg path
#include <locale.h>
//

#include <Efl.h>

typedef struct _Efl_Gfx_Shape_Data Efl_Gfx_Shape_Data;
struct _Efl_Gfx_Shape_Data
{
   struct {
      double x;
      double y;
   } current, current_ctrl;

   Efl_Gfx_Path_Command *commands;
   double *points;

   unsigned int commands_count;
   unsigned int points_count;
};

static inline unsigned int
_efl_gfx_path_command_length(Efl_Gfx_Path_Command command)
{
   switch (command)
     {
      case EFL_GFX_PATH_COMMAND_TYPE_END: return 0;
      case EFL_GFX_PATH_COMMAND_TYPE_MOVE_TO: return 2;
      case EFL_GFX_PATH_COMMAND_TYPE_LINE_TO: return 2;
      case EFL_GFX_PATH_COMMAND_TYPE_CUBIC_TO: return 6;
      case EFL_GFX_PATH_COMMAND_TYPE_CLOSE: return 0;
      case EFL_GFX_PATH_COMMAND_TYPE_LAST: return 0;
     }
   return 0;
}

static inline void
_efl_gfx_path_length(const Efl_Gfx_Path_Command *commands,
                     unsigned int *cmd_length,
                     unsigned int *pts_length)
{
   if (commands)
     while (commands[*cmd_length] != EFL_GFX_PATH_COMMAND_TYPE_END)
       {
          *pts_length += _efl_gfx_path_command_length(commands[*cmd_length]);
          (*cmd_length)++;
       }

   // Accounting for END command and handle gracefully the NULL case at the same time
   (*cmd_length)++;
}

static inline Eina_Bool
efl_gfx_path_grow(Efl_Gfx_Path_Command command,
                  Efl_Gfx_Shape_Data *pd,
                  double **offset_point)
{
   Efl_Gfx_Path_Command *cmd_tmp;
   double *pts_tmp;
   unsigned int cmd_length = 0, pts_length = 0;

   cmd_length = pd->commands_count ? pd->commands_count : 1;
   pts_length = pd->points_count;

   if (_efl_gfx_path_command_length(command))
     {
        pts_length += _efl_gfx_path_command_length(command);
        pts_tmp = realloc(pd->points, pts_length * sizeof (double));
        if (!pts_tmp) return EINA_FALSE;

        pd->points = pts_tmp;
        *offset_point = pd->points +
          pts_length - _efl_gfx_path_command_length(command);
     }

   cmd_tmp = realloc(pd->commands,
                     (cmd_length + 1) * sizeof (Efl_Gfx_Path_Command));
   if (!cmd_tmp) return EINA_FALSE;
   pd->commands = cmd_tmp;

   pd->commands_count = cmd_length + 1;
   pd->points_count = pts_length;

   // Append the command
   cmd_tmp[cmd_length - 1] = command;
   // NULL terminate the stream
   cmd_tmp[cmd_length] = EFL_GFX_PATH_COMMAND_TYPE_END;

   return EINA_TRUE;
}

static Eina_Bool
_efl_gfx_path_current_search(const Efl_Gfx_Path_Command *cmd,
                             const double *points,
                             double *current_x, double *current_y,
                             double *current_ctrl_x, double *current_ctrl_y)
{
   unsigned int i;

   if (current_x) *current_x = 0;
   if (current_y) *current_y = 0;
   if (current_ctrl_x) *current_ctrl_x = 0;
   if (current_ctrl_y) *current_ctrl_y = 0;
   if (!cmd || !points) return EINA_FALSE;

   for (i = 0; cmd[i] != EFL_GFX_PATH_COMMAND_TYPE_END; i++)
     {
        switch (cmd[i])
          {
           case EFL_GFX_PATH_COMMAND_TYPE_END:
              break;
           case EFL_GFX_PATH_COMMAND_TYPE_MOVE_TO:
           case EFL_GFX_PATH_COMMAND_TYPE_LINE_TO:
              if (current_x) *current_x = points[0];
              if (current_y) *current_y = points[1];

              points += 2;
              break;
           case EFL_GFX_PATH_COMMAND_TYPE_CUBIC_TO:
              if (current_x) *current_x = points[0];
              if (current_y) *current_y = points[1];
              if (current_ctrl_x) *current_ctrl_x = points[4];
              if (current_ctrl_y) *current_ctrl_y = points[5];

              points += 6;
              break;
           case EFL_GFX_PATH_COMMAND_TYPE_CLOSE:
              break;
           case EFL_GFX_PATH_COMMAND_TYPE_LAST:
           default:
              return EINA_FALSE;
          }
     }

   return EINA_TRUE;
}

static void
_efl_gfx_shape_path_set(Eo *obj, Efl_Gfx_Shape_Data *pd,
                        const Efl_Gfx_Path_Command *commands,
                        const double *points)
{
   Efl_Gfx_Path_Command *cmds;
   double *pts;
   unsigned int cmds_length = 0, pts_length = 0;

   if (!commands)
     {
        free(pd->commands); pd->commands = NULL;
        free(pd->points); pd->points = NULL;
        pd->current.x = pd->current.y = 0;
        pd->current_ctrl.x = pd->current_ctrl.y = 0;
        goto end;
     }

   _efl_gfx_path_length(commands, &cmds_length, &pts_length);

   cmds = realloc(pd->commands,
                  sizeof (Efl_Gfx_Path_Command) * cmds_length);
   if (!cmds) return ;
   pd->commands = cmds;

   pts = realloc(pd->points,
                 sizeof (double) * pts_length);
   if (!pts) return ;
   pd->points = pts;

   pd->commands_count = cmds_length;
   pd->points_count = pts_length;

   memcpy(pd->commands, commands, sizeof (Efl_Gfx_Path_Command) * cmds_length);
   memcpy(pd->points, points, sizeof (double) * pts_length);

   _efl_gfx_path_current_search(pd->commands, pd->points,
                                &pd->current.x, &pd->current.y,
                                &pd->current_ctrl.x, &pd->current_ctrl.y);

 end:
   eo_do(obj,
         eo_event_callback_call(EFL_GFX_PATH_CHANGED, NULL),
         eo_event_callback_call(EFL_GFX_CHANGED, NULL));
}

static void
_efl_gfx_shape_path_get(Eo *obj EINA_UNUSED, Efl_Gfx_Shape_Data *pd,
                        const Efl_Gfx_Path_Command **commands,
                        const double **points)
{
   if (commands) *commands = pd->commands;
   if (points) *points = pd->points;
}

static void
_efl_gfx_shape_path_length_get(Eo *obj EINA_UNUSED, Efl_Gfx_Shape_Data *pd,
                               unsigned int *commands, unsigned int *points)
{
   if (commands) *commands = pd->commands_count;
   if (points) *points = pd->points_count;
}

static void
_efl_gfx_shape_bounds_get(Eo *obj EINA_UNUSED,
                          Efl_Gfx_Shape_Data *pd,
                          Eina_Rectangle *r)
{
   double minx, miny, maxx, maxy;
   unsigned int i;

   EINA_RECTANGLE_SET(r, 0, 0, 0, 0);

   if (pd->points_count <= 0) return ;

   minx = pd->points[0];
   miny = pd->points[1];
   maxx = pd->points[0];
   maxy = pd->points[1];

   for (i = 1; i < pd->points_count; i += 2)
     {
        minx = minx < pd->points[i] ? minx : pd->points[i];
        miny = miny < pd->points[i + 1] ? miny : pd->points[i + 1];
        maxx = maxx > pd->points[i] ? maxx : pd->points[i];
        maxy = maxy > pd->points[i + 1] ? maxy : pd->points[i + 1];
     }

   EINA_RECTANGLE_SET(r,
                      minx, miny,
                      maxx - minx, maxy - miny);
}

static void
_efl_gfx_shape_current_get(Eo *obj EINA_UNUSED, Efl_Gfx_Shape_Data *pd,
                           double *x, double *y)
{
   if (x) *x = pd->current.x;
   if (y) *y = pd->current.y;
}

static void
_efl_gfx_shape_current_ctrl_get(Eo *obj EINA_UNUSED, Efl_Gfx_Shape_Data *pd,
                                double *x, double *y)
{
   if (x) *x = pd->current_ctrl.x;
   if (y) *y = pd->current_ctrl.y;
}

static Eina_Bool
_efl_gfx_shape_equal_commands_internal(Efl_Gfx_Shape_Data *a,
                                       Efl_Gfx_Shape_Data *b)
{
   unsigned int i;

   if (a->commands_count != b->commands_count) return EINA_FALSE;
   if (a->commands_count <= 0) return EINA_TRUE;

   for (i = 0; a->commands[i] == b->commands[i] &&
          a->commands[i] != EFL_GFX_PATH_COMMAND_TYPE_END; i++)
     ;

   return (a->commands[i] == b->commands[i]);
}

static inline double
interpolate(double from, double to, double pos_map)
{
   return (from * pos_map) + (to * (1.0 - pos_map));
}

static inline int
interpolatei(int from, int to, double pos_map)
{
   return (from * pos_map) + (to * (1.0 - pos_map));
}

typedef struct _Efl_Gfx_Property Efl_Gfx_Property;
struct _Efl_Gfx_Property
{
   double scale;
   int r, g, b, a;
   int fr, fg, fb, fa; 
   double w;
   double centered;
   const Efl_Gfx_Dash *dash;
   unsigned int dash_length;
   Efl_Gfx_Cap c;
   Efl_Gfx_Join j;
};

static inline void
gfx_property_get(const Eo *obj, Efl_Gfx_Property *property)
{
   eo_do(obj,
         property->scale = efl_gfx_shape_stroke_scale_get(),
         efl_gfx_shape_stroke_color_get(&property->r, &property->g, &property->b, &property->a),
         efl_gfx_color_get(&property->fr, &property->fg, &property->fb, &property->fa),
         property->w = efl_gfx_shape_stroke_width_get(),
         property->centered = efl_gfx_shape_stroke_location_get(),
         efl_gfx_shape_stroke_dash_get(&property->dash, &property->dash_length),
         property->c = efl_gfx_shape_stroke_cap_get(),
         property->j = efl_gfx_shape_stroke_join_get());
}

static Eina_Bool
_efl_gfx_shape_interpolate(Eo *obj, Efl_Gfx_Shape_Data *pd,
                           const Eo *from, const Eo *to, double pos_map)
{
   Efl_Gfx_Shape_Data *from_pd, *to_pd;
   Efl_Gfx_Path_Command *cmds;
   double *pts, *from_pts, *to_pts;
   unsigned int i, j;
   Efl_Gfx_Property property_from, property_to;
   Efl_Gfx_Dash *dash = NULL;

   from_pd = eo_data_scope_get(from, EFL_GFX_SHAPE_MIXIN);
   to_pd = eo_data_scope_get(to, EFL_GFX_SHAPE_MIXIN);
   if (!eo_isa(from, EFL_GFX_SHAPE_MIXIN) ||
       !eo_isa(to, EFL_GFX_SHAPE_MIXIN)) return EINA_FALSE;
   if (pd == from_pd || pd == to_pd) return EINA_FALSE;
   if (!_efl_gfx_shape_equal_commands_internal(from_pd, to_pd))
     return EINA_FALSE;

   gfx_property_get(from, &property_from);
   gfx_property_get(to, &property_to);

   if (property_from.dash_length != property_to.dash_length) return EINA_FALSE;

   cmds = realloc(pd->commands,
                  sizeof (Efl_Gfx_Path_Command) * from_pd->commands_count);
   if (!cmds && from_pd->commands_count) return EINA_FALSE;
   pd->commands = cmds;

   pts = realloc(pd->points,
                 sizeof (double) * from_pd->points_count);
   if (!pts && from_pd->points_count) return EINA_FALSE;
   pd->points = pts;

   if (cmds)
     {
        memcpy(cmds, from_pd->commands,
               sizeof (Efl_Gfx_Path_Command) * from_pd->commands_count);

        to_pts = to_pd->points;
        from_pts = from_pd->points;

        for (i = 0; cmds[i] != EFL_GFX_PATH_COMMAND_TYPE_END; i++)
          for (j = 0; j < _efl_gfx_path_command_length(cmds[i]); j++)
            {
               *pts = interpolate(*from_pts, *to_pts, pos_map);

               pts++;
               from_pts++;
               to_pts++;
            }
     }

   pd->points_count = from_pd->points_count;
   pd->commands_count = from_pd->commands_count;

   pd->current.x = interpolate(from_pd->current.x,
                               to_pd->current.x,
                               pos_map);
   pd->current.y = interpolate(from_pd->current.y,
                               to_pd->current.y,
                               pos_map);
   pd->current_ctrl.x = interpolate(from_pd->current_ctrl.x,
                                    to_pd->current_ctrl.x,
                                    pos_map);
   pd->current_ctrl.y = interpolate(from_pd->current_ctrl.y,
                                    to_pd->current_ctrl.y,
                                    pos_map);

   if (property_to.dash_length)
     {
        dash = malloc(sizeof (Efl_Gfx_Dash) * property_to.dash_length);
        if (!dash) return EINA_FALSE;

        for (i = 0; i < property_to.dash_length; i++)
          {
             dash[i].length = interpolate(property_from.dash[i].length,
                                          property_to.dash[i].length, pos_map);
             dash[i].gap = interpolate(property_from.dash[i].gap,
                                       property_to.dash[i].gap, pos_map);
          }
     }


   eo_do(obj,
         efl_gfx_shape_stroke_scale_set(interpolate(property_to.scale, property_from.scale, pos_map)),
         efl_gfx_shape_stroke_color_set(interpolatei(property_to.r, property_from.r, pos_map),
                                        interpolatei(property_to.g, property_from.g, pos_map),
                                        interpolatei(property_to.b, property_from.b, pos_map),
                                        interpolatei(property_to.a, property_from.a, pos_map)),
         efl_gfx_color_set(interpolatei(property_to.fr, property_from.fr, pos_map),
                           interpolatei(property_to.fg, property_from.fg, pos_map),
                           interpolatei(property_to.fb, property_from.fb, pos_map),
                           interpolatei(property_to.fa, property_from.fa, pos_map)),
         efl_gfx_shape_stroke_width_set(interpolate(property_to.w, property_from.w, pos_map)),
         efl_gfx_shape_stroke_location_set(interpolate(property_to.centered, property_from.centered, pos_map)),
         efl_gfx_shape_stroke_dash_set(dash, property_to.dash_length),
         efl_gfx_shape_stroke_cap_set(pos_map < 0.5 ? property_from.c : property_to.c),
         efl_gfx_shape_stroke_join_set(pos_map < 0.5 ? property_from.j : property_to.j),

         eo_event_callback_call(EFL_GFX_PATH_CHANGED, NULL),
         eo_event_callback_call(EFL_GFX_CHANGED, NULL));

   return EINA_TRUE;
}

static Eina_Bool
_efl_gfx_shape_equal_commands(Eo *obj EINA_UNUSED,
                              Efl_Gfx_Shape_Data *pd,
                              const Eo *with)
{
   Efl_Gfx_Shape_Data *with_pd;

   with_pd = eo_data_scope_get(with, EFL_GFX_SHAPE_MIXIN);
   if (!with_pd) return EINA_FALSE;

   return _efl_gfx_shape_equal_commands_internal(with_pd, pd);
}

static void
_efl_gfx_shape_dup(Eo *obj, Efl_Gfx_Shape_Data *pd, const Eo *dup_from)
{
   const Efl_Gfx_Dash *dash = NULL;
   Efl_Gfx_Shape_Data *from;
   unsigned int dash_length = 0;
   Efl_Gfx_Cap cap;
   Efl_Gfx_Join j;
   int sr, sg, sb, sa;
   double scale, location;
   double sw;

   if (obj == dup_from) return ;
   from = eo_data_scope_get(dup_from, EFL_GFX_SHAPE_MIXIN);
   if (!from) return ;

   eo_do(dup_from,
         scale = efl_gfx_shape_stroke_scale_get(),
         efl_gfx_shape_stroke_color_get(&sr, &sg, &sb, &sa),
         sw = efl_gfx_shape_stroke_width_get(),
         location = efl_gfx_shape_stroke_location_get(),
         efl_gfx_shape_stroke_dash_get(&dash, &dash_length),
         cap = efl_gfx_shape_stroke_cap_get(),
         j = efl_gfx_shape_stroke_join_get());
   eo_do(obj,
         efl_gfx_shape_stroke_scale_set(scale),
         efl_gfx_shape_stroke_color_set(sr, sg, sb, sa),
         efl_gfx_shape_stroke_width_set(sw),
         efl_gfx_shape_stroke_location_set(location),
         efl_gfx_shape_stroke_dash_set(dash, dash_length),
         efl_gfx_shape_stroke_cap_set(cap),
         efl_gfx_shape_stroke_join_set(j));

   _efl_gfx_shape_path_set(obj, pd, from->commands, from->points);

   eo_do(obj,
         eo_event_callback_call(EFL_GFX_PATH_CHANGED, NULL),
         eo_event_callback_call(EFL_GFX_CHANGED, NULL));
}

static void
_efl_gfx_shape_reset(Eo *obj, Efl_Gfx_Shape_Data *pd)
{
   free(pd->commands);
   pd->commands = NULL;
   pd->commands_count = 0;

   free(pd->points);
   pd->points = NULL;
   pd->points_count = 0;

   pd->current.x = 0;
   pd->current.y = 0;
   pd->current_ctrl.x = 0;
   pd->current_ctrl.y = 0;

   eo_do(obj,
         eo_event_callback_call(EFL_GFX_PATH_CHANGED, NULL),
         eo_event_callback_call(EFL_GFX_CHANGED, NULL));
}

static void
_efl_gfx_shape_append_move_to(Eo *obj, Efl_Gfx_Shape_Data *pd,
                              double x, double y)
{
   double *offset_point;

   if (!efl_gfx_path_grow(EFL_GFX_PATH_COMMAND_TYPE_MOVE_TO,
                          pd, &offset_point))
     return ;

   offset_point[0] = x;
   offset_point[1] = y;

   pd->current.x = x;
   pd->current.y = y;

   eo_do(obj,
         eo_event_callback_call(EFL_GFX_PATH_CHANGED, NULL),
         eo_event_callback_call(EFL_GFX_CHANGED, NULL));
}

static void
_efl_gfx_shape_append_line_to(Eo *obj, Efl_Gfx_Shape_Data *pd,
                              double x, double y)
{
   double *offset_point;

   if (!efl_gfx_path_grow(EFL_GFX_PATH_COMMAND_TYPE_LINE_TO,
                          pd, &offset_point))
     return ;

   offset_point[0] = x;
   offset_point[1] = y;

   pd->current.x = x;
   pd->current.y = y;

   eo_do(obj,
         eo_event_callback_call(EFL_GFX_PATH_CHANGED, NULL),
         eo_event_callback_call(EFL_GFX_CHANGED, NULL));
}

static void
_efl_gfx_shape_append_cubic_to(Eo *obj, Efl_Gfx_Shape_Data *pd,
                               double x, double y,
                               double ctrl_x0, double ctrl_y0,
                               double ctrl_x1, double ctrl_y1)
{
   double *offset_point;

   if (!efl_gfx_path_grow(EFL_GFX_PATH_COMMAND_TYPE_CUBIC_TO,
                          pd, &offset_point))
     return ;

   offset_point[0] = x;
   offset_point[1] = y;
   offset_point[2] = ctrl_x0;
   offset_point[3] = ctrl_y0;
   offset_point[4] = ctrl_x1;
   offset_point[5] = ctrl_y1;

   pd->current.x = x;
   pd->current.y = y;
   pd->current_ctrl.x = ctrl_x1;
   pd->current_ctrl.y = ctrl_y1;

   eo_do(obj,
         eo_event_callback_call(EFL_GFX_PATH_CHANGED, NULL),
         eo_event_callback_call(EFL_GFX_CHANGED, NULL));
}

static void
_efl_gfx_shape_append_scubic_to(Eo *obj, Efl_Gfx_Shape_Data *pd,
                                double x, double y,
                                double ctrl_x, double ctrl_y)
{
   double ctrl_x0, ctrl_y0;
   double current_x = 0, current_y = 0;
   double current_ctrl_x = 0, current_ctrl_y = 0;

   current_x = pd->current.x;
   current_y = pd->current.x;
   current_ctrl_x = pd->current_ctrl.x;
   current_ctrl_y = pd->current_ctrl.y;

   ctrl_x0 = 2 * current_x - current_ctrl_x;
   ctrl_y0 = 2 * current_y - current_ctrl_y;

   _efl_gfx_shape_append_cubic_to(obj, pd, x, y,
                                  ctrl_x0, ctrl_y0, ctrl_x, ctrl_y);
}

static void
_efl_gfx_shape_append_quadratic_to(Eo *obj, Efl_Gfx_Shape_Data *pd,
                                   double x, double y,
                                   double ctrl_x, double ctrl_y)
{
   double current_x = 0, current_y = 0;
   double ctrl_x0, ctrl_y0, ctrl_x1, ctrl_y1;

   current_x = pd->current.x;
   current_y = pd->current.y;

   // Convert quadratic bezier to cubic
   ctrl_x0 = (current_x + 2 * ctrl_x) * (1.0 / 3.0);
   ctrl_y0 = (current_y + 2 * ctrl_y) * (1.0 / 3.0);
   ctrl_x1 = (x + 2 * ctrl_x) * (1.0 / 3.0);
   ctrl_y1 = (y + 2 * ctrl_y) * (1.0 / 3.0);

   _efl_gfx_shape_append_cubic_to(obj, pd, x, y,
                                  ctrl_x0, ctrl_y0, ctrl_x1, ctrl_y1);
}

static void
_efl_gfx_shape_append_squadratic_to(Eo *obj, Efl_Gfx_Shape_Data *pd,
                                    double x, double y)
{
   double xc, yc; /* quadratic control point */
   double ctrl_x0, ctrl_y0, ctrl_x1, ctrl_y1;
   double current_x = 0, current_y = 0;
   double current_ctrl_x = 0, current_ctrl_y = 0;

   current_x = pd->current.x;
   current_y = pd->current.x;
   current_ctrl_x = pd->current_ctrl.x;
   current_ctrl_y = pd->current_ctrl.y;

   xc = 2 * current_x - current_ctrl_x;
   yc = 2 * current_y - current_ctrl_y;
   /* generate a quadratic bezier with control point = xc, yc */
   ctrl_x0 = (current_x + 2 * xc) * (1.0 / 3.0);
   ctrl_y0 = (current_y + 2 * yc) * (1.0 / 3.0);
   ctrl_x1 = (x + 2 * xc) * (1.0 / 3.0);
   ctrl_y1 = (y + 2 * yc) * (1.0 / 3.0);

   _efl_gfx_shape_append_cubic_to(obj, pd, x, y,
                                  ctrl_x0, ctrl_y0,
                                  ctrl_x1, ctrl_y1);
}

/*
 * code adapted from enesim which was adapted from moonlight sources
 */
static void
_efl_gfx_shape_append_arc_to(Eo *obj, Efl_Gfx_Shape_Data *pd,
                             double x, double y,
                             double rx, double ry,
                             double angle,
                             Eina_Bool large_arc, Eina_Bool sweep)
{
   double cxp, cyp, cx, cy;
   double sx, sy;
   double cos_phi, sin_phi;
   double dx2, dy2;
   double x1p, y1p;
   double x1p2, y1p2;
   double rx2, ry2;
   double lambda;
   double c;
   double at;
   double theta1, delta_theta;
   double nat;
   double delta, bcp;
   double cos_phi_rx, cos_phi_ry;
   double sin_phi_rx, sin_phi_ry;
   double cos_theta1, sin_theta1;
   int segments, i;

   // some helpful stuff is available here:
   // http://www.w3.org/TR/SVG/implnote.html#ArcImplementationNotes
   sx = pd->current.x;
   sy = pd->current.y;

   // if start and end points are identical, then no arc is drawn
   if ((fabs(x - sx) < (1 / 256.0)) && (fabs(y - sy) < (1 / 256.0)))
     return;

   // Correction of out-of-range radii, see F6.6.1 (step 2)
   rx = fabs(rx);
   ry = fabs(ry);
   if ((rx < 0.5) || (ry < 0.5))
     {
        _efl_gfx_shape_append_line_to(obj, pd, x, y);
        return;
     }

   angle = angle * M_PI / 180.0;
   cos_phi = cos(angle);
   sin_phi = sin(angle);
   dx2 = (sx - x) / 2.0;
   dy2 = (sy - y) / 2.0;
   x1p = cos_phi * dx2 + sin_phi * dy2;
   y1p = cos_phi * dy2 - sin_phi * dx2;
   x1p2 = x1p * x1p;
   y1p2 = y1p * y1p;
   rx2 = rx * rx;
   ry2 = ry * ry;
   lambda = (x1p2 / rx2) + (y1p2 / ry2);

   // Correction of out-of-range radii, see F6.6.2 (step 4)
   if (lambda > 1.0)
     {
        // see F6.6.3
        double lambda_root = sqrt(lambda);

        rx *= lambda_root;
        ry *= lambda_root;
        // update rx2 and ry2
        rx2 = rx * rx;
        ry2 = ry * ry;
     }

   c = (rx2 * ry2) - (rx2 * y1p2) - (ry2 * x1p2);

   // check if there is no possible solution
   // (i.e. we can't do a square root of a negative value)
   if (c < 0.0)
     {
        // scale uniformly until we have a single solution
        // (see F6.2) i.e. when c == 0.0
        double scale = sqrt(1.0 - c / (rx2 * ry2));
        rx *= scale;
        ry *= scale;
        // update rx2 and ry2
        rx2 = rx * rx;
        ry2 = ry * ry;

        // step 2 (F6.5.2) - simplified since c == 0.0
        cxp = 0.0;
        cyp = 0.0;
        // step 3 (F6.5.3 first part) - simplified since cxp and cyp == 0.0
        cx = 0.0;
        cy = 0.0;
     }
   else
     {
        // complete c calculation
        c = sqrt(c / ((rx2 * y1p2) + (ry2 * x1p2)));
        // inverse sign if Fa == Fs
        if (large_arc == sweep)
          c = -c;

        // step 2 (F6.5.2)
        cxp = c * ( rx * y1p / ry);
        cyp = c * (-ry * x1p / rx);

        // step 3 (F6.5.3 first part)
        cx = cos_phi * cxp - sin_phi * cyp;
        cy = sin_phi * cxp + cos_phi * cyp;
     }

   // step 3 (F6.5.3 second part) we now have the center point of the ellipse
   cx += (sx + x) / 2.0;
   cy += (sy + y) / 2.0;

   // step 4 (F6.5.4)
   // we dont' use arccos (as per w3c doc),
   // see http://www.euclideanspace.com/maths/algebra/vectors/angleBetween/index.htm
   // note: atan2 (0.0, 1.0) == 0.0
   at = atan2(((y1p - cyp) / ry), ((x1p - cxp) / rx));
   theta1 = (at < 0.0) ? 2.0 * M_PI + at : at;

   nat = atan2(((-y1p - cyp) / ry), ((-x1p - cxp) / rx));
   delta_theta = (nat < at) ? 2.0 * M_PI - at + nat : nat - at;

   if (sweep)
     {
        // ensure delta theta < 0 or else add 360 degrees
        if (delta_theta < 0.0)
          delta_theta += 2.0 * M_PI;
     }
   else
     {
        // ensure delta theta > 0 or else substract 360 degrees
        if (delta_theta > 0.0)
          delta_theta -= 2.0 * M_PI;
     }

   // add several cubic bezier to approximate the arc
   // (smaller than 90 degrees)
   // we add one extra segment because we want something
   // smaller than 90deg (i.e. not 90 itself)
   segments = (int) (fabs(delta_theta / M_PI_2)) + 1;
   delta = delta_theta / segments;

   // http://www.stillhq.com/ctpfaq/2001/comp.text.pdf-faq-2001-04.txt (section 2.13)
   bcp = 4.0 / 3 * (1 - cos(delta / 2)) / sin(delta / 2);

   cos_phi_rx = cos_phi * rx;
   cos_phi_ry = cos_phi * ry;
   sin_phi_rx = sin_phi * rx;
   sin_phi_ry = sin_phi * ry;

   cos_theta1 = cos(theta1);
   sin_theta1 = sin(theta1);

   for (i = 0; i < segments; ++i)
     {
        // end angle (for this segment) = current + delta
        double c1x, c1y, ex, ey, c2x, c2y;
        double theta2 = theta1 + delta;
        double cos_theta2 = cos(theta2);
        double sin_theta2 = sin(theta2);

        // first control point (based on start point sx,sy)
        c1x = sx - bcp * (cos_phi_rx * sin_theta1 + sin_phi_ry * cos_theta1);
        c1y = sy + bcp * (cos_phi_ry * cos_theta1 - sin_phi_rx * sin_theta1);

        // end point (for this segment)
        ex = cx + (cos_phi_rx * cos_theta2 - sin_phi_ry * sin_theta2);
        ey = cy + (sin_phi_rx * cos_theta2 + cos_phi_ry * sin_theta2);

        // second control point (based on end point ex,ey)
        c2x = ex + bcp * (cos_phi_rx * sin_theta2 + sin_phi_ry * cos_theta2);
        c2y = ey + bcp * (sin_phi_rx * sin_theta2 - cos_phi_ry * cos_theta2);

        _efl_gfx_shape_append_cubic_to(obj, pd, ex, ey, c1x, c1y, c2x, c2y);

        // next start point is the current end point (same for angle)
        sx = ex;
        sy = ey;
        theta1 = theta2;
        // avoid recomputations
        cos_theta1 = cos_theta2;
        sin_theta1 = sin_theta2;
     }
}

// append arc implementation
typedef struct _Bezier
{
   double x1, y1, x2, y2, x3, y3, x4, y4;
}Bezier;

typedef struct _Point
{
   double x;
   double y;
}Point;

static
Bezier bezier_from_points(Point p1, Point p2,
                          Point p3, Point p4)
{
   Bezier b;
   b.x1 = p1.x;
   b.y1 = p1.y;
   b.x2 = p2.x;
   b.y2 = p2.y;
   b.x3 = p3.x;
   b.y3 = p3.y;
   b.x4 = p4.x;
   b.y4 = p4.y;
   return b;
}

inline static void
parameter_split_left(Bezier *b, double t, Bezier *left)
{
   left->x1 = b->x1;
   left->y1 = b->y1;

   left->x2 = b->x1 + t * ( b->x2 - b->x1 );
   left->y2 = b->y1 + t * ( b->y2 - b->y1 );

   left->x3 = b->x2 + t * ( b->x3 - b->x2 ); // temporary holding spot
   left->y3 = b->y2 + t * ( b->y3 - b->y2 ); // temporary holding spot

   b->x3 = b->x3 + t * ( b->x4 - b->x3 );
   b->y3 = b->y3 + t * ( b->y4 - b->y3 );

   b->x2 = left->x3 + t * ( b->x3 - left->x3);
   b->y2 = left->y3 + t * ( b->y3 - left->y3);

   left->x3 = left->x2 + t * ( left->x3 - left->x2 );
   left->y3 = left->y2 + t * ( left->y3 - left->y2 );

   left->x4 = b->x1 = left->x3 + t * (b->x2 - left->x3);
   left->y4 = b->y1 = left->y3 + t * (b->y2 - left->y3);
}

static
Bezier bezier_on_interval(Bezier *b, double t0, double t1)
{
   if (t0 == 0 && t1 == 1)
     return *b;

   Bezier result;
   parameter_split_left(b, t0, &result);
   double trueT = (t1-t0)/(1-t0);
   parameter_split_left(b, trueT, &result);

   return result;
}

inline static void
_efl_gfx_bezier_coefficients(double t, double *ap, double *bp, double *cp, double *dp)
{
   double a,b,c,d;
   double m_t = 1. - t;
   b = m_t * m_t;
   c = t * t;
   d = c * t;
   a = b * m_t;
   b *= 3. * t;
   c *= 3. * m_t;
   *ap = a;
   *bp = b;
   *cp = c;
   *dp = d;
}

#define PATH_KAPPA 0.5522847498
static double
_efl_gfx_t_for_arc_angle(double angle)
{
   if (angle < 0.00001)
     return 0;

   if (angle == 90.0)
     return 1;

   double radians = M_PI * angle / 180;
   double cosAngle = cos(radians);
   double sinAngle = sin(radians);

   // initial guess
   double tc = angle / 90;
   // do some iterations of newton's method to approximate cosAngle
   // finds the zero of the function b.pointAt(tc).x() - cosAngle
   tc -= ((((2-3*PATH_KAPPA) * tc + 3*(PATH_KAPPA-1)) * tc) * tc + 1 - cosAngle) // value
   / (((6-9*PATH_KAPPA) * tc + 6*(PATH_KAPPA-1)) * tc); // derivative
   tc -= ((((2-3*PATH_KAPPA) * tc + 3*(PATH_KAPPA-1)) * tc) * tc + 1 - cosAngle) // value
   / (((6-9*PATH_KAPPA) * tc + 6*(PATH_KAPPA-1)) * tc); // derivative

   // initial guess
   double ts = tc;
   // do some iterations of newton's method to approximate sinAngle
   // finds the zero of the function b.pointAt(tc).y() - sinAngle
   ts -= ((((3*PATH_KAPPA-2) * ts -  6*PATH_KAPPA + 3) * ts + 3*PATH_KAPPA) * ts - sinAngle)
   / (((9*PATH_KAPPA-6) * ts + 12*PATH_KAPPA - 6) * ts + 3*PATH_KAPPA);
   ts -= ((((3*PATH_KAPPA-2) * ts -  6*PATH_KAPPA + 3) * ts + 3*PATH_KAPPA) * ts - sinAngle)
   / (((9*PATH_KAPPA-6) * ts + 12*PATH_KAPPA - 6) * ts + 3*PATH_KAPPA);

   // use the average of the t that best approximates cosAngle
   // and the t that best approximates sinAngle
   double t = 0.5 * (tc + ts);
   return t;
}

static void
_find_ellipse_coords(double x, double y, double w, double h, double angle, double length,
                     Point* start_point, Point *end_point)
{
   if (!w || !h )
     {
        if (start_point)
          start_point->x = 0 , start_point->y = 0;
        if (end_point)
          end_point->x = 0 , end_point->y = 0;
        return;
     }

   double w2 = w / 2;
   double h2 = h / 2;

   double angles[2] = { angle, angle + length };
   Point *points[2] = { start_point, end_point };
   int i =0;
   for (i = 0; i < 2; ++i)
     {
        if (!points[i])
          continue;

        double theta = angles[i] - 360 * floor(angles[i] / 360);
        double t = theta / 90;
        // truncate
        int quadrant = (int)t;
        t -= quadrant;

        t = _efl_gfx_t_for_arc_angle(90 * t);

        // swap x and y?
        if (quadrant & 1)
          t = 1 - t;

        double a, b, c, d;
        _efl_gfx_bezier_coefficients(t, &a, &b, &c, &d);
        double px = a + b + c*PATH_KAPPA;
        double py = d + c + b*PATH_KAPPA;

        // left quadrants
        if (quadrant == 1 || quadrant == 2)
          px = -px;

        // top quadrants
        if (quadrant == 0 || quadrant == 1)
          py = -py;
        double cx = x+w/2;
        double cy = y+h/2;
        points[i]->x = cx + w2 * px;
        points[i]->y = cy + h2 * py;
     }
}

//// The return value is the starting point of the arc
static
Point _curves_for_arc(double x, double y, double w, double h,
                      double start_angle, double sweep_length,
                      Point *curves, int *point_count)
{
   *point_count = 0;
   double w2 = w / 2;
   double w2k = w2 * PATH_KAPPA;

   double h2 = h / 2;
   double h2k = h2 * PATH_KAPPA;

   Point points[16] =
   {
   // start point
   { x + w, y + h2 },

   // 0 -> 270 degrees
   { x + w, y + h2 + h2k },
   { x + w2 + w2k, y + h },
   { x + w2, y + h },

   // 270 -> 180 degrees
   { x + w2 - w2k, y + h },
   { x, y + h2 + h2k },
   { x, y + h2 },

   // 180 -> 90 degrees
   { x, y + h2 - h2k },
   { x + w2 - w2k, y },
   { x + w2, y },

   // 90 -> 0 degrees
   { x + w2 + w2k, y },
   { x + w, y + h2 - h2k },
   { x + w, y + h2 }
   };

   if (sweep_length > 360) sweep_length = 360;
   else if (sweep_length < -360) sweep_length = -360;

   // Special case fast paths
   if (start_angle == 0)
     {
        if (sweep_length == 360)
          {
             int i;
             for (i = 11; i >= 0; --i)
               curves[(*point_count)++] = points[i];
             return points[12];
          }
        else if (sweep_length == -360)
          {
             int i ;
             for (i = 1; i <= 12; ++i)
               curves[(*point_count)++] = points[i];
             return points[0];
          }
     }

   int start_segment = (int)(floor(start_angle / 90));
   int end_segment = (int)(floor((start_angle + sweep_length) / 90));

   double start_t = (start_angle - start_segment * 90) / 90;
   double end_t = (start_angle + sweep_length - end_segment * 90) / 90;

   int delta = sweep_length > 0 ? 1 : -1;
   if (delta < 0)
     {
        start_t = 1 - start_t;
        end_t = 1 - end_t;
     }

   // avoid empty start segment
   if (start_t == 1.0)
     {
        start_t = 0;
        start_segment += delta;
     }

   // avoid empty end segment
   if (end_t == 0)
     {
        end_t = 1;
        end_segment -= delta;
     }

   start_t = _efl_gfx_t_for_arc_angle(start_t * 90);
   end_t = _efl_gfx_t_for_arc_angle(end_t * 90);

   Eina_Bool split_at_start = !(fabs(start_t) <= 0.00001f);
   Eina_Bool split_at_end = !(fabs(end_t - 1.0) <= 0.00001f);

   const int end = end_segment + delta;

   // empty arc?
   if (start_segment == end)
     {
        const int quadrant = 3 - ((start_segment % 4) + 4) % 4;
        const int j = 3 * quadrant;
        return delta > 0 ? points[j + 3] : points[j];
     }

   Point start_point, end_point;
   _find_ellipse_coords(x, y, w, h, start_angle, sweep_length, &start_point, &end_point);
   int i;
   for (i = start_segment; i != end; i += delta)
     {
        const int quadrant = 3 - ((i % 4) + 4) % 4;
        const int j = 3 * quadrant;

        Bezier b;
        if (delta > 0)
            b = bezier_from_points(points[j + 3], points[j + 2], points[j + 1], points[j]);
        else
            b = bezier_from_points(points[j], points[j + 1], points[j + 2], points[j + 3]);

        // empty arc?
        if (start_segment == end_segment && (start_t == end_t))
            return start_point;

        if (i == start_segment)
          {
             if (i == end_segment && split_at_end)
               b = bezier_on_interval(&b, start_t, end_t);
            else if (split_at_start)
              b = bezier_on_interval(&b, start_t, 1);
          }
        else if (i == end_segment && split_at_end)
          {
             b = bezier_on_interval(&b, 0, end_t);
          }

        // push control points
        curves[(*point_count)].x = b.x2;
        curves[(*point_count)++].y = b.y2;
        curves[(*point_count)].x = b.x3;
        curves[(*point_count)++].y = b.y3;
        curves[(*point_count)].x = b.x4;
        curves[(*point_count)++].y = b.y4;
     }

   curves[*(point_count)-1] = end_point;

   return start_point;
}

static void
_efl_gfx_shape_append_arc(Eo *obj, Efl_Gfx_Shape_Data *pd,
                          double x, double y, double w, double h,
                          double start_angle, double sweep_length)
{
   int point_count;
   Point pts[15];

   Point curve_start = _curves_for_arc(x, y, w, h, start_angle, sweep_length, pts, &point_count);
   int i;
   if (pd->commands_count)
     _efl_gfx_shape_append_line_to(obj, pd, curve_start.x, curve_start.y);
   else
     _efl_gfx_shape_append_move_to(obj, pd, curve_start.x, curve_start.y);
   for (i = 0; i < point_count; i += 3)
     {
        _efl_gfx_shape_append_cubic_to(obj, pd,
                                       pts[i+2].x, pts[i+2].y,
                                       pts[i].x, pts[i].y,
                                       pts[i+1].x, pts[i+1].y);
     }
}

static void
_efl_gfx_shape_append_close(Eo *obj, Efl_Gfx_Shape_Data *pd)
{
   double *offset_point;

   efl_gfx_path_grow(EFL_GFX_PATH_COMMAND_TYPE_CLOSE,
                     pd, &offset_point);

   eo_do(obj,
         eo_event_callback_call(EFL_GFX_PATH_CHANGED, NULL),
         eo_event_callback_call(EFL_GFX_CHANGED, NULL));
}

static void
_efl_gfx_shape_append_circle(Eo *obj, Efl_Gfx_Shape_Data *pd,
                             double xc, double yc, double radius)
{
   _efl_gfx_shape_append_arc(obj, pd, xc - radius, yc - radius, 2*radius, 2*radius, 0, 360);
}

static void
_efl_gfx_shape_append_rect(Eo *obj, Efl_Gfx_Shape_Data *pd,
                           double x, double y, double w, double h,
                           double rx, double ry)
{
   if (!rx || !ry)
     {
         _efl_gfx_shape_append_move_to(obj, pd, x, y);
         _efl_gfx_shape_append_line_to(obj, pd, x + w, y);
         _efl_gfx_shape_append_line_to(obj, pd, x + w, y + h);
         _efl_gfx_shape_append_line_to(obj, pd, x, y + h);
         _efl_gfx_shape_append_close(obj, pd);

         return;
     }
   // clamp the rx and ry radius value.
   rx = 2*rx;
   ry = 2*ry;
   if (rx > w) rx = w;
   if (ry > h) ry = h;

   _efl_gfx_shape_append_move_to(obj, pd, x, y + h/2);
   _efl_gfx_shape_append_arc(obj, pd, x, y, rx, ry, 180, -90);
   _efl_gfx_shape_append_arc(obj, pd, x + w - rx, y, rx, ry, 90, -90);
   _efl_gfx_shape_append_arc(obj, pd, x + w - rx, y + h - ry, rx, ry, 0, -90);
   _efl_gfx_shape_append_arc(obj, pd, x, y + h - ry, rx, ry, 270, -90);
   _efl_gfx_shape_append_close(obj, pd);
}

static void
_efl_gfx_path_append_horizontal_to(Eo *obj, Efl_Gfx_Shape_Data *pd,
                                   double d, double current_x EINA_UNUSED, double current_y)
{
   _efl_gfx_shape_append_line_to(obj, pd, d, current_y);
}

static void
_efl_gfx_path_append_vertical_to(Eo *obj, Efl_Gfx_Shape_Data *pd,
                                 double d, double current_x, double current_y EINA_UNUSED)
{
   _efl_gfx_shape_append_line_to(obj, pd, current_x, d);
}

static char *
_skipcomma(const char *content)
{
   while (*content && isspace(*content)) content++;
   if (*content == ',') return (char*) content + 1;
   return (char*) content;
}

static inline Eina_Bool
_next_isnumber(const char *content)
{
   char *tmp = NULL;

   (void) strtod(content, &tmp);
   return content != tmp;
}

static inline Eina_Bool
_parse_number(char **content, double *number)
{
   char *end = NULL;
   *number = strtod(*content, &end);
   // if the start of string is not number 
   if ((*content) == end) return EINA_FALSE;
   //skip comma if any
   *content = _skipcomma(end);
   return EINA_TRUE;
}

static Eina_Bool
_efl_gfx_path_parse_pair(const char *content, char **end, double *x, double *y)
{
   char *str = (char *) content;

   if (_parse_number(&str, x))
     if (_parse_number(&str, y))
       {
          *end = str;
          return EINA_TRUE;
       }
   return EINA_FALSE;
}

static Eina_Bool
_efl_gfx_path_parse_pair_to(const char *content, char **end,
                            Eo *obj, Efl_Gfx_Shape_Data *pd,
                            double *current_x, double *current_y,
                            void (*func)(Eo *obj, Efl_Gfx_Shape_Data *pd, double x, double y),
                            Eina_Bool rel)
{
   double x, y;

   *end = (char*) content;
   do
     {
        Eina_Bool r;

        r = _efl_gfx_path_parse_pair(content, end, &x, &y);
        if (!r) return EINA_FALSE;

        if (rel)
          {
             x += *current_x;
             y += *current_y;
          }

        func(obj, pd, x, y);
        content = *end;

        *current_x = x;
        *current_y = y;
     }
   while (_next_isnumber(content));

   return EINA_TRUE;
}

static Eina_Bool
_efl_gfx_path_parse_double_to(const char *content, char **end,
                              Eo *obj, Efl_Gfx_Shape_Data *pd,
                              double *current, double current_x, double current_y,
                              void (*func)(Eo *obj, Efl_Gfx_Shape_Data *pd, double d, double current_x, double current_y),
                              Eina_Bool rel)
{
   double d;
   Eina_Bool first = EINA_FALSE;

   *end = (char*) content;
   do
     {
        d = strtod(content, end);
        if (content == *end)
          return first;
        first = EINA_TRUE;

        if (rel)
          {
             d += *current;
          }

        func(obj, pd, d, current_x, current_y);
        content = *end;

        *current = d;
     }
   while (1); // This is an optimisation as we have only one parameter.

   return EINA_TRUE;
}

static Eina_Bool
_efl_gfx_path_parse_six(const char *content, char **end,
                        double *x, double *y,
                        double *ctrl_x0, double *ctrl_y0,
                        double *ctrl_x1, double *ctrl_y1)
{
   char *str = (char *) content;
   if (_parse_number(&str, ctrl_x0))
     if (_parse_number(&str, ctrl_y0))
       if (_parse_number(&str, ctrl_x1))
         if (_parse_number(&str, ctrl_y1))
           if (_parse_number(&str, x))
             if (_parse_number(&str, y))
               {
                  *end = str;
                  return EINA_TRUE;
               }
   return EINA_FALSE;
}

static Eina_Bool
_efl_gfx_path_parse_six_to(const char *content, char **end,
                           Eo *obj, Efl_Gfx_Shape_Data *pd,
                           double *current_x, double *current_y,
                           void (*func)(Eo *obj, Efl_Gfx_Shape_Data *pd, double x, double y, double ctrl_x0, double ctrl_y0, double ctrl_x1, double ctrl_y1),
                           Eina_Bool rel)
{
   double x, y, ctrl_x0, ctrl_y0, ctrl_x1, ctrl_y1;

   *end = (char*) content;
   do
     {
        Eina_Bool r;

        r = _efl_gfx_path_parse_six(content, end,
                                    &x, &y,
                                    &ctrl_x0, &ctrl_y0,
                                    &ctrl_x1, &ctrl_y1);
        if (!r) return EINA_FALSE;

        if (rel)
          {
             x += *current_x;
             y += *current_y;
             ctrl_x0 += *current_x;
             ctrl_y0 += *current_y;
             ctrl_x1 += *current_x;
             ctrl_y1 += *current_y;
          }
        func(obj, pd, x, y, ctrl_x0, ctrl_y0, ctrl_x1, ctrl_y1);
        content = *end;

        *current_x = x;
        *current_y = y;
     }
   while (_next_isnumber(content));

   return EINA_TRUE;
}

static Eina_Bool
_efl_gfx_path_parse_quad(const char *content, char **end,
                         double *x, double *y,
                         double *ctrl_x0, double *ctrl_y0)
{
   char *str = (char *) content;

   if (_parse_number(&str, ctrl_x0))
     if (_parse_number(&str, ctrl_y0))
       if (_parse_number(&str, x))
         if (_parse_number(&str, y))
           {
              *end = str;
              return EINA_TRUE;
           }
   return EINA_FALSE;
}

static Eina_Bool
_efl_gfx_path_parse_quad_to(const char *content, char **end,
                            Eo *obj, Efl_Gfx_Shape_Data *pd,
                            double *current_x, double *current_y,
                            void (*func)(Eo *obj, Efl_Gfx_Shape_Data *pd,
                                         double x, double y, double ctrl_x0, double ctrl_y0),
                            Eina_Bool rel)
{
   double x, y, ctrl_x0, ctrl_y0;

   *end = (char*) content;
   do
     {
        Eina_Bool r;

        r = _efl_gfx_path_parse_quad(content, end,
                                     &x, &y,
                                     &ctrl_x0, &ctrl_y0);
        if (!r) return EINA_FALSE;

        if (rel)
          {
             x += *current_x;
             y += *current_y;
          }

        func(obj, pd, x, y, ctrl_x0, ctrl_y0);
        content = *end;

        *current_x = x;
        *current_y = y;
     }
   while (_next_isnumber(content));

   return EINA_TRUE;
}

static Eina_Bool
_efl_gfx_path_parse_arc(const char *content, char **end,
                        double *x, double *y,
                        double *rx, double *ry,
                        double *radius,
                        Eina_Bool *large_arc, Eina_Bool *sweep)
{
   char *str = (char *) content;
   char *end1 = NULL;
   char *end2 = NULL;

   if (_parse_number(&str, rx))
     if (_parse_number(&str, ry))
       if (_parse_number(&str, radius))
         {
            // large_arc
            *large_arc = strtol(str, &end1, 10) ? EINA_TRUE : EINA_FALSE;
            if (!end1 || (str == end1)) return EINA_FALSE;
            end1 = _skipcomma(end1);

            // sweeo
            *sweep = strtol(end1, &end2, 10) ? EINA_TRUE : EINA_FALSE;
            if (!end2 || (end1 == end2)) return EINA_FALSE;
            str = _skipcomma(end2);

            if (_parse_number(&str, x))
              if (_parse_number(&str, y))
                {
                   *end = str;
                   return EINA_TRUE;
                }
         }
   return EINA_FALSE;
}

static Eina_Bool
_efl_gfx_path_parse_arc_to(const char *content, char **end,
                           Eo *obj, Efl_Gfx_Shape_Data *pd,
                           double *current_x, double *current_y,
                           void (*func)(Eo *obj, Efl_Gfx_Shape_Data *pd,
                                        double x, double y, double rx, double ry, double angle,
                                        Eina_Bool large_arc, Eina_Bool sweep),
                           Eina_Bool rel)
{
   double x, y, rx, ry, angle;
   Eina_Bool large_arc, sweep; // FIXME: handle those flag

   *end = (char*) content;
   do
     {
        Eina_Bool r;

        r = _efl_gfx_path_parse_arc(content, end,
                                    &x, &y,
                                    &rx, &ry,
                                    &angle,
                                    &large_arc, &sweep);
        if (!r) return EINA_FALSE;

        if (rel)
          {
             x += *current_x;
             y += *current_y;
          }

        func(obj, pd, x, y, rx, ry, angle, large_arc, sweep);
        content = *end;

        *current_x = x;
        *current_y = y;
     }
   while (_next_isnumber(content));

   return EINA_TRUE;
}

static void
_efl_gfx_shape_append_svg_path(Eo *obj, Efl_Gfx_Shape_Data *pd,
                               const char *svg_path_data)
{
   double current_x = 0, current_y = 0;
   char *content = (char*) svg_path_data;
   char *cur_locale;
   if (!content) return ;

// TIZEN_ONLY(20160420): efl/interface: update locale before parsing svg path
   cur_locale = setlocale(LC_NUMERIC, NULL);
   if (cur_locale)
     cur_locale = strdup(cur_locale);
   setlocale(LC_NUMERIC, "POSIX");
//

   while (content[0] != '\0')
     {
        while (isspace(content[0])) content++;

        switch (content[0])
          {
           case 'M':
              if (!_efl_gfx_path_parse_pair_to(&content[1],
                                               &content,
                                               obj, pd,
                                               &current_x, &current_y,
                                               _efl_gfx_shape_append_move_to,
                                               EINA_FALSE))
                goto error;
              break;
           case 'm':
              if (!_efl_gfx_path_parse_pair_to(&content[1],
                                               &content,
                                               obj, pd,
                                               &current_x, &current_y,
                                               _efl_gfx_shape_append_move_to,
                                               EINA_TRUE))
                goto error;
              break;
           case 'z':
           case 'Z':
              _efl_gfx_shape_append_close(obj, pd);
              content++;
              break;
           case 'L':
              if (!_efl_gfx_path_parse_pair_to(&content[1],
                                               &content,
                                               obj, pd,
                                               &current_x, &current_y,
                                               _efl_gfx_shape_append_line_to,
                                               EINA_FALSE))
                goto error;
              break;
           case 'l':
              if (!_efl_gfx_path_parse_pair_to(&content[1],
                                               &content,
                                               obj, pd,
                                               &current_x, &current_y,
                                               _efl_gfx_shape_append_line_to,
                                               EINA_TRUE))
                goto error;
              break;
           case 'H':
              if (!_efl_gfx_path_parse_double_to(&content[1],
                                                 &content,
                                                 obj, pd,
                                                 &current_x, current_x, current_y,
                                                 _efl_gfx_path_append_horizontal_to,
                                                 EINA_FALSE))
                goto error;
              break;
           case 'h':
              if (!_efl_gfx_path_parse_double_to(&content[1],
                                                 &content,
                                                 obj, pd,
                                                 &current_x, current_x, current_y,
                                                 _efl_gfx_path_append_horizontal_to,
                                                 EINA_TRUE))
                goto error;
              break;
           case 'V':
              if (!_efl_gfx_path_parse_double_to(&content[1],
                                                 &content,
                                                 obj, pd,
                                                 &current_y, current_x, current_y,
                                                 _efl_gfx_path_append_vertical_to,
                                                 EINA_FALSE))
                goto error;
              break;
           case 'v':
              if (!_efl_gfx_path_parse_double_to(&content[1],
                                                 &content,
                                                 obj, pd,
                                                 &current_y, current_x, current_y,
                                                 _efl_gfx_path_append_vertical_to,
                                                 EINA_TRUE))
                goto error;
              break;
           case 'C':
              if (!_efl_gfx_path_parse_six_to(&content[1],
                                              &content,
                                              obj, pd,
                                              &current_x, &current_y,
                                              _efl_gfx_shape_append_cubic_to,
                                              EINA_FALSE))
                goto error;
              break;
           case 'c':
              if (!_efl_gfx_path_parse_six_to(&content[1],
                                              &content,
                                              obj, pd,
                                              &current_x, &current_y,
                                              _efl_gfx_shape_append_cubic_to,
                                              EINA_TRUE))
                goto error;
              break;
           case 'S':
              if (!_efl_gfx_path_parse_quad_to(&content[1],
                                               &content,
                                               obj, pd,
                                               &current_x, &current_y,
                                               _efl_gfx_shape_append_scubic_to,
                                               EINA_FALSE))
                goto error;
              break;
           case 's':
              if (!_efl_gfx_path_parse_quad_to(&content[1],
                                               &content,
                                               obj, pd,
                                               &current_x, &current_y,
                                               _efl_gfx_shape_append_scubic_to,
                                               EINA_TRUE))
                goto error;
              break;
           case 'Q':
              if (!_efl_gfx_path_parse_quad_to(&content[1],
                                               &content,
                                               obj, pd,
                                               &current_x, &current_y,
                                               _efl_gfx_shape_append_quadratic_to,
                                               EINA_FALSE))
                goto error;
              break;
           case 'q':
              if (!_efl_gfx_path_parse_quad_to(&content[1],
                                               &content,
                                               obj, pd,
                                               &current_x, &current_y,
                                               _efl_gfx_shape_append_quadratic_to,
                                               EINA_TRUE))
                goto error;
              break;
           case 'T':
              if (!_efl_gfx_path_parse_pair_to(&content[1],
                                               &content,
                                               obj, pd,
                                               &current_x, &current_y,
                                               _efl_gfx_shape_append_squadratic_to,
                                               EINA_FALSE))
                goto error;
              break;
           case 't':
              if (!_efl_gfx_path_parse_pair_to(&content[1],
                                               &content,
                                               obj, pd,
                                               &current_x, &current_y,
                                               _efl_gfx_shape_append_squadratic_to,
                                               EINA_TRUE))
                goto error;
              break;
           case 'A':
              if (!_efl_gfx_path_parse_arc_to(&content[1],
                                              &content,
                                              obj, pd,
                                              &current_x, &current_y,
                                              _efl_gfx_shape_append_arc_to,
                                              EINA_FALSE))
                goto error;
              break;
           case 'a':
              if (!_efl_gfx_path_parse_arc_to(&content[1],
                                              &content,
                                              obj, pd,
                                              &current_x, &current_y,
                                              _efl_gfx_shape_append_arc_to,
                                              EINA_TRUE))
                goto error;
              break;
           default:
              goto error;
          }
     }
// TIZEN_ONLY(20160420): efl/interface: update locale before parsing svg path
error:
   setlocale(LC_NUMERIC, cur_locale);
   if (cur_locale)
     free(cur_locale);
//
}

#include "interfaces/efl_gfx_shape.eo.c"
