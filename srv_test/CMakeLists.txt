CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
AUX_SOURCE_DIRECTORY( . SOURCE_DIRECTORY)	
ADD_EXECUTABLE(srv_test ${SOURCE_DIRECTORY})
INCLUDE_DIRECTORIES( ../easy/src/base ../tinynet)

IF(UNIX)
	OPTION(USE_32BITS "Compile for 32bits processors(linux only)" OFF)
	#	32 bit
	IF(USE_32BITS)  
		message(STATUS "using 32bits")  
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")  
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")   
	#	64 bit
	ELSE()  
		message(STATUS "using 64bits")  
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64")  
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")    
	ENDIF(USE_32BITS)  
	SET(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
	SET(CMAKE_CXX_FLAGS "-Wl,--no-as-needed -std=c++11 -pthread") 
ELSE(WIN32)

ENDIF(UNIX)

IF(UNIX)
	TARGET_LINK_LIBRARIES(srv_test tinynet )
ELSEIF(WIN32)
	#	64 bit
	TARGET_LINK_LIBRARIES(srv_test tinynet  ws2_32)
	#	32 bit
	#TARGET_LINK_LIBRARIES(srv_test tinynet  ws2_32)
ENDIF(UNIX)

SET(EXECUTABLE_OUTPUT_PATH ../bin)

