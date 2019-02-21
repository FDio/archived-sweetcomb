# - Try to find LibPUGIXML
# Once done this will define
#
#  LIBPUGIXML_FOUND - system has LibPUGIXML
#  LIBPUGIXML_INCLUDE_DIRS - the LibPUGIXML include directory
#  LIBPUGIXML_LIBRARIES - Link these to use LIBPUGIXML

if (LIBPUGIXML_LIBRARIES AND LIBPUGIXML_INCLUDE_DIRS)
  # in cache already
  set(LIBPUGIXML_FOUND TRUE)
else (LIBPUGIXML_LIBRARIES AND LIBPUGIXML_INCLUDE_DIRS)

  find_path(LIBPUGIXML_INCLUDE_DIR
    NAMES
      pugixml.hpp
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
      ${CMAKE_INCLUDE_PATH}
      ${CMAKE_INSTALL_PREFIX}/include
  )

  find_library(LIBPUGIXML_LIBRARY
    NAMES
      pugixml
    PATHS
      /usr/lib
      /usr/lib64
      /usr/local/lib
      /usr/local/lib64
      /opt/local/lib
      ${CMAKE_LIBRARY_PATH}
      ${CMAKE_INSTALL_PREFIX}/lib
  )

  if (LIBPUGIXML_INCLUDE_DIR AND LIBPUGIXML_LIBRARY)
    set(LIBPUGIXML_FOUND TRUE)
  else (LIBPUGIXML_INCLUDE_DIR AND LIBPUGIXML_LIBRARY)
    set(LIBPUGIXML_FOUND FALSE)
  endif (LIBPUGIXML_INCLUDE_DIR AND LIBPUGIXML_LIBRARY)

  set(LIBPUGIXML_INCLUDE_DIRS ${LIBPUGIXML_INCLUDE_DIR})
  set(LIBPUGIXML_LIBRARIES ${LIBPUGIXML_LIBRARY})

  # show the LIBPUGIXML_INCLUDE_DIRS and LIBPUGIXML_LIBRARIES variables only in the advanced view
  mark_as_advanced(LIBPUGIXML_INCLUDE_DIRS LIBPUGIXML_LIBRARIES)

endif (LIBPUGIXML_LIBRARIES AND LIBPUGIXML_INCLUDE_DIRS)



