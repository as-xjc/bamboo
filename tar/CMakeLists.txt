include(ExternalProject)

#配置需要编译的boost所用的库
set(BUILD_BOOST_LIBS "system,filesystem,program_options")
ExternalProject_Add(libboost
        URL https://dl.bintray.com/boostorg/release/1.67.0/source/boost_1_67_0.tar.gz
        CONFIGURE_COMMAND ./bootstrap.sh --with-libraries=${BUILD_BOOST_LIBS}
        BUILD_IN_SOURCE 1
        BUILD_COMMAND ""
        INSTALL_COMMAND ./b2 cxxflags=--std=c++11 link=static install --prefix=${PROJECT_BINARY_DIR}
        )

ExternalProject_Add(libgtest
        URL https://github.com/google/googletest/archive/release-1.8.0.tar.gz
        CMAKE_COMMAND ${CMAKE_COMMAND} . -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${PROJECT_BINARY_DIR}
        BUILD_COMMAND make
        INSTALL_COMMAND make install
        )

ExternalProject_Add(libzookeeper
        URL ${PROJECT_SOURCE_DIR}/tar/zookeeper-c-3.4.12.tar
        CONFIGURE_COMMAND ./configure --prefix=${PROJECT_BINARY_DIR} --enable-shared=no
        BUILD_IN_SOURCE 1
        BUILD_COMMAND make
        INSTALL_COMMAND make install
        )

ExternalProject_Add(libhiredis
        URL https://github.com/redis/hiredis/archive/v0.13.3.tar.gz
        CONFIGURE_COMMAND ""
        BUILD_IN_SOURCE 1
        BUILD_COMMAND make
        INSTALL_COMMAND make PREFIX=${PROJECT_BINARY_DIR} install
        )