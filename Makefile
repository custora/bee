RCPP_GEN_SRC = R/RcppExports.R inst/include/bee_RcppExports.h src/RcppExports.cpp

install: $(RCPP_GEN_SRC)
	R CMD INSTALL .

package:
	R CMD build .

clean:
	rm -rf bee_*.tar.gz **/*.o inst/include/tcli_thrift src/tcli_thrift $(RCPP_GEN_SRC)

.PHONY: install package clean

$(RCPP_GEN_SRC): src/extension.cpp
	$(eval tmp := $(shell mktemp -d))
	echo 'library(Rcpp)' > $(tmp)/in.R
	echo 'compileAttributes()' >> $(tmp)/in.R
	R CMD BATCH $(tmp)/in.R $(tmp)/out
	rm -rf $(tmp)/in.R $(tmp)/out
