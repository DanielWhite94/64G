CPP = clang++
CFLAGS = -Wall -std=c++20 -Wno-c99-designator
LFLAGS = -lSDL2 -lSDL2_image -lSDL2_ttf -lm -lpng -lpthread

OBJS = ./graphics/camera.o ./graphics/renderer.o ./graphics/texture.o ./map/map.o ./map/mapgen.o ./map/mapobject.o ../engine/map/mappnglib.o ./map/mapregion.o ./map/maptexture.o ./map/maptiled.o ./map/maptile.o ./physics/hitmask.o ./physics/coord.o ./fbnnoise.o ./perlinnoise.o ./prng.o ./util.o engine.o

debug: CFLAGS += -O0 -g -ggdb3
release: CFLAGS += -O3 -DNDEBUG

debug: $(OBJS)
	$(CPP) $(CFLAGS) $(OBJS) -o ../../bin/engine $(LFLAGS)

release: $(OBJS)
	$(CPP) $(CFLAGS) $(OBJS) -o ../../bin/engine $(LFLAGS)

%.o: %.cpp %.h
	$(CPP) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CPP) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS)
