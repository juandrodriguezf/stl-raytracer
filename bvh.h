#ifndef BVH_H
#define BVH_H

#include "stl.h"

typedef struct {
  VEC3_T min;
  VEC3_T max;
} AABB_T;

typedef struct {
  AABB_T box;
  int start;
  int count;
  int left;
  int right;
} BVH_NODE_T;

typedef struct {
  BVH_NODE_T *nodes;
  int node_count;
  int node_capacity;
  int *indices;
  int tri_count;
} BVH_T;

typedef struct {
  float t;
  int triangle;
  float u;
  float v;
  VEC3_T point;
  VEC3_T normal;
  VEC3_T geo_normal;
} HIT_T;

int BVH_build(BVH_T *bvh, const MESH_T *mesh);
void BVH_free(BVH_T *bvh);
int BVH_intersect(const BVH_T *bvh, const MESH_T *mesh, VEC3_T origin,
                  VEC3_T dir, float tmin, float tmax, HIT_T *hit);
int BVH_occluded(const BVH_T *bvh, const MESH_T *mesh, VEC3_T origin,
                 VEC3_T dir, float tmin, float tmax);

#endif
