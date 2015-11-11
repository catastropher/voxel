#include <cstring>

template<typename T>
class Grid3D {
private:
  T* data;
  int x_size, y_size, z_size;
  float dx, dy, dz;
  T empty;
  
  size_t dataUsage() {
    return x_size * y_size * z_size * sizeof(T);
  }
  
  
public:
  Grid3D(int xx_size, int yy_size, int zz_size) : x_size(xx_size),
    y_size(yy_size), z_size(zz_size) {
      
    data = new T[x_size * y_size * z_size];
  }
  
  Grid3D(Grid3D& g) {
    x_size = g.x_size;
    y_size = g.y_size;
    z_size = g.z_size;
    
    dx = g.dx;
    dy = g.dy;
    dz = g.dz;
    
    data = new T[x_size * y_size * z_size];
    memcpy(data, g.data, dataUsage());
  }
  
  ~Grid3D() {
    delete [] data;
  }
  
  T& at(int x, int y, int z) {
    return data[x + y * y_size + z * y_size * z_size];
  }
};
  