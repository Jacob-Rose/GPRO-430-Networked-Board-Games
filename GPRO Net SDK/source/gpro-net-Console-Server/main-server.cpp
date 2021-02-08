/*
   Copyright 2021 Daniel S. Buckstein

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

	   http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

/*
	GPRO Net SDK: Networking framework.
	By Daniel S. Buckstein

	main-server.c/.cpp
	Main source for console server application.
*/

#include <stdio.h>
#include <string.h>
#include <vector>
#include <future>
#include <limits>
#include <iostream>
#include <list>
#include <map>

//RakNet
#include <RakNet/RakPeerInterface.h>
#include <RakNet/MessageIdentifiers.h>
#include <RakNet/RakNetTypes.h>
#include <RakNet/GetTime.h>
#include <RakNet/BitStream.h>
#include <RakNet/RakNetTypes.h>  // MessageID

#include "gpro-net/shared-net.h"

struct ServerState 
{
	// not much need for anything else rn
	RakNet::RakPeerInterface* peer;
	std::vector<ChatMessage> messageCache;


	std::map<RakNet::SystemAddress, std::string> m_DisplayNames;
};

void handleInput(ServerState* ss) 
{
	RakNet::Packet* packet;
	for (packet = ss->peer->Receive(); packet; ss->peer->DeallocatePacket(packet), packet = ss->peer->Receive())
	{
		RakNet::MessageID msg;
		RakNet::BitStream bsIn(packet->data, packet->length, false);
		bsIn.Read(msg);


		RakNet::Time timestamp = NULL;
		if (msg == ID_TIMESTAMP)
		{
			//todo handle time
			bsIn.Read(timestamp);
			bsIn.Read(msg);//now we update to show the real message
		}

		switch (msg)
		{
		case ID_REMOTE_DISCONNECTION_NOTIFICATION:
			printf("Another client has disconnected.\n");
			break;
		case ID_REMOTE_CONNECTION_LOST:
			printf("Another client has lost the connection.\n");
			break;
		case ID_REMOTE_NEW_INCOMING_CONNECTION:
			printf("A client has connected.\n");
			//Todo, send them all the display names currently active
			break;
		case ID_NEW_INCOMING_CONNECTION:
			printf("A connection is incoming.\n");
			break;
		case ID_NO_FREE_INCOMING_CONNECTIONS:
			printf("The server is full.\n");
			break;
		case ID_DISCONNECTION_NOTIFICATION:
			printf("A client has disconnected.\n");
			//todo remove display name and relay to clients
			break;
		case ID_CONNECTION_LOST:
			printf("A client lost the connection.\n");
			//todo remove display name and relay to clients
			break;
		case ID_CHAT_MESSAGE:
		{
			RakNet::RakString msgStr;
			bsIn.Read(msgStr);
			ChatMessage msg{
				timestamp,
				packet->systemAddress,
				msgStr.C_String()
			};
			
			//Save message to file
			//output message


			//Broadcast Message To Everyone (except one who sent it)
			RakNet::BitStream untamperedBS(packet->data, packet->length, false); //so we send the whole message
			ss->peer->Send(&untamperedBS, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true);
		}
		break;
		case ID_DISPLAY_NAME_UPDATED:
		{
			RakNet::SystemAddress sender;
			RakNet::RakString displayName;

			bsIn.Read(sender);
			bsIn.Read(displayName);

			if (sender == packet->systemAddress) //make sure the client is changing their name only
			{
				ss->m_DisplayNames[packet->systemAddress] = displayName.C_String();
			}

			//Broadcast Message To Everyone (except one who sent it)
			RakNet::BitStream untamperedBS(packet->data, packet->length, false); //so we send the whole message
			ss->peer->Send(&untamperedBS, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true);
		}
		break;
		default:
			printf("Message with identifier %i has arrived.\n", packet->data[0]);
			break;
		}
	}
}

void handleUpdate(ServerState* ss)
{
	
}

void handleOutput(ServerState* ss)
{
	
}



int main(void)
{
	const unsigned short SERVER_PORT = 7777;
	const unsigned short MAX_CLIENTS = 10;

	ServerState ss[1] = { 0 };

	ss->peer = RakNet::RakPeerInterface::GetInstance();
	RakNet::SocketDescriptor sd(SERVER_PORT, 0);
	ss->peer->Startup(MAX_CLIENTS, &sd, 1);
	ss->peer->SetMaximumIncomingConnections(MAX_CLIENTS);
	printf("Starting the server.\n");
	// We need to let the server accept incoming connections from the clients


	while (1)
	{
		
		handleInput(ss);

		handleUpdate(ss);

		handleOutput(ss);
	}


	RakNet::RakPeerInterface::DestroyInstance(ss->peer);

	return 0;
}
