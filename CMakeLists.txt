project(final)
cmake_minimum_required(VERSION 2.8)

#include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
#conan_basic_setup()

find_package( Boost REQUIRED COMPONENTS system thread date_time filesystem regex locale chrono log_setup log program_options )

IF ( ${CMAKE_SYSTEM_NAME} MATCHES "Linux" )
    #link_directories( "/usr/lib/i386-linux-gnu"  )
    #include_directories( "/usr/lib/i386-linux-gnu"  )
    set( CMAKE_CXX_FLAGS "-finput-charset=CP1251" )
    find_package (Threads)
ENDIF ( ${CMAKE_SYSTEM_NAME} MATCHES "Linux" )

IF ( WIN32 )
    #add_definitions( -D_WIN32_WINNT=0x0501 ) 
    #add_definitions( -D_SCL_SECURE_NO_WARNINGS ) 
ENDIF ( WIN32 )

include_directories( src )
aux_source_directory( src SRC_LIST)

add_executable(${PROJECT_NAME} ${SRC_LIST} )

set_target_properties( ${PROJECT_NAME}
	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

#target_link_libraries( ${PROJECT_NAME}  ${CONAN_LIBS}  )

target_link_libraries( ${PROJECT_NAME}  ${Boost_LIBRARIES}  )
