find_program(DOTNET_EXECUTABLE NAMES dotnet)
if (NOT DOTNET_EXECUTABLE)
    message(STATUS "Dotnet SDK not found. Skipping e2e managed tests.")
    return()
endif()

file(GLOB E2E_TEST_PROJECTS CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.tests.csproj")

if (NOT E2E_TEST_PROJECTS)
    message(STATUS "No e2e test projects found under tests/e2e")
    return()
endif()

set(E2E_TEST_NAMES)
set(E2E_CPP_IMPL_SOURCES)
set(E2E_TEST_CS_PROJECTS)
set(E2E_REGISTRY_INCLUDES "")
set(E2E_REGISTRY_CALLS "")

file(GLOB_RECURSE E2E_MODULE_HEADERS CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*/test_*.hpp")
list(SORT E2E_MODULE_HEADERS)

if (NOT E2E_MODULE_HEADERS)
    message(FATAL_ERROR "No e2e test modules found. Expected tests/*/test_*.hpp")
endif()

foreach(module_header IN LISTS E2E_MODULE_HEADERS)
    get_filename_component(module_dir "${module_header}" DIRECTORY)
    get_filename_component(module_basename "${module_header}" NAME_WE)
    string(REGEX REPLACE "^test_(.+)$" "\\1" test_name "${module_basename}")

    if (test_name STREQUAL module_basename)
        message(FATAL_ERROR "Invalid e2e module header name: ${module_header}. Expected test_<name>.hpp")
    endif()

    set(module_cs "${module_dir}/test_${test_name}.cs")
    if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${module_cs}")
        message(FATAL_ERROR "Missing e2e managed test file for module '${test_name}': ${module_cs}")
    endif()

    list(APPEND E2E_TEST_NAMES "${test_name}")

    set(module_cpp "${module_dir}/test_${test_name}.cpp")
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${module_cpp}")
        list(APPEND E2E_CPP_IMPL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${module_cpp}")
    endif()

    string(APPEND E2E_REGISTRY_INCLUDES "#include \"tests/${module_header}\"\n")
    string(APPEND E2E_REGISTRY_CALLS "    register_bindings_${test_name}(generator, \"${test_name}\");\n")
endforeach()

foreach(test_project IN LISTS E2E_TEST_PROJECTS)
    list(APPEND E2E_TEST_CS_PROJECTS "${test_project}")
    csbind23_register_managed_project("tests/${test_project}")
endforeach()

set(E2E_GENERATED_ROOT "${CMAKE_BINARY_DIR}/binding_e2e/generated")
set(E2E_GENERATED_STAMP "${CMAKE_BINARY_DIR}/binding_e2e/generated.stamp")
set(E2E_NATIVE_ROOT "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
set(E2E_REGISTRY_CPP "${CMAKE_CURRENT_BINARY_DIR}/e2e_bindings_registry.cpp")

set(E2E_GENERATED_CABI_FILES)
foreach(test_name IN LISTS E2E_TEST_NAMES)
    if (test_name STREQUAL "class")
        set(cabi_module_name "class_module")
    elseif (test_name STREQUAL "enum")
        set(cabi_module_name "enum_module")
    elseif (test_name STREQUAL "module_import")
        list(APPEND E2E_GENERATED_CABI_FILES
            "${E2E_GENERATED_ROOT}/cabi/module_import_animals_cabi.cpp"
            "${E2E_GENERATED_ROOT}/cabi/module_import_consumer_cabi.cpp")
        continue()
    elseif (test_name STREQUAL "virtual")
        set(cabi_module_name "virtual_module")
    else()
        set(cabi_module_name "${test_name}")
    endif()
    list(APPEND E2E_GENERATED_CABI_FILES "${E2E_GENERATED_ROOT}/cabi/${cabi_module_name}_cabi.cpp")
endforeach()
list(APPEND E2E_GENERATED_CABI_FILES "${E2E_GENERATED_ROOT}/cabi/csbind23_core_cabi.cpp")
list(APPEND E2E_GENERATED_CABI_FILES "${E2E_GENERATED_ROOT}/cabi/csbind23_stl_cabi.cpp")

set(E2E_REGISTRY_CONTENT "#include \"csbind23/bindings_generator.hpp\"\n\n")
string(APPEND E2E_REGISTRY_CONTENT "${E2E_REGISTRY_INCLUDES}")
string(APPEND E2E_REGISTRY_CONTENT "namespace csbind23::testing {\n\n")
string(APPEND E2E_REGISTRY_CONTENT "void register_bindings(BindingsGenerator& generator) {\n")
string(APPEND E2E_REGISTRY_CONTENT "${E2E_REGISTRY_CALLS}")
string(APPEND E2E_REGISTRY_CONTENT "}\n\n")
string(APPEND E2E_REGISTRY_CONTENT "} // namespace csbind23::testing\n")
file(GENERATE OUTPUT "${E2E_REGISTRY_CPP}" CONTENT "${E2E_REGISTRY_CONTENT}")

set(E2E_GENERATOR_TARGET "e2e.gen")
add_executable(${E2E_GENERATOR_TARGET}
    ${PROJECT_SOURCE_DIR}/tests/main_generator.cpp
    ${E2E_REGISTRY_CPP}
    ${E2E_CPP_IMPL_SOURCES}
)

target_link_libraries(${E2E_GENERATOR_TARGET}
    PRIVATE
        csbind23
)

target_include_directories(${E2E_GENERATOR_TARGET}
    PRIVATE
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/include
)

add_custom_command(
    OUTPUT ${E2E_GENERATED_STAMP} ${E2E_GENERATED_CABI_FILES}
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${E2E_GENERATED_ROOT}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${E2E_GENERATED_ROOT}
    COMMAND $<TARGET_FILE:${E2E_GENERATOR_TARGET}> ${E2E_GENERATED_ROOT}
    COMMAND ${CMAKE_COMMAND} -E touch ${E2E_GENERATED_STAMP}
    DEPENDS ${E2E_GENERATOR_TARGET}
    COMMENT "Generating binding-e2e artifacts for all tests"
    VERBATIM
)

add_custom_target(e2e.gen.run
    DEPENDS ${E2E_GENERATED_STAMP}
)

csbind23_register_solution_build_dependency(e2e.gen.run)

set(E2E_NATIVE_TARGET "e2e.C")
add_library(${E2E_NATIVE_TARGET} SHARED
    ${E2E_GENERATED_CABI_FILES}
    ${E2E_CPP_IMPL_SOURCES}
)

set_target_properties(${E2E_NATIVE_TARGET}
    PROPERTIES
    PREFIX ""
    SUFFIX ""
    OUTPUT_NAME "e2e.C${CMAKE_SHARED_LIBRARY_SUFFIX}"
        RUNTIME_OUTPUT_DIRECTORY "${E2E_NATIVE_ROOT}"
        LIBRARY_OUTPUT_DIRECTORY "${E2E_NATIVE_ROOT}"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/binding_e2e/lib/$<CONFIG>"
)

add_dependencies(${E2E_NATIVE_TARGET} e2e.gen.run)

target_include_directories(${E2E_NATIVE_TARGET}
    PRIVATE
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/include
)

set_source_files_properties(${E2E_GENERATED_CABI_FILES}
    PROPERTIES
        GENERATED TRUE
)

set(E2E_MANAGED_BUILD_STAMPS)
set(E2E_MANAGED_TEST_STAMPS)
foreach(test_project IN LISTS E2E_TEST_CS_PROJECTS)
    string(REGEX REPLACE "\\.tests\\.csproj$" "" test_base "${test_project}")
    set(test_root "${CMAKE_BINARY_DIR}/binding_e2e/${test_base}")
    set(dotnet_root "${test_root}/dotnet")
    set(results_root "${test_root}/results")
    set(dotnet_build_stamp "${test_root}/dotnet_build.stamp")
    set(dotnet_test_stamp "${test_root}/dotnet_test.stamp")
    set(test_csproj "${PROJECT_SOURCE_DIR}/tests/${test_project}")

    add_custom_command(
        OUTPUT ${dotnet_build_stamp}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${dotnet_root}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${results_root}
        COMMAND ${DOTNET_EXECUTABLE} restore ${test_csproj} -p:CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR} -p:GeneratedBindingsDir=${E2E_GENERATED_ROOT} -p:BaseOutputPath=${dotnet_root}/
        COMMAND ${DOTNET_EXECUTABLE} build ${test_csproj} --configuration $<CONFIG> --no-restore -p:CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR} -p:GeneratedBindingsDir=${E2E_GENERATED_ROOT} -p:BaseOutputPath=${dotnet_root}/
        COMMAND ${CMAKE_COMMAND} -E touch ${dotnet_build_stamp}
        DEPENDS ${E2E_NATIVE_TARGET} e2e.gen.run
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Restoring and building binding-e2e managed target for ${test_base}"
        VERBATIM
    )

    add_custom_command(
        OUTPUT ${dotnet_test_stamp}
        COMMAND ${CMAKE_COMMAND} -E env LD_LIBRARY_PATH=${E2E_NATIVE_ROOT}:$ENV{LD_LIBRARY_PATH} ${DOTNET_EXECUTABLE} test ${test_csproj} --configuration $<CONFIG> --no-build --no-restore --results-directory ${results_root} -p:CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR} -p:GeneratedBindingsDir=${E2E_GENERATED_ROOT} -p:BaseOutputPath=${dotnet_root}/
        COMMAND ${CMAKE_COMMAND} -E touch ${dotnet_test_stamp}
        DEPENDS ${dotnet_build_stamp}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Running binding-e2e C# xUnit tests for ${test_base}"
        VERBATIM
    )

    list(APPEND E2E_MANAGED_BUILD_STAMPS ${dotnet_build_stamp})
    list(APPEND E2E_MANAGED_TEST_STAMPS ${dotnet_test_stamp})
endforeach()

set(E2E_ALL_STAMPS ${E2E_GENERATED_STAMP} ${E2E_MANAGED_BUILD_STAMPS} ${E2E_MANAGED_TEST_STAMPS})
add_custom_target(e2e.clean.stamps
    COMMAND ${CMAKE_COMMAND} -E rm -f ${E2E_ALL_STAMPS}
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${E2E_GENERATED_ROOT}
    COMMENT "Clearing binding-e2e stamp files"
    VERBATIM
)

if (E2E_MANAGED_BUILD_STAMPS)
    add_custom_target(e2e.Net
        DEPENDS ${E2E_MANAGED_BUILD_STAMPS}
    )
    csbind23_register_solution_build_dependency(e2e.Net)
endif()

if (E2E_MANAGED_TEST_STAMPS)
    add_custom_target(net.tests
        DEPENDS ${E2E_MANAGED_TEST_STAMPS}
    )
    if (TARGET e2e.Net)
        add_dependencies(net.tests e2e.Net)
    endif()
    add_dependencies(net.tests ${E2E_NATIVE_TARGET})
endif()
