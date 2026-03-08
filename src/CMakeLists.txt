add_library(csbind23)

set_target_properties(csbind23
    PROPERTIES
        POSITION_INDEPENDENT_CODE ON
)

target_sources(csbind23
    PRIVATE
        bindings_generator.cpp
        emit/cabi_emitter.cpp
        emit/cabi_emitter_utils.cpp
        emit/emitter_ir_utils.cpp
        emit/csharp_naming.cpp
        emit/csharp_emitter_text_utils.cpp
        emit/generic_dispatch_utils.cpp
        emit/csharp_emitter.cpp
        OffsetAllocator/offsetAllocator.cpp
        temporary_memory_allocator.cpp
)

target_include_directories(csbind23
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_definitions(csbind23
    PUBLIC
        CSBIND23_TEMPORARY_ALLOCATOR_SIZE=${CSBIND23_TEMPORARY_ALLOCATOR_SIZE}
)
