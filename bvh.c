#include "bvh.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define BVH_LEAF_SIZE 8
#define BVH_STACK_SIZE 64
#define BVH_EPSILON 1.0e-9f

static const MESH_T *g_build_mesh;
static int g_build_axis;

static void aabb_init(AABB_T *b) {
  b->min = VEC3_set(1.0e30f, 1.0e30f, 1.0e30f);
  b->max = VEC3_set(-1.0e30f, -1.0e30f, -1.0e30f);
}

static void aabb_expand(AABB_T *b, VEC3_T p) {
  if (p.x < b->min.x) b->min.x = p.x;
  if (p.y < b->min.y) b->min.y = p.y;
  if (p.z < b->min.z) b->min.z = p.z;
  if (p.x > b->max.x) b->max.x = p.x;
  if (p.y > b->max.y) b->max.y = p.y;
  if (p.z > b->max.z) b->max.z = p.z;
}

static VEC3_T tri_centroid(const MESH_T *mesh, int ti) {
  const TRIANGLE_T *tr = &mesh->triangles[ti];
  VEC3_T a = mesh->vertices[tr->v[0]];
  VEC3_T b = mesh->vertices[tr->v[1]];
  VEC3_T c = mesh->vertices[tr->v[2]];
  return VEC3_scale(VEC3_add(VEC3_add(a, b), c), 1.0f / 3.0f);
}

static AABB_T tri_aabb(const MESH_T *mesh, int ti) {
  const TRIANGLE_T *tr = &mesh->triangles[ti];
  AABB_T box;
  aabb_init(&box);
  aabb_expand(&box, mesh->vertices[tr->v[0]]);
  aabb_expand(&box, mesh->vertices[tr->v[1]]);
  aabb_expand(&box, mesh->vertices[tr->v[2]]);
  return box;
}

static float centroid_axis(VEC3_T c, int axis) {
  if (axis == 0) return c.x;
  if (axis == 1) return c.y;
  return c.z;
}

static int cmp_tri(const void *pa, const void *pb) {
  int ia = *(const int *)pa;
  int ib = *(const int *)pb;
  float ca = centroid_axis(tri_centroid(g_build_mesh, ia), g_build_axis);
  float cb = centroid_axis(tri_centroid(g_build_mesh, ib), g_build_axis);

  if (ca < cb) return -1;
  if (ca > cb) return 1;
  return 0;
}

static int build_node(BVH_T *bvh, const MESH_T *mesh, int start, int count) {
  int index = bvh->node_count++;
  BVH_NODE_T *node = &bvh->nodes[index];
  AABB_T box;
  AABB_T centroid_box;
  VEC3_T extent;
  int axis;
  int i;
  int mid;
  int left;
  int right;

  aabb_init(&box);
  for (i = start; i < start + count; i++) {
    AABB_T tb = tri_aabb(mesh, bvh->indices[i]);
    aabb_expand(&box, tb.min);
    aabb_expand(&box, tb.max);
  }
  node->box = box;
  node->start = start;
  node->count = count;
  node->left = -1;
  node->right = -1;

  if (count <= BVH_LEAF_SIZE) {
    return index;
  }

  aabb_init(&centroid_box);
  for (i = start; i < start + count; i++) {
    aabb_expand(&centroid_box, tri_centroid(mesh, bvh->indices[i]));
  }
  extent = VEC3_sub(centroid_box.max, centroid_box.min);
  axis = 0;
  if (extent.y > extent.x) {
    axis = 1;
  }
  if ((axis == 0 ? extent.x : extent.y) < extent.z) {
    axis = 2;
  }

  g_build_mesh = mesh;
  g_build_axis = axis;
  qsort(&bvh->indices[start], (size_t)count, sizeof(int), cmp_tri);

  mid = count / 2;
  left = build_node(bvh, mesh, start, mid);
  right = build_node(bvh, mesh, start + mid, count - mid);
  node = &bvh->nodes[index];
  node->left = left;
  node->right = right;
  node->count = 0;
  return index;
}

int BVH_build(BVH_T *bvh, const MESH_T *mesh) {
  int i;

  if (bvh == NULL) {
    return 0;
  }
  memset(bvh, 0, sizeof(*bvh));
  if (mesh == NULL || mesh->triangle_count <= 0) {
    return 0;
  }

  bvh->tri_count = mesh->triangle_count;
  bvh->node_capacity = mesh->triangle_count * 2;
  bvh->nodes =
      (BVH_NODE_T *)malloc((size_t)bvh->node_capacity * sizeof(BVH_NODE_T));
  bvh->indices =
      (int *)malloc((size_t)mesh->triangle_count * sizeof(int));
  if (bvh->nodes == NULL || bvh->indices == NULL) {
    BVH_free(bvh);
    return 0;
  }

  for (i = 0; i < mesh->triangle_count; i++) {
    bvh->indices[i] = i;
  }
  bvh->node_count = 0;
  (void)build_node(bvh, mesh, 0, mesh->triangle_count);

  if (bvh->node_count < bvh->node_capacity) {
    BVH_NODE_T *tmp = (BVH_NODE_T *)realloc(
        bvh->nodes, (size_t)bvh->node_count * sizeof(BVH_NODE_T));
    if (tmp != NULL) {
      bvh->nodes = tmp;
      bvh->node_capacity = bvh->node_count;
    }
  }
  return 1;
}

void BVH_free(BVH_T *bvh) {
  if (bvh == NULL) {
    return;
  }
  free(bvh->nodes);
  free(bvh->indices);
  memset(bvh, 0, sizeof(*bvh));
}

static VEC3_T inv_dir(VEC3_T d) {
  VEC3_T inv;
  inv.x = fabsf(d.x) > 1.0e-12f ? 1.0f / d.x : (d.x < 0.0f ? -1.0e30f : 1.0e30f);
  inv.y = fabsf(d.y) > 1.0e-12f ? 1.0f / d.y : (d.y < 0.0f ? -1.0e30f : 1.0e30f);
  inv.z = fabsf(d.z) > 1.0e-12f ? 1.0f / d.z : (d.z < 0.0f ? -1.0e30f : 1.0e30f);
  return inv;
}

static int aabb_hit(AABB_T box, VEC3_T origin, VEC3_T inv, float tmin,
                    float tmax) {
  float t0;
  float t1;
  float tmp;

  t0 = (box.min.x - origin.x) * inv.x;
  t1 = (box.max.x - origin.x) * inv.x;
  if (t0 > t1) {
    tmp = t0;
    t0 = t1;
    t1 = tmp;
  }
  tmin = fmaxf(tmin, t0);
  tmax = fminf(tmax, t1);
  if (tmax < tmin) {
    return 0;
  }

  t0 = (box.min.y - origin.y) * inv.y;
  t1 = (box.max.y - origin.y) * inv.y;
  if (t0 > t1) {
    tmp = t0;
    t0 = t1;
    t1 = tmp;
  }
  tmin = fmaxf(tmin, t0);
  tmax = fminf(tmax, t1);
  if (tmax < tmin) {
    return 0;
  }

  t0 = (box.min.z - origin.z) * inv.z;
  t1 = (box.max.z - origin.z) * inv.z;
  if (t0 > t1) {
    tmp = t0;
    t0 = t1;
    t1 = tmp;
  }
  tmin = fmaxf(tmin, t0);
  tmax = fminf(tmax, t1);
  if (tmax < tmin) {
    return 0;
  }

  return 1;
}

static int tri_hit(const MESH_T *mesh, int ti, VEC3_T origin, VEC3_T dir,
                   float tmin, float tmax, float *tout, float *uout,
                   float *vout) {
  const TRIANGLE_T *tr = &mesh->triangles[ti];
  VEC3_T v0 = mesh->vertices[tr->v[0]];
  VEC3_T v1 = mesh->vertices[tr->v[1]];
  VEC3_T v2 = mesh->vertices[tr->v[2]];
  VEC3_T e1 = VEC3_sub(v1, v0);
  VEC3_T e2 = VEC3_sub(v2, v0);
  VEC3_T pvec = VEC3_cross(dir, e2);
  float det = VEC3_dot(e1, pvec);
  float inv_det;
  VEC3_T tvec;
  float u;
  VEC3_T qvec;
  float v;
  float t;

  if (fabsf(det) < BVH_EPSILON) {
    return 0;
  }
  inv_det = 1.0f / det;
  tvec = VEC3_sub(origin, v0);
  u = VEC3_dot(tvec, pvec) * inv_det;
  if (u < 0.0f || u > 1.0f) {
    return 0;
  }
  qvec = VEC3_cross(tvec, e1);
  v = VEC3_dot(dir, qvec) * inv_det;
  if (v < 0.0f || u + v > 1.0f) {
    return 0;
  }
  t = VEC3_dot(e2, qvec) * inv_det;
  if (t < tmin || t > tmax) {
    return 0;
  }

  *tout = t;
  *uout = u;
  *vout = v;
  return 1;
}

static void fill_hit(const MESH_T *mesh, int ti, VEC3_T origin, VEC3_T dir,
                     float t, float u, float v, HIT_T *hit) {
  const TRIANGLE_T *tr = &mesh->triangles[ti];
  float w = 1.0f - u - v;
  VEC3_T n = VEC3_add(
      VEC3_add(VEC3_scale(tr->n[0], w), VEC3_scale(tr->n[1], u)),
      VEC3_scale(tr->n[2], v));
  VEC3_T geo = tr->normal;

  n = VEC3_normalize(n);
  if (VEC3_lenght(n) < 1.0e-8f) {
    n = geo;
  }
  if (VEC3_dot(n, dir) > 0.0f) {
    n = VEC3_scale(n, -1.0f);
  }
  if (VEC3_dot(geo, dir) > 0.0f) {
    geo = VEC3_scale(geo, -1.0f);
  }

  hit->t = t;
  hit->triangle = ti;
  hit->u = u;
  hit->v = v;
  hit->point = VEC3_add(origin, VEC3_scale(dir, t));
  hit->normal = n;
  hit->geo_normal = geo;
}

int BVH_intersect(const BVH_T *bvh, const MESH_T *mesh, VEC3_T origin,
                  VEC3_T dir, float tmin, float tmax, HIT_T *hit) {
  int stack[BVH_STACK_SIZE];
  int sp = 0;
  int found = 0;
  HIT_T best;
  float best_t = tmax;
  VEC3_T inv;

  if (bvh == NULL || mesh == NULL || bvh->node_count <= 0) {
    return 0;
  }
  inv = inv_dir(dir);
  stack[sp++] = 0;
  while (sp > 0) {
    int ni = stack[--sp];
    const BVH_NODE_T *node = &bvh->nodes[ni];

    if (!aabb_hit(node->box, origin, inv, tmin, best_t)) {
      continue;
    }
    if (node->left < 0) {
      int i;
      for (i = node->start; i < node->start + node->count; i++) {
        int ti = bvh->indices[i];
        float t;
        float u;
        float v;
        if (tri_hit(mesh, ti, origin, dir, tmin, best_t, &t, &u, &v)) {
          best_t = t;
          found = 1;
          fill_hit(mesh, ti, origin, dir, t, u, v, &best);
        }
      }
    } else {
      if (sp < BVH_STACK_SIZE) {
        stack[sp++] = node->left;
      }
      if (sp < BVH_STACK_SIZE) {
        stack[sp++] = node->right;
      }
    }
  }

  if (found && hit != NULL) {
    *hit = best;
  }
  return found;
}

int BVH_occluded(const BVH_T *bvh, const MESH_T *mesh, VEC3_T origin,
                 VEC3_T dir, float tmin, float tmax) {
  int stack[BVH_STACK_SIZE];
  int sp = 0;
  VEC3_T inv;

  if (bvh == NULL || mesh == NULL || bvh->node_count <= 0) {
    return 0;
  }
  inv = inv_dir(dir);
  stack[sp++] = 0;
  while (sp > 0) {
    int ni = stack[--sp];
    const BVH_NODE_T *node = &bvh->nodes[ni];

    if (!aabb_hit(node->box, origin, inv, tmin, tmax)) {
      continue;
    }
    if (node->left < 0) {
      int i;
      for (i = node->start; i < node->start + node->count; i++) {
        int ti = bvh->indices[i];
        float t;
        float u;
        float v;
        if (tri_hit(mesh, ti, origin, dir, tmin, tmax, &t, &u, &v)) {
          return 1;
        }
      }
    } else {
      if (sp < BVH_STACK_SIZE) {
        stack[sp++] = node->left;
      }
      if (sp < BVH_STACK_SIZE) {
        stack[sp++] = node->right;
      }
    }
  }
  return 0;
}
