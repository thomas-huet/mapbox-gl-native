# Load Node.js
set(NODE_MODULE_CACHE_DIR "${CMAKE_SOURCE_DIR}/build/headers")
include(node_modules/@mapbox/cmake-node-module/module.cmake)

add_library(mbgl-loop-node INTERFACE)

target_sources(mbgl-loop-node INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/default/async_task.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/default/run_loop.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/default/timer.cpp
)

target_include_directories(mbgl-loop-node INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

create_source_groups(mbgl-loop-node)


add_node_module(mbgl-node
    INSTALL_DIR "lib"
    NAN_VERSION "2.10.0"
    EXCLUDE_NODE_ABIS 47 51 59 # Don't build old beta ABIs 5.x, 7.x, and 9.x
)

target_sources(mbgl-node INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/node/src/node_mapbox_gl_native.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/node/src/node_mapbox_gl_native.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/node/src/node_logging.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/node/src/node_logging.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/node/src/node_conversion.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/node/src/node_map.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/node/src/node_map.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/node/src/node_request.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/node/src/node_request.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/node/src/node_feature.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/node/src/node_feature.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/node/src/node_thread_pool.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/node/src/node_thread_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/node/src/node_expression.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/node/src/node_expression.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/node/src/util/async_queue.hpp
)

target_include_directories(mbgl-node INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/default
)

target_link_libraries(mbgl-node INTERFACE
    mbgl-core
    mbgl-loop-node
)

target_add_mason_package(mbgl-node INTERFACE geojson)

add_custom_target(mbgl-node.active DEPENDS mbgl-node.abi-${NodeJS_ABI})

mbgl_platform_node()

create_source_groups(mbgl-node)
