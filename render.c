#include "render.h"

#include <math.h>
#include <stddef.h>

#define RENDER_PI 3.14159265358979323846f
#define RENDER_TMIN 1.0e-4f
#define RENDER_TMAX 1.0e30f
#define RENDER_SHADOW_BIAS 1.0e-3f

static VEC3_T rotate_y(VEC3_T v, float c, float s) {
  return VEC3_set(v.x * c + v.z * s, v.y, -v.x * s + v.z * c);
}

void CAMERA_init(CAMERA_T *camera, VEC3_T position, VEC3_T target, VEC3_T up,
                 float fov_deg, float aspect) {
  camera->position = position;
  camera->target = target;
  camera->forward = VEC3_normalize(VEC3_sub(target, position));
  camera->right = VEC3_normalize(VEC3_cross(camera->forward, up));
  camera->up = VEC3_cross(camera->right, camera->forward);
  camera->tan_half_fov = tanf(fov_deg * RENDER_PI / 360.0f);
  camera->aspect = aspect;
}

VEC3_T CAMERA_ray(const CAMERA_T *camera, int x, int y, int width, int height,
                  float ox, float oy) {
  float px = (2.0f * ((float)x + ox) / (float)width - 1.0f) * camera->aspect *
             camera->tan_half_fov;
  float py = (1.0f - 2.0f * ((float)y + oy) / (float)height) *
             camera->tan_half_fov;
  VEC3_T dir = VEC3_add(
      camera->forward,
      VEC3_add(VEC3_scale(camera->right, px), VEC3_scale(camera->up, py)));
  return VEC3_normalize(dir);
}

void RENDER_frame(const SCENE_T *scene, float theta, unsigned char *pixels) {
  VEC3_T origin_model;
  VEC3_T light_model;
  float c;
  float s;
  float inv_samples;
  int sample_total;
  int x;
  int y;
  int sx;
  int sy;

  if (scene == NULL || pixels == NULL || scene->samples <= 0 ||
      scene->width <= 0 || scene->height <= 0) {
    return;
  }

  c = cosf(theta);
  s = sinf(theta);
  origin_model = rotate_y(scene->camera.position, c, -s);
  light_model = rotate_y(scene->light, c, -s);
  sample_total = scene->samples * scene->samples;
  inv_samples = 1.0f / (float)sample_total;

  for (y = 0; y < scene->height; y++) {
    for (x = 0; x < scene->width; x++) {
      float acc = 0.0f;
      for (sy = 0; sy < scene->samples; sy++) {
        for (sx = 0; sx < scene->samples; sx++) {
          float ox = ((float)sx + 0.5f) / (float)scene->samples;
          float oy = ((float)sy + 0.5f) / (float)scene->samples;
          VEC3_T dir_world =
              CAMERA_ray(&scene->camera, x, y, scene->width, scene->height, ox,
                         oy);
          VEC3_T dir_model = rotate_y(dir_world, c, -s);
          HIT_T hit;
          float intensity;

          if (BVH_intersect(scene->bvh, scene->mesh, origin_model, dir_model,
                            RENDER_TMIN, RENDER_TMAX, &hit)) {
            float ndotl = VEC3_dot(hit.normal, light_model);
            float shadow = 1.0f;

            if (ndotl < 0.0f) {
              ndotl = 0.0f;
            } else if (ndotl > 1.0f) {
              ndotl = 1.0f;
            }
            if (ndotl > 0.0f) {
              VEC3_T shadow_origin =
                  VEC3_add(hit.point,
                           VEC3_scale(hit.geo_normal, RENDER_SHADOW_BIAS));
              if (BVH_occluded(scene->bvh, scene->mesh, shadow_origin,
                               light_model, RENDER_SHADOW_BIAS, RENDER_TMAX)) {
                shadow = 0.0f;
              }
            }
            intensity =
                scene->ambient + (1.0f - scene->ambient) * ndotl * shadow;
          } else {
            intensity = scene->background;
          }
          acc += intensity;
        }
      }
      acc *= inv_samples;
      if (acc < 0.0f) {
        acc = 0.0f;
      } else if (acc > 1.0f) {
        acc = 1.0f;
      }
      acc = powf(acc, 1.0f / 2.2f);
      pixels[y * scene->width + x] =
          (unsigned char)(acc * 255.0f + 0.5f);
    }
  }
}
