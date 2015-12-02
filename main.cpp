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

struct Color {
  float r, g, b;
  
  Color randomShade() {
    float scale = (rand() % 100000) / 100000.0;
    
    return (Color) { r * scale, g * scale, b * scale };
  }
};

const Color COLOR_RED = (Color) { 1.0, 0, 0 };
const Color COLOR_GREEN = (Color) { 0, 1.0, 0 };
const Color COLOR_BLUE = (Color) { 0, 0, 1.0 };
const Color COLOR_ORANGE = (Color) { 1.0, .5468, 0 };

struct BoundSphere {
  float r;
  glm::vec3 pos;
  
  bool intersect(BoundSphere& b, glm::vec3 abs_pos, glm::vec3 abs_b_pos) {
    glm::vec3 d = (pos + abs_pos) - (b.pos + abs_b_pos);
    
    float rr = r + b.r;
    
    float dist = d.x * d.x + d.y * d.y + d.z * d.z;
    
    //std::cout << "Dist: " << sqrt(dist) << " " << rr << std::endl;
    
    return dist <= rr * rr;
  }
};

struct Vex3D {
  int x, y, z;
};

struct BoundNode {
  BoundSphere s;
  int x1, y1, z1;
  int x2, y2, z2;
  int count;
  
  float sum_x, sum_y, sum_z;
  
  int total_children;
  BoundNode* children[8];
  BoundNode* parent;
  
  bool partition(int xx1, int yy1, int zz1, int xx2, int yy2, int zz2, Grid3D<int>& g, BoundNode* node_parent, int empty);
  void print(int indent) {
    for(int i = 0; i < indent; ++i) {
      std::cout << "\t";
    }
    
    std::cout << s.r<< std::endl;
    
    for(int i = 0; i < total_children; ++i) {
      children[i]->print(indent + 1);
    }
  }
  
  ~BoundNode() {
    for(int i = 0; i < total_children; ++i)
      delete children[i];
  }
  
  int countVoxelIntersect(BoundNode* node, glm::vec3 pos, glm::vec3 node_pos, std::vector<Vex3D>& inter) {
    if(!s.intersect(node->s, pos, node_pos)) {
      return 0;
    }
    else {
      if(count == 1) {
        if(node->count == 1) {
          inter.push_back((Vex3D) { x1, y1, z1 });
          return 1;
        }
        else {
          for(int i = 0; i < node->total_children; ++i) {
            if(countVoxelIntersect(node->children[i], pos, node_pos, inter) != 0) {
              return 1;
            }
          }
          
          return 0;
        }
      }
      
      int c = 0;
      for(int i = 0; i < total_children; ++i) {
        c += children[i]->countVoxelIntersect(node, pos, node_pos, inter);
      }
      
      return c;
    }
  }
};

bool BoundNode::partition(int xx1, int yy1, int zz1, int xx2, int yy2, int zz2, Grid3D<int>& g, BoundNode* node_parent, int empty) {
  x1 = xx1;
  y1 = yy1;
  z1 = zz1;
  
  x2 = xx2;
  y2 = yy2;
  z2 = zz2;
  
  parent = node_parent;
  
  for(int i = 0; i < 8; ++i)
    children[i] = NULL;
  
  total_children = 0;
  
  if(x2 == x1 + 1 && y2 == y1 + 1 && z2 == z1 + 1) {
    if(g.get(x1, y1, z1) != empty) {
      count = 1;
      s.pos.x = (x1 + .5) * g.grid_dx;
      s.pos.y = (y1 + .5) * g.grid_dy;
      s.pos.z = (z1 + .5) * g.grid_dz;
      
      s.r = g.voxel_radius;
      
      return true;
    }
    else {
      return false;
    }
  }
  else {
    int mx = (x1 + x2) / 2;
    int my = (y1 + y2) / 2;
    int mz = (z1 + z2) / 2;
    
    int dx = mx - x1;
    int dy = my - y1;
    int dz = mz - z1;
    
    sum_x = 0;
    sum_y = 0;
    sum_z = 0;
    count = 0;
    
    
    s.r = 0;
    
    for(int x = x1; x <= mx; x += dx) {
      for(int y = y1; y <= my; y += dy) {
        for(int z = z1; z <= mz; z += dz) {
          children[total_children] = new BoundNode;
        
          if(!children[total_children]->partition(x, y, z, x + dx, y + dy, z + dz, g, this, empty)) {
            delete children[total_children];
          }
          else {
            sum_x += children[total_children]->s.pos.x;
            sum_y += children[total_children]->s.pos.y;
            sum_z += children[total_children]->s.pos.z;
            count += children[total_children]->count;
            
            ++total_children;
          }
        }
      }
    }
    
    if(total_children > 0 && count > 0) {
      s.pos.x = sum_x / total_children;
      s.pos.y = sum_y / total_children;
      s.pos.z = sum_z / total_children;
      
      for(int i = 0; i < total_children; ++i) {
        float dx = children[i]->s.pos.x - s.pos.x;
        float dy = children[i]->s.pos.y - s.pos.y;
        float dz = children[i]->s.pos.z - s.pos.z;
        float r = sqrt(dx * dx + dy * dy + dz * dz) + children[i]->s.r;
        
        s.r = std::max(s.r, r);
      }
      
      return true;
    }
    else {
      return false;
    }
  }
}
    
    
    


struct ModelTriangle {
  int v[3];
};

class Model {
private:
  std::vector<Triangle> tri;
  
public:
  GLuint vertexBuffer;
  GLuint colorBuffer;
  Grid3D<int>* grid;
  BoundNode bound_root;
  
  void createBound() {
    std::cout << "Create bounding tree" << std::endl;
    bound_root.partition(0, 0, 0, grid->x_size, grid->y_size, grid->z_size, *grid, NULL, 0);
    
    std::cout << bound_root.s.pos.x << " " << bound_root.s.pos.y << " " << bound_root.s.pos.z << " " << std::endl;
  }
  
  void createGrid(int xx, int yy, int zz, float dx, float dy, float dz, int default_value) {
    grid = new Grid3D<int>(xx, yy, zz, dx, dy, dz, default_value);
  }
  
  void colorModel(Color c) {
    GLfloat* color_data = new GLfloat[tri.size() * 16];
    
    for(int i = 0; i < tri.size(); ++i) {
      for(int d = 0; d < 3; ++d) {
        Color rc = c.randomShade();
        
        color_data[i * 12 + d * 4 + 0] = rc.r;
        color_data[i * 12 + d * 4 + 1] = rc.g;
        color_data[i * 12 + d * 4 + 2] = rc.b;
        color_data[i * 12 + d * 4 + 3] = 1;
      }
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * 16 * tri.size(), color_data);
    
    delete [] color_data;
  }
  
  void deleteVoxel(int x, int y, int z, Color c) {
    TriangleRun* run = &grid->triangle_run[grid->index(x, y, z)];
    GLfloat value = 0.0f;
    int& val = grid->get(x, y, z);
    
    if(val != 0) {
      while(run) {
        if(run->start >= 0 && run->end >= 0 && run->start <= run->end) {
        
          //std::cout << "Run start: " << run->start << " " << run->end << std::endl;
          
          glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
          
          for(int i = run->start; i <= run->end; ++i) {
            int offset = i * 48 + 12;
            
            glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(GLfloat), &value);
            glBufferSubData(GL_ARRAY_BUFFER, offset + 16, sizeof(GLfloat), &value);
            glBufferSubData(GL_ARRAY_BUFFER, offset + 32, sizeof(GLfloat), &value);
          }
        }
        
        run = run->next;
      }
      
      val = 0;
      int start = tri.size();
      grid->updateDeletedVoxelNeighbors(x, y, z, tri, 0);
      
      int total = tri.size() - start;
      GLfloat* color_data = new GLfloat[total * 16];
      GLfloat* vertex_data = new GLfloat[total * 12];
      
      //std::cout << "Total: " << total << std::endl;
      
      GLfloat* ptr = vertex_data;
      
      for(int i = 0; i < total; ++i) {
        for(int d = 0; d < 3; ++d) {
          Color rc = c.randomShade();
          
          color_data[i * 12 + d * 4 + 0] = rc.r;
          color_data[i * 12 + d * 4 + 1] = rc.g;
          color_data[i * 12 + d * 4 + 2] = rc.b;
          color_data[i * 12 + d * 4 + 3] = 1;
        }
        
        
        ptr[0] = tri[i + start].v[0].x;
        ptr[1] = tri[i + start].v[0].y;
        ptr[2] = tri[i + start].v[0].z;
        
        ptr[3] = tri[i + start].v[1].x;
        ptr[4] = tri[i + start].v[1].y;
        ptr[5] = tri[i + start].v[1].z;
        
        ptr[6] = tri[i + start].v[2].x;
        ptr[7] = tri[i + start].v[2].y;
        ptr[8] = tri[i + start].v[2].z;
        
        ptr += 9;
      }
      
      glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
      glBufferSubData(GL_ARRAY_BUFFER, start * 48, sizeof(GLfloat) * 16 * total, color_data);
      
      glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
      glBufferSubData(GL_ARRAY_BUFFER, start * 36, sizeof(GLfloat) * 12 * total, vertex_data);
      
    }
  }
    
  //int addTriangle(
  
  
  
  void setTriangles(std::vector<Triangle>& t) {
    tri = t;
    printf("Create model (%d triangles)\n", tri.size());
    
#if 0
    for(int i = 0; i < tri.size(); ++i) {
      for(int d = 0; d < 3; ++d) {
        printf("{ %f, %f, %f }\n", tri[i].v[d].x, tri[i].v[d].y, tri[i].v[d].z);
      }
      
      printf("\n");
    }
#endif
  
#if 0
    // An array of 3 vectors which represents 3 vertices
    static const GLfloat g_vertex_buffer_data[] = {
      -1.0f, -1.0f, 0.0f,
      1.0f, -1.0f, 0.0f,
      0.0f,  1.0f, 0.0f,
    };
#endif
    
    const int EXTRA = 10;
    
    GLfloat* vertex_buffer_data = new GLfloat[tri.size() * 9 * EXTRA];
    GLfloat* ptr = &vertex_buffer_data[0];
    
    GLfloat* color_data = new GLfloat[tri.size() * 12 * EXTRA];
    
    for(int i = 0; i < tri.size(); ++i) {
      ptr[0] = tri[i].v[0].x;
      ptr[1] = tri[i].v[0].y;
      ptr[2] = tri[i].v[0].z;
      
      ptr[3] = tri[i].v[1].x;
      ptr[4] = tri[i].v[1].y;
      ptr[5] = tri[i].v[1].z;
      
      ptr[6] = tri[i].v[2].x;
      ptr[7] = tri[i].v[2].y;
      ptr[8] = tri[i].v[2].z;
      
      ptr += 9;
    }
    
    static int count = 0;
    
    ++count;
    
    for(int i = 0; i < tri.size() * 12; ++i) {
      if((i % 4) != 3) {
        if((i % 4) == 1)
          color_data[i] = (rand() % 10000) / 10000.0;
        else
          color_data[i] = 0;
      }
      else {
        if(count == 1)
          color_data[i] = 1;
        else
          color_data[i] = 1;
      }
    }
    
    // This will identify our vertex buffer
    // Generate 1 buffer, put the resulting identifier in vertexbuffer
    glGenBuffers(1, &vertexBuffer);
    // The following commands will talk about our 'vertexbuffer' buffer
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    // Give our vertices to OpenGL.
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 9 * tri.size() * EXTRA, vertex_buffer_data, GL_STATIC_DRAW);
    
    glGenBuffers(1, &colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 12 * tri.size() * EXTRA, color_data, GL_STATIC_DRAW);
    
    delete [] color_data;
    delete [] vertex_buffer_data;
  }
  
  // Renders the model using the current transformation settings
  void render() {
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glVertexAttribPointer(
      0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
      3,                  // size
      GL_FLOAT,           // type
      GL_FALSE,           // normalized?
      0,                  // stride
      (void*)0            // array buffer offset
    );
    
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
    glVertexAttribPointer(
          1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
          4,                                // size
          GL_FLOAT,                         // type
          GL_FALSE,                         // normalized?
          0,                                // stride
           (void*)0                          // array buffer offset
    );
    
    // Draw the triangle !
    glDrawArrays(GL_TRIANGLES, 0, tri.size() * 3); // Starting from vertex 0; 3 vertices total -> 1 triangle
    glDisableVertexAttribArray(1);
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
    angle = glm::vec3(0.0f, 0.0f, 0.0f);
    mat = glm::mat4x4(1.0f);
    pos = glm::vec3(0.0f, 0.0f, 0.0f);
  }
  
  void updatePos() {
    glm::vec4 p = glm::vec4(pos.x, pos.y, pos.z, 1);
    mat[3] = p;
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
public:
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
    //glShadeModel(GL_FLAT);
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);
    
    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
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
    cam.pos += cam.direction * deltaTime * 6.0f;
  }
  else if(keyDown(SDLK_s)) {
    cam.pos -= cam.direction * deltaTime * 6.0f;
  }
  
  if(keyDown(SDLK_a)) {
    cam.pos -= cam.right * deltaTime * 6.0f;
  }
  else if(keyDown(SDLK_d)) {
    cam.pos += cam.right * deltaTime * 6.0f;
  }
}

int main(int argc, char *argv[]) {
  Engine engine;
  
  if (!engine.init(1024, 768)) {
    fprintf(stderr,  "Failed to initialize engine\n");
    SDL_Quit();
    return 0;
  }
  
  printf("Engine initialized successfully\n");
  
  engine.testInit();
  
  Triangle t = {
    {
      {-1.0f, -1.0f, 0.0f },
      { 1.0f, -1.0f, 0.0f },
      { 0.0f,  1.0f, 0.0f }
    }
  };
  
  Triangle t2 = t;
  
  for(int i = 0; i < 3; ++i) {
    t2.v[i].z += 1;
  }
  
  Actor actor;
  actor.model = new Model;
  
  actor.model->createGrid(64, 64, 64, 1, 1, 1, 1);
  Grid3D<int>* g = actor.model->grid;

  //g->generate(Grid3D_Helper<int>::generateCircle);
  //g->generate(Grid3D_Helper<int>::generateCone);
  
  std::vector<Triangle> tt = g->triangulate(0);
  actor.model->setTriangles(tt);
  actor.model->createBound();
  
  //======================================================
  
  Actor actor2;
  actor2.model = new Model;
  
  actor2.model->createGrid(16, 16, 16, 1, 1, 1, 1);
  Grid3D<int>* g2 = actor2.model->grid;
  
  g2->generate(Grid3D_Helper<int>::generateCircle);
  //g->generate(Grid3D_Helper<int>::generateCone);
  
  std::vector<Triangle> tt2 = g2->triangulate(0);
  actor2.model->setTriangles(tt2);
  actor2.model->createBound();
  
  //======================================================
  
  actor2.model->colorModel(COLOR_RED);
  
  Color colors[] = {
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_ORANGE
  };
  
  Color color = colors[0];
  
  while(!engine.quit) {
    /* Process incoming events. */
    
    
    
    for(int i = SDLK_1; i <= SDLK_4; ++i) {
      if(engine.keyDown(i)) {
        actor2.model->colorModel(colors[i - SDLK_1]);
        color = colors[i - SDLK_1];
      }
    }
    
    if(engine.keyDown(SDLK_LCTRL)) {
      std::vector<Vex3D> inter;
      
      
      actor.model->bound_root.countVoxelIntersect(&actor2.model->bound_root, actor.pos, actor2.pos, inter);
      
      for(int i = 0; i < inter.size(); ++i) {
        actor.model->deleteVoxel(inter[i].x, inter[i].y, inter[i].z, color);
      }
    }
    
    if(engine.keyDown(SDLK_RIGHT)) {
      actor2.pos.x += .1;
      actor2.updatePos();
      
    }
    if(engine.keyDown(SDLK_LEFT)) {
      actor2.pos.x -= .1;
      actor2.updatePos();
    }
    if(engine.keyDown(SDLK_UP)) {
      if(!engine.keyDown(SDLK_LSHIFT)) {
        actor2.pos.z += .1;
        actor2.updatePos();
      }
      else {
        actor2.pos.y -= .1;
        actor2.updatePos();
      }
    }
    
    if(engine.keyDown(SDLK_DOWN)) {
      if(!engine.keyDown(SDLK_LSHIFT)) {
        actor2.pos.z -= .1;
        actor2.updatePos();
      }
      else {
        actor2.pos.y += .1;
        actor2.updatePos();
      }
    }
    
    engine.deltaTime = 1.0 / 60.0;
    
    /* Draw the screen. */
    //draw_screen( );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    engine.handleKeys();
    engine.calcMatrixFromInput();
    
    engine.render();
    engine.renderActor(actor);
    engine.renderActor(actor2);
    
    //SDL_Delay(1);
    
    engine.flipScreen();
  }
}
