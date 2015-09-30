#ifndef __Bee_Client_h__
#define __Bee_Client_h__

#include <string>

#include <Rcpp.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include "tcli_thrift/TCLIService.h"

namespace Bee {
  using namespace apache::thrift;
  using namespace apache::hive::service::cli::thrift;

  class Client {
  public:
    Client(const std::string &hostname, int port, const std::string &user, const std::string &pass);
    void execute(const std::string &sql);
    Rcpp::List fetch(int num_rows);
    bool has_more_rows() const;
    std::string inspect();

  private:
    void write_frame(const std::string &bytes);
    boost::shared_ptr<std::string> read_frame();
    void perform_sasl_handshake();
    void open_session();
    Rcpp::List build_data_frame(const TTableSchema schema, const TRowSet &row_set) const;
    Rcpp::LogicalVector build_logical_column_from_bool_value(const TRowSet &row_set, unsigned int column_index) const;
    Rcpp::IntegerVector build_int_column_from_byte_value(const TRowSet &row_set, unsigned int column_index) const;
    Rcpp::IntegerVector build_int_column_from_i16_value(const TRowSet &row_set, unsigned int column_index) const;
    Rcpp::IntegerVector build_int_column_from_i32_value(const TRowSet &row_set, unsigned int column_index) const;
    Rcpp::IntegerVector build_int_column_from_i64_value(const TRowSet &row_set, unsigned int column_index) const;
    Rcpp::StringVector build_string_column_from_string_value(const TRowSet &row_set, unsigned int column_index) const;
    Rcpp::DoubleVector build_double_column_from_double_value(const TRowSet &row_set, unsigned int column_index) const;
    Rcpp::DoubleVector build_double_column_from_string_value(const TRowSet &row_set, unsigned int column_index) const;
    Rcpp::IntegerVector build_null_column_from_null_value(const TRowSet &row_set) const;
    Rcpp::StringVector build_error_column(const TRowSet &row_set, const char *error) const;
    bool has_session_handle() const;
    bool has_operation_handle() const;

    std::string user_;
    std::string pass_;
    bool has_more_rows_;
    boost::shared_ptr<transport::TSocket> socket_;
    boost::shared_ptr<transport::TFramedTransport> transport_;
    boost::shared_ptr<protocol::TProtocol> protocol_;
    boost::shared_ptr<TCLIServiceClient> client_;

    TSessionHandle session_handle_;
    TOperationHandle operation_handle_;
  };
}

#endif // __Bee_Client_h__
