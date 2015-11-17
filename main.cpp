#define GL_GLEXT_PROTOTYPES

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <map>

#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <vector>
#include <fstream>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "grid.hpp"

class Model {
private:
  std::vector<glm::vec3> mesh_v;
  
  int addVertex(glm::vec3 v) {
    // Check existing points
    for(int i = 0; i < (int)mesh_v.size(); ++i) {
      if(v == mesh_v[i]) {
	return i;
      }
    }
    
    mesh_v.push_back(v);
    return mesh_v.size() - 1;
  }
public:
  GLuint vertexBuffer;
  int total_v;
  
  Model() {
    total_v = 3;
    
    printf("Create model\n");
    
    // An array of 3 vectors which represents 3 vertices
    static const GLfloat g_vertex_buffer_data[] = {
      -1.0f, -1.0f, 0.0f,
      1.0f, -1.0f, 0.0f,
      0.0f,  1.0f, 0.0f,
    };
    
    // This will identify our vertex buffer
    // Generate 1 buffer, put the resulting identifier in vertexbuffer
    glGenBuffers(1, &vertexBuffer);
    // The following commands will talk about our 'vertexbuffer' buffer
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    // Give our vertices to OpenGL.
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);
  }
  
  // Renders the model using the current transformation settings
  void render() {
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glVertexAttribPointer(
      0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
      total_v,                  // size
      GL_FLOAT,           // type
      GL_FALSE,           // normalized?
      0,                  // stride
      (void*)0            // array buffer offset
    );
    // Draw the triangle !
    glDrawArrays(GL_TRIANGLES, 0, 3); // Starting from vertex 0; 3 vertices total -> 1 triangle
    glDisableVertexAttribArray(0);
  }
};

class Actor {
public:
  Model* model;
  glm::vec3 pos;
  glm::vec3 angle;
  glm::mat4x4 mat;
  glm::vec3 direction;
  glm::vec3 right;
  
  Actor() {
    model = NULL;
    angle = glm::vec3(0, 0, 0);
    mat = glm::mat4x4(1.0f);
  }
  
  void render() {
    if(model) {
      model->render();
    }
  }
};

class Camera : public Actor {
public:
  glm::mat4x4 project;
  glm::mat4x4 view;
  glm::mat4x4 project_view;
  
  float near, far;
};

class Engine {
private:
  SDL_Surface* screen;
  GLuint programID;
  Camera cam;
  GLuint mvpMatrixID;
  int mouse_dx, mouse_dy;
  std::map<int, bool> keyMap;
  bool lockMouse;
  
  bool keyDown(int key) {
    return keyMap.count(key) && keyMap[key];
  }
  
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
    
    lockMouse = false;
    
    screen = SDL_SetVideoMode(w, h, bpp, SDL_OPENGL);// | SDL_FULLSCREEN);
    
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
    
    cam.near = 1;
    cam.far = 1024;
    
    gluPerspective(60, ratio, cam.near, cam.far);
    
    glClearColor(0.0f, 0.f, 0.0f, 0.0f); 
    
    programID = loadShaders("../vertex.glsl",  "../fragment.glsl");
    
    if (programID == 0) {
      return false;
    }
    
    return true;
  }
  
public:
  bool quit;
  
  Engine() {
    screen = NULL;
  }
  
  bool init(int w, int h, float fov = 60.0) {
    if(!initSDL(w, h) || !initOpenGL(w, h)) {
      return false;
    }
    
    // Initialize camera
    cam.project = glm::perspective(glm::radians(fov), (float)w / h, cam.near, cam.far);
    

    cam.view = glm::lookAt(
      glm::vec3(0,0,3), // Camera is at (4,3,3), in World Space
      glm::vec3(0,0,0), // and looks at the origin
      glm::vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
    );
    
    cam.project_view = cam.project * cam.view;
    
    mvpMatrixID = glGetUniformLocation(programID, "MVP");
    
    quit = false;
    mouse_dx = 0;
    mouse_dy = 0;
    
    cam.angle.x = 0;
    cam.angle.y = 3.14;
    cam.angle.z = 0;
    
    cam.pos = glm::vec3(0, 0, 3);
    
    
    return true;
  }
  
  void calcMatrixFromInput() {
    float mouse_speed = 0.005f * 16;
    float delta_time = 1.0 / 60;
    
    cam.angle.x += mouse_dx * mouse_speed * delta_time;
    cam.angle.y += mouse_dy * mouse_speed * delta_time;
    
    mouse_dx = 0;
    mouse_dy = 0;
    
    cam.direction = glm::vec3(
      cos(cam.angle.y) * sin(cam.angle.x),
      sin(cam.angle.y),
      cos(cam.angle.y) * cos(cam.angle.x)
    );
    
    // Right vector
    cam.right = glm::vec3(
      sin(cam.angle.x - 3.14f/2.0f),
      0,
      cos(cam.angle.x - 3.14f/2.0f)
    );
    
    glm::vec3 up = glm::cross(cam.right, cam.direction);
        
    cam.view = glm::lookAt(
      cam.pos,
      cam.pos + cam.direction,
      up
    );
    
    cam.project_view = cam.project * cam.view;
  }
  
  void flipScreen() {
    SDL_GL_SwapBuffers();
  }
  
  void render() {
    glUseProgram(programID);
  }
  
  
  void testInit();
  void test();
  
  GLuint loadShaders(const char * vertex_file_path, const char* fragment_file_path);
  
  void renderActor(Actor& a) {
    glm::mat4x4 mvp = cam.project_view * a.mat; 
    glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);
    a.render();
  }
  
  float deltaTime;
  
  void handleKeys();
  
  ~Engine() {
    SDL_Quit();
  }
};

// Credits go to http://www.opengl-tutorial.org/ for this function
GLuint Engine::loadShaders(const char * vertex_file_path, const char* fragment_file_path) {

    // Create the shaders
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    // Read the Vertex Shader code from the file
    std::string VertexShaderCode;
    std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
    if(VertexShaderStream.is_open()) {
        std::string Line = "";
        while(getline(VertexShaderStream, Line))
            VertexShaderCode += "\n" + Line;
        VertexShaderStream.close();
    } else {
        printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
        return 0;
    }

    // Read the Fragment Shader code from the file
    std::string FragmentShaderCode;
    std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
    if(FragmentShaderStream.is_open()) {
        std::string Line = "";
        while(getline(FragmentShaderStream, Line))
            FragmentShaderCode += "\n" + Line;
        FragmentShaderStream.close();
    }

    GLint Result = GL_FALSE;
    int InfoLogLength;


    // Compile Vertex Shader
    printf("Compiling shader: %s\n", vertex_file_path);
    char const * VertexSourcePointer = VertexShaderCode.c_str();
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
    glCompileShader(VertexShaderID);

    // Check Vertex Shader
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ) {
        std::vector<char> VertexShaderErrorMessage(InfoLogLength+1);
        glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
        printf("%s\n", &VertexShaderErrorMessage[0]);
    }



    // Compile Fragment Shader
    printf("Compiling shader: %s\n", fragment_file_path);
    char const * FragmentSourcePointer = FragmentShaderCode.c_str();
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
    glCompileShader(FragmentShaderID);

    // Check Fragment Shader
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ) {
        std::vector<char> FragmentShaderErrorMessage(InfoLogLength+1);
        glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
        printf("%s\n", &FragmentShaderErrorMessage[0]);
    }



    // Link the program
    printf("Linking program\n");
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    // Check the program
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ) {
        std::vector<char> ProgramErrorMessage(InfoLogLength+1);
        glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
        printf("%s\n", &ProgramErrorMessage[0]);
    }


    glDetachShader(ProgramID, VertexShaderID);
    glDetachShader(ProgramID, FragmentShaderID);

    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);
    
    return ProgramID;
}

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
}

void Engine::test() {
  
}

void Engine::handleKeys() {
  SDL_Event event;
  
  /* Grab all the events off the queue. */
  while(SDL_PollEvent(&event)) {
    
    switch(event.type) {
      case SDL_MOUSEMOTION:
	if(lockMouse) {
	  mouse_dx += event.motion.xrel;
	  mouse_dy += event.motion.yrel;
	}
        break;
      
      case SDL_KEYDOWN:
        /* Handle key presses. */
        
        switch(event.key.keysym.sym) {
          case SDLK_ESCAPE:
            quit = true;
            break;
	    
	  case SDLK_TAB:
	    lockMouse = !lockMouse;
	    
	    if(lockMouse) {
	      // Relative mouse mode
	      SDL_WM_GrabInput(SDL_GRAB_ON);
	      SDL_ShowCursor(SDL_DISABLE);
	    }
	    else {
	      // Relative mouse mode
	      SDL_WM_GrabInput(SDL_GRAB_OFF);
	      SDL_ShowCursor(SDL_ENABLE);
	    }
	    
	    break;
            
          default:
            keyMap[event.key.keysym.sym] = true;
            break;
        }
        
        break;
        
        
      case SDL_KEYUP:
        keyMap[event.key.keysym.sym] = false;
        break;
      case SDL_QUIT:
        /* Handle quit requests (like Ctrl-c). */
        quit = true;
        break;
    }
    
  }
  
  if(keyDown(SDLK_w)) {
    cam.pos += cam.direction * deltaTime * 3.0f;
  }
  else if(keyDown(SDLK_s)) {
    cam.pos -= cam.direction * deltaTime * 3.0f;
  }
  
  if(keyDown(SDLK_a)) {
    cam.pos -= cam.right * deltaTime * 3.0f;
  }
  else if(keyDown(SDLK_d)) {
    cam.pos += cam.right * deltaTime * 3.0f;
  }
}

int main(int argc, char *argv[]) {
  Engine engine;
  
  if (!engine.init(640, 480)) {
    fprintf(stderr,  "Failed to initialize engine\n");
    SDL_Quit();
    return 0;
  }
  
  printf("Engine initialized successfully\n");
  
  engine.testInit();
  
  Actor actor;
  actor.model = new Model;
  
  
  while(!engine.quit) {
    /* Process incoming events. */
    
    engine.deltaTime = 1.0 / 60.0;
    
    /* Draw the screen. */
    //draw_screen( );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    engine.handleKeys();
    engine.calcMatrixFromInput();
    
    engine.render();
    engine.renderActor(actor);
    
    //SDL_Delay(1);
    
    engine.flipScreen();
  }
}
