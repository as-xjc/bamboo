add_executable(async-redis main.cpp)
add_dependencies(async-redis bamboo libredis-asio)
target_link_libraries(async-redis bamboo libredis-asio.a libhiredis.a)