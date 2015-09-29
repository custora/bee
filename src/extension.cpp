#include <Rcpp.h>
#include "HiveClient/Client.h"

// [[Rcpp::interfaces(cpp, r)]]

// [[Rcpp::export]]
Rcpp::XPtr<HiveClient::Client> init(std::string hostname, int port, std::string user, std::string pass) {
  HiveClient::Client *client = new HiveClient::Client(hostname, port, user, pass);
  return Rcpp::XPtr<HiveClient::Client>(client);
}

// [[Rcpp::export]]
Rcpp::XPtr<HiveClient::Client> execute(SEXP client_sexp, std::string sql) {
  Rcpp::XPtr<HiveClient::Client> client_xptr(client_sexp);
  HiveClient::Client *client = client_xptr.get();
  client->execute(sql);
  return client_xptr;
}

// [[Rcpp::export]]
Rcpp::List fetch(SEXP client_sexp, int num_rows) {
  Rcpp::XPtr<HiveClient::Client> client_xptr(client_sexp);
  HiveClient::Client *client = client_xptr.get();
  return client->fetch(num_rows);
}

// [[Rcpp::export]]
bool has_more_rows(SEXP client_sexp) {
  Rcpp::XPtr<HiveClient::Client> client_xptr(client_sexp);
  HiveClient::Client *client = client_xptr.get();
  return client->has_more_rows();
}
