# Sets:
# GOOGLE_PERFTOOLS_INCLUDE_DIR  = where google/profiler.h can be found
# GOOGLE_PERFTOOLS_LIBRARY      = the library to link against
# CF_HAVE_GOOGLE_PERFTOOLS      = set to true after finding the library

OPTION ( CF_SKIP_GOOGLE_PERFTOOLS "Skip search for google-perftools" OFF )

IF ( NOT CF_SKIP_GOOGLE_PERFTOOLS )

  SET_TRIAL_INCLUDE_PATH ("") # clear include search path
  SET_TRIAL_LIBRARY_PATH ("") # clear library search path

  ADD_TRIAL_INCLUDE_PATH( ${GOOGLE_PERFTOOLS_ROOT}/include )
  ADD_TRIAL_INCLUDE_PATH( $ENV{GOOGLE_PERFTOOLS_ROOT}/include )

  FIND_PATH(GOOGLE_PERFTOOLS_INCLUDE_DIR google/profiler.h ${TRIAL_INCLUDE_PATHS}  NO_DEFAULT_PATH)
  FIND_PATH(GOOGLE_PERFTOOLS_INCLUDE_DIR google/profiler.h )

  ADD_TRIAL_LIBRARY_PATH(    ${GOOGLE_PERFTOOLS_ROOT}/lib )
  ADD_TRIAL_LIBRARY_PATH( $ENV{GOOGLE_PERFTOOLS_ROOT}/lib )

  FIND_LIBRARY(GOOGLE_PERFTOOLS_PROFILER_LIB profiler ${TRIAL_LIBRARY_PATHS} NO_DEFAULT_PATH)
  FIND_LIBRARY(GOOGLE_PERFTOOLS_PROFILER_LIB profiler )

  FIND_LIBRARY(GOOGLE_PERFTOOLS_TCMALLOC_LIB tcmalloc ${TRIAL_LIBRARY_PATHS} NO_DEFAULT_PATH)
  FIND_LIBRARY(GOOGLE_PERFTOOLS_TCMALLOC_LIB tcmalloc )

  if ( GOOGLE_PERFTOOLS_INCLUDE_DIR AND GOOGLE_PERFTOOLS_PROFILER_LIB AND GOOGLE_PERFTOOLS_TCMALLOC_LIB )
    SET ( CF_HAVE_GOOGLE_PERFTOOLS 1 CACHE BOOL "Found google-perftools" )
  else()
    SET ( CF_HAVE_GOOGLE_PERFTOOLS 0 CACHE BOOL "Not fount google-perftools" )
  endif()

ELSE()

    SET(CF_HAVE_GOOGLE_PERFTOOLS 0 CACHE BOOL "Skipped google-perftools")

ENDIF()

SET ( GOOGLE_PERFTOOLS_LIBS ${GOOGLE_PERFTOOLS_PROFILER_LIB} ${GOOGLE_PERFTOOLS_TCMALLOC_LIB} )

MARK_AS_ADVANCED ( GOOGLE_PERFTOOLS_INCLUDE_DIR
                   GOOGLE_PERFTOOLS_LIBS
                   GOOGLE_PERFTOOLS_PROFILER_LIB
                   GOOGLE_PERFTOOLS_TCMALLOC_LIB
                   CF_HAVE_GOOGLE_PERFTOOLS )

LOG ( "CF_HAVE_GOOGLE_PERFTOOLS: [${CF_HAVE_GOOGLE_PERFTOOLS}]" )
LOGFILE ( "  GOOGLE_PERFTOOLS_INCLUDE_DIR:  [${GOOGLE_PERFTOOLS_INCLUDE_DIR}]" )
LOGFILE ( "  GOOGLE_PERFTOOLS_PROFILER_LIB: [${GOOGLE_PERFTOOLS_PROFILER_LIB}]" )
LOGFILE ( "  GOOGLE_PERFTOOLS_TCMALLOC_LIB: [${GOOGLE_PERFTOOLS_TCMALLOC_LIB}]" )
LOGFILE ( "  GOOGLE_PERFTOOLS_LIBS:         [${GOOGLE_PERFTOOLS_LIBS}]" )

