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

        cache/l2cache.cpp
        cache/redis.cpp
        cache/asyncredisif.cpp
        cache/asyncredis.cpp
        cache/asyncredissubscriber.cpp
        )

add_dependencies(bamboo libboost libzookeeper libleveldb libhiredis)
target_link_libraries(bamboo
        boost_system.a
        boost_filesystem.a
        libboost_program_options.a
        libzookeeper_mt.a
        libleveldb.a
        ${PROJECT_BINARY_DIR}/lib/libhiredis.a)