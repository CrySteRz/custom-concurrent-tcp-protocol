add_executable(server main.cpp client_controller.cpp)
target_link_libraries(server PRIVATE common)

set_target_properties(server PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
