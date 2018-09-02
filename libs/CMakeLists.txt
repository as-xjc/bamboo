add_library(bamboo
        env.cpp

        aio/aioif.cpp
        aio/aio.cpp
        aio/multiaio.cpp

        buffer/dynamicbuffer.cpp

        log/log.cpp

        net/acceptorif.cpp
        net/simpleacceptor.cpp
        net/connmanagerif.cpp
        net/simpleconnmanager.cpp
        net/socketif.cpp
        net/socket.cpp
        net/connectorif.cpp

        protocol/protocolif.cpp

        server/serverif.cpp
        server/simpleserver.cpp
        server/console.cpp

        schedule/scheduler.cpp

        concurrency/taskpool.cpp
        concurrency/asyncrun.cpp

        utility/timemeasure.cpp

        distributed/registry.cpp
        )

add_dependencies(bamboo libboost libzookeeper libhiredis)
target_link_libraries(bamboo
        boost_system.a
        boost_filesystem.a
        libboost_program_options.a
        libzookeeper_mt.a)