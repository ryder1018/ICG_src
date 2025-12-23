add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)

add_executable(ICG_2024_HW3_Animated
"main_animated.cpp"
"stb_image.cpp"
"shader.cpp"
"animated_model.cpp"
)
target_link_libraries(ICG_2024_HW3_Animated
glfw
glm::glm
glad
assimp
)
add_custom_command(TARGET ICG_2024_HW3_Animated POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders ${CMAKE_CURRENT_BINARY_DIR}/shaders)

add_custom_command(TARGET ICG_2024_HW3_Animated POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory  
    ${CMAKE_CURRENT_SOURCE_DIR}/asset ${CMAKE_CURRENT_BINARY_DIR}/asset)