#include <cstring>
#include <vector>
#include <algorithm>

#include "glm/glm.hpp"

struct Triangle {
  glm::vec3 v[3];
  float r, g, b;
};

struct Quad {
  glm::vec3 v[4];
  
  void triangulate(Triangle& a, Triangle& b) {
    a.v[0] = v[0];
    a.v[1] = v[1];
    a.v[2] = v[2];
    
    b.v[0] = v[3];
    b.v[1] = v[0];
    b.v[2] = v[2];
  }
};

enum {
  FACE_TOP,
  FACE_BOTTOM,
  FACE_LEFT,
  FACE_BACK,
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
      
      if(face == FACE_TOP) {
        std::reverse(q.v, q.v + 4);
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

struct TriangleRun {
  int start;
  int end;
};

template<typename T>
class Grid3D {
public:
  T* data;
  int x_size, y_size, z_size;
  float grid_dx, grid_dy, grid_dz;
  float voxel_radius;
  TriangleRun* triangle_run;
  
  
  Grid3D(int xx, int yy, int zz, float dx, float dy, float dz, T default_value) {
    data = new T[xx * yy * zz];
    triangle_run = new TriangleRun[xx * yy * zz];
    
    x_size = xx;
    y_size = yy;
    z_size = zz;
    
    grid_dx = dx;
    grid_dy = dy;
    grid_dz = dz;
    
    for(int i = 0; i < xx * yy * zz; ++i) {
      data[i] = default_value;
      triangle_run[i].start = -1;
      triangle_run[i].end = -1;
    }
    
    float r = std::max(grid_dx, std::max(grid_dy, grid_dz)) / 2.0;
    voxel_radius = sqrt(3 * r * r);
  }
  
  bool validPos(int x, int y, int z) {
    return x >= 0 && x < x_size && y >= 0 && y < y_size && z >= 0 && z < z_size;
  }
  
  T& get(int x, int y, int z) {
    return data[x + y * x_size + z * y_size * x_size];
  }
  
  bool shouldGeneratePoly(int x, int y, int z, int face, T& empty) {
    enum {
      FACE_TOP,
      FACE_BOTTOM,
      FACE_LEFT,
      FACE_BACK,
      FACE_RIGHT,
      FACE_FRONT
      
    };
    
    int offset[6][3] = {
      { 0, -1, 0 },
      { 0, 1, 0 },
      { -1, 0, 0 },
      { 0, 0, 1 },
      { 1, 0, 0 },
      { 0, 0, -1 }
    };
    
    x += offset[face][0];
    y += offset[face][1];
    z += offset[face][2];
    
    return !validPos(x, y, z) || get(x, y, z) == empty;
  }
  
  void generate(T (*eval)(int x, int y, int z, Grid3D& g)) {
    T* ptr = data;
    
    for(int z = 0; z < z_size; ++z) {
      for(int y = 0; y < y_size; ++y) {
        for(int x = 0; x < x_size; ++x) {
          *ptr = eval(x, y, z, *this);
          ++ptr;
        }
      }
    }
  }
  
  std::vector<Triangle> triangulate(T empty) {
    T* ptr = data;
    std::vector<Triangle> t;
    
    for(int z = 0; z < z_size; ++z) {
      for(int y = 0; y < y_size; ++y) {
        for(int x = 0; x < x_size; ++x) {
          if(*ptr != empty) {
            int pos = ptr - data;
            
            triangle_run[pos].start = t.size();
            triangle_run[pos].end = (int)t.size() - 1;
            
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
                t.push_back(a);
                t.push_back(b);
                triangle_run[pos].end += 2;
              }
            }
          }
          
          ++ptr;
        }
      }
    }
    
    return t;
  }
  
  
};

template<typename T>
class Grid3D_Helper {
public:
  static T generateCircle(int x, int y, int z, Grid3D<T> &g) {
    int r = std::min(g.x_size, std::min(g.y_size, g.z_size)) / 2;
    
    x = x - g.x_size / 2;
    y = y - g.y_size / 2;
    z = z - g.z_size / 2;
    
    return x * x + y * y + z * z < r * r;
  }
  
  static T generateCone(int x, int y, int z, Grid3D<T> &g) {
    int r = (std::min(g.x_size, g.z_size) / 2) * (float)y / g.y_size;
    
    x = x - g.x_size / 2;
    z = z - g.z_size / 2;
    
    return x * x + z * z < r * r;
  }
};

