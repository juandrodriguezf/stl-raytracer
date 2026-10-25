#ifndef STL_H
#define STL_H

#include "vector.h"

typedef struct {
  VEC3_T min;
  VEC3_T max;
} BBOX_T;

typedef struct {
  int v[3];
  VEC3_T normal;
  VEC3_T n[3];
} TRIANGLE_T;

typedef struct {
  VEC3_T *vertices;
  int vertex_count;
  TRIANGLE_T *triangles;
  int triangle_count;
  BBOX_T bbox;
  VEC3_T center;
  float radius;
} MESH_T;

int STL_load(const char *path, MESH_T *mesh);
void MESH_free(MESH_T *mesh);
void BBOX_print(const char *label, BBOX_T b);

#endif
