add_executable(echo-client main.cpp)
add_dependencies(echo-client bamboo)
target_link_libraries(echo-client bamboo)