#include <Rcpp.h>
#include "Bee/Client.h"

// [[Rcpp::interfaces(cpp, r)]]

// [[Rcpp::export]]
Rcpp::XPtr<Bee::Client> init(std::string hostname, int port, std::string user, std::string pass) {
  Bee::Client *client = new Bee::Client(hostname, port, user, pass);
  return Rcpp::XPtr<Bee::Client>(client);
}

// [[Rcpp::export]]
Rcpp::XPtr<Bee::Client> execute(SEXP client_sexp, std::string sql) {
  Rcpp::XPtr<Bee::Client> client_xptr(client_sexp);
  Bee::Client *client = client_xptr.get();
  client->execute(sql);
  return client_xptr;
}

// [[Rcpp::export]]
Rcpp::List fetch(SEXP client_sexp, int num_rows) {
  Rcpp::XPtr<Bee::Client> client_xptr(client_sexp);
  Bee::Client *client = client_xptr.get();
  return client->fetch(num_rows);
}

// [[Rcpp::export]]
bool has_more_rows(SEXP client_sexp) {
  Rcpp::XPtr<Bee::Client> client_xptr(client_sexp);
  Bee::Client *client = client_xptr.get();
  return client->has_more_rows();
}
