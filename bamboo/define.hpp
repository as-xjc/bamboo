#pragma once

#include <boost/asio.hpp>

#include <bamboo/log/log.hpp>

#ifdef NDEBUG
#define BB_ASSERT(x)
#else
#define BB_ASSERT(x) assert(x)
#endif