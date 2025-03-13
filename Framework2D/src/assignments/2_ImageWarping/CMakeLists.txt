project(2_ImageWarping)

# 收集源文件
file(GLOB source
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.h" 
  "${CMAKE_CURRENT_SOURCE_DIR}/warper/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/warper/*.h" 
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${source})

# 添加头文件路径
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 添加 /bigobj 编译选项（仅适用于 MSVC）
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE "/bigobj")
endif()

# 添加 Dlib 支持（可选）
find_package(dlib QUIET)
if(dlib_FOUND)
  message(STATUS "Dlib found")
  target_link_libraries(${PROJECT_NAME} PRIVATE dlib::dlib)
else()
  message(STATUS "Dlib not found")
endif()

# 设置目标属性
set_target_properties(${PROJECT_NAME} PROPERTIES 
  DEBUG_POSTFIX "_d"
  RUNTIME_OUTPUT_DIRECTORY "${BINARY_DIR}"
  LIBRARY_OUTPUT_DIRECTORY "${LIBRARY_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY "${LIBRARY_DIR}")

# 链接 common 库
target_link_libraries(${PROJECT_NAME} PUBLIC common)

# 添加编译定义
target_compile_definitions(${PROJECT_NAME} PRIVATE -DDATA_PATH="${FRAMEWORK2D_DIR}/../Homeworks/2_image_warping/data")