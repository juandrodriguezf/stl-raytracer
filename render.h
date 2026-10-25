#ifndef RENDER_H
#define RENDER_H

#include "bvh.h"
#include "stl.h"
#include "vector.h"

typedef struct {
  VEC3_T position;
  VEC3_T target;
  VEC3_T forward;
  VEC3_T right;
  VEC3_T up;
  float tan_half_fov;
  float aspect;
} CAMERA_T;

typedef struct {
  const MESH_T *mesh;
  const BVH_T *bvh;
  CAMERA_T camera;
  VEC3_T light;
  float ambient;
  float background;
  int width;
  int height;
  int samples;
} SCENE_T;

void CAMERA_init(CAMERA_T *camera, VEC3_T position, VEC3_T target, VEC3_T up,
                 float fov_deg, float aspect);
VEC3_T CAMERA_ray(const CAMERA_T *camera, int x, int y, int width, int height,
                  float ox, float oy);
void RENDER_frame(const SCENE_T *scene, float theta, unsigned char *pixels);

#endif
