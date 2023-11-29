#ifndef	ENGINE_NET_RUDPPACKET_H
#define	ENGINE_NET_RUDPPACKET_H

#include <cassert>
#include <cstdint>
#include <cstring>

namespace Engine {
	namespace Net {
		class RudpPacket {
		public:
			static const unsigned maxSize=512;

			typedef uint32_t SeqNum; // sequential numbers given to subsequent packets (wrapping)

			typedef uint32_t AckBitset; // where set bits indiciate previous packets we have received
			static const unsigned AckBitsetSize=32;

			// is a after b 'chonologically'? (considering wrapping)
			static bool seqNumIsAfter(SeqNum a, SeqNum b) {
				SeqNum amb=seqNumSub(a, b);
				SeqNum bma=seqNumSub(b, a);

				return (amb<bma && amb<((SeqNum)0x3FFFFFFF)); // constant should be less than 1/2 of 2^32, we use 1/4 to be safe
			}

			static SeqNum seqNumInc(SeqNum seqNum) {
				return (((uint64_t)seqNum)+1)&((uint32_t)0xFFFFFFFF);
			}

			static SeqNum seqNumSub(SeqNum a, SeqNum b) {
				return (((uint64_t)a)+((uint64_t)0x100000000)-b)&((uint64_t)0xFFFFFFFF);
			}

			RudpPacket() {
				// Clear first 16 bytes to create an invalid header
				((uint64_t *)data)[0]=0;
				((uint64_t *)data)[1]=0;
			}

			RudpPacket(SeqNum seqNum, SeqNum ackSeqNum, AckBitset ackBitset, const void *payload, uint8_t payloadLen) {
				assert(payload!=NULL || payloadLen==0);

				// Format data array to represent packet
				uint8_t *next=data;

				// (checksum calculated at the end)
				next+=2;

				// Payload length
				next=Util::writeDataLE8(next, payloadLen);

				// Reserved byte
				next=Util::writeDataLE8(next, 0);

				// Sequence number
				next=Util::writeDataLE32(next, seqNum);

				// Ack stuff
				next=Util::writeDataLE32(next, ackSeqNum);
				next=Util::writeDataLE32(next, ackBitset);

				// Payload
				if (payload!=NULL)
					memcpy(next, payload, payloadLen);

				// Checksum (XOR)
				// TODO: improve this e.g. CRC16
				Util::writeDataLE16(data, 0);
				uint16_t checksum=computeChecksum();
				Util::writeDataLE16(data, checksum);
			}

			~RudpPacket() {
			}

			uint16_t getChecksum(void) {
				return Util::readDataLE16(data);
			}

			SeqNum getSeqNum(void) {
				return Util::readDataLE32(data+4);
			}

			SeqNum getAckSeqNum(void) {
				return Util::readDataLE32(data+8);
			}

			AckBitset getAckBitset(void) {
				return Util::readDataLE32(data+12);
			}

			uint8_t getPayloadLen(void) {
				return Util::readDataLE8(data+2);
			}

			void *getPayload(void) {
				return data+16;
			}

			uint16_t getDataLen(void) {
				return getPayloadLen()+16;
			}

			void *getData(void) {
				return data;
			}

			uint16_t computeChecksum() {
				unsigned dataLen=getDataLen();
				data[dataLen]=0; // add final null byte to simplify checksum calculation (not actually sent just done temporarily here)

				uint16_t checksum=31337;
				for(unsigned i=0; i<dataLen; i+=2)
					checksum^=Util::readDataLE16(data+i);

				return checksum;
			}

			bool isConsistent(void) {
				// Verify checksum (compare against 0 as checksum bytes in packet are already set to the checksum instead of 0)
				if (computeChecksum()!=0)
					return false;

				return true;
			}

		private:
			uint8_t data[maxSize];
		};
	};
};

#endif
