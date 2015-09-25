R = R
HIVE_SRC = ~/src/hive
RCPP_GEN_SRC = R/RcppExports.R inst/include/HiveClient_RcppExports.h src/RcppExports.cpp

package: $(RCPP_GEN_SRC)
	R CMD BUILD .

install: package
	R CMD INSTALL HiveClient_1.0.tar.gz

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
	echo 'library(Rcpp)' > tmp.$$.R
	echo 'compileAttributes()' >> tmp.$$.R
	$(R) CMD BATCH tmp.$$.R tmp.$$.Rout
	rm -f tmp.$$.R tmp.$$.Rout

.PHONY: package install thrift clean
