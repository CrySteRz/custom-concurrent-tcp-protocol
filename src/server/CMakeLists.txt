include_directories("/usr/include")

add_executable(server main.cpp client_controller.cpp)
target_link_libraries(server PRIVATE common db)
target_link_libraries(server PRIVATE zstd)
target_link_libraries(server PRIVATE lz4)
target_link_libraries(server PRIVATE lzma)
target_link_libraries(server PRIVATE z)
target_link_libraries(server PRIVATE zip)
target_link_libraries(server PRIVATE archive)
set_target_properties(server PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
