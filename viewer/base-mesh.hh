#pragma once

#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>

struct BaseTraits : public OpenMesh::DefaultTraits {
  using Point  = OpenMesh::Vec3d; // the default would be Vec3f
  using Normal = OpenMesh::Vec3d;
  VertexTraits {
    double mean;              // approximated mean curvature
    double gauss;             // approximated Gaussian curvature
  };
  FaceTraits {
    size_t group;
  };
};

using BaseMesh = OpenMesh::TriMesh_ArrayKernelT<BaseTraits>;
using Vector = OpenMesh::VectorT<double,3>;
