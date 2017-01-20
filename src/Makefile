CPP = clang++
CFLAGS = -Wall -std=c++11 -O3 -ggdb3
LFLAGS = -lSDL2 -lSDL2_image

OBJS = ./engine/graphics/camera.o ./engine/graphics/renderer.o ./engine/map/map.o ./engine/map/mapregion.o ./engine/map/maptile.o ./engine/physics/hitmask.o ./engine/physics/coord.o ./engine/util.o main.o

debug: $(OBJS)
	$(CPP) $(CFLAGS) $(OBJS) -o ./main $(LFLAGS)

release: $(OBJS)
	$(CPP) $(CFLAGS) $(OBJS) -o ./main $(LFLAGS)

%.o: %.cpp %.h
	$(CPP) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CPP) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS)
