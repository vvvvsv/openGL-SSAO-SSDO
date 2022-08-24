add_executable(SSDO
        gl_env.h
        main.cpp)

target_link_libraries(SSDO PRIVATE assimp::assimp glew_s glm stb glfw)
target_include_directories(SSDO PRIVATE
        ../third_party/glew/include
        ${CMAKE_CURRENT_BINARY_DIR})

target_compile_features(SSDO PRIVATE cxx_std_11)

configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)