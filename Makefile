all: mgbs2obj

CDGBS=../libcdgbs
EIGEN=/usr/include/eigen3
LIBGEOM=../libgeom
OPENMESH=/usr
TRIANGLE=$(CDGBS)/build/triangle_build/Triangle

FLAGS=-std=c++20 -Wall -pedantic -O3 -DNDEBUG
INCLUDES=\
	-I$(CDGBS)/include \
	-I$(LIBGEOM) \
	-I$(EIGEN) \
	-I$(OPENMESH)/include
LIBS=\
	-L$(CDGBS)/build -llibcdgbs \
	-L$(TRIANGLE) -ltriangle-api -ltriangle \
	-L$(LIBGEOM)/release -lgeom \
	-L$(OPENMESH)/lib -lOpenMeshCore

mgbs2obj: mgbs2obj.cc
	$(CXX) -o $@ $< $(FLAGS) $(INCLUDES) $(LIBS)
