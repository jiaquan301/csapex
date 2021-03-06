# Get package location hint from environment variable (if any)
if(NOT CSAPEX_ROOT_DIR AND DEFINED ENV{CSAPEX_ROOT_DIR})
  set(CSAPEX_ROOT_DIR "$ENV{CSAPEX_ROOT_DIR}" CACHE PATH
      "csapex base directory location (optional, used for nonstandard installation paths)")
endif()

# Search path for nonstandard package locations
if(CSAPEX_ROOT_DIR)
  set(CSAPEX_INCLUDE_PATH PATHS "${CSAPEX_ROOT_DIR}/include" NO_DEFAULT_PATH)
  set(CSAPEX_LIBRARY_PATH PATHS "${CSAPEX_ROOT_DIR}/lib"     NO_DEFAULT_PATH)
endif()

# Find headers and libraries
find_path(CSAPEX_INCLUDE_DIR NAMES CSAPEX.h PATH_SUFFIXES "csapex" ${CSAPEX_INCLUDE_PATH})
find_library(CSAPEX_LIBRARY  NAMES CSAPEX   PATH_SUFFIXES "csapex" ${CSAPEX_LIBRARY_PATH})

mark_as_advanced(CSAPEX_INCLUDE_DIR
                 CSAPEX_LIBRARY)

# Output variables generation
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CSAPEX DEFAULT_MSG CSAPEX_LIBRARY
                                                      CSAPEX_INCLUDE_DIR)

set(CSAPEX_FOUND ${CSAPEX_FOUND}) # Enforce case-correctness: Set appropriately cased variable...
unset(CSAPEX_FOUND) # ...and unset uppercase variable generated by find_package_handle_standard_args

if(CSAPEX_FOUND)
  set(CSAPEX_INCLUDE_DIRS ${CSAPEX_INCLUDE_DIR})
  set(csapex_LIBRARIES ${CSAPEX_LIBRARY})
endif()
