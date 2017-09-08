##
## Code & Maintenance by Yiend <yiendxie@gmail.com>
##

PREFIX?=./_install

CPPFLAGS = -Wall -Werror -std=c++11 -Iinclude -Isrc -DCTP_FIX_CLOSE_TIME
LDFLAGS = -lpthread -lboost_date_time -lsqlite3 -lglog lib/*.so

all: kong

kong: src/main.o src/conf.o src/datacore.o src/ctp/market_if.o
	$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CPPFLAGS) -c -o $@ $<

clean:
	rm -f src/*.o
	rm -f kong

install: all
	mkdir -p $(PREFIX)
	mkdir -p $(PREFIX)/lib
	mkdir -p $(PREFIX)/log
	install -m 755 kong $(PREFIX)
	install -m 755 lib/* $(PREFIX)/lib
	install -m 644 kong.sql kong.xml $(PREFIX)

.PHONY: all clean install
