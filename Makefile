install:
	R CMD INSTALL .

package:
	R CMD build .

clean:
	rm -rf bee_*.tar.gz **/*.o inst/include/tcli_thrift src/tcli_thrift

.PHONY: install package clean
