#ifndef _BM_STDCXX_H_
#define _BM_STDCXX_H_

#include <bm/config.h>

namespace thrift_provider = apache::thrift;

#ifdef BM_HAVE_THRIFT_STDCXX_H
#include <thrift/stdcxx.h>
namespace stdcxx = thrift_provider::stdcxx;
#else
namespace stdcxx = boost;
#endif

#endif
