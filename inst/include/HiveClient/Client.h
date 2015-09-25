#ifndef __HiveClient_Client_h__
#define __HiveClient_Client_h__

#include <string>

#include <Rcpp.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include "tcli_thrift/TCLIService.h"

namespace HiveClient {
  using namespace apache::thrift;
  using namespace apache::hive::service::cli::thrift;

  class Client {
  public:
    Client(const std::string &hostname, int port, const std::string &user, const std::string &pass);
    void execute(const std::string &sql);
    Rcpp::List fetch(int num_rows);
    std::string inspect();

  private:
    void perform_sasl_handshake();
    void open_session();
    bool has_session_handle() const;
    bool has_operation_handle() const;

    std::string user;
    std::string pass;
    boost::shared_ptr<transport::TSocket> socket;
    boost::shared_ptr<transport::TFramedTransport> transport;
    boost::shared_ptr<protocol::TProtocol> protocol;
    boost::shared_ptr<TCLIServiceClient> client;

    TSessionHandle session_handle;
    TOperationHandle operation_handle;
  };
}

#endif // __HiveClient_Client_h__
