CPP = clang++
CFLAGS = -Wall -std=c++20 -Wno-c99-designator
FLAGS = -lSDL2 -lSDL2_image -lSDL2_ttf -lm -lpng -lpthread

OBJS = ../common/gen/edgedetect.o ../common/gen/floodfill.o ../common/gen/forest.o ../common/gen/house.o ../common/gen/modifytiles.o ../common/gen/particleflow.o ../common/gen/pathfind.o ../common/gen/search.o ../common/gen/stats.o ../common/gen/town.o ./graphics/camera.o ./graphics/renderer.o ./graphics/texture.o ../common/emap/map.o ../common/emap/mapobject.o ../common/emap/mappnglib.o ../common/emap/mapregion.o ../common/emap/mapitem.o ../common/emap/mapkingdom.o ../common/emap/maplandmass.o ../common/emap/maptexture.o ../common/emap/maptiled.o ../common/net/rudpsocket.o ../common/emap/maptile.o ../common/physics/hitmask.o ./client.o ../common/physics/coord.o ../common/fbnnoise.o ../common/perlinnoise.o ../common/prng.o ../common/util.o main.o

debug: CFLAGS += -O0 -g -ggdb3
release: CFLAGS += -O3 -DNDEBUG

debug: $(GENOBJS) $(OBJS)
	$(CPP) $(CFLAGS) $(OBJS) -o ../../bin/client $(FLAGS)

release: $(GENOBJS) $(OBJS)
	$(CPP) $(CFLAGS) $(OBJS) -o ../../bin/client $(FLAGS)

%.o: %.cpp %.h
	$(CPP) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CPP) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(GENOBJS)
	rm -f $(OBJS)
