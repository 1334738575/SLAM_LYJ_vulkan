cmake_minimum_required(VERSION 3.20)
project(Vulkan_LYJ)

set(CMAKE_CXX_STANDARD 17)

#cmake
if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/../cmake/FuncLyj.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/../cmake/FuncLyj.cmake)
else()
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/FuncLyj.cmake)
endif()

#package
find_package(Vulkan)
include_directories(${VULKAN_INCLUDE_DIRS})
#glfw
find_package(Glfw)
include_directories(${GLFW_INCLUDE_DIRS})
#glm
find_package(glm)

# vulkan shader compile
macro(GenShader args)
    foreach(arg ${args})
        execute_process(COMMAND ${SHADERCOMPILER} -V ${arg} -o ${arg}.spv)
    endforeach()
endmacro()
if(NOT USE_OLD_SHADER)
    file(GLOB ALLSHADERS ${CMAKE_CURRENT_SOURCE_DIR}/shader/*)
    foreach(SHADER ${ALLSHADERS})
        if(IS_DIRECTORY ${SHADER})
            file(GLOB VERTRSHADERS ${SHADER}/*.vert)
            list(LENGTH VERTRSHADERS lenTmp)
            if(NOT lenTmp EQUAL 0)
                GenShader(${VERTRSHADERS})
            endif()
            file(GLOB FRAGSHADERS ${SHADER}/*.frag)
            list(LENGTH FRAGSHADERS lenTmp)
            if(NOT lenTmp EQUAL 0)
                GenShader(${FRAGSHADERS})
            endif()
            file(GLOB COMPSHADERS ${SHADER}/*.comp)
            list(LENGTH COMPSHADERS lenTmp)
            if(NOT lenTmp EQUAL 0)
                GenShader(${COMPSHADERS})
            endif()
        endif()
    endforeach()
endif()

#project sources
set(VULKAN_LYJ_SRC_FILES)
GroupFiles(VULKAN_LYJ_SRC_FILES ${CMAKE_CURRENT_LIST_DIR} ".")
add_library(${PROJECT_NAME} SHARED ${VULKAN_LYJ_SRC_FILES})

#dependencies headers and libraries
target_include_directories(${PROJECT_NAME} 
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}
    PUBLIC ${VULKAN_INCLUDE_DIRS}
    PUBLIC ${GLFW_INCLUDE_DIRS}
)
        
target_link_libraries(${PROJECT_NAME} ${VULKAN_LIBRARIES} glm::glm ${GLFW_LIBRARIES}  ${OpenCV_LIBS} )

#output
if(DEFINED LIBRARY_LYJ_OUTPUT_PATH)
    set_target_properties(${PROJECT_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_LYJ_OUTPUT_PATH}
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_LYJ_OUTPUT_PATH}
    RUNTIME_OUTPUT_DIRECTORY ${LIBRARY_LYJ_OUTPUT_PATH}
    )
endif()