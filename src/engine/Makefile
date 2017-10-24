CPP = clang++
CFLAGS = -Wall -std=c++11
LFLAGS = -lSDL2 -lSDL2_image -lm

OBJS = ./graphics/camera.o ./graphics/renderer.o ./graphics/texture.o ./map/map.o ./map/mapgen.o ./map/mapobject.o ./map/mapregion.o ./map/maptexture.o ./map/maptile.o ./physics/hitmask.o ./physics/coord.o ./fbnnoise.o ./noisearray.o ./opensimplexnoise.o ./util.o engine.o

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
