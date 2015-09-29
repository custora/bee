#include <stdlib.h>
#include <exception>
#include <sstream>

#include <boost/format.hpp>
#include "Rinternals.h"

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
  TFetchResultsReq result_request;
  TFetchResultsResp result_response;
  TGetResultSetMetadataReq metadata_request;
  TGetResultSetMetadataResp metadata_response;

  result_request.operationHandle = this->operation_handle;
  result_request.orientation = TFetchOrientation::FETCH_NEXT;
  result_request.maxRows = num_rows;
  this->client->FetchResults(result_response, result_request);

  metadata_request.operationHandle = this->operation_handle;
  this->client->GetResultSetMetadata(metadata_response, metadata_request);

  return build_data_frame(metadata_response.schema, result_response.results);
}

Rcpp::List HiveClient::Client::build_data_frame(const TTableSchema schema, const TRowSet &row_set) const {
  const unsigned int num_columns = schema.columns.size();
  const unsigned int num_rows = row_set.rows.size();

  Rcpp::List result(num_columns);
  result.attr("class") = "data.frame";
  result.attr("row.names") = Rcpp::IntegerVector::create(NA_INTEGER, -num_rows);

  Rcpp::StringVector names(num_columns, NA_STRING);
  for (unsigned int c = 0; c < num_columns; ++c) {
    names[c] = schema.columns[c].columnName;
  }
  result.attr("names") = names;

  for (unsigned int c = 0; c < num_columns; ++c) {
    if (schema.columns[c].typeDesc.types.size() < 1) {
      throw std::runtime_error("BUG: empty type");
    }

    if (!schema.columns[c].typeDesc.types[0].__isset.primitiveEntry) {
      throw std::runtime_error("ERROR: only primitive types currently supported");
    }

    switch (schema.columns[c].typeDesc.types[0].primitiveEntry.type) {
    case TTypeId::BOOLEAN_TYPE:
      result[c] = build_logical_column_from_bool_value(row_set, c);
      break;

    case TTypeId::TINYINT_TYPE:
      result[c] = build_int_column_from_byte_value(row_set, c);
      break;

    case TTypeId::SMALLINT_TYPE:
      result[c] = build_int_column_from_i16_value(row_set, c);
      break;

    case TTypeId::INT_TYPE:
      result[c] = build_int_column_from_i32_value(row_set, c);
      break;

    case TTypeId::BIGINT_TYPE:
      result[c] = build_int_column_from_i64_value(row_set, c);
      break;

    case TTypeId::FLOAT_TYPE:
    case TTypeId::DOUBLE_TYPE:
      result[c] = build_double_column_from_double_value(row_set, c);
      break;

    case TTypeId::STRING_TYPE:
    case TTypeId::VARCHAR_TYPE:
    case TTypeId::CHAR_TYPE:
    case TTypeId::BINARY_TYPE:
    case TTypeId::TIMESTAMP_TYPE:
    case TTypeId::DATE_TYPE:
      result[c] = build_string_column_from_string_value(row_set, c);
      break;

    case TTypeId::DECIMAL_TYPE:
      result[c] = build_double_column_from_string_value(row_set, c);
      break;

    case TTypeId::NULL_TYPE:  // for "select null"
      result[c] = build_null_column_from_null_value(row_set);
      break;

    case TTypeId::ARRAY_TYPE:
    case TTypeId::MAP_TYPE:
    case TTypeId::STRUCT_TYPE:
    case TTypeId::UNION_TYPE:
    case TTypeId::USER_DEFINED_TYPE:
    default:
      TTypeId::type type_id = schema.columns[c].typeDesc.types[0].primitiveEntry.type;
      boost::format format("ERROR: unsupported column type id: %1%");
      throw std::runtime_error(boost::str(format % type_id));
      build_error_column(row_set, "unsupported column type");
    }
  }

  return result;
}

Rcpp::LogicalVector HiveClient::Client::build_logical_column_from_bool_value(const TRowSet &row_set, unsigned int column_index) const {
  unsigned int num_rows = row_set.rows.size();
  Rcpp::LogicalVector result(num_rows, NA_LOGICAL);
  for (unsigned int i = 0; i < num_rows; ++i) {
    result[i] = row_set.rows[i].colVals[column_index].boolVal.value;
  }
  return result;
}

Rcpp::IntegerVector HiveClient::Client::build_int_column_from_byte_value(const TRowSet &row_set, unsigned int column_index) const {
  unsigned int num_rows = row_set.rows.size();
  Rcpp::IntegerVector result(num_rows, NA_INTEGER);
  for (unsigned int i = 0; i < num_rows; ++i) {
    result[i] = row_set.rows[i].colVals[column_index].byteVal.value;
  }
  return result;
}

Rcpp::IntegerVector HiveClient::Client::build_int_column_from_i16_value(const TRowSet &row_set, unsigned int column_index) const {
  unsigned int num_rows = row_set.rows.size();
  Rcpp::IntegerVector result(num_rows, NA_INTEGER);
  for (unsigned int i = 0; i < num_rows; ++i) {
    result[i] = row_set.rows[i].colVals[column_index].i16Val.value;
  }
  return result;
}

Rcpp::IntegerVector HiveClient::Client::build_int_column_from_i32_value(const TRowSet &row_set, unsigned int column_index) const {
  unsigned int num_rows = row_set.rows.size();
  Rcpp::IntegerVector result(num_rows, NA_INTEGER);
  for (unsigned int i = 0; i < num_rows; ++i) {
    result[i] = row_set.rows[i].colVals[column_index].i32Val.value;
  }
  return result;
}

Rcpp::IntegerVector HiveClient::Client::build_int_column_from_i64_value(const TRowSet &row_set, unsigned int column_index) const {
  unsigned int num_rows = row_set.rows.size();
  Rcpp::IntegerVector result(num_rows, NA_INTEGER);
  for (unsigned int i = 0; i < num_rows; ++i) {
    result[i] = row_set.rows[i].colVals[column_index].i64Val.value;
  }
  return result;
}

Rcpp::StringVector HiveClient::Client::build_string_column_from_string_value(const TRowSet &row_set, unsigned int column_index) const {
  unsigned int num_rows = row_set.rows.size();
  Rcpp::StringVector result(num_rows, NA_STRING);
  for (unsigned int i = 0; i < num_rows; ++i) {
    result[i] = row_set.rows[i].colVals[column_index].stringVal.value;
  }
  return result;
}

Rcpp::DoubleVector HiveClient::Client::build_double_column_from_double_value(const TRowSet &row_set, unsigned int column_index) const {
  unsigned int num_rows = row_set.rows.size();
  Rcpp::DoubleVector result(num_rows, NA_REAL);
  for (unsigned int i = 0; i < num_rows; ++i) {
    result[i] = row_set.rows[i].colVals[column_index].doubleVal.value;
  }
  return result;
}

Rcpp::DoubleVector HiveClient::Client::build_double_column_from_string_value(const TRowSet &row_set, unsigned int column_index) const {
  unsigned int num_rows = row_set.rows.size();
  Rcpp::DoubleVector result(num_rows, NA_REAL);
  for (unsigned int i = 0; i < num_rows; ++i) {
    result[i] = atof(row_set.rows[i].colVals[column_index].stringVal.value.c_str());
  }
  return result;
}

Rcpp::IntegerVector HiveClient::Client::build_null_column_from_null_value(const TRowSet &row_set) const {
  return Rcpp::IntegerVector(row_set.rows.size(), NA_INTEGER);
}

Rcpp::StringVector HiveClient::Client::build_error_column(const TRowSet &row_set, const char *error) const {
  unsigned int num_rows = row_set.rows.size();
  Rcpp::StringVector result(num_rows, NA_STRING);
  for (unsigned int i = 0; i < num_rows; ++i) {
    result[i] = error;
  }
  return result;
}
