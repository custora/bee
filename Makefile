R = R
THRIFT = thrift
RCPP_GEN_SRC = R/RcppExports.R inst/include/bee_RcppExports.h src/RcppExports.cpp
THRIFT_GEN_HEADERS =                   \
  inst/include/tcli_thrift/TCLIService.h           \
	inst/include/tcli_thrift/TCLIService_constants.h \
	inst/include/tcli_thrift/TCLIService_types.h
THRIFT_GEN_SRC =                \
  src/tcli_thrift/TCLIService.cpp           \
	src/tcli_thrift/TCLIService_constants.cpp \
	src/tcli_thrift/TCLIService_types.cpp

package: $(RCPP_GEN_SRC) $(THRIFT_GEN_HEADERS) $(THRIFT_GEN_SRC)
	$(R) CMD build .

install: package
	$(R) CMD INSTALL bee_1.0.tar.gz

clean:
	rm -rf $(RCPP_GEN_SRC) inst/include/tcli_thrift src/tcli_thrift

$(THRIFT_GEN_HEADERS) $(THRIFT_GEN_SRC): TCLIService.thrift
	rm -rf thrift
	mkdir thrift
	$(THRIFT) -I $(HIVE_SRC) -r --gen cpp -o thrift $<

	rm -rf inst/include/tcli_thrift
	mkdir -p inst/include/tcli_thrift
	cp thrift/gen-cpp/*.h inst/include/tcli_thrift/

	rm -rf src/tcli_thrift
	mkdir -p src/tcli_thrift
	cp thrift/gen-cpp/*.cpp src/tcli_thrift/

	rm -rf thrift

$(RCPP_GEN_SRC): src/extension.cpp
	$(eval tmp := $(shell mktemp -d))
	echo 'library(Rcpp)' > $(tmp)/in.R
	echo 'compileAttributes()' >> $(tmp)/in.R
	$(R) CMD BATCH $(tmp)/in.R $(tmp)/out
	rm -rf $(tmp)/in.R $(tmp)/out

.PHONY: package install clean
