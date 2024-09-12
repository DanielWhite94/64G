CPP = clang++
CFLAGS = -Wall -std=c++20 -Wno-c99-designator `pkg-config --cflags gtk+-3.0`
LFLAGS = `pkg-config --libs gtk+-3.0` -lm -lpng -lpthread

OBJS = ../common/gen/edgedetect.o ../common/gen/floodfill.o ../common/gen/forest.o ../common/gen/house.o ../common/gen/kingdom.o ../common/gen/marchingsquares.o ../common/gen/modifytiles.o ../common/gen/particleflow.o ../common/gen/pathfind.o ../common/gen/search.o ../common/gen/stats.o ../common/gen/town.o ../common/emap/map.o ../common/emap/mapobject.o ../common/emap/mappnglib.o ../common/emap/mapregion.o ../common/emap/mapitem.o ../common/emap/mapkingdom.o ../common/emap/maplandmass.o ../common/emap/maptexture.o ../common/emap/maptiled.o ../common/emap/maptile.o ../common/physics/hitmask.o ../common/physics/coord.o ../common/fbnnoise.o ../common/perlinnoise.o ../common/prng.o ../common/util.o cleardialogue.o heighttemperaturedialogue.o kingdomterritorydialogue.o main.o mainwindow.o newdialogue.o progressdialogue.o util.o

debug: CFLAGS += -O0 -g -ggdb3
release: CFLAGS += -O3 -DNDEBUG

debug: $(OBJS)
	$(CPP) $(CFLAGS) $(OBJS) -o ../../bin/editor $(LFLAGS)
	cp -f cleardialogue.glade ../../bin/cleardialogue.glade
	cp -f heighttemperaturedialogue.glade ../../bin/heighttemperaturedialogue.glade
	cp -f kingdomterritorydialogue.glade ../../bin/kingdomterritorydialogue.glade
	cp -f mainwindow.glade ../../bin/mainwindow.glade
	cp -f newdialogue.glade ../../bin/newdialogue.glade
	cp -f style.css ../../bin/style.css

release: $(OBJS)
	$(CPP) $(CFLAGS) $(OBJS) -o ../../bin/editor $(LFLAGS)
	cp -f cleardialogue.glade ../../bin/cleardialogue.glade
	cp -f heighttemperaturedialogue.glade ../../bin/heighttemperaturedialogue.glade
	cp -f kingdomterritorydialogue.glade ../../bin/kingdomterritorydialogue.glade
	cp -f mainwindow.glade ../../bin/mainwindow.glade
	cp -f newdialogue.glade ../../bin/newdialogue.glade
	cp -f style.css ../../bin/style.css

%.o: %.cpp %.h
	$(CPP) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CPP) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS)
