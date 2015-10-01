R = R
HIVE_SRC = ~/src/hive
RCPP_GEN_SRC = R/RcppExports.R inst/include/bee_RcppExports.h src/RcppExports.cpp

package: $(RCPP_GEN_SRC)
	R CMD build .

install: package
	R CMD INSTALL bee_1.0.tar.gz

clean:
	rm -f $(RCPP_GEN_SRC)

thrift: inst/include/tcli_thrift src/tcli_thrift

inst/include/tcli_thrift:
	mkdir -p $@
	thrift -I $(HIVE_SRC) -r --gen cpp -o $@ $(HIVE_SRC)/service/if/TCLIService.thrift
	mv $@/gen-cpp/*.h $@/
	rm -rf $@/gen-cpp

src/tcli_thrift:
	mkdir -p $@
	thrift -I $(HIVE_SRC) -r --gen cpp -o $@ $(HIVE_SRC)/service/if/TCLIService.thrift
	mv $@/gen-cpp/*.cpp $@/
	rm -rf $@/gen-cpp

$(RCPP_GEN_SRC): src/extension.cpp
	$(eval tmp := $(shell mktemp -d))
	echo 'library(Rcpp)' > $(tmp)/in.R
	echo 'compileAttributes()' >> $(tmp)/in.R
	$(R) CMD BATCH $(tmp)/in.R $(tmp)/out
	rm -rf $(tmp)/in.R $(tmp)/out

.PHONY: package install thrift clean
