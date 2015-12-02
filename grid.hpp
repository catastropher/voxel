#include <cstring>
#include <vector>
#include <algorithm>
#include <string>
#include <cctype>

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
  
  static int oppositeFace(int f) {
    if(f < 2)
      return !f;
    else
      return (f % 4) + 2;
  }
  
};

struct TriangleRun {
  int start;
  int end;
  
  TriangleRun* next;
  
  TriangleRun() {
    next = NULL;
  }
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
    voxel_radius = sqrt(3 * r * r) * .75;
  }
  
  bool validPos(int x, int y, int z) {
    return x >= 0 && x < x_size && y >= 0 && y < y_size && z >= 0 && z < z_size;
  }
  
  T& get(int x, int y, int z) {
    return data[x + y * x_size + z * y_size * x_size];
  }
  
  int index(int x, int y, int z) {
    return x + y * x_size + z * y_size * x_size;
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
  
  void updateDeletedVoxelNeighbors(int x, int y, int z, std::vector<Triangle>& v, T empty) {
    int offset[6][3] = {
      { 0, -1, 0 },
      { 0, 1, 0 },
      { -1, 0, 0 },
      { 0, 0, 1 },
      { 1, 0, 0 },
      { 0, 0, -1 }
    };
    
    for(int i = 0; i < 6; ++i) {
      int xx = x + offset[i][0];
      int yy = y + offset[i][1];
      int zz = z + offset[i][2];
      
      if(validPos(xx, yy, zz) && get(xx, yy, zz) != 0 && shouldGeneratePoly(xx, yy, zz, Cube::oppositeFace(i), empty)) {
        TriangleRun* run = &triangle_run[index(xx, yy, zz)];
        
        while(run->next) {
          run = run->next;
        }
        
        TriangleRun* new_run = new TriangleRun;
        
        new_run->start = v.size();
        new_run->end = (int)v.size() - 1;
        
        Cube c;
        c.x_size = grid_dx;
        c.y_size = grid_dy;
        c.z_size = grid_dz;
        
        c.setPos(glm::vec3(xx * grid_dx, yy * grid_dy, zz * grid_dz));
        
        Quad q;
        Triangle a, b;
        
        c.getFace(Cube::oppositeFace(i)).triangulate(a, b);
        v.push_back(a);
        v.push_back(b);
        new_run->end += 2;
        
        run->next = new_run;
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
  static std::string expression;
  
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
  
  static bool isOperator(char c) {
    return c == '+' ||
      c == '-' ||
      c == '*' ||
      c == '/' ||
      c == '^' ||
      c == '=' ||
      c == '<' ||
      c == '>' ||
      c == '!' ||
      c == '@' ||
      c == '&' ||
      c == '|';
  }
  
  static float evalParenthesis(char* start, char* end) {
    return evalParenthesis(start + 1, end - 1);
  }
  
  static float evaluateExpression(char* start, char* end, int x, int y, int z, Grid3D<T> &g) {
    std::vector<float> stack;
    std::vector<std::string> tokens;
    int r = std::min(g.x_size, std::min(g.y_size, g.z_size)) / 2;
    
    while(start < end) {
      while(start < end && *start == ' ')
        ++start;
      
      if(isdigit(*start)) {
        std::string token;
        
        while(isdigit(*start) || *start == '.') {
          token += *start;
          ++start;
        }
        
        tokens.push_back(token);
      }
      else if(isOperator(*start)) {
        int offset = 1;
        
        //printf("Start: %c\n", *start);
        
        char old_start = *start;
        
        if((*start == '<' || *start == '>') && start[1] == '=') {
          ++offset;
          
          if(*start == '<') {
            *start = '!';
          }
          else {
            *start = '@';
          }
        }
        
        
        tokens.push_back(std::string(start, start + 1));
        
        *start = old_start;
        
        start += offset;
      }
      else if(isalpha(*start) || *start == '(') {
        std::string token;
        bool par = false;
        
        while(isalpha(*start) || *start == '(') {
          token += *start;
          par |= *start == '(';
          ++start;
        }
        
        if(par) {
          while(start < end && *start != ')') {
            token += *start;
            ++start;
          }
          
          if(*start != ')') {
            throw "Unmatched parenthesis in expression";
          }
          else {
            ++start;
          }
          
          token = "~" + token;
        }
        
        tokens.push_back(token);
      }
      else {
        throw "Unexpected character: " + std::string(start, start + 1);
      }
      
    }
    
    for(int i = 0; i < tokens.size(); ++i) {
      float value;
      
      if(isdigit(tokens[i][0])) {
        stack.push_back(atof(tokens[i].c_str()));
      }
      else if(isOperator(tokens[i][0])) {
        if(stack.size() < 2) {
          throw "Too few operands for operator '" + tokens[i] + "'";
        }
        
        float val2 = stack[stack.size() - 1];
        stack.pop_back();
        
        float val1 = stack[stack.size() - 1];
        stack.pop_back();
        
        switch(tokens[i][0]) {
          case '+':
            value = val1 + val2;
            break;
            
          case '-':
            value = val1 - val2;
            break;
            
          case '*':
            value = val1 * val2;
            break;
            
          case '/':
            value = val1 / val2;
            break;
            
          case '=':
            value = abs(val1 - val2) < 1;
            break;
            
          case '<':
            value = val1 < val2;
            break;
            
          case '>':
            value = val1 > val2;
            break;
            
          case '!':
            value = val1 <= val2;
            break;
            
          case '@':
            value = val1 >= val2;
            break;
            
          case '^':
            value = pow(val1, val2);
            break;
            
          case '&':
            value = val1 && val2;
            break;
            
          case '|':
            value = val1 || val2;
            break;
        }
        
        stack.push_back(value);
      }
      else if(tokens[i][0] != '~') {
        std::string t = tokens[i];
        
        if(t == "x")
          value = x;
        else if(t == "y")
          value = y;
        else if(t == "z")
          value = z;
        else if(t == "PI")
          value = 3.1415926;
        else if(t == "E")
          value = 2.71828;
        else if(t == "cx")
          value = x - r;
        else if(t == "cy")
          value = y - r;
        else if(t == "cz")
          value = z - r;
        else if(t == "r") {
          value = r;
        }
        else if(t == "sr") {
          float xx = x - r;
          float yy = y - r;
          float zz = z - r;
          
          value = sqrt(xx * xx + yy * yy + zz * zz);
        }
        else if(t == "cr") {
          float xx = x - r;
          float yy = y - r;
          float zz = z - r;
          
          value = sqrt(xx * xx + zz * zz);
        }
        else if(t == "cos") {
          float v = stack[stack.size() - 1];
          stack.pop_back();
          
          value = cos(v);
        }
        else if(t == "sin") {
          float v = stack[stack.size() - 1];
          stack.pop_back();
          
          value = cos(v);
        }
        
        else if(t == "sqrt") {
          float v = stack[stack.size() - 1];
          stack.pop_back();
          
          value = sqrt(v);
        }
        else if(t == "abs") {
          float v = stack[stack.size() - 1];
          stack.pop_back();
          
          value = abs(v);
        }
        else if(t == "not") {
          float v = stack[stack.size() - 1];
          stack.pop_back();
          
          value = !v;
        }
        
        else if(t == "sphere") {
          int r = std::min(g.x_size, std::min(g.y_size, g.z_size)) / 2;
          
          float xx = x - r;
          float yy = y - r;
          float zz = z - r;
          
          value = xx * xx + yy * yy + zz * zz < r * r;
        }
        
        stack.push_back(value);
        
      }
    }
    
    return stack[0];
  }
  
  static T evalulateVoxelExpression(int x, int y, int z, Grid3D<T>& g) {
    return evaluateExpression(&expression[0], &expression[expression.size()], x, y, z, g);
  }
  
  static T evaluateFormula(Grid3D<T>& g, std::string exp) {
    expression = exp;
    g.generate(evalulateVoxelExpression);
  }
  
};

template<typename T>
std::string Grid3D_Helper<T>::expression;