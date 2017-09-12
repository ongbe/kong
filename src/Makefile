##
## Code & Maintenance by yon2kong <yon2kong@gmail.com>
##

CPPFLAGS = -Wall -Werror -std=gnu++11 -I. -I../include -DCTP_FIX_CLOSE_TIME -DCTP_FIX_CALL_AUCTION
LDFLAGS = -L../lib -lboost_date_time -lsqlite3 -lglog -ly

all: kong

kong: datacore/main.o dc.so
	$(CXX) $(LDFLAGS) -o $@ $^

dc.so: datacore/datacore.o datacore/conf.o
	$(CXX) $(LDFLAGS) -shared -o $@ $^

plugins: plugins/ctp_market.so

plugins/ctp_market.so: plugins/ctp_market.o plugins/ctp_market_if.o
	$(CXX) $(LDFLAGS) -shared -lpthread -o $@ $^ ../lib/thostmduserapi.so

ana: analysis/main.o analysis/conf.o
	$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CPPFLAGS) -fPIC -c -o $@ $<

clean:
	find . -regex '.*\.\(so\|o\)' -type f -exec rm -f {} \;
	rm -f kong ana

.PHONY: all plugins clean
