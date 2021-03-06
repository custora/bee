#include <Rcpp.h>
#include "Bee/Client.h"

// [[Rcpp::interfaces(cpp, r)]]

// [[Rcpp::export]]
Rcpp::XPtr<Bee::Client> bee_connect(
  std::string host="127.0.0.1",
  int port=10000,
  std::string user="anonymous",
  std::string pass="anonymous"
) {
  Rcpp::XPtr<Bee::Client> client(new Bee::Client(host, port, user, pass));
  client.attr("class") = "bee.client";
  return client;
}

// [[Rcpp::export]]
Rcpp::XPtr<Bee::Client> bee_disconnect(SEXP client_sexp) {
  Rcpp::XPtr<Bee::Client> client_xptr(client_sexp);
  Bee::Client *client = client_xptr.get();
  client->disconnect();
  return client_xptr;
}

// [[Rcpp::export]]
Rcpp::XPtr<Bee::Client> bee_execute(SEXP client_sexp, std::string sql) {
  Rcpp::XPtr<Bee::Client> client_xptr(client_sexp);
  Bee::Client *client = client_xptr.get();
  client->execute(sql);
  return client_xptr;
}

// [[Rcpp::export]]
Rcpp::List bee_fetch(SEXP client_sexp, int num_rows) {
  Rcpp::XPtr<Bee::Client> client_xptr(client_sexp);
  Bee::Client *client = client_xptr.get();
  return client->fetch(num_rows);
}

// [[Rcpp::export]]
bool bee_has_more_rows(SEXP client_sexp) {
  Rcpp::XPtr<Bee::Client> client_xptr(client_sexp);
  Bee::Client *client = client_xptr.get();
  return client->has_more_rows();
}
