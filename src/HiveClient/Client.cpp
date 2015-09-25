#include <exception>
#include <sstream>
#include "HiveClient/Client.h"

using namespace apache::thrift;
using namespace apache::hive::service::cli::thrift;

HiveClient::Client::Client(const std::string &hostname, int port, const std::string &user, const std::string &pass) :
  user(user),
  pass(pass),
  socket(new transport::TSocket(hostname, port)),
  transport(new transport::TFramedTransport(socket)),
  protocol(new protocol::TBinaryProtocol(transport)),
  client(new apache::hive::service::cli::thrift::TCLIServiceClient(protocol)),
  session_handle(),
  operation_handle() {
    this->transport->open();
    this->perform_sasl_handshake();
    this->open_session();
}

void HiveClient::Client::perform_sasl_handshake() {
  this->socket->write(reinterpret_cast<const unsigned char *>("\x01\x00\x00\x00\x05" "PLAIN"), 10);  // START PLAIN
  this->socket->write(reinterpret_cast<const unsigned char *>("\x02\x00\x00\x00\x1b" "[PLAIN]" "\x00" "anonymous" "\x00" "anonymous"), 32);  // OK PLAIN user pass
  this->socket->flush();

  unsigned char buffer[6];
  int message_length = 0;
  this->socket->read(buffer, 5);
  buffer[5] = '\0';
  switch (buffer[0]) {
    case 3: // bad
    case 4: { // error
      for (int i = 0; i < 4; ++i)
      message_length += static_cast<int>(buffer[1 + i]) << i;
      char message[message_length + 1];
      this->socket->read(reinterpret_cast<unsigned char *>(message), message_length);
      message[message_length] = '\0';
      throw std::runtime_error(message);
      break;
    }

    case 5: { // complete
      for (int i = 0; i < 4; ++i)
      message_length += static_cast<int>(buffer[1 + i]) << i;
      char message[message_length + 1];
      this->socket->read(reinterpret_cast<unsigned char *>(message), message_length);
      // message[message_length] = '\0';
      // challenge = message;
      break;
    }

    case 2: // ok
    throw std::runtime_error("failed to complete challenge exchange");
    break;

    default:
    throw std::runtime_error("unexpected response from sasl handshake");
  }
}

void HiveClient::Client::open_session() {
  TOpenSessionReq request;
  TOpenSessionResp response;
  request.client_protocol = TProtocolVersion::HIVE_CLI_SERVICE_PROTOCOL_V3;

  client->OpenSession(response, request);
  this->session_handle = response.sessionHandle;
}

std::string HiveClient::Client::inspect() {
  std::ostringstream out;
  out << "HiveClient::Client[" <<
    "hostname=" << this->socket->getHost() <<
    ", port=" << this->socket->getPort() <<
    ", user=" << this->user <<
    ", pass=" << this->pass <<
    (this->has_session_handle() ? ", in session" : "") <<
    (this->has_operation_handle() ? ", in operation" : "") <<
    "]";
  return out.str();
}

bool HiveClient::Client::has_session_handle() const {
  return !this->session_handle.sessionId.guid.empty();
}

bool HiveClient::Client::has_operation_handle() const {
  return !this->operation_handle.operationId.guid.empty();
}

void HiveClient::Client::execute(const std::string &sql) {
  TExecuteStatementReq request;
  TExecuteStatementResp response;
  request.sessionHandle = this->session_handle;
  request.statement = sql;

  this->client->ExecuteStatement(response, request);
  this->operation_handle = response.operationHandle;
}

Rcpp::List HiveClient::Client::fetch(int num_rows) {
  TFetchResultsReq request;
  TFetchResultsResp response;
  request.operationHandle = this->operation_handle;
  request.orientation = TFetchOrientation::FETCH_NEXT;
  request.maxRows = num_rows;

  this->client->FetchResults(response, request);

  Rcpp::List result(1);
  result.attr("names") = Rcpp::StringVector::create("result");
  result.attr("class") = "data.frame";
  result.attr("row.names") = Rcpp::IntegerVector::create(NA_INTEGER, -1);
  result[0] = Rcpp::IntegerVector::create(response.results.rows[0].colVals[0].i32Val.value);
  return result;
}
