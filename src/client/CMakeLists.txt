add_executable(client main.cpp)
target_link_libraries(client PRIVATE common)
set_target_properties(client PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
