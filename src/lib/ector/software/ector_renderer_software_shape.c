#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <math.h>
#include <float.h>

#include <Eina.h>
#include <Ector.h>
#include <software/Ector_Software.h>

#include "ector_private.h"
#include "ector_software_private.h"


typedef struct _Ector_Renderer_Software_Shape_Data Ector_Renderer_Software_Shape_Data;
struct _Ector_Renderer_Software_Shape_Data
{
   Ector_Software_Surface_Data         *surface;
   Ector_Renderer_Generic_Shape_Data   *shape;
   Ector_Renderer_Generic_Base_Data    *base;
   Shape_Rle_Data                      *shape_data;
   Shape_Rle_Data                      *outline_data;                    
};

typedef struct _Outline
{
   SW_FT_Outline ft_outline;
   int points_alloc;
   int contours_alloc;
}Outline;

static Outline * 
_outline_create()
{
  Outline *outline = (Outline *) calloc(1, sizeof(Outline));
  
  outline->ft_outline.points = (SW_FT_Vector *) calloc(50, sizeof(SW_FT_Vector));
  outline->ft_outline.tags = (char *) calloc(50, sizeof(char));

  outline->ft_outline.contours = (short *) calloc(5, sizeof(short));

  outline->points_alloc = 50;
  outline->contours_alloc = 5;  
  return outline;
} 

static 
void _outline_destroy(Outline *outline)
{
    if (outline)
    {
      free(outline->ft_outline.points);
      free(outline->ft_outline.tags);
      free(outline->ft_outline.contours);
      free(outline);
      outline = NULL;
    }

}

static void
_outline_move_to(Outline *outline, double x, double y)
{
  SW_FT_Outline *ft_outline = &outline->ft_outline;

  if (ft_outline->n_contours == outline->contours_alloc)
  {
    outline->contours_alloc += 5;
    ft_outline->contours = (short *) realloc(ft_outline->contours, outline->contours_alloc * sizeof(short));
  }
  ft_outline->points[ft_outline->n_points].x = x;
  ft_outline->points[ft_outline->n_points].y = y;
  ft_outline->tags[ft_outline->n_points] = SW_FT_CURVE_TAG_ON;

  if (ft_outline->n_points)
  {
     ft_outline->contours[ft_outline->n_contours] = ft_outline->n_points - 1;
     ft_outline->n_contours++;
  }

  ft_outline->n_points++;
}

static void
_outline_end(Outline *outline)
{
    SW_FT_Outline *ft_outline = &outline->ft_outline;
    if (ft_outline->n_contours == outline->contours_alloc)
    {
      outline->contours_alloc += 1;
      ft_outline->contours = (short *) realloc(ft_outline->contours, outline->contours_alloc * sizeof(short));
    }

    if (ft_outline->n_points)
    {
       ft_outline->contours[ft_outline->n_contours] = ft_outline->n_points - 1;
       ft_outline->n_contours++;
    }
}


static void  _outline_line_to(Outline *outline, double x, double y)
{
  SW_FT_Outline *ft_outline = &outline->ft_outline;

  if (ft_outline->n_points == outline->points_alloc)
  {
    outline->points_alloc += 50;
    ft_outline->points = (SW_FT_Vector *) realloc(ft_outline->points, outline->points_alloc * sizeof(SW_FT_Vector));
    ft_outline->tags = (char *) realloc(ft_outline->tags, outline->points_alloc * sizeof(char));
  }
  ft_outline->points[ft_outline->n_points].x = x;
  ft_outline->points[ft_outline->n_points].y = y;
  ft_outline->tags[ft_outline->n_points] = SW_FT_CURVE_TAG_ON;
  ft_outline->n_points++;
}


static Eina_Bool
_outline_close_path(Outline *outline)
{
   SW_FT_Outline *ft_outline = &outline->ft_outline;
   int index ;
   
   if (ft_outline->n_contours)
   {
      index = ft_outline->contours[ft_outline->n_contours - 1] + 1;

   } else {
      // first path
      index = 0;
   }
   
   // make sure there is atleast one point in the current path
   if (ft_outline->n_points == index) return EINA_FALSE;
   
   _outline_line_to(outline, ft_outline->points[index].x, ft_outline->points[index].y);
   return EINA_TRUE;
}


static void  _outline_cubic_to(Outline *outline, double cx1, double cy1, double cx2, double cy2, double x, double y)
{
  SW_FT_Outline *ft_outline = &outline->ft_outline;

  if (ft_outline->n_points == outline->points_alloc)
  {
    outline->points_alloc += 50;
    ft_outline->points = (SW_FT_Vector *) realloc(ft_outline->points, outline->points_alloc * sizeof(SW_FT_Vector));
    ft_outline->tags = (char *) realloc(ft_outline->tags, outline->points_alloc * sizeof(char));
  }

  ft_outline->points[ft_outline->n_points].x = cx1;
  ft_outline->points[ft_outline->n_points].y = cy1;
  ft_outline->tags[ft_outline->n_points] = SW_FT_CURVE_TAG_CUBIC;
  ft_outline->n_points++;
  
  ft_outline->points[ft_outline->n_points].x = cx2;
  ft_outline->points[ft_outline->n_points].y = cy2;
  ft_outline->tags[ft_outline->n_points] = SW_FT_CURVE_TAG_CUBIC;
  ft_outline->n_points++;

  ft_outline->points[ft_outline->n_points].x = x;
  ft_outline->points[ft_outline->n_points].y = y;
  ft_outline->tags[ft_outline->n_points] = SW_FT_CURVE_TAG_ON;
  ft_outline->n_points++;
}

static void _outline_transform(Outline *outline, Eina_Matrix3 *m)
{
    SW_FT_Outline *ft_outline = &outline->ft_outline;
    if(m) {
        int i = 0;
        double x, y;
        for(i = 0; i < ft_outline->n_points ; i++) {
            eina_matrix3_point_transform(m, ft_outline->points[i].x, ft_outline->points[i].y, &x, &y);
            ft_outline->points[i].x = (lrint(x))<<6;// to freetype 26.6 coordinate.
            ft_outline->points[i].y = (lrint(y))<<6;
        }
    } else {
        int i = 0;
        for(i = 0; i < ft_outline->n_points ; i++) {
            ft_outline->points[i].x = ft_outline->points[i].x <<6;// to freetype 26.6 coordinate.
            ft_outline->points[i].y = ft_outline->points[i].y <<6;
        }
    }  
}


// This function come from librsvg rsvg-path.c
static void
_ector_arc_segment(Outline *outline,
                   double xc, double yc,
                   double th0, double th1, double rx, double ry,
                   double x_axis_rotation)
{
   double x1, y1, x2, y2, x3, y3;
   double t;
   double th_half;
   double f, sinf, cosf;

   f = x_axis_rotation * M_PI / 180.0;
   sinf = sin(f);
   cosf = cos(f);

   th_half = 0.5 * (th1 - th0);
   t = (8.0 / 3.0) * sin(th_half * 0.5) * sin(th_half * 0.5) / sin(th_half);
   x1 = rx * (cos(th0) - t * sin(th0));
   y1 = ry * (sin(th0) + t * cos(th0));
   x3 = rx* cos(th1);
   y3 = ry* sin(th1);
   x2 = x3 + rx * (t * sin(th1));
   y2 = y3 + ry * (-t * cos(th1));

   _outline_cubic_to(outline,
                     xc + cosf * x1 - sinf * y1,
                     yc + sinf * x1 + cosf * y1,
                     xc + cosf * x2 - sinf * y2,
                     yc + sinf * x2 + cosf * y2,
                     xc + cosf * x3 - sinf * y3,
                     yc + sinf * x3 + cosf * y3);
}

// This function come from librsvg rsvg-path.c
static void
_ector_arc_to(Outline *outline,
              double *current_x, double *current_y,
              double rx, double ry, double x_axis_rotation,
              Eina_Bool large_arc_flag, Eina_Bool sweep_flag,
              double x, double y)
{
   /* See Appendix F.6 Elliptical arc implementation notes
      http://www.w3.org/TR/SVG/implnote.html#ArcImplementationNotes */

   double f, sinf, cosf;
   double x1, y1, x2, y2;
   double x1_, y1_;
   double cx_, cy_, cx, cy;
   double gamma;
   double theta1, delta_theta;
   double k1, k2, k3, k4, k5;

   int i, n_segs;

   /* Start and end of path segment */
   x1 = *current_x;
   y1 = *current_y;

   x2 = x;
   y2 = y;

   if (x1 == x2 && y1 == y2)
     return;

   /* X-axis */
   f = x_axis_rotation * M_PI / 180.0;
   sinf = sin(f);
   cosf = cos(f);

   /* Check the radius against floading point underflow.
      See http://bugs.debian.org/508443 */
   if ((fabs(rx) < DBL_EPSILON) || (fabs(ry) < DBL_EPSILON)) {

      _outline_line_to(outline, x, y);

      *current_x = x;
      *current_y = y;
      return;
   }

   if (rx < 0) rx = -rx;
   if (ry < 0) ry = -ry;

   k1 = (x1 - x2) / 2;
   k2 = (y1 - y2) / 2;

   x1_ = cosf * k1 + sinf * k2;
   y1_ = -sinf * k1 + cosf * k2;

   gamma = (x1_ * x1_) / (rx * rx) + (y1_ * y1_) / (ry * ry);
   if (gamma > 1) {
      rx *= sqrt(gamma);
      ry *= sqrt(gamma);
   }

   /* Compute the center */
   k1 = rx * rx * y1_ * y1_ + ry * ry * x1_ * x1_;
   if (k1 == 0)
     return;

   k1 = sqrt(fabs((rx * rx * ry * ry) / k1 - 1));
   if (sweep_flag == large_arc_flag)
     k1 = -k1;

   cx_ = k1 * rx * y1_ / ry;
   cy_ = -k1 * ry * x1_ / rx;

   cx = cosf * cx_ - sinf * cy_ + (x1 + x2) / 2;
   cy = sinf * cx_ + cosf * cy_ + (y1 + y2) / 2;

   /* Compute start angle */
   k1 = (x1_ - cx_) / rx;
   k2 = (y1_ - cy_) / ry;
   k3 = (-x1_ - cx_) / rx;
   k4 = (-y1_ - cy_) / ry;

   k5 = sqrt(fabs(k1 * k1 + k2 * k2));
   if (k5 == 0) return;

   k5 = k1 / k5;
   if (k5 < -1) k5 = -1;
   else if(k5 > 1) k5 = 1;

   theta1 = acos(k5);
   if(k2 < 0) theta1 = -theta1;

   /* Compute delta_theta */
   k5 = sqrt(fabs((k1 * k1 + k2 * k2) * (k3 * k3 + k4 * k4)));
   if (k5 == 0) return;

   k5 = (k1 * k3 + k2 * k4) / k5;
   if (k5 < -1) k5 = -1;
   else if (k5 > 1) k5 = 1;
   delta_theta = acos(k5);
   if(k1 * k4 - k3 * k2 < 0) delta_theta = -delta_theta;

   if (sweep_flag && delta_theta < 0)
     delta_theta += M_PI*2;
   else if (!sweep_flag && delta_theta > 0)
     delta_theta -= M_PI*2;

   /* Now draw the arc */
   n_segs = ceil (fabs (delta_theta / (M_PI * 0.5 + 0.001)));

   for (i = 0; i < n_segs; i++)
     _ector_arc_segment(outline,
                        cx, cy,
                        theta1 + i * delta_theta / n_segs,
                        theta1 + (i + 1) * delta_theta / n_segs,
                        rx, ry, x_axis_rotation);

   *current_x = x;
   *current_y = y;
}

static Eina_Bool
_ector_renderer_software_shape_ector_renderer_generic_base_prepare(Eo *obj, Ector_Renderer_Software_Shape_Data *pd)
{
   // FIXME: shouldn't that be part of the shape generic implementation ?
   if (pd->shape->fill)
     eo_do(pd->shape->fill, ector_renderer_prepare());
   if (pd->shape->stroke.fill)
     eo_do(pd->shape->stroke.fill, ector_renderer_prepare());
   if (pd->shape->stroke.marker)
     eo_do(pd->shape->stroke.marker, ector_renderer_prepare());

   // shouldn't that be moved to the software base object
   if (!pd->surface)
     {
        Eo *parent;
        eo_do(obj, parent = eo_parent_get());
        if (!parent) return EINA_FALSE;
        pd->surface = eo_data_xref(parent, ECTOR_SOFTWARE_SURFACE_CLASS, obj);
        if (!pd->surface) return EINA_FALSE;
     }

   if (!pd->shape_data && pd->shape->path.cmd)
     {
        double *pts;
        double current_x = 0, current_y = 0;
        double current_ctrl_x = 0, current_ctrl_y = 0;
        unsigned int i;
        Eina_Bool close_path = EINA_FALSE;
        Outline * outline = _outline_create();

        pts = pd->shape->path.pts;
        for (i = 0; pd->shape->path.cmd[i] != EFL_GRAPHICS_PATH_COMMAND_TYPE_END; i++)
          {
             switch (pd->shape->path.cmd[i])
               {
                case EFL_GRAPHICS_PATH_COMMAND_TYPE_MOVE_TO:

                   _outline_move_to(outline, pts[0], pts[1]);

                   current_ctrl_x = current_x = pts[0];
                   current_ctrl_y = current_y = pts[1];

                   pts += 2;
                   break;
                case EFL_GRAPHICS_PATH_COMMAND_TYPE_LINE_TO:

                   _outline_line_to(outline, pts[0], pts[1]);

                   current_ctrl_x = current_x = pts[0];
                   current_ctrl_y = current_y = pts[1];

                   pts += 2;
                   break;
                case EFL_GRAPHICS_PATH_COMMAND_TYPE_CUBIC_TO:

                   // Be careful, we do have a different order than
                   // cairo, first is destination point, followed by
                   // the control point. The opposite of cairo.
                   _outline_cubic_to(outline,
                                  pts[2], pts[3], pts[4], pts[5], // control points
                                  pts[0], pts[1]); // destination point

                   current_ctrl_x = pts[4];
                   current_ctrl_y = pts[5];
                   current_x = pts[0];
                   current_y = pts[1];

                   pts += 6;
                   break;
                case EFL_GRAPHICS_PATH_COMMAND_TYPE_ARC_TO:
                   _ector_arc_to(outline,
                                 &current_x, &current_y,
                                 pts[2], pts[3], pts[4],
                                 0, 0, // FIXME: need to get the large arc and sweep flag
                                 pts[0], pts[1]);

                   pts += 5;
                   break;
                case EFL_GRAPHICS_PATH_COMMAND_TYPE_CLOSE:

                   close_path = _outline_close_path(outline);
                   break;
                case EFL_GRAPHICS_PATH_COMMAND_TYPE_QUADRATIC_TO:
                  {
                     double x1, y1, x2, y2, x3, y3;
                     // This code come from librsvg rsvg-path.c
                     // Be careful, we do have a different order than
                     // cairo, first is destination point, followed by
                     // the control point. The opposite of cairo.
                     /* raise quadratic bezier to cubic */
                     x1 = (current_x + 2 * pts[2]) * (1.0 / 3.0);
                     y1 = (current_y + 2 * pts[3]) * (1.0 / 3.0);
                     x3 = pts[0];
                     y3 = pts[1];
                     x2 = (x3 + 2 * pts[2]) * (1.0 / 3.0);
                     y2 = (y3 + 2 * pts[3]) * (1.0 / 3.0);

                     _outline_cubic_to(outline,
                                    x1, y1, x2, y2, // control points
                                    x3, y3); // destination point

                     current_ctrl_x = pts[2];
                     current_ctrl_y = pts[3];

                     current_x = x3;
                     current_y = y3;
                     break;
                  }
                case EFL_GRAPHICS_PATH_COMMAND_TYPE_SQUADRATIC_TO:
                  {
                     // This code come from librsvg rsvg-path.c
                     // Smooth quadratic basically reusing the last control
                     // point in a meaningful way.
                     double xc, yc; /* quadratic control point */
                     double x1, y1, x2, y2, x3, y3;

                     xc = 2 * current_x - current_ctrl_x;
                     yc = 2 * current_y - current_ctrl_y;
                     /* generate a quadratic bezier with control point = xc, yc */
                     x1 = (current_x + 2 * xc) * (1.0 / 3.0);
                     y1 = (current_y + 2 * yc) * (1.0 / 3.0);
                     x3 = pts[0];
                     y3 = pts[1];
                     x2 = (x3 + 2 * xc) * (1.0 / 3.0);
                     y2 = (y3 + 2 * yc) * (1.0 / 3.0);

                     _outline_cubic_to(outline,
                                    x1, y1, x2, y2, x3, y3);

                     current_ctrl_x = xc;
                     current_ctrl_y = yc;

                     current_x = x3;
                     current_y = y3;

                     break;
                  }
                case EFL_GRAPHICS_PATH_COMMAND_TYPE_SCUBIC_TO:
                  {
                     // This code come from librsvg rsvg-path.c
                     // Smooth cubic basically reusing the last control point
                     // in a meaningful way.
                     double x1, y1, x2, y2, x3, y3;

                     x1 = 2 * current_x - current_ctrl_x;
                     y1 = 2 * current_y - current_ctrl_y;
                     x2 = pts[2];
                     y2 = pts[3];
                     x3 = pts[0];
                     y3 = pts[1];

                     _outline_cubic_to(outline,
                                    x1, y1, x2, y2, x3, y3);

                     current_ctrl_x = x2;
                     current_ctrl_y = y2;
                     current_x = x3;
                     current_y = y3;
                     break;
                  }
                case EFL_GRAPHICS_PATH_COMMAND_TYPE_LAST:
                case EFL_GRAPHICS_PATH_COMMAND_TYPE_END:
                   break;
               }
          }
          _outline_end(outline);
          _outline_transform(outline, pd->base->m);
          // generate the shape data.
          pd->shape_data = ector_software_rasterizer_generate_rle_data(pd->surface->software, &outline->ft_outline);
          if (!pd->outline_data) {
             ector_software_rasterizer_stroke_set(pd->surface->software, (pd->shape->stroke.width * pd->shape->stroke.scale), pd->shape->stroke.cap,
                                                  pd->shape->stroke.join);
             pd->outline_data = ector_software_rasterizer_generate_stroke_rle_data(pd->surface->software, &outline->ft_outline, close_path);
          }

          _outline_destroy(outline);
    }  

   return EINA_TRUE;
}

static Eina_Bool
_ector_renderer_software_shape_ector_renderer_generic_base_draw(Eo *obj, Ector_Renderer_Software_Shape_Data *pd, Ector_Rop op, Eina_Array *clips, int x, int y, unsigned int mul_col)
{
   // TODO remove me when double to int conversion is fixed.
   //printf("%.0f",pd->base->origin.x);

   int offx = 0 , offy = 0;
   offx = x + lrint(pd->base->origin.x);
   offy = y + lrint(pd->base->origin.y);
   // fill the span_data structure
   ector_software_rasterizer_clip_rect_set(pd->surface->software, clips);
   ector_software_rasterizer_transform_set(pd->surface->software, pd->base->m);

   if (pd->shape->fill) {
     eo_do(pd->shape->fill, ector_renderer_software_base_fill());
     ector_software_rasterizer_draw_rle_data(pd->surface->software, offx, offy, mul_col, op, pd->shape_data);
   } else {
      if (pd->base->color.a > 0) {
          ector_software_rasterizer_color_set(pd->surface->software, pd->base->color.r, pd->base->color.g, pd->base->color.b, pd->base->color.a);
          ector_software_rasterizer_draw_rle_data(pd->surface->software, offx, offy, mul_col, op, pd->shape_data);
      }
   }

  if (pd->shape->stroke.fill) {
    eo_do(pd->shape->stroke.fill, ector_renderer_software_base_fill());
    ector_software_rasterizer_draw_rle_data(pd->surface->software, offx, offy, mul_col, op, pd->outline_data);
  } else {
    if (pd->shape->stroke.color.a > 0) {
      ector_software_rasterizer_color_set(pd->surface->software, pd->shape->stroke.color.r, pd->shape->stroke.color.g, 
                                          pd->shape->stroke.color.b, pd->shape->stroke.color.a);
      ector_software_rasterizer_draw_rle_data(pd->surface->software, offx, offy, mul_col, op, pd->outline_data);
    }

  }
   return EINA_TRUE;
}

static Eina_Bool
_ector_renderer_software_shape_ector_renderer_software_base_fill(Eo *obj EINA_UNUSED, Ector_Renderer_Software_Shape_Data *pd EINA_UNUSED)
{
   // FIXME: let's find out how to fill a shape with a shape later.
   // I need to read SVG specification and see how to map that with software.
  return EINA_FALSE;
}

static Eina_Bool
_ector_renderer_software_shape_efl_graphics_shape_path_set(Eo *obj, Ector_Renderer_Software_Shape_Data *pd,
                                                        const Efl_Graphics_Path_Command *op, const double *points)
{
   Eina_Bool r;

   if(pd->shape_data) ector_software_rasterizer_destroy_rle_data(pd->shape_data);
   if(pd->outline_data) ector_software_rasterizer_destroy_rle_data(pd->outline_data);
   pd->shape_data = NULL;
   pd->outline_data = NULL;

   eo_do_super(obj, ECTOR_RENDERER_SOFTWARE_SHAPE_CLASS, r = efl_graphics_shape_path_set(op, points));

   return r;
}


void
_ector_renderer_software_shape_eo_base_constructor(Eo *obj, Ector_Renderer_Software_Shape_Data *pd)
{
   eo_do_super(obj, ECTOR_RENDERER_SOFTWARE_SHAPE_CLASS, eo_constructor());
   pd->shape = eo_data_xref(obj, ECTOR_RENDERER_GENERIC_SHAPE_CLASS, obj);
   pd->base = eo_data_xref(obj, ECTOR_RENDERER_GENERIC_BASE_CLASS, obj);
}

void
_ector_renderer_software_shape_eo_base_destructor(Eo *obj, Ector_Renderer_Software_Shape_Data *pd)
{
   Eo *parent;

   if(pd->shape_data) ector_software_rasterizer_destroy_rle_data(pd->shape_data);
   if(pd->outline_data) ector_software_rasterizer_destroy_rle_data(pd->outline_data);

   eo_do(obj, parent = eo_parent_get());
   eo_data_xunref(parent, pd->surface, obj);

   eo_data_xunref(obj, pd->shape, obj);
   eo_data_xunref(obj, pd->base, obj);
   eo_do_super(obj, ECTOR_RENDERER_SOFTWARE_SHAPE_CLASS, eo_destructor());
}


#include "ector_renderer_software_shape.eo.c"
