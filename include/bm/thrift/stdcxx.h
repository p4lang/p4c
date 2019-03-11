#ifndef _BM_STDCXX_H_
#define _BM_STDCXX_H_

#include <bm/config.h>

#ifdef BM_P4THRIFT
namespace thrift_provider = p4::thrift;
#else
namespace thrift_provider = apache::thrift;
#endif

#ifdef BM_HAVE_THRIFT_STDCXX_H
#include <thrift/stdcxx.h>
namespace stdcxx = thrift_provider::stdcxx;
#else
namespace stdcxx = boost;
#endif

#endif
