##
## Code & Maintenance by yon2kong <yon2kong@gmail.com>
##

CPPFLAGS = -g -Wall -Werror -std=gnu++11 -I. -I../include -DCTP_FIX_CLOSE_TIME
LDFLAGS = -L../lib -lboost_date_time -lsqlite3 -lglog -ly

all: kong plugins/ctp_market.so

kong: main.o datacore.so
	$(CXX) $(LDFLAGS) -o $@ $^

datacore.so: datacore.o conf.o
	$(CXX) $(LDFLAGS) -shared -o $@ $^

plugins/ctp_market.so: plugins/ctp_market.o plugins/ctp_market_if.o
	$(CXX) $(LDFLAGS) -shared -lpthread -o $@ $^ ../lib/thostmduserapi.so

%.o: %.cpp
	$(CXX) $(CPPFLAGS) -fPIC -c -o $@ $<

clean:
	find . -regex '.*\.\(so\|o\)' -type f -exec rm -f {} \;
	rm -f kong
