#pragma once

#if !defined(APIENTRY) && defined(WIN32)
#define APIENTRY __stdcall
#endif

#if !defined(WINGDIAPI)
#define WINGDIAPI
#endif

#if __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "base-mesh.hh"

enum class VisType { PLAIN, MEAN, GAUSS, SLICING, ISOPHOTES };

struct Visualization {
  Visualization();

  // Flags
  VisType type;
  bool show_control_points, show_solid, show_wireframe, show_offsets, show_chamfers, show_cage;
  enum class BoundaryType { NONE, CP, CURVE } boundaries;

  // Mean curvature
  double mean_min, mean_max, mean_cutoff_ratio;
  // Gaussian curvature
  double gauss_min, gauss_max, gauss_cutoff_ratio;

  // Slicing
  Vector slicing_dir;
  double slicing_scaling;

  // Textures
  static GLuint isophote_texture, environment_texture, slicing_texture;
  GLuint current_isophote_texture;

  // Utilities
  static Vector colorMap(double min, double max, double d);
};
