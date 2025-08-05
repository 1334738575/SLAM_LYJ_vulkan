cmake_minimum_required(VERSION 3.20)
project(Vulkan_LYJ)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE "Release")
if(WIN32)
    if(MSVC)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
        # set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Od /Ob0")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG")
        add_link_options(/DEBUG) #生成调试信息（测试在visual studio中有效）
    endif()
elseif(LINUX)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -g")
endif()

set(${PROJECT_NAME}_LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/Output)
set(${PROJECT_NAME}_CMAKE_FILE)
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install)

#cmake
if(DEFINED ${PROJECT_NAME}_CMAKE_FILE)
    include(${${PROJECT_NAME}_CMAKE_FILE})
elseif(EXISTS ${CMAKE_CURRENT_LIST_DIR}/../cmake/FuncLyj.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/../cmake/FuncLyj.cmake)
else()
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/FuncLyj.cmake)
endif()

set(${PROJECT_NAME}_BINARY_DIRS)
#package
list(APPEND CMAKE_MODULE_PATH "E:/vulkan")
find_package(Vulkan)
# include_directories(${VULKAN_INCLUDE_DIRS})
#glfw
list(APPEND CMAKE_MODULE_PATH "D:/testLyj/GLFW/glfw-3.4.bin.WIN64")
find_package(Glfw)
# include_directories(${GLFW_INCLUDE_DIRS})
list(APPEND ${PROJECT_NAME}_BINARY_DIRS "D:/testLyj/GLFW/glfw-3.4.bin.WIN64/lib-vc2022;")
#glm
list(APPEND CMAKE_MODULE_PATH "D:/testLyj/GLM/glm-1.0.1")
find_package(glm)
# include_directories(${GLM_INCLUDE_DIRS})
#OpenCV
set(OpenCV_DIR "D:/SLAM_LYJ/dependencies/Opencv/lib")
find_package(OpenCV)
# include_directories(${OpenCV_INCLUDE_DIRS})
list(APPEND ${PROJECT_NAME}_BINARY_DIRS "${OpenCV_INSTALL_PATH}/bin;")

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
                # GenShader(${VERTRSHADERS})
                foreach(arg ${VERTRSHADERS})
                    execute_process(COMMAND ${SHADERCOMPILER} -V ${arg} -o ${arg}.spv)
                endforeach()
            endif()
            file(GLOB FRAGSHADERS ${SHADER}/*.frag)
            list(LENGTH FRAGSHADERS lenTmp)
            if(NOT lenTmp EQUAL 0)
                # GenShader(${FRAGSHADERS})
                foreach(arg ${FRAGSHADERS})
                    execute_process(COMMAND ${SHADERCOMPILER} -V ${arg} -o ${arg}.spv)
                endforeach()
            endif()
            file(GLOB COMPSHADERS ${SHADER}/*.comp)
            list(LENGTH COMPSHADERS lenTmp)
            if(NOT lenTmp EQUAL 0)
                # GenShader(${COMPSHADERS})
                foreach(arg ${COMPSHADERS})
                    execute_process(COMMAND ${SHADERCOMPILER} -V ${arg} -o ${arg}.spv)
                endforeach()
            endif()
        endif()
    endforeach()
endif()

#project sources
set(VULKAN_LYJ_SRC_FILES)
GroupFiles(VULKAN_LYJ_SRC_FILES ${CMAKE_CURRENT_LIST_DIR} ".")
add_library(${PROJECT_NAME} SHARED ${VULKAN_LYJ_SRC_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

#dependencies headers and libraries
target_include_directories(${PROJECT_NAME} 
    PUBLIC ${VULKAN_INCLUDE_DIRS}
    PUBLIC ${GLFW_INCLUDE_DIRS}
    PUBLIC ${GLM_INCLUDE_DIRS}
)
        
# target_link_libraries(${PROJECT_NAME} ${VULKAN_LIBRARIES} glm::glm ${GLFW_LIBRARIES}  ${OpenCV_LIBS} )
target_link_libraries(${PROJECT_NAME} PUBLIC ${VULKAN_LIBRARIES} ${GLFW_LIBRARIES})

target_compile_definitions(${PROJECT_NAME} PRIVATE SLAM_LYJ_API_EXPORTS) # TODO

#output
if(DEFINED ${PROJECT_NAME}_LIBRARY_OUTPUT_PATH)
    set_target_properties(${PROJECT_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${${PROJECT_NAME}_LIBRARY_OUTPUT_PATH}
    ARCHIVE_OUTPUT_DIRECTORY ${${PROJECT_NAME}_LIBRARY_OUTPUT_PATH}
    RUNTIME_OUTPUT_DIRECTORY ${${PROJECT_NAME}_LIBRARY_OUTPUT_PATH}
    )
endif()

# ################################example##################################
add_executable(${PROJECT_NAME}Bin example/VulkanTest.cpp example/VulkanTest.h)
target_include_directories(${PROJECT_NAME}Bin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${OpenCV_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}Bin PRIVATE ${PROJECT_NAME} ${OpenCV_LIBS})

if(DEFINED ${PROJECT_NAME}_LIBRARY_OUTPUT_PATH)
    set_target_properties(${PROJECT_NAME}Bin PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${${PROJECT_NAME}_LIBRARY_OUTPUT_PATH}
        ARCHIVE_OUTPUT_DIRECTORY ${${PROJECT_NAME}_LIBRARY_OUTPUT_PATH}
        RUNTIME_OUTPUT_DIRECTORY ${${PROJECT_NAME}_LIBRARY_OUTPUT_PATH}
    )
endif()

if(MSVC)
    set_target_properties(${PROJECT_NAME}Bin PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=${${PROJECT_NAME}_BINARY_DIRS}")
endif()

# ################################install##################################
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    "@PACKAGE_INIT@\ninclude(\"\${CMAKE_CURRENT_LIST_DIR}/${PROJECT_NAME}Targets.cmake\" )"
)

# 安装库和头文件
install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib # DLL 在 Windows 下会被安装到 bin/
    RUNTIME DESTINATION bin # 适用于 DLL 的运行时组件
    INCLUDES DESTINATION include
)

# #file
# install(FILES test.h
# DESTINATION include
# PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)
# #directory
# install(DIRECTORY ./          # 注意末尾的斜杠
# DESTINATION include
# FILES_MATCHING PATTERN "*.h" # 仅匹配 .h 文件
# PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
# )
# install(DIRECTORY ./
# DESTINATION include
# FILES_MATCHING
# PATTERN "*.h"            # 包含所有 .h 文件
# PATTERN "build/*"             # 匹配子目录名
# EXCLUDE                  # 排除该目录及其内容
# PATTERN "cmake/*"             # 匹配子目录名
# EXCLUDE                  # 排除该目录及其内容
# PATTERN "install/*"             # 匹配子目录名
# EXCLUDE                  # 排除该目录及其内容
# PATTERN "Lib/*"             # 匹配子目录名
# EXCLUDE                  # 排除该目录及其内容
# PERMISSIONS
# OWNER_READ GROUP_READ WORLD_READ
# )
install(DIRECTORY ./
    DESTINATION include
    FILES_MATCHING
    PATTERN "*.h" # 包含所有 .h 文件

    # 排除指定目录（递归匹配）
    PATTERN "build" EXCLUDE # 排除所有名为 build 的目录及内容
    PATTERN "cmake" EXCLUDE # 排除所有名为 cmake 的目录及内容
    PATTERN "install" EXCLUDE # 排除所有名为 install 的目录及内容
    PATTERN "Lib" EXCLUDE # 排除所有名为 Lib 的目录及内容
    PATTERN ".git" EXCLUDE # 排除所有名为 Lib 的目录及内容
    PATTERN ".vscode" EXCLUDE # 排除所有名为 Lib 的目录及内容
    PATTERN "example" EXCLUDE # 排除所有名为 Lib 的目录及内容
    PATTERN "Output" EXCLUDE # 排除所有名为 Lib 的目录及内容

    # 或使用正则表达式精确匹配路径（更严格）
    # PATTERN "/(build|cmake|install|Lib)(/.*)?$" EXCLUDE
    # 设置权限
    PERMISSIONS
    OWNER_READ GROUP_READ WORLD_READ
)

# 安装后删除空目录（可选）
install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/cleanup.cmake")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/cleanup.cmake [[
file(REMOVE_RECURSE "${CMAKE_INSTALL_PREFIX}/include/build")
file(REMOVE_RECURSE "${CMAKE_INSTALL_PREFIX}/include/cmake")
file(REMOVE_RECURSE "${CMAKE_INSTALL_PREFIX}/include/install")
file(REMOVE_RECURSE "${CMAKE_INSTALL_PREFIX}/include/Lib")
]])

# set(TARGETNAME ${PROJECT_NAME}Targets)
# 导出目标配置
install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION lib/cmake
)

# 生成包配置文件
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION lib/cmake
)
write_basic_package_version_file(
    ${PROJECT_NAME}ConfigVersion.cmake
    VERSION 1.0.0
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION lib/cmake
)