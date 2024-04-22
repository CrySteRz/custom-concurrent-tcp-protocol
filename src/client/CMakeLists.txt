add_executable(client main.cpp)
target_link_libraries(client PRIVATE common)
target_include_directories(client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../common)
set_target_properties(client PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
