
PREFIX = /usr/local
INCLUDEDIR = ./include
LIBDIR = ./lib

CPPFLAGS = -Wall -O2 -g -std=gnu++11 -I$(INCLUDEDIR)
LDFLAGS = -lboost_date_time -lsqlite3 -lglog $(LIBDIR)/*.so

# targets
CTP = ctp
TARGETS = $(CTP)

all: $(TARGETS)

$(CTP): src/main.o src/rc.o src/*.hpp
	g++ $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGETS)
	rm -f src/*.o

.PHONY: clean
