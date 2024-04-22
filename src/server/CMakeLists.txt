add_executable(server main.cpp)
target_link_libraries(server PRIVATE common)
target_include_directories(server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../common)

set_target_properties(server PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
