add_executable(console-server main.cpp)
add_dependencies(console-server bamboo)
target_link_libraries(console-server bamboo)