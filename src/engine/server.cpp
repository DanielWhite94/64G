#include <cstring>
#include <cstdio>
#include <unistd.h>

#include "server.h"
#include "util.h"

using namespace Engine;

namespace Engine {

void serverReceiptFunctorWrapper(RudpSocket::ConnectionId conId, RudpPacket::SeqNum seqNum, bool success, void *userData);
void serverRecvFunctorWrapper(RudpSocket::ConnectionId conId, RudpPacket::SeqNum seqNum, const void *data, size_t len, void *userData);
void serverConnectFunctorWrapper(RudpSocket::ConnectionId conId, void *userData);
void serverDisconnectFunctorWrapper(RudpSocket::ConnectionId conId, void *userData);

Server::Server() {
	// Clear session array (by setting token to an invalid value for each slot)
	sessionArray[0].token=1; // so that lower 16 bits do not match the array index so we know this slot is unused (as are all others)
	for(size_t i=1; i<SessionArraySize; ++i)
		sessionArray[i].token=0;

	// Connect socket using custom 'reliable' UDP protocol
	uint16_t port=9000;
	Util::TimeMs receiptTimeoutMs=1200;
	Util::TimeMs keepAliveTimeMs=500;
	Util::TimeMs disconnectTimeoutMs=4000;

	sock=new RudpSocket(receiptTimeoutMs, keepAliveTimeMs, disconnectTimeoutMs, &serverReceiptFunctorWrapper, &serverRecvFunctorWrapper, &serverConnectFunctorWrapper, &serverDisconnectFunctorWrapper, this);
	if (!sock->connect(NULL, port)) {
		delete sock;
		sock=NULL; // this will be picked up by first iteration of tick function
		return;
	}
}

Server::~Server() {
	// Not connected?
	if (sock==NULL)
		return;

	// Disconnect and free socket
	sock->disconnect();

	delete sock;
	sock=NULL;
}

bool Server::tick(void) {
	// Not connected?
	if (sock==NULL)
		return false;

	// Handle received data and timeouts
	if (!sock->tick())
		return false;

	return true;
}

void Server::receiptFunctor(RudpSocket::ConnectionId conId, RudpPacket::SeqNum seqNum, bool success) {
}

void Server::recvFunctor(RudpSocket::ConnectionId conId, RudpPacket::SeqNum seqNum, const void *gdata, size_t len) {
	const uint8_t *data=(const uint8_t *)gdata;

	// Handle received data
	GameMessage message;
	while(1) {
		// Attempt to parse and consume some data into a message
		uint8_t messageLen=message.parseData(data, len);
		if (messageLen==0)
			break; // if no match then do nothing - don't even bother sending an error response - also don't trust this entire rudp packet

		data+=messageLen;
		len-=messageLen;

		// Handle the message
		switch(message.getCommandId()) {
			case GameMessage::CommandIdNone: {
				// Nothing to do
			} break;
			case GameMessage::CommandIdGenericResponse: {
				// Nothing to do
			} break;
			case GameMessage::CommandIdGameJoinRequest: {
				// Grab desired username
				char username[GameMessage::maxUsernameSize+1];
				message.commandGameJoinRequestGetUsername(username);

				// Check if this connection already has an associated session
				SessionData *sessionData=getSessionDataByConId(conId);
				if (sessionData!=NULL) {
					// If the username matches then simply repeat the response we sent last time (in case it was not received)
					// If the username does not match then give already joined error
					GameMessage reply;
					if (strcmp(username, sessionData->username)==0)
						reply.commandGameJoinResponseCreate(sessionData->token);
					else
						reply.commandGenericResponseCreate(GameMessage::GenericResponse::AlreadyJoined);
					sock->write(conId, reply.getData(), reply.getDataLen());
					break;
				}

				// Verify desired username
				/*

				.....

				want to verify the username too (i.e. no bad characters)
				GenericResponse::UsernameBad

				*/

				// Ensure desired username is not already in use
				if (getSessionDataByUsername(username)!=NULL) {
					GameMessage reply;
					reply.commandGenericResponseCreate(GameMessage::GenericResponse::UsernameUsed);
					sock->write(conId, reply.getData(), reply.getDataLen());
					break;
				}

				// Create session data to represent this user
				sessionData=Server::addSessionData(conId, username);

				// Send response
				GameMessage reply;
				reply.commandGameJoinResponseCreate(sessionData->token);
				sock->write(conId, reply.getData(), reply.getDataLen());
			} break;
			case GameMessage::CommandIdGameJoinResponse: {
				// We should not receive this command
			} break;
		}
	}
}

void Server::connectFunctor(RudpSocket::ConnectionId conId) {
}

void Server::disconnectFunctor(RudpSocket::ConnectionId conId) {
	removeSessionData(conId);
}

Server::SessionData *Server::getSessionDataByConId(RudpSocket::ConnectionId conId) {
	SessionData *data=sessionArray+conId;

	// Check this slot is active
	if ((data->token&0xFFFF)!=conId)
		return NULL;

	return data;
}

Server::SessionData *Server::getSessionDataByUsername(const char *username) {
	// TODO: think of more efficient structure/algo

	for(size_t i=0; i<SessionArraySize; ++i) {
		if ((sessionArray[i].token&0xFFFF)==i && strcmp(sessionArray[i].username,  username)==0)
			return sessionArray+i;
	}

	return NULL;
}

Server::SessionData *Server::addSessionData(RudpSocket::ConnectionId conId, const char *username) {
	SessionData *data=sessionArray+conId;

	// Create token based on random bits and the connection id
	data->token=((Util::rand64()<<16)|conId);

	// Set other fields
	strcpy(data->username, username);

	// ..... temp debugging
	printf("@@@@@ server session add: conId=%u, username='%s', token=0x%016lX\n", conId, username, data->token); // .....

	return data;
}

void Server::removeSessionData(RudpSocket::ConnectionId conId) {
	SessionData *data=sessionArray+conId;

	// ..... temp debugging
	printf("@@@@@ server session remove: conId=%u, username='%s', token=0x%016lX\n", conId, data->username, data->token); // .....

	// Set token to an invalid value for this slot to indicate it is unused
	data->token=(conId==0 ? 1 : 0);
}

void serverReceiptFunctorWrapper(RudpSocket::ConnectionId conId, RudpPacket::SeqNum seqNum, bool success, void *userData) {
	Server *server=(Server *)userData;
	server->receiptFunctor(conId, seqNum, success);
}

void serverRecvFunctorWrapper(RudpSocket::ConnectionId conId, RudpPacket::SeqNum seqNum, const void *data, size_t len, void *userData) {
	Server *server=(Server *)userData;
	server->recvFunctor(conId, seqNum, data, len);
}

void serverConnectFunctorWrapper(RudpSocket::ConnectionId conId, void *userData) {
	Server *server=(Server *)userData;
	server->connectFunctor(conId);
}

void serverDisconnectFunctorWrapper(RudpSocket::ConnectionId conId, void *userData) {
	Server *server=(Server *)userData;
	server->disconnectFunctor(conId);
}

};
