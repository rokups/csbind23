add_library(csbind23_bindings_generator)
add_library(csbind23::bindings_generator ALIAS csbind23_bindings_generator)

target_sources(csbind23_bindings_generator
    PRIVATE
        bindings_generator.cpp
        emit/cabi_emitter.cpp
        emit/csharp_emitter.cpp
)

target_include_directories(csbind23_bindings_generator
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)
