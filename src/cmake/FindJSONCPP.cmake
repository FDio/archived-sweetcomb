# - Try to find LibJSONCPP
# Once done this will define
#
#  LIBJSONCPP_FOUND - system has LibJSONCPP
#  LIBJSONCPP_INCLUDE_DIRS - the LibJSONCPP include directory
#  LIBJSONCPP_LIBRARIES - Link these to use LIBJSONCPP


if (LIBJSONCPP_LIBRARIES AND LIBJSONCPP_INCLUDE_DIRS)
  # in cache already
  set(LIBJSONCPP_FOUND TRUE)
else (LIBJSONCPP_LIBRARIES AND LIBJSONCPP_INCLUDE_DIRS)

  find_path(LIBJSONCPP_INCLUDE_DIR
    NAMES
      json/json.h
    PATHS
      /usr/include
      /usr/local/include
      /usr/include/jsoncpp
      /opt/local/include
      ${CMAKE_INCLUDE_PATH}
      ${CMAKE_INSTALL_PREFIX}/include
  )

  find_library(LIBJSONCPP_LIBRARY
    NAMES
      jsoncpp
    PATHS
      /usr/lib
      /usr/lib64
      /usr/lib/x86_64-linux-gnu
      /usr/local/lib
      /usr/local/lib64
      /opt/local/lib
      ${CMAKE_LIBRARY_PATH}
      ${CMAKE_INSTALL_PREFIX}/lib
  )

  if (LIBJSONCPP_INCLUDE_DIR AND LIBJSONCPP_LIBRARY)
    set(LIBJSONCPP_FOUND TRUE)
  else (LIBJSONCPP_INCLUDE_DIR AND LIBJSONCPP_LIBRARY)
    set(LIBJSONCPP_FOUND FALSE)
  endif (LIBJSONCPP_INCLUDE_DIR AND LIBJSONCPP_LIBRARY)

  set(LIBJSONCPP_INCLUDE_DIRS ${LIBJSONCPP_INCLUDE_DIR})
  set(LIBJSONCPP_LIBRARIES ${LIBJSONCPP_LIBRARY})

  # show the LIBJSONCPP_INCLUDE_DIRS and LIBJSONCPP_LIBRARIES variables only in the advanced view
  mark_as_advanced(LIBJSONCPP_INCLUDE_DIRS LIBJSONCPP_LIBRARIES)

endif (LIBJSONCPP_LIBRARIES AND LIBJSONCPP_INCLUDE_DIRS)




