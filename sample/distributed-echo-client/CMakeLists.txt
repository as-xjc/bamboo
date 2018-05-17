add_executable(distributed-echo-client main.cpp)
add_dependencies(distributed-echo-client bamboo)
target_link_libraries(distributed-echo-client bamboo)