SOURCES=$(wildcard *.cpp)
OBJECTS=$(SOURCES:.cpp=.o)
S_OBJS := $(wildcard ../src/*.o)
STOCKFISH_OBJECTS=$(filter-out ../src/main.o,$(S_OBJS))
DEPS=$(SOURCES:.cpp=.d)
BINS=tests

all: $(BINS)
	./tests

tests: $(OBJECTS) $(STOCKFISH_OBJECTS) 
	g++ -DUSE_BSFQ -o tests $(OBJECTS) $(STOCKFISH_OBJECTS) -lpthread

clean:
	$(RM) $(OBJECTS) $(DEPS) $(BINS)
