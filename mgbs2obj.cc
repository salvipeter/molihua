#include <OpenMesh/Core/IO/MeshIO.hh>
#include <libcdgbs/SurfGBS.hpp>

int main(int argc, char **argv) {
  if (argc != 4) {
    std::cerr << "Usage: " << argv[0]
              << " <input.mgbs> <output.obj> <target_length>" << std::endl;
    return 1;
  }
  libcdgbs::Mesh mesh;
  libcdgbs::SurfGBS surf;
  surf.readMGBS(argv[1], std::atof(argv[3]));
  surf.compute_domain_boundary();
  surf.compute_domain_mesh();
  surf.compute_local_parameters();
  surf.compute_blend_functions();
  surf.evaluate_mesh(mesh);
  OpenMesh::IO::write_mesh(mesh, argv[2]);
}
