#ifndef vector_H
#define vector_H

typedef struct {
  float x,y,z;
} VEC3_T;

/*
typedef struct VEC3_T VEC3_T;
struct VEC3_T {
  float x,y,z;
};

typedef struct VEC3_T VEC3_T;
struct VEC3_T {
  float e[3];
};

typedef VEC3_T[3];
*/

VEC3_T VEC3_add(VEC3_T a, VEC3_T b);
VEC3_T VEC3_sub(VEC3_T a, VEC3_T b);
float VEC3_dot(VEC3_T a, VEC3_T b);
VEC3_T VEC3_cross(VEC3_T a, VEC3_T b);
VEC3_T VEC3_normalize(VEC3_T a);
VEC3_T VEC3_scale(VEC3_T a, float b);
VEC3_T VEC3_zero();
float VEC3_lenght(VEC3_T a);
VEC3_T VEC3_set(float x, float y, float z);
float VEC3_get_x(VEC3_T a);
float VEC3_get_y(VEC3_T a);
float VEC3_get_z(VEC3_T a);
float VEC3_get_R(VEC3_T a);
float VEC3_get_G(VEC3_T a);
float VEC3_get_B(VEC3_T a);
void VEC3_print(VEC3_T a);

#endif /* EDM_H */
