##
## Code & Maintenance by Yiend <yiendxie@gmail.com>
##

PREFIX?=./_install

CPPFLAGS = -Wall -Werror -std=c++11 -Iinclude -Isrc
LDFLAGS = -lpthread -lboost_date_time -lsqlite3 -lglog lib/*.so

all: ctp

ctp: src/main.o src/conf.o src/analyzer.o src/ctp/market_if.o
	g++ $(LDFLAGS) -o $@ $^

%.o: %.cpp
	g++ $(CPPFLAGS) -c -o $@ $<

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
