add_executable(admin_client main.cpp)
target_link_libraries(admin_client PRIVATE common)
target_include_directories(admin_client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../common)
set_target_properties(admin_client PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
