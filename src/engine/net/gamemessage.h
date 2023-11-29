#ifndef	ENGINE_NET_GAMEMESSAGE_H
#define	ENGINE_NET_GAMEMESSAGE_H

#include <cassert>
#include <cstdint>
#include <cstring>

#include "../util.h"

namespace Engine {
	namespace Net {

class GameMessage {
public:
	typedef uint16_t CommandId;
	static const CommandId CommandIdNone=0;
	static const CommandId CommandIdGenericResponse=1; // can be sent in either direction and is used for simple (numeric) responses
	static const CommandId CommandIdGameJoinRequest=2; // sent from client to server to request to join the game (TEMPORARY until we have accounts etc)
	static const CommandId CommandIdGameJoinResponse=3; // sent from server to client on success to give client token (TEMPORARY until we have accounts etc)

	enum GenericResponse {
		UsernameBad, // username is not valid (e.g. too long, contains bad characters) (reply to GameJoin)
		UsernameUsed, // username already in use (reply to GameJoin)
		AlreadyJoined, // this connection has already joined (if username etc. match then a CommandIdGameJoinResponse is sent again with the same token as before) (reply to GameJoin)
	};

	static const unsigned maxSize=256;

	static const unsigned maxUsernameSize=15;

	GameMessage() {
		commandNoneCreate();
	}

	~GameMessage() {
	}

	// Returns length of parsed message on success, or 0 on failure
	uint8_t parseData(const void *gdata, size_t dataLen) {
		assert(data!=NULL);

		// Ensure we have enough data (need at least 2 bytes for the command id)
		if (dataLen<2)
			return 0;

		// Copy data
		if (dataLen>maxSize)
			dataLen=maxSize;
		memcpy(data, gdata, dataLen);

		// Return length of data used (can be 0 if bad data)
		return getDataLen();
	}

	CommandId getCommandId(void) {
		return (CommandId)Util::readDataLE16(data+0);
	}

	uint16_t getDataLen(void) {
		switch(getCommandId()) {
			case CommandIdNone:
				return 2;
			break;
			case CommandIdGenericResponse:
				return 3;
			break;
			case CommandIdGameJoinRequest:
				return 3+commandGameJoinRequestGetUsernameLen();
			break;
			case CommandIdGameJoinResponse:
				return 10;
			break;
		}
		return 0;
	}

	void *getData(void) {
		return data;
	}

	void commandNoneCreate(void) {
		// Add command id
		Util::writeDataLE16(data+0, CommandIdNone);
	}

	void commandGenericResponseCreate(GenericResponse response) {
		// Add command id
		Util::writeDataLE16(data+0, CommandIdGenericResponse);

		// Add response value
		Util::writeDataLE8(data+2, response);
	}

	GenericResponse commandGenericResponseGetResponse(void) {
		assert(getCommandId()==CommandIdGenericResponse);

		return (GenericResponse)Util::readDataLE8(data+2);
	}

	void commandGameJoinRequestCreate(const char *username) {
		// Add command id
		Util::writeDataLE16(data+0, CommandIdGameJoinRequest);

		// Add username length and string itself
		size_t usernameLen=strlen(username);
		if (usernameLen>maxUsernameSize)
			usernameLen=maxUsernameSize;

		Util::writeDataLE8(data+2, usernameLen);
		memcpy(data+3, username, usernameLen);
	}

	uint8_t commandGameJoinRequestGetUsernameLen(void) {
		assert(getCommandId()==CommandIdGameJoinRequest);

		uint8_t len=Util::readDataLE8(data+2);
		if (len>maxUsernameSize)
			len=maxUsernameSize;

		return len;
	}

	void commandGameJoinRequestGetUsername(char *username) {
		assert(getCommandId()==CommandIdGameJoinRequest);

		uint8_t len=commandGameJoinRequestGetUsernameLen();
		memcpy(username, data+3, len);

		username[len]='\0';
	}

	void commandGameJoinResponseCreate(uint64_t token) {
		// Add command id
		Util::writeDataLE16(data+0, CommandIdGameJoinResponse);

		// Add token
		Util::writeDataLE64(data+2, token);
	}

	uint64_t commandGameJoinResponseGetToken(void) {
		assert(getCommandId()==CommandIdGameJoinResponse);

		return Util::readDataLE64(data+2);
	}

private:
	uint8_t data[maxSize];
};

};

};

#endif
