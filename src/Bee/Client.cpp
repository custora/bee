#include <stdlib.h>
#include <exception>
#include <sstream>

#include <boost/format.hpp>
#include "Rinternals.h"

#include "Bee/Client.h"

using namespace apache::thrift;
using namespace apache::hive::service::cli::thrift;

typedef struct {
  unsigned char bytes[4];
} Quad;

static Quad big_endian(int i) {
  return Quad {
    .bytes = {
      static_cast<unsigned char>((i & 0xff000000) >> 24),
      static_cast<unsigned char>((i & 0x00ff0000) >> 16),
      static_cast<unsigned char>((i & 0x0000ff00) >>  8),
      static_cast<unsigned char>((i & 0x000000ff) >>  0)
    }
  };
}

Bee::Client::Client(const std::string &hostname, int port, const std::string &user, const std::string &pass) :
  user_(user),
  pass_(pass),
  socket_(new transport::TSocket(hostname, port)),
  transport_(new transport::TFramedTransport(socket_)),
  protocol_(new protocol::TBinaryProtocol(transport_)),
  client_(new apache::hive::service::cli::thrift::TCLIServiceClient(protocol_)),
  session_handle_(),
  operation_handle_() {
    this->transport_->open();
    this->perform_sasl_handshake();
    this->open_session();
}

void Bee::Client::execute(const std::string &sql) {
  TExecuteStatementReq request;
  TExecuteStatementResp response;
  request.sessionHandle = this->session_handle_;
  request.statement = sql;

  this->client_->ExecuteStatement(response, request);
  this->operation_handle_ = response.operationHandle;
}

Rcpp::List Bee::Client::fetch(int num_rows) {
  if (num_rows < 1) {
    throw std::runtime_error("num_rows must be > 1");
  }

  TFetchResultsReq result_request;
  TFetchResultsResp result_response;
  TGetResultSetMetadataReq metadata_request;
  TGetResultSetMetadataResp metadata_response;

  result_request.operationHandle = this->operation_handle_;
  result_request.orientation = TFetchOrientation::FETCH_NEXT;
  result_request.maxRows = num_rows;
  this->client_->FetchResults(result_response, result_request);

  // TFetchResultsResp.hasMoreRows is currently broken, so just keep going until
  // we fall short.
  this->has_more_rows_ = result_response.results.rows.size() >= num_rows;

  metadata_request.operationHandle = this->operation_handle_;
  this->client_->GetResultSetMetadata(metadata_response, metadata_request);

  return build_data_frame(metadata_response.schema, result_response.results);
}

bool Bee::Client::has_more_rows() const {
  return this->has_more_rows_;
}

std::string Bee::Client::inspect() {
  std::ostringstream out;
  out << "Bee::Client[" <<
    "hostname=" << this->socket_->getHost() <<
    ", port=" << this->socket_->getPort() <<
    ", user=" << this->user_ <<
    ", pass=" << this->pass_ <<
    (this->has_session_handle() ? ", in session" : "") <<
    (this->has_operation_handle() ? ", in operation" : "") <<
    "]";
  return out.str();
}

void Bee::Client::write_frame(const std::string &bytes) {
  Quad length = big_endian(bytes.size());
  this->socket_->write(length.bytes, 4);
  this->socket_->write(reinterpret_cast<const unsigned char *>(bytes.c_str()), bytes.size());
}

boost::shared_ptr<std::string> Bee::Client::read_frame() {
  unsigned int length = 0;
  unsigned char length_buffer[5];
  this->socket_->read(length_buffer, 4);
  length_buffer[4] = '\0';
  for (int i = 0; i < 4; ++i) {
    length += static_cast<int>(length_buffer[1 + i]) << i;
  }

  if (length == 0) {
    return boost::shared_ptr<std::string>(new std::string(""));
  } else {
    char message[length];
    this->socket_->read(reinterpret_cast<unsigned char *>(message), length);
    return boost::shared_ptr<std::string>(new std::string(message, length));
  }
}

void Bee::Client::perform_sasl_handshake() {
  // START PLAIN
  {
    unsigned char code = 0x01;
    this->socket_->write(&code, 1);
    this->write_frame("PLAIN");
  }

  // OK PLAIN user pass
  {
    unsigned char code = 0x02;
    this->socket_->write(&code, 1);
    std::ostringstream frame("[PLAIN]\x00");
    frame << user_ << '\0' << pass_;
    this->write_frame(frame.str());
  }

  this->socket_->flush();

  unsigned char status;
  this->socket_->read(&status, 1);

  switch (status) {
  case 3: // bad
  case 4: // error
      throw std::runtime_error(*this->read_frame());

  case 5: // complete
      this->read_frame();  // challenge -- ignored
      break;

  case 2: // ok
    {
      boost::shared_ptr<std::string> message(this->read_frame());
      throw std::runtime_error(std::string("ERROR: auth failed: ") + *message);
      break;
    }

  default:
    throw std::runtime_error("unexpected response from sasl handshake");
  }
}

void Bee::Client::open_session() {
  TOpenSessionReq request;
  TOpenSessionResp response;
  request.client_protocol = TProtocolVersion::HIVE_CLI_SERVICE_PROTOCOL_V3;

  client_->OpenSession(response, request);
  this->session_handle_ = response.sessionHandle;
}

bool Bee::Client::has_session_handle() const {
  return !this->session_handle_.sessionId.guid.empty();
}

bool Bee::Client::has_operation_handle() const {
  return !this->operation_handle_.operationId.guid.empty();
}

Rcpp::List Bee::Client::build_data_frame(const TTableSchema schema, const TRowSet &row_set) const {
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

Rcpp::LogicalVector Bee::Client::build_logical_column_from_bool_value(const TRowSet &row_set, unsigned int column_index) const {
  unsigned int num_rows = row_set.rows.size();
  Rcpp::LogicalVector result(num_rows, NA_LOGICAL);
  for (unsigned int i = 0; i < num_rows; ++i) {
    result[i] = row_set.rows[i].colVals[column_index].boolVal.value;
  }
  return result;
}

Rcpp::IntegerVector Bee::Client::build_int_column_from_byte_value(const TRowSet &row_set, unsigned int column_index) const {
  unsigned int num_rows = row_set.rows.size();
  Rcpp::IntegerVector result(num_rows, NA_INTEGER);
  for (unsigned int i = 0; i < num_rows; ++i) {
    result[i] = row_set.rows[i].colVals[column_index].byteVal.value;
  }
  return result;
}

Rcpp::IntegerVector Bee::Client::build_int_column_from_i16_value(const TRowSet &row_set, unsigned int column_index) const {
  unsigned int num_rows = row_set.rows.size();
  Rcpp::IntegerVector result(num_rows, NA_INTEGER);
  for (unsigned int i = 0; i < num_rows; ++i) {
    result[i] = row_set.rows[i].colVals[column_index].i16Val.value;
  }
  return result;
}

Rcpp::IntegerVector Bee::Client::build_int_column_from_i32_value(const TRowSet &row_set, unsigned int column_index) const {
  unsigned int num_rows = row_set.rows.size();
  Rcpp::IntegerVector result(num_rows, NA_INTEGER);
  for (unsigned int i = 0; i < num_rows; ++i) {
    result[i] = row_set.rows[i].colVals[column_index].i32Val.value;
  }
  return result;
}

Rcpp::IntegerVector Bee::Client::build_int_column_from_i64_value(const TRowSet &row_set, unsigned int column_index) const {
  unsigned int num_rows = row_set.rows.size();
  Rcpp::IntegerVector result(num_rows, NA_INTEGER);
  for (unsigned int i = 0; i < num_rows; ++i) {
    result[i] = row_set.rows[i].colVals[column_index].i64Val.value;
  }
  return result;
}

Rcpp::StringVector Bee::Client::build_string_column_from_string_value(const TRowSet &row_set, unsigned int column_index) const {
  unsigned int num_rows = row_set.rows.size();
  Rcpp::StringVector result(num_rows, NA_STRING);
  for (unsigned int i = 0; i < num_rows; ++i) {
    result[i] = row_set.rows[i].colVals[column_index].stringVal.value;
  }
  return result;
}

Rcpp::DoubleVector Bee::Client::build_double_column_from_double_value(const TRowSet &row_set, unsigned int column_index) const {
  unsigned int num_rows = row_set.rows.size();
  Rcpp::DoubleVector result(num_rows, NA_REAL);
  for (unsigned int i = 0; i < num_rows; ++i) {
    result[i] = row_set.rows[i].colVals[column_index].doubleVal.value;
  }
  return result;
}

Rcpp::DoubleVector Bee::Client::build_double_column_from_string_value(const TRowSet &row_set, unsigned int column_index) const {
  unsigned int num_rows = row_set.rows.size();
  Rcpp::DoubleVector result(num_rows, NA_REAL);
  for (unsigned int i = 0; i < num_rows; ++i) {
    result[i] = atof(row_set.rows[i].colVals[column_index].stringVal.value.c_str());
  }
  return result;
}

Rcpp::IntegerVector Bee::Client::build_null_column_from_null_value(const TRowSet &row_set) const {
  return Rcpp::IntegerVector(row_set.rows.size(), NA_INTEGER);
}

Rcpp::StringVector Bee::Client::build_error_column(const TRowSet &row_set, const char *error) const {
  unsigned int num_rows = row_set.rows.size();
  Rcpp::StringVector result(num_rows, NA_STRING);
  for (unsigned int i = 0; i < num_rows; ++i) {
    result[i] = error;
  }
  return result;
}
