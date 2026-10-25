#include "bvh.h"
#include "render.h"
#include "stl.h"
#include "vector.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#define MAIN_MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define MAIN_MKDIR(path) mkdir((path), 0755)
#endif

#define MAIN_PI 3.14159265358979323846f
#define MAIN_PATH_SIZE 1024

static void print_usage(const char *program) {
  fprintf(stderr,
          "Uso: %s [-i modelo.stl] [-o carpeta] [-n frames] [-w ancho] "
          "[-H alto] [-s muestras]\n"
          "  -i  STL binario de entrada (busto.stl)\n"
          "  -o  carpeta de salida PGM (frames)\n"
          "  -n  numero de frames (120)\n"
          "  -w  ancho en pixeles (400)\n"
          "  -H  alto en pixeles (400)\n"
          "  -s  muestras por eje para antialiasing (2)\n",
          program);
}

static int parse_int(const char *text, int minimum, int maximum, int *value) {
  char *end = NULL;
  long parsed;

  errno = 0;
  parsed = strtol(text, &end, 10);
  if (errno != 0 || end == text || *end != '\0') {
    return 0;
  }
  if (parsed < (long)minimum || parsed > (long)maximum) {
    return 0;
  }
  *value = (int)parsed;
  return 1;
}

static int ensure_directory(const char *path) {
  if (MAIN_MKDIR(path) == 0) {
    return 1;
  }
  if (errno == EEXIST) {
    return 1;
  }
  fprintf(stderr, "No se pudo crear la carpeta de salida: %s\n", path);
  return 0;
}

static int write_pgm(const char *path, const unsigned char *pixels, int width,
                     int height) {
  FILE *file = NULL;
  size_t total = (size_t)width * (size_t)height;
  int ok = 1;

  file = fopen(path, "wb");
  if (file == NULL) {
    fprintf(stderr, "No se pudo crear el PGM: %s\n", path);
    return 0;
  }
  if (fprintf(file, "P5\n%d %d\n255\n", width, height) < 0) {
    ok = 0;
  }
  if (ok && fwrite(pixels, 1, total, file) != total) {
    ok = 0;
  }
  if (fclose(file) != 0) {
    ok = 0;
  }
  if (!ok) {
    fprintf(stderr, "Error al escribir el PGM: %s\n", path);
  }
  return ok;
}

int main(int argc, char **argv) {
  const char *input_path = "busto.stl";
  const char *output_dir = "frames";
  int frame_total = 120;
  int width = 400;
  int height = 400;
  int samples = 2;
  int i;
  MESH_T mesh;
  BVH_T bvh;
  CAMERA_T camera;
  SCENE_T scene;
  unsigned char *pixels;
  size_t pixel_total;
  clock_t start;
  double elapsed;

  memset(&mesh, 0, sizeof(mesh));
  memset(&bvh, 0, sizeof(bvh));
  memset(&scene, 0, sizeof(scene));

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      print_usage(argv[0]);
      return 0;
    }
    if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
      input_path = argv[++i];
    } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
      output_dir = argv[++i];
    } else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
      if (!parse_int(argv[++i], 1, 10000, &frame_total)) {
        fprintf(stderr, "Numero de frames invalido\n");
        return 2;
      }
    } else if (strcmp(argv[i], "-w") == 0 && i + 1 < argc) {
      if (!parse_int(argv[++i], 1, 8192, &width)) {
        fprintf(stderr, "Ancho invalido\n");
        return 2;
      }
    } else if (strcmp(argv[i], "-H") == 0 && i + 1 < argc) {
      if (!parse_int(argv[++i], 1, 8192, &height)) {
        fprintf(stderr, "Alto invalido\n");
        return 2;
      }
    } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
      if (!parse_int(argv[++i], 1, 16, &samples)) {
        fprintf(stderr, "Muestras invalidas\n");
        return 2;
      }
    } else {
      print_usage(argv[0]);
      return 2;
    }
  }

  if (output_dir[0] == '\0') {
    fprintf(stderr, "La carpeta de salida no puede estar vacia\n");
    return 2;
  }
  if (!ensure_directory(output_dir)) {
    return 1;
  }

  printf("Cargando %s\n", input_path);
  if (!STL_load(input_path, &mesh)) {
    return 1;
  }
  printf("Vertices: %d  Triangulos: %d\n", mesh.vertex_count,
         mesh.triangle_count);

  if (!BVH_build(&bvh, &mesh)) {
    fprintf(stderr, "No se pudo construir el BVH\n");
    MESH_free(&mesh);
    return 1;
  }

  CAMERA_init(&camera, VEC3_set(0.0f, 0.2f, 3.6f),
              VEC3_set(0.0f, 0.05f, 0.0f), VEC3_set(0.0f, 1.0f, 0.0f), 35.0f,
              (float)width / (float)height);
  scene.mesh = &mesh;
  scene.bvh = &bvh;
  scene.camera = camera;
  scene.light = VEC3_normalize(VEC3_set(-0.75f, 0.45f, 0.50f));
  scene.ambient = 0.08f;
  scene.background = 0.03f;
  scene.width = width;
  scene.height = height;
  scene.samples = samples;

  pixel_total = (size_t)width * (size_t)height;
  pixels = (unsigned char *)malloc(pixel_total);
  if (pixels == NULL) {
    fprintf(stderr, "Sin memoria para el frame\n");
    BVH_free(&bvh);
    MESH_free(&mesh);
    return 1;
  }

  printf("Renderizando %d frames de %dx%d con %dx%d muestras en %s\n",
         frame_total, width, height, samples, samples, output_dir);
  start = clock();
  for (i = 0; i < frame_total; i++) {
    char filename[MAIN_PATH_SIZE];
    float theta = 2.0f * MAIN_PI * (float)i / (float)frame_total;
    int written;

    RENDER_frame(&scene, theta, pixels);
    written = snprintf(filename, sizeof(filename), "%s/frame_%04d.pgm",
                       output_dir, i);
    if (written < 0 || (size_t)written >= sizeof(filename)) {
      fprintf(stderr, "Ruta de salida demasiado larga\n");
      free(pixels);
      BVH_free(&bvh);
      MESH_free(&mesh);
      return 1;
    }
    if (!write_pgm(filename, pixels, width, height)) {
      free(pixels);
      BVH_free(&bvh);
      MESH_free(&mesh);
      return 1;
    }
    printf("\rFrame %d/%d", i + 1, frame_total);
    fflush(stdout);
  }
  elapsed = (double)(clock() - start) / CLOCKS_PER_SEC;
  printf("\nListo en %.2f s (%.3f s/frame)\n", elapsed,
         elapsed / (double)frame_total);

  free(pixels);
  BVH_free(&bvh);
  MESH_free(&mesh);
  return 0;
}
