add_library(common STATIC protocol.cpp)
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
