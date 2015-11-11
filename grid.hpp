#include <cstring>

template<typename T>
class Grid3D {
private:
  T* data;
  int x_size, y_size, z_size;
  float grid_dx, grid_dy, grid_dz;
  
  size_t dataUsage() {
    return x_size * y_size * z_size * sizeof(T);
  }
  
  size_t dataSize() {
    return x_size * y_size * z_size;
  }
  
  void setShellCells(T empty, Grid3D &g, int x, int y, int z, int dx, int dy, int dz) {
    int depth = 0;
    
    while(this->in_grid(x, y, z)) {
      if(at(x, y, z) == empty) {
	depth = 0;
      }
      else {
	++depth;
	
	if(depth == 1) {
	  g.at(x, y, z) = at(x, y, z);
	}
      }
      
      x += dx;
      y += dy;
      z += dz;
    }
  }
  
  
public:
  Grid3D(int xx_size, int yy_size, int zz_size) : x_size(xx_size),
    y_size(yy_size), z_size(zz_size) {
      
    data = new T[x_size * y_size * z_size];
  }
  
  bool inGrid(int x, int y, int z) {
    return x >= 0 && x < x_size && y >= 0 && y < y_size && z >= 0 && z < z_size;
  }
  
  Grid3D(Grid3D& g) {
    x_size = g.x_size;
    y_size = g.y_size;
    z_size = g.z_size;
    
    grid_dx = g.dx;
    grid_dy = g.dy;
    grid_dz = g.dz;
    
    data = new T[x_size * y_size * z_size];
    memcpy(data, g.data, dataUsage());
  }
  
  ~Grid3D() {
    delete [] data;
  }
  
  T& at(int x, int y, int z) {
    return data[x + y * y_size + z * y_size * z_size];
  }
  
  // Fills the entire grid with the given value
  void fill(T val) {
    for(size_t i = 0; i < dataSize(); ++i) {
      data[i] = val;
    }
  }
  
  void createShell(T empty, Grid3D& dest) {
    // Top/bottom
    for(int x = 0; x < x_size; ++x) {
      for(int z = 0; z < z_size; ++z) {
	setShellCells(dest, x, 0, z, 0, 1, 0);
	setShellCells(dest, x, y_size - 1, z, 0, -1, 0);
      }
    }
    
    // Left/right
    for(int z = 0; z < z_size; ++z) {
      for(int y = 0; y < y_size; ++y) {
	setShellCells(dest, 0, y, z, 1, 0, 0);
	setShellCells(dest, x_size - 1, y, z, -1, 0, 0);
      }
    }
    
    // Front/back
    for(int x = 0; x < x_size; ++x) {
      for(int y = 0; y < y_size; ++y) {
	setShellCells(dest, x, y, 0, 0, 0, 1);
	setShellCells(dest, x, y, z_size - 1, 0, 0, -1);
      }
    }
    
  }
};
  