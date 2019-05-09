#ifndef _BM_STDCXX_H_
#define _BM_STDCXX_H_

#include <bm/config.h>

namespace thrift_provider = apache::thrift;

#if BM_THRIFT_VERSION < 1300
#ifdef BM_HAVE_THRIFT_STDCXX_H
#include <thrift/stdcxx.h>
namespace stdcxx = thrift_provider::stdcxx;
#else
namespace stdcxx = boost;
#endif  // BM_HAVE_THRIFT_STDCXX_H
#else
namespace stdcxx = std;
#endif  // BM_THRIFT_VERSION < 1300

#endif
