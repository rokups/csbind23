add_executable(csbind23_generator_playground
    generator_main.cpp
    bindings_declarations.cpp
)

target_link_libraries(csbind23_generator_playground
    PRIVATE
        csbind23::bindings_generator
)

target_compile_definitions(csbind23_generator_playground
    PRIVATE
        CSBIND23_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)

set(CSBIND23_EXAMPLE_GENERATED_DIR "${PROJECT_SOURCE_DIR}/example/generated")
set(CSBIND23_EXAMPLE_GENERATED_CABI "${CSBIND23_EXAMPLE_GENERATED_DIR}/cabi/playground_cabi.cpp")

add_custom_command(
    OUTPUT ${CSBIND23_EXAMPLE_GENERATED_CABI}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CSBIND23_EXAMPLE_GENERATED_DIR}
    COMMAND $<TARGET_FILE:csbind23_generator_playground> ${CSBIND23_EXAMPLE_GENERATED_DIR}
    DEPENDS csbind23_generator_playground
    COMMENT "Generating example C-ABI bindings"
    VERBATIM
)

add_custom_target(csbind23_generate_playground_cabi
    DEPENDS ${CSBIND23_EXAMPLE_GENERATED_CABI}
)

add_library(csbind23_playground_cabi STATIC
    ${CSBIND23_EXAMPLE_GENERATED_CABI}
    bindings_declarations.cpp
)

add_dependencies(csbind23_playground_cabi csbind23_generate_playground_cabi)

target_include_directories(csbind23_playground_cabi
    PRIVATE
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/include
)

add_library(csbind23_native SHARED
    ${CSBIND23_EXAMPLE_GENERATED_CABI}
    bindings_declarations.cpp
)

add_dependencies(csbind23_native csbind23_generate_playground_cabi)

target_include_directories(csbind23_native
    PRIVATE
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/include
)

find_program(DOTNET_EXECUTABLE NAMES dotnet)

if (DOTNET_EXECUTABLE)
    set(CSBIND23_DOTNET_OUT_DIR "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
    set(CSBIND23_DOTNET_OBJ_DIR "${CMAKE_BINARY_DIR}/obj/dotnet/$<CONFIG>/")

    add_custom_target(csbind23_build_playground_csharp
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CSBIND23_EXAMPLE_GENERATED_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/obj
        COMMAND $<TARGET_FILE:csbind23_generator_playground> ${CSBIND23_EXAMPLE_GENERATED_DIR}
        COMMAND ${DOTNET_EXECUTABLE} build ${CMAKE_CURRENT_SOURCE_DIR}/csharp/playground.runner.csproj --configuration $<CONFIG> --output ${CSBIND23_DOTNET_OUT_DIR} -p:BaseIntermediateOutputPath=${CSBIND23_DOTNET_OBJ_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:csbind23_native> ${CSBIND23_DOTNET_OUT_DIR}
        DEPENDS csbind23_generator_playground csbind23_native
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Generating and building example C# runner"
        VERBATIM
    )
else()
    message(STATUS "Dotnet SDK not found. Skipping csbind23_build_playground_csharp target.")
endif()
