#ifndef ENGINE_GRAPHICS_MAPTILE_H
#define ENGINE_GRAPHICS_MAPTILE_H

namespace Engine {
	struct MapTileLayer {
		unsigned textureId;
	};

	class MapTile {
	public:
		static const unsigned layersMax=16;

		MapTile();
		MapTile(unsigned temp);
		~MapTile();

		const MapTileLayer *getLayer(unsigned z) const;
	private:
		MapTileLayer layers[layersMax];
	};
};

#endif
