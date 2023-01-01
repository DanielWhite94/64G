CPP = clang++
CFLAGS = -Wall -std=c++20 -Wno-c99-designator
GENLFLAGS = -lpng -lpthread
GAMELFLAGS = -lSDL2 -lSDL2_image -lSDL2_ttf -lm -lpng -lpthread

GENOBJS = ../engine/gen/edgedetect.o ../engine/gen/floodfill.o ../engine/gen/forest.o ../engine/gen/modifytiles.o ../engine/gen/particleflow.o ../engine/gen/search.o ../engine/map/map.o ../engine/map/mapgen.o ../engine/map/mapobject.o ../engine/map/mappnglib.o ../engine/map/mapregion.o ../engine/map/mapitem.o ../engine/map/maptexture.o ../engine/map/maptiled.o ../engine/map/maptile.o ../engine/physics/hitmask.o ../engine/physics/coord.o ../engine/fbnnoise.o ../engine/perlinnoise.o ../engine/prng.o ../engine/util.o gen.o
GAMEOBJS = ../engine/gen/edgedetect.o ../engine/gen/floodfill.o ../engine/gen/forest.o ../engine/gen/modifytiles.o ../engine/gen/particleflow.o ../engine/gen/search.o ../engine/graphics/camera.o ../engine/graphics/renderer.o ../engine/graphics/texture.o ../engine/map/map.o ../engine/map/mapgen.o ../engine/map/mapobject.o ../engine/map/mappnglib.o ../engine/map/mapregion.o ../engine/map/mapitem.o ../engine/map/maptexture.o ../engine/map/maptiled.o ../engine/map/maptile.o ../engine/physics/hitmask.o ../engine/physics/coord.o ../engine/fbnnoise.o ../engine/perlinnoise.o ../engine/prng.o ../engine/util.o ../engine/engine.o game.o

debug: CFLAGS += -O0 -g -ggdb3
release: CFLAGS += -O3 -DNDEBUG

debug: $(GENOBJS) $(GAMEOBJS)
	$(CPP) $(CFLAGS) $(GENOBJS) -o ../../bin/demogen $(GENLFLAGS)
	$(CPP) $(CFLAGS) $(GAMEOBJS) -o ../../bin/demogame $(GAMELFLAGS)

release: $(GENOBJS) $(GAMEOBJS)
	$(CPP) $(CFLAGS) $(GENOBJS) -o ../../bin/demogen $(GENLFLAGS)
	$(CPP) $(CFLAGS) $(GAMEOBJS) -o ../../bin/demogame $(GAMELFLAGS)

%.o: %.cpp %.h
	$(CPP) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CPP) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(GENOBJS)
	rm -f $(GAMEOBJS)
