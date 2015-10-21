R = R
THRIFT_GEN_HEADERS =                               \
  inst/include/tcli_thrift/TCLIService.h           \
	inst/include/tcli_thrift/TCLIService_constants.h \
	inst/include/tcli_thrift/TCLIService_types.h
THRIFT_GEN_SRC =                            \
  src/tcli_thrift/TCLIService.cpp           \
	src/tcli_thrift/TCLIService_constants.cpp \
	src/tcli_thrift/TCLIService_types.cpp

package: $(THRIFT_GEN_HEADERS) $(THRIFT_GEN_SRC)
	$(R) CMD build .

install: package
	$(R) CMD INSTALL .

clean:
	$(MAKE) -C src clean
	rm -rf inst/include/tcli_thrift

$(THRIFT_GEN_HEADERS) $(THRIFT_GEN_SRC): TCLIService.thrift
	./configure

.PHONY: package install clean
