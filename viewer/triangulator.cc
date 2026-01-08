#include <map>
#include <triangle.h>

#include "triangulator.hh"

using namespace Geometry;

void addTriangulatedFace(TriMesh &mesh, const std::vector<std::vector<size_t>> &face) {
  const auto &f = face[0];
  Point3D origin = mesh[f[0]];
  Vector3D normal = ((mesh[f[1]] - mesh[f[0]]) ^ (mesh[f[2]] - mesh[f[0]])).normalize();
  size_t di = std::abs(normal[0]) < std::abs(normal[1]) ? 0 : 1;
  if (std::abs(normal[2]) < std::abs(normal[di]))
    di = 2;
  Vector3D d(0, 0, 0); d[di] = 1;
  auto u = (normal ^ d).normalize();
  auto v = (normal ^ u).normalize();

  DoubleVector points;
  std::map<size_t, size_t> index_map, reverse_map;
  size_t pi = 0;
  for (const auto &loop : face)
    for (auto i : loop)
      if (!index_map.contains(i)) {
        index_map[i] = pi;
        reverse_map[pi] = i;
        points.push_back((mesh[i] - origin) * u);
        points.push_back((mesh[i] - origin) * v);
        pi++;
      }
  size_t n = pi;

  size_t m = 0;
  for (const auto &loop : face)
    m += loop.size();
  std::vector<int> segments(m * 2);
  DoubleVector holes(face.size() * 2);
  size_t si = 0, hi = 0;
  for (const auto &loop : face) {
    size_t k = loop.size();
    holes[hi] = 0; holes[hi+1] = 0;
    for (size_t i = 0; i < k; ++i) {
      auto j = index_map.at(loop[i]);
      segments[si++] = j;
      segments[si++] = index_map.at(loop[(i+1)%k]);
      holes[hi] += points[j*2];
      holes[hi+1] += points[j*2+1];
    }
    holes[hi] /= k; holes[hi+1] /= k;
    hi += 2;
  }

  struct triangulateio in, out;
  in.pointlist = &points[0];
  in.numberofpoints = n;
  in.numberofpointattributes = 0;
  in.pointmarkerlist = nullptr;
  in.segmentlist = &segments[0];
  in.numberofsegments = m;
  in.segmentmarkerlist = nullptr;
  in.holelist = face.size() == 1 ? nullptr : &holes[2];
  in.numberofholes = face.size() - 1;
  in.numberofregions = 0;

  out.pointlist = nullptr;
  out.pointattributelist = nullptr;
  out.pointmarkerlist = nullptr;
  out.trianglelist = nullptr;
  out.triangleattributelist = nullptr;
  out.segmentlist = nullptr;
  out.segmentmarkerlist = nullptr;

  triangulate(const_cast<char *>("pBPYYzQ"), &in, &out, (struct triangulateio *)nullptr);

  for (int i = 0; i < out.numberoftriangles; ++i)
    mesh.addTriangle(reverse_map.at(out.trianglelist[3*i]),
                     reverse_map.at(out.trianglelist[3*i+1]),
                     reverse_map.at(out.trianglelist[3*i+2]));

  trifree(reinterpret_cast<int*>(out.pointlist));
  trifree(reinterpret_cast<int*>(out.pointattributelist));
  trifree(out.pointmarkerlist);
  trifree(out.trianglelist);
  trifree(reinterpret_cast<int*>(out.triangleattributelist));
  trifree(out.segmentlist);
  trifree(out.segmentmarkerlist);
}
