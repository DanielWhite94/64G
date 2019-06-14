CPP = clang++
CFLAGS = -Wall -std=c++11
LFLAGS = -lpng

OBJS = ../engine/map/map.o ../engine/map/mapgen.o ../engine/map/mapobject.o ../engine/map/mappnglib.o ../engine/map/mapregion.o ../engine/map/maptexture.o ../engine/map/maptiled.o ../engine/map/maptile.o ../engine/physics/hitmask.o ../engine/physics/coord.o ../engine/fbnnoise.o ../engine/noisearray.o ../engine/opensimplexnoise.o ../engine/util.o demogen.o

debug: CFLAGS += -O0 -g -ggdb3
release: CFLAGS += -O3 -DNDEBUG

debug: $(OBJS)
	$(CPP) $(CFLAGS) $(OBJS) -o ../../bin/demogen $(LFLAGS)

release: $(OBJS)
	$(CPP) $(CFLAGS) $(OBJS) -o ../../bin/demogen $(LFLAGS)

%.o: %.cpp %.h
	$(CPP) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CPP) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS)
