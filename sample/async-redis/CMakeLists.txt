add_executable(async-redis main.cpp)
add_dependencies(async-redis bamboo)
target_link_libraries(async-redis bamboo)