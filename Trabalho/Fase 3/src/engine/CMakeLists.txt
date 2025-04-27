cmake_minimum_required(VERSION 3.5)

project(Engine)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Add source files (include tinyxml2.cpp along with main.cpp)
add_executable(${PROJECT_NAME} main.cpp tinyxml2.cpp)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_11)

find_package(OpenGL REQUIRED)
include_directories(${OpenGL_INCLUDE_DIRS})
link_directories(${OpenGL_LIBRARY_DIRS})
add_definitions(${OpenGL_DEFINITIONS})

if(NOT OPENGL_FOUND)
    message(ERROR "OPENGL not found!")
endif()

if(WIN32)
    message(STATUS "Toolkits_DIR set to: ${TOOLKITS_FOLDER}")
    set(TOOLKITS_FOLDER "" CACHE PATH "Path to Toolkits folder")

    if(NOT EXISTS "${TOOLKITS_FOLDER}/glut/GL/glut.h" OR NOT EXISTS "${TOOLKITS_FOLDER}/glut/glut32.lib")
        message(ERROR ": GLUT not found")
    endif()


    if (NOT EXISTS "${TOOLKITS_FOLDER}/glew/GL/glew.h" OR NOT EXISTS "${TOOLKITS_FOLDER}/glew/glew32.lib")
		message(ERROR ": GLEW not found")
    endif()

    include_directories(${TOOLKITS_FOLDER}/glew)
     target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} ${TOOLKITS_FOLDER}/glew/glew32.lib)

    include_directories(${TOOLKITS_FOLDER}/glut)
    target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} ${TOOLKITS_FOLDER}/glut/glut32.lib)

    if(EXISTS "${TOOLKITS_FOLDER}/glut/glut32.dll")
        file(COPY ${TOOLKITS_FOLDER}/glut/glut32.dll DESTINATION ${CMAKE_BINARY_DIR})
    endif()

    if(EXISTS "${TOOLKITS_FOLDER}/glew/glew32.dll")
        file(COPY ${TOOLKITS_FOLDER}/glew/glew32.dll DESTINATION ${CMAKE_BINARY_DIR})
    endif()


    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
else() # Linux and Mac
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
    find_package(GLUT REQUIRED)
    include_directories(${GLUT_INCLUDE_DIR})
    link_directories(${GLUT_LIBRARY_DIRS})
    add_definitions(${GLUT_DEFINITIONS})

    target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${GLEW_LIBRARIES})
    if(NOT GLUT_FOUND)
        message(ERROR ": GLUT not found!")
    endif()
endif()
