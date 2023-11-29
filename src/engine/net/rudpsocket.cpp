#include "rudpsocket.h"

namespace Engine {
	namespace Net {
		RudpSocket::RudpSocket(Util::TimeMs receiptTimeoutMs, Util::TimeMs keepAliveTimeMs, Util::TimeMs disconnectTimeoutMs, ReceiptFunctor *receiptFunctor, RecvFunctor *recvFunctor, ConnectFunctor *connectFunctor, DisconnectFunctor *disconnectFunctor, void *functorUserData): receiptTimeoutMs(receiptTimeoutMs), keepAliveTimeMs(keepAliveTimeMs), disconnectTimeoutMs(disconnectTimeoutMs), receiptFunctor(receiptFunctor), recvFunctor(recvFunctor), connectFunctor(connectFunctor), disconnectFunctor(disconnectFunctor), functorUserData(functorUserData) {
			isServer=false;
			socketHandle=-1;
			connectionArrayInit();
		}

		RudpSocket::~RudpSocket() {
			disconnect();
			connectionArrayFree();
		}

		bool RudpSocket::connect(const char *hostname, uint16_t port) {
			// Already connected?
			if (socketHandle>=0)
				return false;

			// Reset fields
			isServer=false;
			socketHandle=-1;
			connectionArrayInit();

			// Open socket (datagram/UDP, non-blocking)
			socketHandle=::socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
			if (socketHandle<0)
				return false;

			// Server/client separate logic
			if (hostname!=NULL) {
				// Client mode
				isServer=false;

				// Create address to represent the destination server
				struct sockaddr_in serverAddr;
				serverAddr.sin_family=AF_INET;
				serverAddr.sin_port=htons(port);
				if (inet_aton(hostname, &serverAddr.sin_addr)==0) {
					disconnect();
					return false;
				}

				// Add server to list of 'connections'
				ConnectionData *server=connectionArrayGetData(&serverAddr);
				if (server==NULL) {
					disconnect();
					return false;
				}
			} else {
				// Server mode
				isServer=true;

				// Create server address
				struct sockaddr_in serverAddr;
				memset(&serverAddr, 0, sizeof(serverAddr));

				serverAddr.sin_family=AF_INET;
				serverAddr.sin_port=htons(port);
				serverAddr.sin_addr.s_addr=INADDR_ANY;

				// Bind socket
				if (bind(socketHandle, (const struct sockaddr *)&serverAddr, sizeof(serverAddr))<0) {
					disconnect();
					return false;
				}
			}

			return true;
		}

		void RudpSocket::disconnect(void) {
			if (socketHandle>=0) {
				::close(socketHandle);
				socketHandle=-1;

				connectionArrayFree();
			}
		}

		int RudpSocket::write(ConnectionId conId, const void *data, size_t len) {
			assert(data!=NULL || len==0);

			// Bad connection id?
			if (conId>=connectionArrayNext)
				return -1;

			ConnectionData *connection=connectionArray+conId;
			if (!connection->active)
				return -1;

			// Not connected?
			if (socketHandle<0)
				return -1;

			// If asked for message receipts, first make sure we have space in the queue
			if (receiptFunctor!=NULL && connection->sentQueueNext>=sentQueueSize)
				return -1; // no space to add another entry to the queue - abort write

			// Form packet
			RudpPacket writePacket(connection->nextWriteSeqNum, connection->latestReadSeqNum, connection->readAckBitset, data, len);

			connection->nextWriteSeqNum=RudpPacket::seqNumInc(connection->nextWriteSeqNum);

			// Send packet
			if (::sendto(socketHandle, writePacket.getData(), writePacket.getDataLen(), 0, reinterpret_cast<sockaddr*>(&connection->addr), sizeof(connection->addr))!=writePacket.getDataLen())
				return -1;

			// Update fields
			connection->lastWriteTimeMs=Util::getTimeMs();
			connection->readSinceLastWrite=0;

			// Add to sent queue (only if needed for receipt functor)
			if (receiptFunctor!=NULL) {
				assert(connection->sentQueueNext<sentQueueSize); // due to check above
				SentQueueEntry *entry=connection->sentQueue+connection->sentQueueNext;
				++connection->sentQueueNext;

				entry->seqNum=writePacket.getSeqNum();
				entry->timeoutTimeMs=connection->lastWriteTimeMs+receiptTimeoutMs;
			}

			return (int)writePacket.getSeqNum();
		}

		bool RudpSocket::tick(void) {
			Util::TimeMs currTime=Util::getTimeMs();

			// Handle any new data
			while(1) {
				// See if we have received any data and try to form it into a packet
				struct sockaddr_in connectionAddr;
				socklen_t connectionAddrLen=sizeof(connectionAddr);
				memset(&connectionAddr, 0, connectionAddrLen);

				RudpPacket recvPacket;
				ssize_t recvLen=recvfrom(socketHandle, recvPacket.getData(), RudpPacket::maxSize, 0, (struct sockaddr *)&connectionAddr, &connectionAddrLen);
				if (recvLen<=0)
					break;

				// Bad packet?
				if (!recvPacket.isConsistent())
					continue;

				// Find connection data
				ConnectionData *connection=connectionArrayGetData(&connectionAddr);
				if (connection==NULL || !connection->active)
					continue;

				// Update fields
				connection->lastReadTimeMs=currTime;
				connection->readSinceLastWrite=std::min(connection->readSinceLastWrite+1, 200); // Note: 2nd arg can't be 2^8-1 exactly, and just has to leave enough range to cover threshold below

				// New most recent packet?
				if (RudpPacket::seqNumIsAfter(recvPacket.getSeqNum(), connection->latestReadSeqNum) || connection->readAckBitset==0) {
					// 'Slide' ack bitset based on jump from previous most-recent packet
					RudpPacket::SeqNum diff=RudpPacket::seqNumSub(recvPacket.getSeqNum(), connection->latestReadSeqNum);
					connection->readAckBitset>>=diff;

					// Update last read num
					connection->latestReadSeqNum=recvPacket.getSeqNum();

					// Update ack bitset to indicate we have received this new packet
					connection->readAckBitset|=(((RudpPacket::AckBitset)1)<<(RudpPacket::AckBitsetSize-1));
				}

				// Is it about time we sent a packet to ensure we ack what we have received?
				if (connection->readSinceLastWrite>RudpPacket::AckBitsetSize/4)
					write(connectionArrayGetConnectionId(connection), NULL, 0);

				// Check ack info in received packet to see if need to invoke success functors for any previously sent packets
				if (receiptFunctor!=NULL) {
					RudpPacket::AckBitset recvAckBitset=recvPacket.getAckBitset();
					RudpPacket::SeqNum loopSeqNum=RudpPacket::seqNumSub(recvPacket.getAckSeqNum(), RudpPacket::AckBitsetSize-1);
					for(unsigned i=0; i<RudpPacket::AckBitsetSize; ++i) {
						if (recvAckBitset&((1u)<<i))
							sentQueueEntryAck(connection, loopSeqNum);
						loopSeqNum=RudpPacket::seqNumInc(loopSeqNum);
					}
				}

				// Was there an actual payload in this packet?
				// If so invoke recv callback if needed
				if (recvPacket.getPayloadLen()>0 && recvFunctor!=NULL)
					recvFunctor(connectionArrayGetConnectionId(connection), recvPacket.getSeqNum(), recvPacket.getPayload(), recvPacket.getPayloadLen(), functorUserData);
			}

			// Time to send a keep-alive style packet?
			for(size_t conId=0; conId<connectionArrayNext; ++conId) {
				ConnectionData *connection=&connectionArray[conId];
				if (connection->active && currTime-connection->lastWriteTimeMs>keepAliveTimeMs) {
					// Send packet with empty payload to keep connection alive and also potentially send ack's
					write(conId, NULL, 0);
				}
			}

			// Handle any timeouts from previous packets sent
			for(size_t conId=0; conId<connectionArrayNext; ++conId) {
				ConnectionData *connection=&connectionArray[conId];

				if (!connection->active)
					continue;

				while(connection->sentQueueNext>0) {
					assert(receiptFunctor!=NULL);

					SentQueueEntry *entry=connection->sentQueue;

					// Not timed out yet?
					if (entry->timeoutTimeMs>currTime)
						break; // all subsequent entries will have timeouts even further in the future

					// Invoke callback
					receiptFunctor(conId, entry->seqNum, false, functorUserData);

					// Remove entry from queue
					--connection->sentQueueNext;
					memmove(connection->sentQueue, connection->sentQueue+1, sizeof(SentQueueEntry)*connection->sentQueueNext);
				}
			}

			// Handle any connections which have timed out
			for(size_t conId=0; conId<connectionArrayNext; ++conId) {
				ConnectionData *connection=&connectionArray[conId];

				if (!connection->active)
					continue;

				// Check for timeout (not read anything in a while)
				if (connection->lastReadTimeMs+disconnectTimeoutMs<currTime) {
					// Clear send queue, invoking receipt functors to indicate failure
					assert(receiptFunctor!=NULL || connection->sentQueueNext==0);
					for(size_t qi=0; qi<connection->sentQueueNext; ++qi)
						receiptFunctor(conId, connection->sentQueue[qi].seqNum, false, functorUserData);

					connection->sentQueueNext=0;

					// Invoke disconnect functor
					if (disconnectFunctor!=NULL)
						disconnectFunctor(conId, functorUserData);

					// Remove from connection array
					connection->active=false;

					// If in client mode then this means we have disconnected from the server
					if (!isServer)
						return false;
				}
			}

			return true;
		}

		void RudpSocket::setFunctorUserData(void *data) {
			functorUserData=data;
		}

		void RudpSocket::sentQueueEntryAck(ConnectionData *connection, RudpPacket::SeqNum seqNum) {
			assert(receiptFunctor!=NULL);

			ConnectionId conId=connectionArrayGetConnectionId(connection);

			for(size_t i=0; i<connection->sentQueueNext; ++i) {
				// Found matching entry in the queue?
				if (connection->sentQueue[i].seqNum==seqNum) {
					// Invoke receipt callback
					receiptFunctor(conId, seqNum, true, functorUserData);

					// Remove entry from queue
					--connection->sentQueueNext;
					memmove(connection->sentQueue+i, connection->sentQueue+i+1, sizeof(SentQueueEntry)*(connection->sentQueueNext-i));

					break;
				}
			}
		}

		void RudpSocket::connectionArrayInit(void) {
			connectionArrayNext=0;
		}

		void RudpSocket::connectionArrayFree(void) {
			connectionArrayNext=0;
		}

		RudpSocket::ConnectionData *RudpSocket::connectionArrayGetData(const struct sockaddr_in *addr) {
			// Sanity check
			if (addr->sin_family!=AF_INET)
				return NULL;

			// Look for existing entry matching this addr
			ConnectionId newId=connectionArrayNext;
			for(size_t i=0; i<connectionArrayNext; ++i) {
				if (connectionArray[i].active) {
					if (connectionArrayAddrsEqual(&connectionArray[i].addr, addr))
						return connectionArray+i;
				} else {
					if (newId==connectionArrayNext)
						newId=i;
				}
			}

			// If in client mode there should only ever be a single 'client' (the server itself)
			if (!isServer && connectionArrayNext>0)
				return NULL;

			// Not found initialise new/existing entry
			if (newId==connectionArrayNext) {
				if (connectionArrayNext>=connectionArraySize)
					return NULL; // connection limit hit

				++connectionArrayNext;
			}

			ConnectionData *data=connectionArray+newId;

			data->active=true;
			data->addr=*addr;
			data->lastWriteTimeMs=0;
			data->nextWriteSeqNum=0;
			data->lastReadTimeMs=Util::getTimeMs(); // to avoid immediately thinking we have timed out
			data->latestReadSeqNum=0;
			data->readAckBitset=0;
			data->readSinceLastWrite=0;
			data->sentQueueNext=0;

			// Invoke connect functor
			if (connectFunctor!=NULL)
				connectFunctor(newId, functorUserData);

			return data;
		}

		bool RudpSocket::connectionArrayAddrsEqual(const struct sockaddr_in *a, const struct sockaddr_in *b) {
			assert(a->sin_family==AF_INET);
			assert(b->sin_family==AF_INET);

			return (a->sin_addr.s_addr==b->sin_addr.s_addr && a->sin_port==b->sin_port);
		}

		RudpSocket::ConnectionId RudpSocket::connectionArrayGetConnectionId(ConnectionData *connection) {
			return connection-connectionArray;
		}

	};
};