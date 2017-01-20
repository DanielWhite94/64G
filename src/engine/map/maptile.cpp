#include <assert.h>

#include "maptile.h"

using namespace Engine;

MapTile::MapTile() {
	unsigned i;
	for(i=0; i<layersMax; ++i)
		layers[i].textureId=0;
}

MapTile::MapTile(int temp) {
	layers[0].texture=temp;
	unsigned i;
	for(i=1; i<layersMax; ++i)
		layers[i].texture=0;
}

MapTile::~MapTile() {

}

const MapTileLayer *MapTile::getLayer(unsigned z) const {
	assert(z<layersMax);
	return &layers[z];
}
