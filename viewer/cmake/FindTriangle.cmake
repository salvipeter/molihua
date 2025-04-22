if (NOT Triangle_FOUND)
  find_path(Triangle_INCLUDE_DIR
    NAMES triangle.h triangle_api.h
    DOC "The dircetory containing the header files"
  )
  find_library(Triangle_LIBRARY
    NAMES libtriangle.a
    HINTS ${Triangle_LIB_DIR}
    DOC "Path to the Triangle library"
  )
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Triangle
  REQUIRED_VARS Triangle_INCLUDE_DIR Triangle_LIBRARY
  FOUND_VAR Triangle_FOUND
)
