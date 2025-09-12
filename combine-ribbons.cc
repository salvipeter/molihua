#include <limits>

#include <libcdgbs/SurfGBS.hpp>

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <input.mgbs> <output.mgbs>" << std::endl;
    return 1;
  }

  libcdgbs::SurfGBS surf;
  surf.readMGBS(argv[1], std::numeric_limits<double>::max(), true, 0);
  surf.writeMGBS(argv[2]);
}
