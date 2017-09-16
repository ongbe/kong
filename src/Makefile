##
## Code & Maintenance by yon2kong <yon2kong@gmail.com>
##

CFLAGS = -Wall -O2 -std=gnu99 -I. -I../include
CPPFLAGS = -Wall -O2 -std=gnu++11 -I. -I../include -DCTP_FIX_CLOSE_TIME -DCTP_FIX_CALL_AUCTION
LDFLAGS = -L../lib -lpthread -lsqlite3 -lglog -ly

all: kong

kong: datacore/main.o dc.so
	$(CXX) $(LDFLAGS) -o $@ $^

dc.so: datacore/datacore.o conf.o
	$(CXX) $(LDFLAGS) -shared -o $@ $^

plugins: plugins/ctp_market.so

plugins/ctp_market.so: plugins/ctp_market.o plugins/ctp_market_if.o
	$(CXX) $(LDFLAGS) -shared -o $@ $^ ../lib/thostmduserapi.so

ana: analysis/main.o conf.o console.o utils.o
	$(CXX) $(LDFLAGS) -lreadline -o $@ $^

%.o: %.cpp
	$(CXX) $(CPPFLAGS) -fPIC -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

clean:
	find . -regex '.*\.\(so\|o\)' -type f -exec rm -f {} \;
	rm -f kong ana

.PHONY: all plugins clean
