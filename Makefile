install:
	R CMD INSTALL .

package:
	R CMD build .

clean:
	rm -rf bee_*.tar.gz */*.o */*/*.o src/bee.so inst/include/tcli_thrift src/tcli_thrift $(RCPP_GEN_SRC)

.PHONY: install package clean
