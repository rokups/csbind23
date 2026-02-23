include_guard(GLOBAL)

function(csbind23_add_binding_e2e_test)
    set(options)
    set(oneValueArgs NAME CPP_SOURCE CS_SOURCE CS_PROJECT)
    set(multiValueArgs)
    cmake_parse_arguments(CSBE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT CSBE_NAME)
        message(FATAL_ERROR "csbind23_add_binding_e2e_test requires NAME")
    endif()

    if (NOT CSBE_CPP_SOURCE)
        message(FATAL_ERROR "csbind23_add_binding_e2e_test requires CPP_SOURCE")
    endif()

    if (NOT CSBE_CS_SOURCE)
        message(FATAL_ERROR "csbind23_add_binding_e2e_test requires CS_SOURCE")
    endif()

    find_program(DOTNET_EXECUTABLE NAMES dotnet)
    if (NOT DOTNET_EXECUTABLE)
        message(STATUS "Dotnet SDK not found. Skipping binding e2e test ${CSBE_NAME}.")
        return()
    endif()

    set(test_name "${CSBE_NAME}")
    set(cpp_source "${PROJECT_SOURCE_DIR}/${CSBE_CPP_SOURCE}")
    set(cs_source "${PROJECT_SOURCE_DIR}/${CSBE_CS_SOURCE}")
    get_filename_component(cs_source_dir "${cs_source}" DIRECTORY)

    if (CSBE_CS_PROJECT)
        set(test_csproj "${PROJECT_SOURCE_DIR}/${CSBE_CS_PROJECT}")
    else()
        set(test_csproj "${cs_source_dir}/${test_name}.tests.csproj")
    endif()

    if (NOT EXISTS "${test_csproj}")
        message(FATAL_ERROR "Binding e2e test csproj not found: ${test_csproj}")
    endif()

    get_filename_component(cpp_source_ext "${cpp_source}" EXT)

    set(cpp_impl_sources)
    if (cpp_source_ext STREQUAL ".cpp" OR cpp_source_ext STREQUAL ".cc" OR cpp_source_ext STREQUAL ".cxx")
        list(APPEND cpp_impl_sources "${cpp_source}")
    endif()

    set(test_root "${CMAKE_BINARY_DIR}/binding_e2e/${test_name}")
    set(generated_root "${test_root}/generated")
    set(generated_cabi "${generated_root}/cabi/${test_name}_cabi.cpp")
    set(generated_stamp "${test_root}/generated.stamp")
    set(dotnet_root "${test_root}/dotnet")
    set(dotnet_test_output "${dotnet_root}")
    set(native_root "${test_root}/native/$<CONFIG>")
    set(results_root "${test_root}/results")

    set(generator_target "${test_name}_bindings_generator")
    add_executable(${generator_target}
        ${PROJECT_SOURCE_DIR}/tests/support/binding_e2e_generator_main.cpp
        ${cpp_impl_sources}
    )

    set_target_properties(${generator_target}
        PROPERTIES
            EXCLUDE_FROM_ALL TRUE
            FOLDER "internal/e2e"
    )

    target_link_libraries(${generator_target}
        PRIVATE
            csbind23
    )

    target_include_directories(${generator_target}
        PRIVATE
            ${PROJECT_SOURCE_DIR}
            ${PROJECT_SOURCE_DIR}/include
    )

    target_compile_definitions(${generator_target}
        PRIVATE
            CSBIND23_TEST_MODULE_NAME="${test_name}"
            CSBIND23_TEST_BINDINGS_SOURCE=\"${CSBE_CPP_SOURCE}\"
    )

    add_custom_command(
        OUTPUT ${generated_stamp} ${generated_cabi}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${generated_root}
        COMMAND $<TARGET_FILE:${generator_target}> ${generated_root}
        COMMAND ${CMAKE_COMMAND} -E touch ${generated_stamp}
        DEPENDS ${generator_target}
        COMMENT "Generating binding-e2e artifacts for ${test_name}"
        VERBATIM
    )

    set(native_target "${test_name}_native")
    set(native_loader_file "${dotnet_test_output}/${CMAKE_SHARED_LIBRARY_PREFIX}${test_name}${CMAKE_SHARED_LIBRARY_SUFFIX}")

    add_library(${native_target} SHARED
        ${generated_cabi}
        ${cpp_impl_sources}
    )

    set_target_properties(${native_target}
        PROPERTIES
            PREFIX ""
            SUFFIX ""
            OUTPUT_NAME "${test_name}${CMAKE_SHARED_LIBRARY_SUFFIX}"
            EXCLUDE_FROM_ALL TRUE
            FOLDER "internal/e2e"
    )

    set_target_properties(${native_target}
        PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${native_root}"
            LIBRARY_OUTPUT_DIRECTORY "${native_root}"
            ARCHIVE_OUTPUT_DIRECTORY "${test_root}/lib/$<CONFIG>"
    )

    target_include_directories(${native_target}
        PRIVATE
            ${PROJECT_SOURCE_DIR}
            ${PROJECT_SOURCE_DIR}/include
    )

    set(dotnet_build_stamp "${test_root}/dotnet_build.stamp")
    add_custom_command(
        OUTPUT ${dotnet_build_stamp}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${dotnet_root}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${dotnet_test_output}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${results_root}
        COMMAND ${DOTNET_EXECUTABLE} restore ${test_csproj} -p:CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR} -p:GeneratedBindingsDir=${generated_root} -p:BaseOutputPath=${dotnet_root}/
        COMMAND ${DOTNET_EXECUTABLE} build ${test_csproj} --configuration $<CONFIG> --no-restore -p:CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR} -p:GeneratedBindingsDir=${generated_root} -p:BaseOutputPath=${dotnet_root}/
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${native_target}> ${native_loader_file}
        COMMAND ${CMAKE_COMMAND} -E touch ${dotnet_build_stamp}
        DEPENDS ${native_target}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Restoring and building binding-e2e managed target for ${test_name}"
        VERBATIM
    )

    set(dotnet_test_stamp "${test_root}/dotnet_test.stamp")
    add_custom_command(
        OUTPUT ${dotnet_test_stamp}
        COMMAND ${CMAKE_COMMAND} -E env LD_LIBRARY_PATH=${native_root}:$ENV{LD_LIBRARY_PATH} ${DOTNET_EXECUTABLE} test ${test_csproj} --configuration $<CONFIG> --no-build --no-restore --results-directory ${results_root} -p:CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR} -p:GeneratedBindingsDir=${generated_root} -p:BaseOutputPath=${dotnet_root}/
        COMMAND ${CMAKE_COMMAND} -E touch ${dotnet_test_stamp}
        DEPENDS ${dotnet_build_stamp}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Running binding-e2e C# xUnit tests for ${test_name}"
        VERBATIM
    )

    csbind23_register_managed_project(${test_csproj})
    set_property(GLOBAL APPEND PROPERTY CSBIND23_E2E_MANAGED_TEST_STAMPS ${dotnet_test_stamp})

endfunction()
