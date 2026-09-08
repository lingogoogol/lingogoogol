find_package(CURL CONFIG REQUIRED)
find_package(Freetype REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(
    storage WIN32
    storage.cpp
)

find_program(FXC_EXECUTABLE NAMES fxc REQUIRED)
set(STORAGE_SHADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/shader")
set(STORAGE_SHADER_OUTPUTS
    "${STORAGE_SHADER_DIR}/solid_vertex.cso"
    "${STORAGE_SHADER_DIR}/solid_pixel.cso"
    "${STORAGE_SHADER_DIR}/texture_vertex.cso"
    "${STORAGE_SHADER_DIR}/texture_pixel.cso"
)
add_custom_command(
    OUTPUT ${STORAGE_SHADER_OUTPUTS}
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${STORAGE_SHADER_DIR}"
    COMMAND "${FXC_EXECUTABLE}" /nologo /T vs_5_1 /E vertex /Fo "${STORAGE_SHADER_DIR}/solid_vertex.cso" "${CMAKE_SOURCE_DIR}/compilercpp/shader/solid_vertex.hlsl"
    COMMAND "${FXC_EXECUTABLE}" /nologo /T ps_5_1 /E pixel /Fo "${STORAGE_SHADER_DIR}/solid_pixel.cso" "${CMAKE_SOURCE_DIR}/compilercpp/shader/solid_pixel.hlsl"
    COMMAND "${FXC_EXECUTABLE}" /nologo /T vs_5_1 /E vertex /Fo "${STORAGE_SHADER_DIR}/texture_vertex.cso" "${CMAKE_SOURCE_DIR}/compilercpp/shader/texture_vertex.hlsl"
    COMMAND "${FXC_EXECUTABLE}" /nologo /enable_unbounded_descriptor_tables /T ps_5_1 /E pixel /Fo "${STORAGE_SHADER_DIR}/texture_pixel.cso" "${CMAKE_SOURCE_DIR}/compilercpp/shader/texture_pixel.hlsl"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/compilercpp/shader/solid_vertex.hlsl"
        "${CMAKE_SOURCE_DIR}/compilercpp/shader/solid_pixel.hlsl"
        "${CMAKE_SOURCE_DIR}/compilercpp/shader/texture_vertex.hlsl"
        "${CMAKE_SOURCE_DIR}/compilercpp/shader/texture_pixel.hlsl"
    VERBATIM
)
add_custom_target(storage.shaders DEPENDS ${STORAGE_SHADER_OUTPUTS})
add_dependencies(storage storage.shaders)

add_subdirectory(multipart)
add_subdirectory(api)

target_link_libraries(
    storage
    PRIVATE
        common
        storage.multipart
        storage.api
        lgo
        lgo.dev.error
        lgo.io.file
        CURL::libcurl
        Freetype::Freetype
        nlohmann_json::nlohmann_json
        OpenSSL::Crypto
        OpenSSL::SSL
)

if(MSVC)
    target_link_options(storage PRIVATE "/MAP:${CMAKE_CURRENT_BINARY_DIR}/storage.map")
endif()
