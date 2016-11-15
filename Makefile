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

ctp: src/main.o src/conf.o src/analyzer.o src/market_if.o
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

src/analyzer.o: src/analyzer.cpp src/analyzer.h src/yx_types.h src/yx_bar.hpp \
 src/conf.h
src/conf.o: src/conf.cpp src/conf.h
src/main.o: src/main.cpp src/conf.h src/analyzer.h src/yx_types.h \
 src/yx_bar.hpp src/market_if.h
src/market_if.o: src/market_if.cpp src/market_if.h src/yx_types.h
