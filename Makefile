##
## Code & Maintenance by Yiend <yiendxie@gmail.com>
##

PREFIX?=/usr/local
INCLUDE_PATH?=include
LIBRARY_PATH?=lib
BINARY_PATH?=bin

CPPFLAGS = -Wall -Werror -O2 -std=c++11 -I$(INCLUDE_PATH) -g
LDFLAGS = -lpthread -lboost_date_time -lsqlite3 -lglog $(LIBRARY_PATH)/*.so

# target
TARGET=ctp
all: $(TARGET)

ctp: src/main.o src/rc.o src/analyzer.o src/market_if.o
	g++ $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f src/*.o
	rm -f $(TARGET)

install: $(TARGET)
	mkdir -p $(PREFIX)
	mkdir -p $(PREFIX)/lib
	mkdir -p $(PREFIX)/log
	install -m 755 $(TARGET)       $(PREFIX)
	install -m 755 lib/*           $(PREFIX)/lib
	install -m 644 ctp.sql ctp.xml $(PREFIX)

.PHONY: all clean install

analyzer.o: src/analyzer.cpp src/analyzer.h src/yx_types.h src/yx_bar.hpp \
 src/rc.h
main.o: src/main.cpp src/rc.h src/analyzer.h src/yx_types.h \
 src/yx_bar.hpp src/market_if.h include/ThostFtdcMdApi.h \
 include/ThostFtdcUserApiStruct.h include/ThostFtdcUserApiDataType.h
market_if.o: src/market_if.cpp src/market_if.h src/yx_types.h \
 include/ThostFtdcMdApi.h include/ThostFtdcUserApiStruct.h \
 include/ThostFtdcUserApiDataType.h
rc.o: src/rc.cpp src/rc.h
