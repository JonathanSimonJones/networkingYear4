/* Definitions for the network protocol that the client and server use to communicate */

#ifndef PROTOCOL_H
#define PROTOCOL_H

// Message types.
enum MessageType
{
	MT_UNKNOWN = 0,
	MT_WELCOME = 1,
	MT_KEYPRESS = 2
};

// The message structure.
// This is a "plain old data" type, so we can send it over the network.
// (In a real program, we would want this structure to be packed.)
struct NetMessage
{
	MessageType type;
	int data;

	NetMessage()
		: type(MT_UNKNOWN), data(0)
	{
	}
};

#endif