add_executable(playground.gen
    generator_main.cpp
    bindings_declarations.cpp
)

target_link_libraries(playground.gen
    PRIVATE
        csbind23
)

target_compile_definitions(playground.gen
    PRIVATE
        CSBIND23_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)

set(CSBIND23_EXAMPLE_GENERATED_DIR "${PROJECT_SOURCE_DIR}/example/generated")
set(CSBIND23_EXAMPLE_GENERATED_CABI "${CSBIND23_EXAMPLE_GENERATED_DIR}/cabi/playground_cabi.cpp")

add_custom_command(
    OUTPUT ${CSBIND23_EXAMPLE_GENERATED_CABI} ${CSBIND23_EXAMPLE_GENERATED_DIR}/playground.generated.stamp
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CSBIND23_EXAMPLE_GENERATED_DIR}
    COMMAND $<TARGET_FILE:playground.gen> ${CSBIND23_EXAMPLE_GENERATED_DIR}
    COMMAND ${CMAKE_COMMAND} -E touch ${CSBIND23_EXAMPLE_GENERATED_DIR}/playground.generated.stamp
    DEPENDS playground.gen
    COMMENT "Generating example C-ABI bindings"
    VERBATIM
)

add_custom_target(playground.gen.run
    DEPENDS ${CSBIND23_EXAMPLE_GENERATED_DIR}/playground.generated.stamp
)

csbind23_register_solution_build_dependency(playground.gen.run)

add_library(playground.C SHARED
    ${CSBIND23_EXAMPLE_GENERATED_CABI}
    bindings_declarations.cpp
)

set_target_properties(playground.C
    PROPERTIES
        PREFIX ""
        SUFFIX ""
        OUTPUT_NAME "playground.C${CMAKE_SHARED_LIBRARY_SUFFIX}"
)

add_dependencies(playground.C playground.gen.run)

target_include_directories(playground.C
    PRIVATE
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/include
)

find_program(DOTNET_EXECUTABLE NAMES dotnet)

if (DOTNET_EXECUTABLE)
    set(CSBIND23_DOTNET_OUT_DIR "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
    set(CSBIND23_DOTNET_OBJ_DIR "${CMAKE_BINARY_DIR}/obj/dotnet/$<CONFIG>/")

    add_custom_target(playground.Net
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CSBIND23_EXAMPLE_GENERATED_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/obj
        COMMAND $<TARGET_FILE:playground.gen> ${CSBIND23_EXAMPLE_GENERATED_DIR}
        COMMAND ${DOTNET_EXECUTABLE} build ${CMAKE_CURRENT_SOURCE_DIR}/csharp/playground.runner.csproj --configuration $<CONFIG> --output ${CSBIND23_DOTNET_OUT_DIR} -p:CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR} -p:BaseIntermediateOutputPath=${CSBIND23_DOTNET_OBJ_DIR}
        DEPENDS playground.gen.run playground.C
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Building playground .NET API"
        VERBATIM
    )

    add_custom_target(playground
        DEPENDS playground.C playground.Net
    )

    csbind23_register_managed_project(example/csharp/playground.generated.csproj)
    csbind23_register_managed_project(example/csharp/playground.runner.csproj)
else()
    add_custom_target(playground
        DEPENDS playground.C
    )
    message(STATUS "Dotnet SDK not found. Skipping playground.Net target.")
endif()
