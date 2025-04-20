if (NOT CDGBS_FOUND)
  find_path(CDGBS_INCLUDE_DIR
    NAMES libcdgbs/SurfGBS.hpp
    PATH_SUFFIXES include
    DOC "The directory containing the header files WITHOUT the libcdgbs prefix"
  )
  find_library(CDGBS_LIBRARY
    NAMES libcdgbs
    PATH_SUFFIXES build
    DOC "Path to the libcdgbs library"
  )
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(CDGBS
  REQUIRED_VARS CDGBS_INCLUDE_DIR CDGBS_LIBRARY
  FOUND_VAR CDGBS_FOUND
)
