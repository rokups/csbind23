include_guard(GLOBAL)

function(csbind23_generate_msbuild_props)
    set(cmake_binary_dir "${CMAKE_BINARY_DIR}")
    file(TO_CMAKE_PATH "${cmake_binary_dir}" cmake_binary_dir)

    set(cmake_source_dir "${PROJECT_SOURCE_DIR}")
    file(TO_CMAKE_PATH "${cmake_source_dir}" cmake_source_dir)

    set(cmake_props_content
"<Project>\n  <PropertyGroup>\n    <CMakePropsIncluded>true</CMakePropsIncluded>\n    <CMAKE_BINARY_DIR>${cmake_binary_dir}</CMAKE_BINARY_DIR>\n    <CMAKE_SOURCE_DIR>${cmake_source_dir}</CMAKE_SOURCE_DIR>\n  </PropertyGroup>\n</Project>\n")

    file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/CMake.props" CONTENT "${cmake_props_content}")

    set(last_props_path "${PROJECT_SOURCE_DIR}/.CMakeLast.props")
    set(last_props_content
"<Project>\n  <Import Project=\"${cmake_binary_dir}/CMake.props\" Condition=\"Exists('${cmake_binary_dir}/CMake.props')\" />\n</Project>\n")
    file(WRITE "${last_props_path}" "${last_props_content}")
endfunction()

function(csbind23_register_managed_project project_path)
    if (NOT project_path)
        message(FATAL_ERROR "csbind23_register_managed_project requires project_path")
    endif()

    if (IS_ABSOLUTE "${project_path}")
        set(resolved_path "${project_path}")
    else()
        set(resolved_path "${PROJECT_SOURCE_DIR}/${project_path}")
    endif()

    get_property(existing GLOBAL PROPERTY CSBIND23_MANAGED_PROJECTS)
    if (NOT existing)
        set(existing "")
    endif()

    list(FIND existing "${resolved_path}" existing_idx)
    if (existing_idx EQUAL -1)
        set_property(GLOBAL APPEND PROPERTY CSBIND23_MANAGED_PROJECTS "${resolved_path}")
    endif()
endfunction()

function(csbind23_register_solution_build_dependency target_name)
    if (NOT target_name)
        message(FATAL_ERROR "csbind23_register_solution_build_dependency requires target_name")
    endif()

    get_property(existing GLOBAL PROPERTY CSBIND23_SOLUTION_BUILD_DEPENDENCIES)
    if (NOT existing)
        set(existing "")
    endif()

    list(FIND existing "${target_name}" existing_idx)
    if (existing_idx EQUAL -1)
        set_property(GLOBAL APPEND PROPERTY CSBIND23_SOLUTION_BUILD_DEPENDENCIES "${target_name}")
    endif()
endfunction()

function(csbind23_add_managed_aggregate_targets)
    find_program(DOTNET_EXECUTABLE NAMES dotnet)
    if (NOT DOTNET_EXECUTABLE)
        message(STATUS "Dotnet SDK not found. Skipping managed aggregate targets.")
        return()
    endif()

    get_property(managed_projects GLOBAL PROPERTY CSBIND23_MANAGED_PROJECTS)
    get_property(solution_build_dependencies GLOBAL PROPERTY CSBIND23_SOLUTION_BUILD_DEPENDENCIES)
    if (NOT managed_projects)
        message(STATUS "No managed projects registered. Skipping managed aggregate targets.")
        return()
    endif()

    set(managed_root "${CMAKE_BINARY_DIR}/managed")
    set(managed_solution "${CMAKE_BINARY_DIR}/csbind23.managed.slnx")
    set(restore_stamp "${managed_root}/restore.stamp")

    file(MAKE_DIRECTORY "${managed_root}")

    execute_process(
        COMMAND ${DOTNET_EXECUTABLE} new sln --name csbind23.managed --output ${CMAKE_BINARY_DIR} --force
        RESULT_VARIABLE sln_result
    )
    if (NOT sln_result EQUAL 0)
        message(FATAL_ERROR "Failed to create managed solution at ${managed_solution}")
    endif()

    foreach(project_path IN LISTS managed_projects)
        execute_process(
            COMMAND ${DOTNET_EXECUTABLE} sln ${managed_solution} add ${project_path}
            RESULT_VARIABLE add_result
            OUTPUT_QUIET
            ERROR_QUIET
        )
        if (NOT add_result EQUAL 0)
            message(FATAL_ERROR "Failed adding managed project to solution: ${project_path}")
        endif()
    endforeach()

    add_custom_command(
        OUTPUT ${restore_stamp}
        COMMAND ${DOTNET_EXECUTABLE} restore ${managed_solution} -p:CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E touch ${restore_stamp}
        DEPENDS ${managed_solution}
        VERBATIM
    )

    add_custom_target(solution.restore
        COMMAND ${DOTNET_EXECUTABLE} restore ${managed_solution} -p:CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}
        DEPENDS ${managed_solution}
        COMMENT "Restoring managed solution"
        VERBATIM
    )

    add_custom_target(solution.build
        COMMAND ${DOTNET_EXECUTABLE} build ${managed_solution} --configuration $<CONFIG> --no-restore -p:CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}
        DEPENDS ${restore_stamp} ${solution_build_dependencies}
        COMMENT "Building managed solution"
        VERBATIM
    )
endfunction()
