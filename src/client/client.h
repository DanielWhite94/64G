#ifndef ENGINE_CLIENT_H
#define ENGINE_CLIENT_H

#include <cstdbool>

#include "./graphics/camera.h"
#include "./graphics/renderer.h"

#include "../common/common.h"

using namespace Common;
using namespace Common::Net;

class Client {
public:
	Client(const char *mapPath, int windowWidth, int windowHeight, int defaultZoom, int maxZoom, int fps, bool debug);
	~Client();

	void start(void); // does not return until after stop is called
	void stop(void);

	class Map *getMap(void);

	MapObject *getPlayerObject(void);
	void setPlayerObject(MapObject *object);

	// These are to be considered private
	void receiptFunctor(RudpSocket::ConnectionId conId, RudpPacket::SeqNum seqNum, bool success);
	void recvFunctor(RudpSocket::ConnectionId conId, RudpPacket::SeqNum seqNum, const void *data, size_t len);

private:
	// Flags/parameters
	int maxZoom;
	int fps;
	bool debug;
	bool stopFlag;

	// Game fields
	class Map *map;
	Graphics::Renderer renderer;
	Graphics::Camera camera;
	MapObject *playerObject;

	// Network fields
	char username[GameMessage::maxUsernameSize+1];
	RudpSocket *sock;
	bool sessionActive;
	int sessionRequestSeqNum; // either a (non-negative) RudpPacket::SeqNum or -1
	uint64_t sessionToken;
};

#endif
