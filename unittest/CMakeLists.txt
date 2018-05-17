add_executable(unittest
        main.cpp
        )

add_dependencies(unittest bamboo libgtest)
target_link_libraries(unittest bamboo libgtest.a)

add_test(NAME UnitTest COMMAND unittest)