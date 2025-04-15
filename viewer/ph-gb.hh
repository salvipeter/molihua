#pragma once

#include <OpenMesh/Core/Mesh/PolyMesh_ArrayKernelT.hh>

#include "object.hh"

class PHGB : public Object {
public:
  PHGB(std::string filename);
  virtual ~PHGB();
  virtual void draw(const Visualization &vis) const override;
  virtual void drawWithNames(const Visualization &vis) const override;
  virtual Vector postSelection(int selected) override;
  virtual void movement(int selected, const Vector &pos) override;
  virtual void updateBaseMesh() override;
  virtual bool reload() override;
private:
  using CageMesh = OpenMesh::PolyMesh_ArrayKernelT<BaseTraits>;
  CageMesh cage;
  using Curve = std::vector<Vector>;
  using Ribbon = std::pair<Curve, Curve>;
  using Patch = std::vector<Ribbon>;
  std::vector<Patch> patches;
};
