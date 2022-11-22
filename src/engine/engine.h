#ifndef ENGINE_H
#define ENGINE_H

#include <cstdbool>

#include "./graphics/camera.h"
#include "./graphics/renderer.h"
#include "./map/map.h"
#include "./map/mapobject.h"

namespace Engine {
	class Engine {
	public:
		Engine(const char *mapPath, int windowWidth, int windowHeight, int defaultZoom, int maxZoom, int fps, bool debug);
		~Engine();

		void start(void); // does not return until after stop is called
		void stop(void);

		class Map *getMap(void);

		MapObject *getPlayerObject(void);
		void setPlayerObject(MapObject *object);

	private:
		int maxZoom;
		int fps;
		bool debug;
		bool stopFlag;

		class Map *map;
		Graphics::Renderer renderer;
		Graphics::Camera camera;
		MapObject *playerObject;
	};
};

#endif
