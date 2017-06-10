CPP = clang++
CFLAGS = -Wall -std=c++11
LFLAGS = -lpng

OBJS = ../engine/map/map.o ../engine/map/mapgen.o ../engine/map/mapobject.o ../engine/map/mapregion.o ../engine/map/maptexture.o ../engine/map/maptile.o ../engine/physics/hitmask.o ../engine/physics/coord.o ../engine/fbnnoise.o ../engine/opensimplexnoise.o ../engine/util.o mappng.o

debug: CFLAGS += -O0 -g -ggdb3
release: CFLAGS += -O3 -DNDEBUG

debug: $(OBJS)
	$(CPP) $(CFLAGS) $(OBJS) -o ../../bin/mappng $(LFLAGS)

release: $(OBJS)
	$(CPP) $(CFLAGS) $(OBJS) -o ../../bin/mappng $(LFLAGS)

%.o: %.cpp %.h
	$(CPP) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CPP) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS)