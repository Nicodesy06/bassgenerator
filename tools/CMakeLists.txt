add_executable(pattern_diagnostics pattern_diagnostics.cpp)
target_link_libraries(pattern_diagnostics PRIVATE gfb_core)

add_executable(render_demo render_demo.cpp)
target_link_libraries(render_demo PRIVATE gfb_core)
target_include_directories(render_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(gfb_benchmark benchmark.cpp)
target_link_libraries(gfb_benchmark PRIVATE gfb_core)

add_executable(reference_analyzer reference_analyzer.cpp)
target_link_libraries(reference_analyzer PRIVATE gfb_core)

add_executable(pattern_dump pattern_dump.cpp)
target_link_libraries(pattern_dump PRIVATE gfb_core)
