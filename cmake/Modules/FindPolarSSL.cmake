# - find Polar SSL
# PolarSSL_INCLUDE_DIR - Where to find Polar SSL header files (directory)
# PolarSSL_LIBRARIES - Polar SSL libraries
# PolarSSL_LIBRARY_RELEASE - Where the release library is
# PolarSSL_LIBRARY_DEBUG - Where the debug library is
# PolarSSL_FOUND - Set to TRUE if we found everything (library, includes and executable)

IF( PolarSSL_INCLUDE_DIR AND PolarSSL_LIBRARY_RELEASE AND PolarSSL_LIBRARY_DEBUG )
    SET(PolarSSL_FIND_QUIETLY TRUE)
ENDIF( PolarSSL_INCLUDE_DIR AND PolarSSL_LIBRARY_RELEASE AND PolarSSL_LIBRARY_DEBUG )

IF(WIN32)
FIND_PATH( PolarSSL_INCLUDE_DIR polarssl/aes.h HINTS "C:/MinGW/include" )
ELSE()
FIND_PATH( PolarSSL_INCLUDE_DIR polarssl/aes.h  )
ENDIF()

FIND_LIBRARY(PolarSSL_LIBRARY_RELEASE NAMES polarssl )

FIND_LIBRARY(PolarSSL_LIBRARY_DEBUG NAMES polarssl HINTS /usr/lib/debug/usr/lib/ )

IF( PolarSSL_LIBRARY_RELEASE OR PolarSSL_LIBRARY_DEBUG AND PolarSSL_INCLUDE_DIR )
	SET( PolarSSL_FOUND TRUE )
ENDIF( PolarSSL_LIBRARY_RELEASE OR PolarSSL_LIBRARY_DEBUG AND PolarSSL_INCLUDE_DIR )

IF( PolarSSL_LIBRARY_DEBUG AND PolarSSL_LIBRARY_RELEASE )
	# if the generator supports configuration types then set
	# optimized and debug libraries, or if the CMAKE_BUILD_TYPE has a value
	IF( CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE )
		SET( PolarSSL_LIBRARIES optimized ${PolarSSL_LIBRARY_RELEASE} debug ${PolarSSL_LIBRARY_DEBUG} )
	ELSE( CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE )
	# if there are no configuration types and CMAKE_BUILD_TYPE has no value
	# then just use the release libraries
		SET( PolarSSL_LIBRARIES ${PolarSSL_LIBRARY_RELEASE} )
	ENDIF( CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE )
ELSEIF( PolarSSL_LIBRARY_RELEASE )
	SET( PolarSSL_LIBRARIES ${PolarSSL_LIBRARY_RELEASE} )
ELSE( PolarSSL_LIBRARY_DEBUG AND PolarSSL_LIBRARY_RELEASE )
	SET( PolarSSL_LIBRARIES ${PolarSSL_LIBRARY_DEBUG} )
ENDIF( PolarSSL_LIBRARY_DEBUG AND PolarSSL_LIBRARY_RELEASE )

IF( PolarSSL_FOUND )
	IF( NOT PolarSSL_FIND_QUIETLY )
		MESSAGE( STATUS "Found PolarSSL header file in ${PolarSSL_INCLUDE_DIR}")
		MESSAGE( STATUS "Found PolarSSL libraries: ${PolarSSL_LIBRARIES}")
	ENDIF( NOT PolarSSL_FIND_QUIETLY )
ELSE(PolarSSL_FOUND)
	IF( PolarSSL_FIND_REQUIRED)
		MESSAGE( FATAL_ERROR "Could not find PolarSSL" )
	ELSE( PolarSSL_FIND_REQUIRED)
		MESSAGE( STATUS "Optional package PolarSSL was not found" )
	ENDIF( PolarSSL_FIND_REQUIRED)
ENDIF(PolarSSL_FOUND)
