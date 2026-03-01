add_library(csbind23)

target_sources(csbind23
    PRIVATE
        bindings_generator.cpp
        emit/cabi_emitter.cpp
    emit/csharp_naming.cpp
        emit/csharp_emitter.cpp
)

target_include_directories(csbind23
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)
