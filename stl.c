#include "stl.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  VEC3_T p[3];
} RAW_TRI;

typedef struct {
  int x;
  int y;
  int z;
  int index;
} VHASH_T;

static uint32_t read_u32(FILE *f) {
  unsigned char b[4];
  if (fread(b, 1, 4, f) != 4) {
    return 0;
  }
  return ((uint32_t)b[0]) | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) |
         ((uint32_t)b[3] << 24);
}

static uint16_t read_u16(FILE *f) {
  unsigned char b[2];
  if (fread(b, 1, 2, f) != 2) {
    return 0;
  }
  return (uint16_t)(((uint16_t)b[0]) | ((uint16_t)b[1] << 8));
}

static float read_f32(FILE *f) {
  uint32_t u = read_u32(f);
  float v;
  memcpy(&v, &u, sizeof(v));
  return v;
}

static void bbox_init(BBOX_T *b) {
  b->min = VEC3_set(1.0e30f, 1.0e30f, 1.0e30f);
  b->max = VEC3_set(-1.0e30f, -1.0e30f, -1.0e30f);
}

static void bbox_expand(BBOX_T *b, VEC3_T p) {
  if (p.x < b->min.x) b->min.x = p.x;
  if (p.y < b->min.y) b->min.y = p.y;
  if (p.z < b->min.z) b->min.z = p.z;
  if (p.x > b->max.x) b->max.x = p.x;
  if (p.y > b->max.y) b->max.y = p.y;
  if (p.z > b->max.z) b->max.z = p.z;
}

static uint32_t hash_cell(int x, int y, int z) {
  return ((uint32_t)x * 73856093u) ^ ((uint32_t)y * 19349663u) ^
         ((uint32_t)z * 83492791u);
}

int STL_load(const char *path, MESH_T *mesh) {
  FILE *f = NULL;
  RAW_TRI *raw = NULL;
  VHASH_T *table = NULL;
  VEC3_T *vertices = NULL;
  TRIANGLE_T *triangles = NULL;
  int *counts = NULL;
  int *starts = NULL;
  int *list = NULL;
  int *fill = NULL;
  uint32_t triangle_total = 0;
  size_t table_size = 1;
  size_t si;
  int vertex_count = 0;
  BBOX_T raw_bbox;
  VEC3_T center;
  float radius;
  float scale;
  float max_extent;
  float tolerance;
  uint32_t t;
  int i;

  if (mesh == NULL) {
    return 0;
  }
  memset(mesh, 0, sizeof(*mesh));

  f = fopen(path, "rb");
  if (f == NULL) {
    fprintf(stderr, "No se pudo abrir el STL: %s\n", path);
    return 0;
  }

  {
    unsigned char header[80];
    long file_size;
    long expected_size;

    if (fread(header, 1, 80, f) != 80) {
      fprintf(stderr, "STL invalido: cabecera incompleta\n");
      fclose(f);
      return 0;
    }
    triangle_total = read_u32(f);
    if (ferror(f)) {
      fprintf(stderr, "STL invalido: no se pudo leer el conteo\n");
      fclose(f);
      return 0;
    }
    if (triangle_total == 0 || triangle_total > 5000000u) {
      fprintf(stderr, "STL invalido: conteo fuera de rango (%u)\n",
              (unsigned)triangle_total);
      fclose(f);
      return 0;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
      fclose(f);
      return 0;
    }
    file_size = ftell(f);
    if (file_size < 0) {
      fclose(f);
      return 0;
    }
    expected_size = 84L + 50L * (long)triangle_total;
    if (file_size != expected_size) {
      if (memcmp(header, "solid", 5) == 0) {
        fprintf(stderr,
                "El archivo parece ser STL ASCII. Conviertalo a STL binario.\n");
      } else {
        fprintf(stderr,
                "STL invalido: tamano %ld, esperado %ld para %u triangulos\n",
                file_size, expected_size, (unsigned)triangle_total);
      }
      fclose(f);
      return 0;
    }
    if (fseek(f, 84L, SEEK_SET) != 0) {
      fclose(f);
      return 0;
    }
  }

  raw = (RAW_TRI *)malloc((size_t)triangle_total * sizeof(RAW_TRI));
  if (raw == NULL) {
    fprintf(stderr, "Sin memoria para leer el STL\n");
    fclose(f);
    return 0;
  }

  bbox_init(&raw_bbox);
  for (t = 0; t < triangle_total; t++) {
    int k;
    (void)read_f32(f);
    (void)read_f32(f);
    (void)read_f32(f);
    for (k = 0; k < 3; k++) {
      raw[t].p[k].x = read_f32(f);
      raw[t].p[k].y = read_f32(f);
      raw[t].p[k].z = read_f32(f);
      bbox_expand(&raw_bbox, raw[t].p[k]);
    }
    (void)read_u16(f);
  }
  if (ferror(f) || feof(f)) {
    fprintf(stderr, "Error de lectura en el STL\n");
    fclose(f);
    free(raw);
    return 0;
  }
  fclose(f);
  f = NULL;

  max_extent = fmaxf(raw_bbox.max.x - raw_bbox.min.x,
                     fmaxf(raw_bbox.max.y - raw_bbox.min.y,
                           raw_bbox.max.z - raw_bbox.min.z));
  if (max_extent <= 0.0f) {
    fprintf(stderr, "STL invalido: geometria sin volumen\n");
    free(raw);
    return 0;
  }
  tolerance = max_extent * 1.0e-6f;
  if (tolerance < 1.0e-7f) {
    tolerance = 1.0e-7f;
  }

  while (table_size < (size_t)triangle_total * 6u) {
    table_size <<= 1;
  }
  table = (VHASH_T *)malloc(table_size * sizeof(VHASH_T));
  vertices = (VEC3_T *)malloc((size_t)triangle_total * 3u * sizeof(VEC3_T));
  triangles = (TRIANGLE_T *)calloc((size_t)triangle_total, sizeof(TRIANGLE_T));
  if (table == NULL || vertices == NULL || triangles == NULL) {
    fprintf(stderr, "Sin memoria para procesar el STL\n");
    goto fail;
  }
  for (si = 0; si < table_size; si++) {
    table[si].x = 0;
    table[si].y = 0;
    table[si].z = 0;
    table[si].index = -1;
  }

  for (t = 0; t < triangle_total; t++) {
    int k;
    for (k = 0; k < 3; k++) {
      VEC3_T p = raw[t].p[k];
      int ix = (int)floorf((p.x - raw_bbox.min.x) / tolerance + 0.5f);
      int iy = (int)floorf((p.y - raw_bbox.min.y) / tolerance + 0.5f);
      int iz = (int)floorf((p.z - raw_bbox.min.z) / tolerance + 0.5f);
      uint32_t h = hash_cell(ix, iy, iz);
      size_t slot = (size_t)h & (table_size - 1u);

      while (table[slot].index >= 0) {
        if (table[slot].x == ix && table[slot].y == iy &&
            table[slot].z == iz) {
          break;
        }
        slot = (slot + 1u) & (table_size - 1u);
      }
      if (table[slot].index < 0) {
        table[slot].x = ix;
        table[slot].y = iy;
        table[slot].z = iz;
        table[slot].index = vertex_count;
        vertices[vertex_count] = p;
        vertex_count++;
      }
      triangles[t].v[k] = table[slot].index;
    }
  }

  free(raw);
  raw = NULL;
  free(table);
  table = NULL;

  center = VEC3_scale(VEC3_add(raw_bbox.min, raw_bbox.max), 0.5f);
  radius = 0.5f * VEC3_lenght(VEC3_sub(raw_bbox.max, raw_bbox.min));
  if (radius <= 0.0f) {
    fprintf(stderr, "STL invalido: radio nulo\n");
    goto fail;
  }
  scale = 1.0f / radius;
  for (i = 0; i < vertex_count; i++) {
    vertices[i] = VEC3_scale(VEC3_sub(vertices[i], center), scale);
  }
  {
    VEC3_T *tmp =
        (VEC3_T *)realloc(vertices, (size_t)vertex_count * sizeof(VEC3_T));
    if (tmp != NULL) {
      vertices = tmp;
    }
  }

  bbox_init(&mesh->bbox);
  for (i = 0; i < vertex_count; i++) {
    bbox_expand(&mesh->bbox, vertices[i]);
  }

  for (t = 0; t < triangle_total; t++) {
    VEC3_T v0 = vertices[triangles[t].v[0]];
    VEC3_T v1 = vertices[triangles[t].v[1]];
    VEC3_T v2 = vertices[triangles[t].v[2]];
    VEC3_T n = VEC3_cross(VEC3_sub(v1, v0), VEC3_sub(v2, v0));
    if (VEC3_lenght(n) < 1.0e-12f) {
      n = VEC3_set(0.0f, 1.0f, 0.0f);
    } else {
      n = VEC3_normalize(n);
    }
    triangles[t].normal = n;
  }

  counts = (int *)calloc((size_t)vertex_count, sizeof(int));
  starts = (int *)malloc(((size_t)vertex_count + 1u) * sizeof(int));
  list = (int *)malloc((size_t)triangle_total * 3u * sizeof(int));
  fill = (int *)malloc((size_t)vertex_count * sizeof(int));
  if (counts == NULL || starts == NULL || list == NULL || fill == NULL) {
    fprintf(stderr, "Sin memoria para calcular normales\n");
    goto fail;
  }
  for (t = 0; t < triangle_total; t++) {
    int k;
    for (k = 0; k < 3; k++) {
      counts[triangles[t].v[k]]++;
    }
  }
  starts[0] = 0;
  for (i = 0; i < vertex_count; i++) {
    starts[i + 1] = starts[i] + counts[i];
    fill[i] = starts[i];
  }
  for (t = 0; t < triangle_total; t++) {
    int k;
    for (k = 0; k < 3; k++) {
      int v = triangles[t].v[k];
      list[fill[v]] = (int)t;
      fill[v]++;
    }
  }
  for (t = 0; t < triangle_total; t++) {
    int k;
    for (k = 0; k < 3; k++) {
      int v = triangles[t].v[k];
      VEC3_T acc = VEC3_zero();
      int j;
      for (j = starts[v]; j < starts[v + 1]; j++) {
        int face = list[j];
        if (VEC3_dot(triangles[face].normal, triangles[t].normal) >= 0.5f) {
          acc = VEC3_add(acc, triangles[face].normal);
        }
      }
      if (VEC3_lenght(acc) < 1.0e-12f) {
        acc = triangles[t].normal;
      }
      triangles[t].n[k] = VEC3_normalize(acc);
    }
  }

  free(counts);
  free(starts);
  free(list);
  free(fill);

  mesh->vertices = vertices;
  mesh->vertex_count = vertex_count;
  mesh->triangles = triangles;
  mesh->triangle_count = (int)triangle_total;
  mesh->center = VEC3_zero();
  mesh->radius = 1.0f;
  return 1;

fail:
  if (f != NULL) {
    fclose(f);
  }
  free(raw);
  free(table);
  free(vertices);
  free(triangles);
  free(counts);
  free(starts);
  free(list);
  free(fill);
  return 0;
}

void MESH_free(MESH_T *mesh) {
  if (mesh == NULL) {
    return;
  }
  free(mesh->vertices);
  free(mesh->triangles);
  memset(mesh, 0, sizeof(*mesh));
}

void BBOX_print(const char *label, BBOX_T b) {
  printf("%s min=(%.6f, %.6f, %.6f) max=(%.6f, %.6f, %.6f)\n", label, b.min.x,
         b.min.y, b.min.z, b.max.x, b.max.y, b.max.z);
}
