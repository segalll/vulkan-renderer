cmake_minimum_required(VERSION 3.4.1)

project(vulkan)

set(VULKAN_SDK_DIR "C:/VulkanSDK/1.2.141.2")
set(GLFW_DIR "A:/Coding/glfw")
set(CGLM_DIR "A:/Coding/cglm")
set(MAIN_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/out)

set(LIB_VULKAN ${VULKAN_SDK_DIR}/Lib/vulkan-1.lib)
set(LIB_GLFW ${GLFW_DIR}/lib/glfw3.lib)
set(LIB_CGLM ${CGLM_DIR}/lib/cglm.lib)

include_directories(${VULKAN_SDK_DIR}/Include)
include_directories(${GLFW_DIR}/include)
include_directories(${CGLM_DIR}/include)

file(GLOB_RECURSE C_HEADERS ${MAIN_SOURCE_DIR}/*.h)
file(GLOB_RECURSE C_SOURCES ${MAIN_SOURCE_DIR}/*.c)

add_executable(
	vulkan ${EXECUTABLE_TYPE}
	${C_HEADERS}
	${C_SOURCES}
)

set_property(TARGET vulkan PROPERTY C_STANDARD 11)

target_link_libraries(
	vulkan
	${LIB_VULKAN}
	${LIB_GLFW}
	${LIB_CGLM}
)

set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")

file(GLOB_RECURSE SHADER_SOURCES
	${MAIN_SOURCE_DIR}/shaders/*.frag
	${MAIN_SOURCE_DIR}/shaders/*.vert
)

foreach(SHADER ${SHADER_SOURCES})
	get_filename_component(FILE_NAME ${SHADER} NAME)
	set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
	add_custom_command(
		OUTPUT ${SPIRV}
		COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
		COMMAND ${GLSL_VALIDATOR} -V ${SHADER} -o ${SPIRV}
		DEPENDS ${SHADER}
	)
	list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(SHADER)

add_custom_target(
	shaders
	DEPENDS ${SPIRV_BINARY_FILES}
	SOURCES ${SHADER_SOURCES}
)

add_dependencies(vulkan shaders)

add_custom_command(TARGET vulkan POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:vulkan>/shaders/"
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${PROJECT_BINARY_DIR}/shaders"
		"$<TARGET_FILE_DIR:vulkan>/shaders"
)