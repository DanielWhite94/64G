CPP = clang++
CFLAGS = -Wall -std=c++20 -Wno-c99-designator
LFLAGS = -lpng -lpthread

OBJS = ../engine/gen/edgedetect.o ../engine/gen/floodfill.o ../engine/gen/forest.o ../engine/gen/house.o ../engine/gen/modifytiles.o ../engine/gen/particleflow.o ../engine/gen/pathfind.o ../engine/gen/search.o ../engine/gen/stats.o ../engine/gen/town.o ../engine/map/map.o ../engine/map/mapobject.o ../engine/map/mappnglib.o ../engine/map/mapregion.o ../engine/map/mapitem.o ../engine/map/maptexture.o ../engine/map/maptiled.o ../engine/map/maptile.o ../engine/physics/hitmask.o ../engine/physics/coord.o ../engine/fbnnoise.o ../engine/perlinnoise.o ../engine/prng.o ../engine/util.o mappng.o

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
