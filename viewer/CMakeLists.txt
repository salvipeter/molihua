cmake_minimum_required(VERSION 3.20)
project(viewer)

option(WITH_JET_FITTING "Use jet fitting for normals" OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(OpenGL_GL_PREFERENCE LEGACY)

if(WITH_JET_FITTING)
  find_package(Eigen3 3.3 REQUIRED NO_MODULE)
  find_package(CGAL REQUIRED NO_MODULE)
endif(WITH_JET_FITTING)
find_package(OpenGL REQUIRED)
find_package(OpenMesh REQUIRED)
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
find_package(Qt6 REQUIRED COMPONENTS Core OpenGL OpenGLWidgets Widgets Xml)
find_package(QGLViewer REQUIRED)
find_package(LibGeom REQUIRED)
find_package(Triangle REQUIRED)
find_package(CDGBS REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GUILE REQUIRED guile-3.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "-Wall -Wno-volatile -fPIC -Wno-reorder")
  set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -DDEBUG -fsanitize=address")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g -DNDEBUG")
endif(CMAKE_COMPILER_IS_GNUCXX)

set(geo_HEADERS # headers processed by MOC
  domain-window.hh
  options.hh
  viewer.hh
  window.hh
  )

set(CMAKE_AUTORCC ON)
qt6_wrap_cpp(geo_HEADERS_MOC ${geo_HEADERS})

add_definitions(${Qt6Widgets_DEFINITIONS} -D_USE_MATH_DEFINES)

if(WITH_JET_FITTING)
  add_definitions(-DUSE_JET_FITTING)
endif()

include_directories(
  .
  ${CGAL_INCLUDE_DIRS}
  ${EIGEN3_INCLUDE_DIR}
  ${OPENGL_INCLUDE_DIR}
  ${OPENMESH_INCLUDE_DIR}
  ${EIGEN3_INCLUDE_DIR}
  ${Qt6OpenGL_INCLUDE_DIRS}
  ${Qt6Widgets_INCLUDE_DIRS}
  ${Qt6Xml_INCLUDE_DIRS}
  ${QGLVIEWER_INCLUDE_DIR}
  ${LIBGEOM_INCLUDE_DIR}
  ${TRIANGLE_INCLUDE_DIR}
  ${CDGBS_INCLUDE_DIR}
  ${GUILE_INCLUDE_DIRS}
  )

add_executable(viewer WIN32
  ${geo_HEADERS_MOC}
  textures.qrc
  domain-window.cc
  main.cc
  object.cc
  options.cc
  ph-gb.cc
  viewer.cc
  visualization.cc
  window.cc
  )

if(WITH_JET_FITTING)
  target_sources(viewer PUBLIC jet-wrapper.cc)
endif(WITH_JET_FITTING)

if(UNIX)
  add_custom_command(OUTPUT tags
    COMMAND etags *.hh *.cc
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
  target_sources(viewer PRIVATE tags)
endif(UNIX)

target_link_libraries(viewer PUBLIC
  ${OPENGL_LIBRARIES}
  ${OPENMESH_LIBRARIES}
  Qt6::Core
  Qt6::OpenGL
  Qt6::OpenGLWidgets
  Qt6::Widgets
  Qt6::Xml
  ${QGLVIEWER_LIBRARY}
  ${LIBGEOM_LIBRARY}
  ${CDGBS_LIBRARY}
  ${Triangle_LIBRARY}
  ${GUILE_LIBRARIES}
  gmp
  )
