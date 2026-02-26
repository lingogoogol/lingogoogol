#ifdef LGO_IMPORT

import <iostream>;

import <boost/asio.hpp>

import lgo.basic;
import lgo.math;
import lgo.encode;

#else

#include <iostream>

#include <sdkddkver.h>
#include <boost/asio.hpp>
#undef near
#undef far
#undef DELETE

#include <lgo/basic/.hpp>
#include <lgo/math/.hpp>
#include <lgo/encode/.hpp>

#endif