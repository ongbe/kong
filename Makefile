##
## Code & Maintenance by Yiend <yiendxie@gmail.com>
##

PREFIX?=/usr/local
INCLUDE_PATH?=include
LIBRARY_PATH?=lib
BINARY_PATH?=bin

CPPFLAGS = -Wall -Werror -std=c++11 -I$(INCLUDE_PATH) -Isrc
LDFLAGS = -lpthread -lboost_date_time -lsqlite3 -lglog $(LIBRARY_PATH)/*.so

all: ctp

clean:
	rm -f src/*.o
	rm -f ctp

install: all
	mkdir -p $(PREFIX)
	mkdir -p $(PREFIX)/lib
	mkdir -p $(PREFIX)/log
	install -m 755 ctp $(PREFIX)
	install -m 755 lib/* $(PREFIX)/lib
	install -m 644 ctp.sql ctp.xml $(PREFIX)

.PHONY: all clean install

ctp: src/main.o src/conf.o src/analyzer.o src/market_if.o
	g++ $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

src/analyzer.o: src/analyzer.cpp src/analyzer.h src/yx/types.h \
 src/yx/xbar.hpp src/yx/types.h src/yx/bar_base.hpp src/conf.h
src/conf.o: src/conf.cpp src/conf.h
src/main.o: src/main.cpp src/conf.h src/analyzer.h src/yx/types.h \
 src/yx/xbar.hpp src/yx/types.h src/yx/bar_base.hpp src/market_if.h
src/market_if.o: src/market_if.cpp src/market_if.h src/yx/types.h
