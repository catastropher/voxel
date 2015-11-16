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
  
  ~Engine() {
    SDL_Quit();
  }
};


int main(int argc, char *argv[]) {
  Engine engine;
  
  engine.init(640, 480);
  
  SDL_Delay(2000);
}
