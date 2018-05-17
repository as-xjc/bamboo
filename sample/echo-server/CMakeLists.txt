add_executable(echo-server main.cpp)
add_dependencies(echo-server bamboo)
target_link_libraries(echo-server bamboo)