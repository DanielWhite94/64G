#ifndef ENGINE_NET_RUDPSOCKET_H
#define ENGINE_NET_RUDPSOCKET_H

#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <cstdint>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "../util.h"
#include "rudppacket.h"

namespace Engine {
	namespace Net {

		class RudpSocket {
		public:
			// Connection IDs are used in server mode to distinguish between connected clients.
			// When in client mode these IDs should always be 0.
			typedef uint16_t ConnectionId;

			// this functor is called from tick when a written packet is either ack'd (so received), or we timeout waiting
			// in client mode conId is always 0
			typedef void ReceiptFunctor(ConnectionId conId, RudpPacket::SeqNum seqNum, bool success, void *userData);

			// this functor is called from tick when we receive a packet from the host (in client mode) or a client (in server mode)
			// in client mode conId is always 0
			typedef void RecvFunctor(ConnectionId conId, RudpPacket::SeqNum seqNum, const void *data, size_t len, void *userData);

			// these functors are called when a client (or the server when in client mode) connects/disconnects
			typedef void ConnectFunctor(ConnectionId conId, void *userData);
			typedef void DisconnectFunctor(ConnectionId conId, void *userData);

			RudpSocket(Util::TimeMs receiptTimeoutMs, Util::TimeMs keepAliveTimeMs, Util::TimeMs disconnectTimeoutMs, ReceiptFunctor *receiptFunctor, RecvFunctor *recvFunctor, ConnectFunctor *connectFunctor, DisconnectFunctor *disconnectFunctor, void *functorUserData);
			~RudpSocket();

			bool connect(const char *hostname, uint16_t port); // pass NULL for hostname to specify server mode
			void disconnect(void);

			// write data
			// on success returns (non-negative) RudpPacket::SeqNum of sent packet, on failure returns -1
			// if successful then ReceiptFunctor is guaranteed to be invoked exactly once for the returned seq num
			// if empty payload set len=0 (data can be NULL or non-NULL in this case), can be used as a keep-alive packet for example
			// in client mode pass 0 for conId
			int write(ConnectionId conId, const void *data, size_t len);

			// tick reads any incoming data and potentially invokes receipt or recv functors, and handles timeouts
			// returns false if there was an error and the socket should be disconnected
			bool tick(void);

			void setFunctorUserData(void *data);
		private:
			Util::TimeMs receiptTimeoutMs;
			Util::TimeMs keepAliveTimeMs;
			Util::TimeMs disconnectTimeoutMs;

			ReceiptFunctor *receiptFunctor;
			RecvFunctor *recvFunctor;
			ConnectFunctor *connectFunctor;
			DisconnectFunctor *disconnectFunctor;
			void *functorUserData;

			bool isServer;
			int socketHandle;

			struct SentQueueEntry {
				RudpPacket::SeqNum seqNum;
				Util::TimeMs timeoutTimeMs;
			};
			static const size_t sentQueueSize=256;

			struct ConnectionData {
				bool active;

				struct sockaddr_in addr;

				Util::TimeMs lastWriteTimeMs;
				RudpPacket::SeqNum nextWriteSeqNum;

				Util::TimeMs lastReadTimeMs;
				RudpPacket::SeqNum latestReadSeqNum;
				RudpPacket::AckBitset readAckBitset;

				uint8_t readSinceLastWrite; // an approximation of sorts - used as a heuristic for sending packets to keep up with acks

				SentQueueEntry sentQueue[sentQueueSize];
				size_t sentQueueNext;
			};

			static const size_t connectionArraySize=65536;
			ConnectionData connectionArray[connectionArraySize];
			size_t connectionArrayNext;

			void sentQueueEntryAck(ConnectionData *connection, RudpPacket::SeqNum seqNum);

			void connectionArrayInit(void);
			void connectionArrayFree(void);
			ConnectionData *connectionArrayGetData(const struct sockaddr_in *addr);
			bool connectionArrayAddrsEqual(const struct sockaddr_in *a, const struct sockaddr_in *b);
			ConnectionId connectionArrayGetConnectionId(ConnectionData *connection);
		};
	};
};

#endif
