

set(SOURCES
    quantize.cpp
    trace.cpp
    trace_wrapper.cpp
)

add_library(GlaxnimateTrace STATIC ${SOURCES})
add_library(Glaxnimate::Trace ALIAS GlaxnimateTrace)

# Potrace
option(GLAXNIMATE_SYSTEM_POTRACE "Use the system potrace library" ON)
if (NOT GLAXNIMATE_SYSTEM_POTRACE)
    # Custom potrace
    message(STATUS "Using bundled potrace")
    set(CMAKE_C_COMPILE_OBJECT ${CMAKE_CXX_COMPILE_OBJECT})
    add_subdirectory(external/potrace/)
    set(Potrace_LIBRARIES potrace)
    set(Potrace_INCLUDE_DIRS external/potrace/src/)
else()
    # system potrace
    find_package(Potrace REQUIRED)
endif()

target_include_directories(GlaxnimateTrace PUBLIC ${Potrace_INCLUDE_DIRS})
target_link_libraries(GlaxnimateTrace PUBLIC ${Potrace_LIBRARIES} Qt::Gui Glaxnimate::Core)
