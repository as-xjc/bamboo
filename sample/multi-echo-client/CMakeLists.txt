add_executable(multi-echo-client main.cpp)
add_dependencies(multi-echo-client bamboo)
target_link_libraries(multi-echo-client bamboo)