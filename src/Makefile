R = R
RCPP_GEN_SRC = R/RcppExports.R inst/include/bee_RcppExports.h src/RcppExports.cpp

all:

clean:
	rm -rf $(RCPP_GEN_SRC) tcli_thrift

$(THRIFT_GEN_HEADERS) $(THRIFT_GEN_SRC): TCLIService.thrift
	./configure

.PHONY: package install clean
