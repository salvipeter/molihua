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
  bool show_solid, show_wireframe, show_offsets, show_misc_lines, show_auxiliary, transparent;
  enum class BoundaryType { NONE, CP, CURVE } boundaries;
  enum class RibbonType { NONE, NET, SURFACE } ribbons;
  enum class CageType { NONE, NET, SURFACE } cage;
  enum class ChamferType { NONE, NET, SURFACE } chamfers;

  // Ribbon
  double ribbon_hmax;
  size_t ribbon_sres, ribbon_hres;

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

  // Transparency
  double transparency;

  // Utilities
  static Vector HSV2RGB(Vector hsv);
  static Vector colorMap(double min, double max, double d);
};
