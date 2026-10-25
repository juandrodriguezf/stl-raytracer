#include "vector.h"
#include <math.h>
#include <stdio.h>

VEC3_T VEC3_add(VEC3_T a, VEC3_T b) {
  return (VEC3_T){a.x + b.x, a.y + b.y, a.z + b.z};
}

VEC3_T VEC3_sub(VEC3_T a, VEC3_T b) {
  return (VEC3_T){a.x - b.x, a.y - b.y, a.z - b.z};
}

float VEC3_dot(VEC3_T a, VEC3_T b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

VEC3_T VEC3_cross(VEC3_T a, VEC3_T b) {
  return (VEC3_T){a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                  a.x * b.y - a.y * b.x};
}

VEC3_T VEC3_normalize(VEC3_T a) {
  float len = VEC3_lenght(a);
  return len > 0 ? VEC3_scale(a, 1 / len) : VEC3_zero();
}

VEC3_T VEC3_scale(VEC3_T a, float b) {
  return (VEC3_T){a.x * b, a.y * b, a.z * b};
}

VEC3_T VEC3_zero() { return (VEC3_T){0, 0, 0}; }

float VEC3_lenght(VEC3_T a) { return sqrt(VEC3_dot(a, a)); }

VEC3_T VEC3_set(float x, float y, float z) {
    return (VEC3_T){x, y, z};
    }

float VEC3_get_x(VEC3_T a) { return a.x; }

float VEC3_get_y(VEC3_T a) { return a.y; }

float VEC3_get_z(VEC3_T a) { return a.z; }

float VEC3_get_R(VEC3_T a) { return a.x; }

float VEC3_get_G(VEC3_T a) { return a.y; }

float VEC3_get_B(VEC3_T a) { return a.z; }

void VEC3_print(VEC3_T a) { printf("(%f, %f, %f)\n", a.x, a.y, a.z); }
