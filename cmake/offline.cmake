add_executable(mbgl-offline
    bin/offline.cpp
)

target_sources(mbgl-offline
    PRIVATE platform/default/mbgl/util/default_styles.hpp
)

target_include_directories(mbgl-offline
    PRIVATE platform/default
)

target_link_libraries(mbgl-offline
    PRIVATE mbgl-core
)

target_add_mason_package(mbgl-offline PRIVATE boost)
target_add_mason_package(mbgl-offline PRIVATE args)

mbgl_platform_offline()

create_source_groups(mbgl-offline)
