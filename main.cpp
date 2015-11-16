#define GL_GLEXT_PROTOTYPES

#include <iostream>
#include <cstdio>
#include <cstdlib>

#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "grid.hpp"

class Engine {
private:
  SDL_Surface* screen;
  
  
  
  bool initSDL(int w, int h) {
    // Init SDL
    if(SDL_Init(SDL_INIT_EVERYTHING)) {
      fprintf(stderr, "Failed to initialize SDL\n");
      return false;
    }
    
    const SDL_VideoInfo* info = SDL_GetVideoInfo();
    int bpp = info->vfmt->BitsPerPixel;
    
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    
    screen = SDL_SetVideoMode(w, h, bpp, SDL_OPENGL | SDL_FULLSCREEN);
    
    if(!screen) {
      fprintf(stderr, "Failed to set video mode\n");
      return false;
    }
    
    return true;
  }
  
  bool initOpenGL(int w, int h) {
    float ratio = (float)w / h;
    
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_CULL_FACE);
    
    glShadeModel(GL_SMOOTH);
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, ratio, 1, 1024);
    
    return true;
  }
  
public:
  Engine() {
    screen = NULL;
  }
  
  bool init(int w, int h) {
    return initSDL(w, h) && initOpenGL(w, h);
  }
  
  void flipScreen() {
    SDL_GL_SwapBuffers();
  }
  
  GLuint vertexbuffer;
  void testInit();
  void test();
  
  ~Engine() {
    SDL_Quit();
  }
};

bool should_rotate = false;

//====================================================================
static bool handle_key_down( SDL_keysym* keysym )
{
  
  /* 
   * We're only interested if 'Esc' has
   * been presssed.
   *
   * EXERCISE: 
   * Handle the arrow keys and have that change the
   * viewing position/angle.
   */
  switch( keysym->sym ) {
    case SDLK_ESCAPE:
      return false;
      break;
    case SDLK_SPACE:
      should_rotate = !should_rotate;
      break;
    default:
      break;
  }
  
  return true;
}

static bool process_events( void )
{
  /* Our SDL event placeholder. */
  SDL_Event event;
  
  /* Grab all the events off the queue. */
  while( SDL_PollEvent( &event ) ) {
    
    switch( event.type ) {
      case SDL_KEYDOWN:
        /* Handle key presses. */
        return handle_key_down( &event.key.keysym );
        break;
      case SDL_QUIT:
        /* Handle quit requests (like Ctrl-c). */
        return false;
    }
    
  }
  
  return true;
  
}

static void draw_screen( void )
{
  /* Our angle of rotation. */
  static float angle = 0.0f;
  
  /*
   * EXERCISE:
   * Replace this awful mess with vertex
   * arrays and a call to glDrawElements.
   *
   * EXERCISE:
   * After completing the above, change
   * it to use compiled vertex arrays.
   *
   * EXERCISE:
   * Verify my windings are correct here ;).
   */
  static GLfloat v0[] = { -1.0f, -1.0f,  1.0f };
  static GLfloat v1[] = {  1.0f, -1.0f,  1.0f };
  static GLfloat v2[] = {  1.0f,  1.0f,  1.0f };
  static GLfloat v3[] = { -1.0f,  1.0f,  1.0f };
  static GLfloat v4[] = { -1.0f, -1.0f, -1.0f };
  static GLfloat v5[] = {  1.0f, -1.0f, -1.0f };
  static GLfloat v6[] = {  1.0f,  1.0f, -1.0f };
  static GLfloat v7[] = { -1.0f,  1.0f, -1.0f };
  static GLubyte red[]    = { 255,   0,   0, 255 };
  static GLubyte green[]  = {   0, 255,   0, 255 };
  static GLubyte blue[]   = {   0,   0, 255, 255 };
  static GLubyte white[]  = { 255, 255, 255, 255 };
  static GLubyte yellow[] = {   0, 255, 255, 255 };
  static GLubyte black[]  = {   0,   0,   0, 255 };
  static GLubyte orange[] = { 255, 255,   0, 255 };
  static GLubyte purple[] = { 255,   0, 255,   0 };
  
  /* Clear the color and depth buffers. */
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  
  /* We don't want to modify the projection matrix. */
  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity( );
  
  /* Move down the z-axis. */
  glTranslatef( 0.0, 0.0, -5.0 );
  
  /* Rotate. */
  glRotatef( angle, 0.0, 1.0, 0.0 );
  
  if( should_rotate ) {
    
    if( ++angle > 360.0f ) {
      angle = 0.0f;
    }
    
  }
  
  /* Send our triangle data to the pipeline. */
  glBegin( GL_TRIANGLES );
  
  glColor4ubv( red );
  glVertex3fv( v0 );
  glColor4ubv( green );
  glVertex3fv( v1 );
  glColor4ubv( blue );
  glVertex3fv( v2 );
  
  glColor4ubv( red );
  glVertex3fv( v0 );
  glColor4ubv( blue );
  glVertex3fv( v2 );
  glColor4ubv( white );
  glVertex3fv( v3 );
  
  glColor4ubv( green );
  glVertex3fv( v1 );
  glColor4ubv( black );
  glVertex3fv( v5 );
  glColor4ubv( orange );
  glVertex3fv( v6 );
  
  glColor4ubv( green );
  glVertex3fv( v1 );
  glColor4ubv( orange );
  glVertex3fv( v6 );
  glColor4ubv( blue );
  glVertex3fv( v2 );
  
  glColor4ubv( black );
  glVertex3fv( v5 );
  glColor4ubv( yellow );
  glVertex3fv( v4 );
  glColor4ubv( purple );
  glVertex3fv( v7 );
  
  glColor4ubv( black );
  glVertex3fv( v5 );
  glColor4ubv( purple );
  glVertex3fv( v7 );
  glColor4ubv( orange );
  glVertex3fv( v6 );
  
  glColor4ubv( yellow );
  glVertex3fv( v4 );
  glColor4ubv( red );
  glVertex3fv( v0 );
  glColor4ubv( white );
  glVertex3fv( v3 );
  
  glColor4ubv( yellow );
  glVertex3fv( v4 );
  glColor4ubv( white );
  glVertex3fv( v3 );
  glColor4ubv( purple );
  glVertex3fv( v7 );
  
  glColor4ubv( white );
  glVertex3fv( v3 );
  glColor4ubv( blue );
  glVertex3fv( v2 );
  glColor4ubv( orange );
  glVertex3fv( v6 );
  
  glColor4ubv( white );
  glVertex3fv( v3 );
  glColor4ubv( orange );
  glVertex3fv( v6 );
  glColor4ubv( purple );
  glVertex3fv( v7 );
  
  glColor4ubv( green );
  glVertex3fv( v1 );
  glColor4ubv( red );
  glVertex3fv( v0 );
  glColor4ubv( yellow );
  glVertex3fv( v4 );
  
  glColor4ubv( green );
  glVertex3fv( v1 );
  glColor4ubv( yellow );
  glVertex3fv( v4 );
  glColor4ubv( black );
  glVertex3fv( v5 );
  
  glEnd( );
  
  /*
   * EXERCISE:
   * Draw text telling the user that 'Spc'
   * pauses the rotation and 'Esc' quits.
   * Do it using vetors and textured quads.
   */
  
  /*
   * Swap the buffers. This this tells the driver to
   * render the next frame from the contents of the
   * back-buffer, and to set all rendering operations
   * to occur on what was the front-buffer.
   *
   * Double buffering prevents nasty visual tearing
   * from the application drawing on areas of the
   * screen that are being updated at the same time.
   */
  SDL_GL_SwapBuffers( );
}
//====================================================================


void Engine::testInit() {
  // An array of 3 vectors which represents 3 vertices
  static const GLfloat g_vertex_buffer_data[] = {
    -1.0f, -1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    0.0f,  1.0f, 0.0f,
  };
  
  // This will identify our vertex buffer
  // Generate 1 buffer, put the resulting identifier in vertexbuffer
  glGenBuffers(1, &vertexbuffer);
  // The following commands will talk about our 'vertexbuffer' buffer
  glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
  // Give our vertices to OpenGL.
  glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);
}

void Engine::test() {
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
  glVertexAttribPointer(
      0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
      3,                  // size
      GL_FLOAT,           // type
      GL_FALSE,           // normalized?
      0,                  // stride
      (void*)0            // array buffer offset
    );
  // Draw the triangle !
  glDrawArrays(GL_TRIANGLES, 0, 3); // Starting from vertex 0; 3 vertices total -> 1 triangle
  glDisableVertexAttribArray(0);
}

int main(int argc, char *argv[]) {
  Engine engine;
  
  engine.init(640, 480);
  
  engine.testInit();
  
  while( 1 ) {
    /* Process incoming events. */
    if(!process_events())
      break;
    
    /* Draw the screen. */
    //draw_screen( );
    engine.test();
    engine.flipScreen();
  }
}
