#ifndef ENGINE_SERVER_H
#define ENGINE_SERVER_H

#include <cstdbool>

#include "net/gamemessage.h"
#include "net/rudpsocket.h"

namespace Engine {
	using namespace Net;

	class Server {
	public:
		Server();
		~Server();

		bool tick(void);

		// These should be considered private
		void receiptFunctor(RudpSocket::ConnectionId conId, RudpPacket::SeqNum seqNum, bool success);
		void recvFunctor(RudpSocket::ConnectionId conId, RudpPacket::SeqNum seqNum, const void *data, size_t len);
		void connectFunctor(RudpSocket::ConnectionId conId);
		void disconnectFunctor(RudpSocket::ConnectionId conId);

	private:
		RudpSocket *sock;

		struct SessionData {
			uint64_t token;
			char username[GameMessage::maxUsernameSize+1];
		};

		static const size_t SessionArraySize=65536;
		SessionData sessionArray[SessionArraySize];

		SessionData *getSessionDataByConId(RudpSocket::ConnectionId conId);
		SessionData *getSessionDataByUsername(const char *username);
		SessionData *addSessionData(RudpSocket::ConnectionId conId, const char *username);
		void removeSessionData(RudpSocket::ConnectionId conId);
	};
};

#endif
