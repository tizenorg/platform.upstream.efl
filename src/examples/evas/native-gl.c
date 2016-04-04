/**
 * Evas example illustrating work with GL APIs.
 *
 * Evas_GL.h should be added for Evas GL APIs access.
 *
 * @verbatim
 gcc -o native-gl native-gl.c `pkg-config --libs --cflags ecore ecore-win gles20` -lm
 gcc -o evas-gl evas-gl.c `pkg-config --libs --cflags evas ecore ecore-evas` -lm
 * @endverbatim
 */

#include <Ecore.h>
#include <Ecore_Win.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <math.h>

#define WIDTH  720
#define HEIGHT 1280

//GL related data here...
typedef struct _GLData
{
  Ecore_Window window;
  Ecore_Display   display;
  Ecore_Surface eglwindow;

  EGLDisplay eglDisplay;
  EGLSurface eglSurface;
  EGLContext eglContext;
  EGLConfig config;

   unsigned int     program;
   unsigned int     vtx_shader;
   unsigned int     fgmt_shader;
   unsigned int     vbo;
   float            view[16];

   float xangle;
   float yangle;
   Eina_Bool mouse_down : 1;
   Eina_Bool initialized : 1;

} GLData;

static GLData gldata;

//Define the cube's vertices
//Each vertex consist of x, y, z, r, g, b
const float cube_vertices[] =
{
   //front surface is blue
   0.5,  0.5,  0.5,  0.0, 0.0, 1.0,
   -0.5, -0.5,  0.5, 0.0, 0.0, 1.0,
   0.5, -0.5,  0.5,  0.0, 0.0, 1.0,
   0.5,  0.5,  0.5,  0.0, 0.0, 1.0,
   -0.5,  0.5,  0.5, 0.0, 0.0, 1.0,
   -0.5, -0.5,  0.5, 0.0, 0.0, 1.0,
   //left surface is green
   -0.5,  0.5,  0.5, 0.0, 1.0, 0.0,
   -0.5, -0.5, -0.5, 0.0, 1.0, 0.0,
   -0.5, -0.5,  0.5, 0.0, 1.0, 0.0,
   -0.5,  0.5,  0.5, 0.0, 1.0, 0.0,
   -0.5,  0.5, -0.5, 0.0, 1.0, 0.0,
   -0.5, -0.5, -0.5, 0.0, 1.0, 0.0,
   //top surface is red
   -0.5,  0.5,  0.5, 1.0, 0.0, 0.0,
   0.5,  0.5, -0.5,  1.0, 0.0, 0.0,
   -0.5,  0.5, -0.5, 1.0, 0.0, 0.0,
   -0.5,  0.5,  0.5, 1.0, 0.0, 0.0,
   0.5,  0.5,  0.5,  1.0, 0.0, 0.0,
   0.5,  0.5, -0.5,  1.0, 0.0, 0.0,
   //right surface is yellow
   0.5,  0.5, -0.5,  1.0, 1.0, 0.0,
   0.5, -0.5,  0.5,  1.0, 1.0, 0.0,
   0.5, -0.5, -0.5,  1.0, 1.0, 0.0,
   0.5,  0.5, -0.5,  1.0, 1.0, 0.0,
   0.5,  0.5,  0.5,  1.0, 1.0, 0.0,
   0.5, -0.5,  0.5,  1.0, 1.0, 0.0,
   //back surface is cyan
   -0.5,  0.5, -0.5, 0.0, 1.0, 1.0,
   0.5, -0.5, -0.5,  0.0, 1.0, 1.0,
   -0.5, -0.5, -0.5, 0.0, 1.0, 1.0,
   -0.5,  0.5, -0.5, 0.0, 1.0, 1.0,
   0.5,  0.5, -0.5,  0.0, 1.0, 1.0,
   0.5, -0.5, -0.5,  0.0, 1.0, 1.0,
   //bottom surface is magenta
   -0.5, -0.5, -0.5, 1.0, 0.0, 1.0,
   0.5, -0.5,  0.5,  1.0, 0.0, 1.0,
   -0.5, -0.5,  0.5, 1.0, 0.0, 1.0,
   -0.5, -0.5, -0.5, 1.0, 0.0, 1.0,
   0.5, -0.5, -0.5,  1.0, 0.0, 1.0,
   0.5, -0.5,  0.5,  1.0, 0.0, 1.0
};

//Vertext Shader Source
static const char vertex_shader[] =
   "attribute vec4 vPosition;\n"
   "attribute vec3 inColor;\n"
   "uniform mat4 mvpMatrix;"
   "varying vec3 outColor;\n"
   "void main()\n"
   "{\n"
   "   outColor = inColor;\n"
   "   gl_Position = mvpMatrix * vPosition;\n"
   "}\n";

//Fragment Shader Source
static const char fragment_shader[] =
   "#ifdef GL_ES\n"
   "precision mediump float;\n"
   "#endif\n"
   "varying vec3 outColor;\n"
   "void main()\n"
   "{\n"
   "   gl_FragColor = vec4 ( outColor, 1.0 );\n"
   "}\n";

//Rotation Operation related functions here...
static void
init_matrix(float matrix[16])
{
   matrix[0] = 1.0f;
   matrix[1] = 0.0f;
   matrix[2] = 0.0f;
   matrix[3] = 0.0f;

   matrix[4] = 0.0f;
   matrix[5] = 1.0f;
   matrix[6] = 0.0f;
   matrix[7] = 0.0f;

   matrix[8] = 0.0f;
   matrix[9] = 0.0f;
   matrix[10] = 1.0f;
   matrix[11] = 0.0f;

   matrix[12] = 0.0f;
   matrix[13] = 0.0f;
   matrix[14] = 0.0f;
   matrix[15] = 1.0f;
}

static void
multiply_matrix(float matrix[16], const float matrix0[16],
                const float matrix1[16])
{
   int i;
   int row;
   int column;
   float temp[16];

   for (column = 0; column < 4; column++)
     {
        for (row = 0; row < 4; row++)
          {
             temp[(column * 4) + row] = 0.0f;

             for (i = 0; i < 4; i++)
               temp[(column * 4) + row] += matrix0[(i * 4) + row] * matrix1[(column * 4) + i];
          }
     }

   for (i = 0; i < 16; i++)
        matrix[i] = temp[i];
}

static void
rotate_xyz(float matrix[16], const float anglex, const float angley,
           const float anglez)
{
   const float pi = 3.141592f;
   float temp[16];
   float rx = 2.0f * pi * anglex / 360.0f;
   float ry = 2.0f * pi * angley / 360.0f;
   float rz = 2.0f * pi * anglez / 360.0f;
   float sy = sinf(ry);
   float cy = cosf(ry);
   float sx = sinf(rx);
   float cx = cosf(rx);
   float sz = sinf(rz);
   float cz = cosf(rz);

   init_matrix(temp);

   temp[0] = (cy * cz) - (sx * sy * sz);
   temp[1] = (cz * sx * sy) + (cy * sz);
   temp[2] = -cx * sy;

   temp[4] = -cx * sz;
   temp[5] = cx * cz;
   temp[6] = sx;

   temp[8] = (cz * sy) + (cy * sx * sz);
   temp[9] = (-cy * cz * sx) + (sy * sz);
   temp[10] = cx * cy;

   multiply_matrix(matrix, matrix, temp);
}

static int
view_set_ortho(float result[16], const float left, const float right,
               const float bottom, const float top, const float near,
               const float far)
{
   if ((right - left) == 0.0f || (top - bottom) == 0.0f || (far - near) == 0.0f)
        return 0;

    result[0] = 2.0f / (right - left);
    result[1] = 0.0f;
    result[2] = 0.0f;
    result[3] = 0.0f;
    result[4] = 0.0f;
    result[5] = 2.0f / (top - bottom);
    result[6] = 0.0f;
    result[7] = 0.0f;
    result[8] = 0.0f;
    result[9] = 0.0f;
    result[10] = -2.0f / (far - near);
    result[11] = 0.0f;
    result[12] = -(right + left) / (right - left);
    result[13] = -(top + bottom) / (top - bottom);
    result[14] = -(far + near) / (far - near);
    result[15] = 1.0f;

    return 1;
}

static void
init_shaders(GLData *gl_data)
{
   const char *p;

   p = vertex_shader;
   gl_data->vtx_shader =  glCreateShader(GL_VERTEX_SHADER);
   glShaderSource(gl_data->vtx_shader, 1, &p, NULL);
   glCompileShader(gl_data->vtx_shader);

   p = fragment_shader;
   gl_data->fgmt_shader =  glCreateShader(GL_FRAGMENT_SHADER);
   glShaderSource(gl_data->fgmt_shader, 1, &p, NULL);
   glCompileShader(gl_data->fgmt_shader);

   gl_data->program =  glCreateProgram();
   glAttachShader(gl_data->program, gl_data->vtx_shader);
   glAttachShader(gl_data->program, gl_data->fgmt_shader);
   glBindAttribLocation(gl_data->program, 0, "vPosition");
   glBindAttribLocation(gl_data->program, 1, "inColor");

   glLinkProgram(gl_data->program);
   glUseProgram(gl_data->program);
   glEnable(GL_DEPTH_TEST);
}

static void
img_pixel_cb(void *data)
{
   //Define the model view projection matrix
   float model[16], mvp[16];
   GLData *gl_data = data;
   int w = WIDTH;
   int h = HEIGHT;
 
    //Initialize gl stuff just one time.
   if (!gl_data->initialized)
     {
        float aspect;

   //Set rotation variables	
        init_shaders(gl_data);
        glGenBuffers(1, &gl_data->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, gl_data->vbo);
         glBufferData(GL_ARRAY_BUFFER, 3 * 72 * 4, cube_vertices,GL_STATIC_DRAW);
        init_matrix(gl_data->view);
        if(w > h)
          {
             aspect = (float)w/h;
             view_set_ortho(gl_data->view, -1.0 * aspect, 1.0 * aspect, -1.0, 1.0, -1.0, 1.0);
          }
        else
          {
             aspect = (float)h/w;
             view_set_ortho(gl_data->view, -1.0, 1.0, -1.0 * aspect, 1.0 * aspect, -1.0, 1.0);
          }
        gl_data->initialized = EINA_TRUE;
     }

    glViewport(0, 0, w, h);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

   init_matrix(model);
   rotate_xyz(model, gl_data->xangle, gl_data->yangle, 0.0f);
   multiply_matrix(mvp, gl_data->view, model);

    glUseProgram(gl_data->program);
    glBindBuffer(GL_ARRAY_BUFFER, gl_data->vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, gl_data->vbo);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)(sizeof(float)*3));
    glEnableVertexAttribArray(1);

    glUniformMatrix4fv(  glGetUniformLocation(gl_data->program, "mvpMatrix"), 1, GL_FALSE, mvp);
    glDrawArrays(GL_TRIANGLES, 0, 36);

   eglSwapBuffers(gl_data->eglDisplay,gl_data->eglSurface);
}
#if 0
static void
img_del_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   GLData *gl_data = data;
   Evas_GL_API *glapi = gl_data->glapi;

   //Free the gl resources when image object is deleted.
   evas_gl_make_current(gl_data->evasgl, gl_data->sfc, gl_data->ctx);

   glapi->glDeleteShader(gl_data->vtx_shader);
   glapi->glDeleteShader(gl_data->fgmt_shader);
   glapi->glDeleteProgram(gl_data->program);
   glapi->glDeleteBuffers(1, &gl_data->vbo);

   evas_gl_surface_destroy(gl_data->evasgl, gl_data->sfc);
   evas_gl_context_destroy(gl_data->evasgl, gl_data->ctx);

   evas_gl_free(gl_data->evasgl);
}
#endif

static Eina_Bool
_animator_cb(void *data)
{
  img_pixel_cb( data );

   return ECORE_CALLBACK_RENEW;
}

#if 0
static void
_mouse_down_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   GLData *gl_data = data;
   gl_data->mouse_down = EINA_TRUE;
}

static void
_mouse_move_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Evas_Event_Mouse_Move *ev;
   ev = (Evas_Event_Mouse_Move *)event_info;
   GLData *gl_data = data;
   float dx = 0, dy = 0;

   if( gl_data->mouse_down)
     {
        dx = (ev->cur.canvas.x - ev->prev.canvas.x);
        dy = (ev->cur.canvas.y - ev->prev.canvas.y);
        gl_data->xangle += dy;
        gl_data->yangle += dx;
     }
}

static void
_mouse_up_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   GLData *gl_data = data;
   gl_data->mouse_down = EINA_FALSE;
}

static void
_win_resize_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   float aspect;
   Evas_Coord w,h;
   evas_object_geometry_get( obj, NULL, NULL, &w, &h);

   GLData *gl_data = data;
   evas_object_resize(gl_data->img, w, h);

   init_matrix(gl_data->view);
   if(w > h)
     {
        aspect = (float)w/h;
        view_set_ortho(gl_data->view, (-1.0 * aspect), (1.0 * aspect), -1.0, 1.0, -1.0, 1.0);
     }
   else
     {
        aspect = (float)h/w;
        view_set_ortho(gl_data->view, -1.0, 1.0, (-1.0 * aspect), (1.0 * aspect), -1.0, 1.0);
     }
}
 
static void
gl_init(Evas_Object *win)
{
   Evas_Native_Surface ns;

   //Config for the surface for evas gl
   Evas_GL_Config *config = evas_gl_config_new();
   config->color_format = EVAS_GL_RGBA_8888;
   config->depth_bits = EVAS_GL_DEPTH_BIT_24;

   //Get the window size
   Evas_Coord w,h;
   evas_object_geometry_get(win, NULL, NULL, &w, &h);

   //Get the evas gl handle for doing gl things
   gldata.evasgl = evas_gl_new(evas_object_evas_get(win));
   gldata.glapi = evas_gl_api_get(gldata.evasgl);

   //Create a surface and context
   gldata.sfc = evas_gl_surface_create(gldata.evasgl, config, w, h);
   gldata.ctx = evas_gl_context_create(gldata.evasgl, NULL);
   evas_gl_config_free(config);

   //Set rotation variables
   gldata.xangle = 45.0f;
   gldata.yangle = 45.0f;
   gldata.mouse_down = EINA_FALSE;

   //Set up the image object. A filled one by default.
   gldata.img = evas_object_image_filled_add(evas_object_evas_get(win));
   evas_object_event_callback_add(gldata.img, EVAS_CALLBACK_DEL, img_del_cb, &gldata);

   //Set up an actual pixel size for the buffer data. It may be different to the
   //output size. Any windowing sysmtem has something like this, just evas has 2
   //sizes, a pixel size and the output object size.
   evas_object_image_size_set(gldata.img, w, h);

   //Set up the native surface info to use the context and surface created
   //above.
   evas_gl_native_surface_get(gldata.evasgl, gldata.sfc, &ns);
   evas_object_image_native_surface_set(gldata.img, &ns);
   evas_object_image_pixels_get_callback_set(gldata.img, img_pixel_cb, &gldata);

   evas_object_resize(gldata.img, w, h);
   evas_object_show(gldata.img);


}

 
static void
_on_delete_cb(Ecore_Evas *ee EINA_UNUSED)
{
   ecore_main_loop_quit();
}

static void
_on_canvas_resize_cb(Ecore_Evas *ee)
{
   int w, h;
   ecore_evas_geometry_get(ee, NULL, NULL, &w, &h);
   evas_object_resize(gldata.win, w, h);
}
#endif

  void init_EGL(void)
  {
    EGLint major_version, minor_version;
    EGLint context_attrs[3];
    EGLint config_attrs[40];
    int n = 0;
    EGLint num_config;

    printf("init EGL\n");	

   context_attrs[0] = EGL_CONTEXT_CLIENT_VERSION;
   context_attrs[1] = 2;
   context_attrs[2] = EGL_NONE;

   config_attrs[n++] = EGL_SURFACE_TYPE;
   config_attrs[n++] = EGL_WINDOW_BIT;
   config_attrs[n++] = EGL_RENDERABLE_TYPE;
   config_attrs[n++] = EGL_OPENGL_ES2_BIT;

#if 1   
   config_attrs[n++] = EGL_RED_SIZE;
   config_attrs[n++] = 8;
   config_attrs[n++] = EGL_GREEN_SIZE;
   config_attrs[n++] = 8;
   config_attrs[n++] = EGL_BLUE_SIZE;
   config_attrs[n++] = 8;
#endif   
   config_attrs[n++] = EGL_DEPTH_SIZE;
   config_attrs[n++] = 24;
   config_attrs[n++] = EGL_STENCIL_SIZE;
   config_attrs[n++] = 8;
   config_attrs[n++] = EGL_NONE;

   gldata.eglDisplay= eglGetDisplay((EGLNativeDisplayType)gldata.display);
   if (!gldata.eglDisplay)
     {
        printf("eglGetDisplay() fail. code=%#x\n", eglGetError());
        return;
     }
   
   if (!eglInitialize(gldata.eglDisplay,&major_version,&minor_version))
     {
        printf("eglinitialize() fail code=%#x\n", eglGetError());
	return;
     }

   if  (!eglBindAPI(EGL_OPENGL_ES_API))
     {
        printf("eglBindAPI() Fail code=%#x\n", eglGetError());
	return;
     }

   num_config = 0;
   gldata.config = 0;
   if (!eglChooseConfig(gldata.eglDisplay, config_attrs, &gldata.config,1,&num_config)  || (num_config != 1))
     {
        printf("eglChooseConfig() fail code=%#x\n", eglGetError());
        return;
     }
   
   gldata.eglSurface = eglCreateWindowSurface(gldata.eglDisplay, gldata.config,  gldata.eglwindow , NULL);
   if (gldata.eglSurface == EGL_NO_SURFACE)
     {
        printf("eglCreateWindowSurface() fail code=%#x\n", eglGetError());
	return;
     }

   gldata.eglContext = eglCreateContext(gldata.eglDisplay, gldata.config, NULL, context_attrs);
   if (gldata.eglContext == EGL_NO_CONTEXT)
     {
        printf("eglCreateContext() fail code=%#x\n", eglGetError());
        return;
     }

   if (!eglMakeCurrent(gldata.eglDisplay, gldata.eglSurface,gldata.eglSurface,gldata.eglContext))
     {
        printf("eglMakeCurrent() fail code=%#x\n", eglGetError());
	return;
     }
   }

int
main(void)
{
   if (!ecore_win_init()) return 0;

   Ecore_Win *ewin = ecore_win_new("wayland", 0, 0, WIDTH, HEIGHT, NULL);

   if(!ewin ) return 0;

#if 0
   ecore_evas_callback_delete_request_set(ecore_evas, _on_delete_cb);
   ecore_evas_callback_resize_set(ecore_evas, _on_canvas_resize_cb);
#endif   
   ecore_win_show(ewin);

   gldata.window = ecore_win_window_get(ewin);
   printf("window : %p\n",gldata.window);
   gldata.display = ecore_win_display_get(ewin);
   printf("display : %p\n",gldata.display);
   gldata.eglwindow = ecore_win_surface_get(ewin);
   printf("eglwindow : %p\n",gldata.eglwindow);

   gldata.xangle = 45.0f;
   gldata.yangle = 45.0f;
   gldata.mouse_down = EINA_FALSE;
   gldata.initialized = EINA_FALSE;

   init_EGL();

   //Add Mouse Event Callbacks
#if 0
   evas_object_event_callback_add(gldata.img, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down_cb, &gldata);
   evas_object_event_callback_add(gldata.img, EVAS_CALLBACK_MOUSE_UP, _mouse_up_cb, &gldata);
   evas_object_event_callback_add(gldata.img, EVAS_CALLBACK_MOUSE_MOVE, _mouse_move_cb, &gldata);
   //Add Window Resize Event Callback
   evas_object_event_callback_add(win, EVAS_CALLBACK_RESIZE, _win_resize_cb, &gldata);
#endif
   ecore_animator_add(_animator_cb,&gldata );

   ecore_main_loop_begin();
   ecore_win_free(ewin);
   ecore_win_shutdown();

   return 0;
}
