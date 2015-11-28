#include <cstring>
#include <vector>
#include "glm/glm.hpp"

struct Triangle {
  glm::vec3 v[3];
};

struct Quad {
  glm::vec3 v[4];
  
  void triangulate(Triangle& a, Triangle& b) {
    
  }
};

enum {
  FACE_TOP,
  FACE_BOTTOM,
  FACE_BACK,
  FACE_LEFT,
  FACE_RIGHT,
  FACE_FRONT
  
};

struct Cube {
  glm::vec3 v[8];
  float x_size, y_size, z_size;
  
  void setPos(glm::vec3 pos) {
    v[0] = pos;
    v[1] = pos + glm::vec3(0, 0, z_size);
    v[2] = pos + glm::vec3(x_size, 0, z_size);
    v[3] = pos + glm::vec3(x_size, 0, 0);
    
    for(int i = 0; i < 4; ++i) {
      v[i + 4] = v[i] + glm::vec3(0, y_size, 0);
    }
  }
  
  Quad getFace(int face) {
    Quad q;
    
    if(face <= FACE_BOTTOM) {
      for(int i = 0; i < 4; ++i) {
        q.v[i] = v[face * 4 + i];
      }
    }
    else {
      int f = face - 2;
      q.v[0] = v[f];
      q.v[1] = v[(f + 1) & 3];
      q.v[2] = v[((f + 1) & 3) + 4];
      q.v[3] = v[f + 4];
    }
    
    return q;
  }
  
};

template<typename T>
class Grid3D {
public:
  T* data;
  int x_size, y_size, z_size;
  float grid_dx, grid_dy, grid_dz;
  
  bool validPos(int x, int y, int z) {
    return x >= 0 && x < x_size && y >= 0 && y < y_size && z >= 0 && z < z_size;
  }
  
  T& get(int x, int y, int z) {
    return data[x + y * x_size + z * y_size * x_size];
  }
  
  bool shouldGeneratePoly(int x, int y, int z, int face, T& empty) {
    int offset[6][3] = {
      { 0, -1, 0 },
      { 0, 1, 0 },
      { -1, 0, 0 },
      { 1, 0, 0 },
    };
    
    x += offset[0];
    y += offset[1];
    z += offset[2];
    
    return validPos(x, y, z) && get(x, y, z) == empty;
  }
  
  std::vector<Triangle> triangulate(T empty) {
    T* ptr = data;
    
    for(int z = 0; z < z_size; ++z) {
      for(int y = 0; y < y_size; ++y) {
        for(int x = 0; x < x_size; ++x) {
          if(*ptr != empty) {
            for(int i = 0; i < 6; ++i) {
              Cube c;
              c.x_size = grid_dx;
              c.y_size = grid_dy;
              c.z_size = grid_dz;
              
              c.setPos(glm::vec3(x * grid_dx, y * grid_dy, z * grid_dz));
              
              if(shouldGeneratePoly(x, y, z, i, empty)) {
                Quad q;
                Triangle a, b;
                
                c.getFace(i).triangulate(a, b);
              }
            }
          }
        }
      }
    }
  }
  
  
};