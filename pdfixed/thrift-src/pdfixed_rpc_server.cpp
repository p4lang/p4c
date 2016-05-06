#ifdef P4THRIFT
#include <p4thrift/protocol/TBinaryProtocol.h>
#include <p4thrift/server/TSimpleServer.h>
#include <p4thrift/server/TThreadedServer.h>
#include <p4thrift/transport/TServerSocket.h>
#include <p4thrift/transport/TBufferTransports.h>
#include <p4thrift/processor/TMultiplexedProcessor.h>

namespace thrift_provider = p4::thrift;
#else
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/processor/TMultiplexedProcessor.h>

namespace thrift_provider = apache::thrift;
#endif

using namespace ::thrift_provider;
using namespace ::thrift_provider::protocol;
using namespace ::thrift_provider::transport;
using namespace ::thrift_provider::server;

using boost::shared_ptr;

#include <pthread.h>

#include "conn_mgr_pd_rpc_server.ipp"
#include "mc_pd_rpc_server.ipp"
#include "sswitch_pd_rpc_server.ipp"

static pthread_mutex_t cookie_mutex;
static pthread_cond_t cookie_cv;
static void *cookie;

#define BFN_PD_RPC_SERVER_PORT 9090

namespace {

/*
 * Thread wrapper for starting the server
 */

void *rpc_server_thread(void *) {
  int port = BFN_PD_RPC_SERVER_PORT;
  
  shared_ptr<mcHandler> mc_handler(new mcHandler());
  shared_ptr<sswitchHandler> sswitch_handler(new sswitchHandler());
  shared_ptr<conn_mgrHandler> conn_mgr_handler(new conn_mgrHandler());
  
  shared_ptr<TMultiplexedProcessor> processor(new TMultiplexedProcessor());
  processor->registerProcessor(
    "mc", shared_ptr<TProcessor>(new mcProcessor(mc_handler)));
  processor->registerProcessor(
    "sswitch", shared_ptr<TProcessor>(new sswitchProcessor(sswitch_handler)));
  processor->registerProcessor(
    "conn_mgr", shared_ptr<TProcessor>(new conn_mgrProcessor(conn_mgr_handler)));

  shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
  
  TThreadedServer server(processor, serverTransport, transportFactory, protocolFactory);
  
  pthread_mutex_lock(&cookie_mutex);
  cookie = (void *) processor.get();
  pthread_cond_signal(&cookie_cv);
  pthread_mutex_unlock(&cookie_mutex);
  
  server.serve();
  
  return NULL;
}

pthread_t rpc_thread;

}  // namespace

extern "C" {

int start_bfn_pd_rpc_server(void **server_cookie)
{
  pthread_mutex_init(&cookie_mutex, NULL);
  pthread_cond_init(&cookie_cv, NULL);
  
  std::cerr << "Starting RPC server on port " << 
    BFN_PD_RPC_SERVER_PORT << std::endl;

  *server_cookie = NULL;
  
  int status = pthread_create(&rpc_thread, NULL, rpc_server_thread, NULL);

  if(status) return status;
  
  pthread_mutex_lock(&cookie_mutex);
  while(!cookie) {
    pthread_cond_wait(&cookie_cv, &cookie_mutex);
  }
  pthread_mutex_unlock(&cookie_mutex);

  *server_cookie = cookie;

  pthread_mutex_destroy(&cookie_mutex);
  pthread_cond_destroy(&cookie_cv);
  
  return 0;
}

}
