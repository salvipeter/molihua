if (NOT Triangle_FOUND)
  find_path(Triangle_INCLUDE_DIR
    NAMES triangle.h triangle_api.h
    DOC "The dircetory containing the header files"
  )
  find_path(Triangle_LIBRARY
    NAMES libtriangle.a
    DOC "Path to the Triangle library"
  )
  find_path(Triangle_API
    NAMES libtriangle-api.a
    DOC "Path to the Triangle API library"
  )
  set(Triangle_LIBRARIES ${Triangle_API} ${Triangle_LIBRARY})
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Triangle
  REQUIRED_VARS Triangle_INCLUDE_DIR Triangle_LIBRARIES
  FOUND_VAR Triangle_FOUND
)
