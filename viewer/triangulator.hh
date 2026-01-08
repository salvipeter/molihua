#pragma once

#include <geometry.hh>

#include "base-mesh.hh"

void addTriangulatedFace(Geometry::TriMesh &mesh, const std::vector<std::vector<size_t>> &face);
