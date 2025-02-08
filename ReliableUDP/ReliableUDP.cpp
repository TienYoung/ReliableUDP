/*
	Reliability and Flow Control Example
	From "Networking for Game Programmers" - http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "Net.h"
#include "Utilities.h"

#pragma warning(disable: 4996)

//#define SHOW_ACKS

using namespace std;
using namespace net;

const int ServerPort = 30000;
const int ClientPort = 30001;
const int ProtocolId = 0x11223344;
const float DeltaTime = 1.0f / 30.0f;
const float SendRate = 1.0f / 30.0f;
const float TimeOut = 10.0f;
const int PacketSize = 256;

class FlowControl
{
public:

	FlowControl()
	{
		printf("flow control initialized\n");
		Reset();
	}

	void Reset()
	{
		mode = Bad;
		penalty_time = 4.0f;
		good_conditions_time = 0.0f;
		penalty_reduction_accumulator = 0.0f;
	}

	void Update(float deltaTime, float rtt)
	{
		const float RTT_Threshold = 250.0f;

		if (mode == Good)
		{
			if (rtt > RTT_Threshold)
			{
				printf("*** dropping to bad mode ***\n");
				mode = Bad;
				if (good_conditions_time < 10.0f && penalty_time < 60.0f)
				{
					penalty_time *= 2.0f;
					if (penalty_time > 60.0f)
						penalty_time = 60.0f;
					printf("penalty time increased to %.1f\n", penalty_time);
				}
				good_conditions_time = 0.0f;
				penalty_reduction_accumulator = 0.0f;
				return;
			}

			good_conditions_time += deltaTime;
			penalty_reduction_accumulator += deltaTime;

			if (penalty_reduction_accumulator > 10.0f && penalty_time > 1.0f)
			{
				penalty_time /= 2.0f;
				if (penalty_time < 1.0f)
					penalty_time = 1.0f;
				printf("penalty time reduced to %.1f\n", penalty_time);
				penalty_reduction_accumulator = 0.0f;
			}
		}

		if (mode == Bad)
		{
			if (rtt <= RTT_Threshold)
				good_conditions_time += deltaTime;
			else
				good_conditions_time = 0.0f;

			if (good_conditions_time > penalty_time)
			{
				printf("*** upgrading to good mode ***\n");
				good_conditions_time = 0.0f;
				penalty_reduction_accumulator = 0.0f;
				mode = Good;
				return;
			}
		}
	}

	float GetSendRate()
	{
		return mode == Good ? 30.0f : 10.0f;
	}

	bool metadata_sent;

private:

	enum Mode
	{
		Good,
		Bad
	};

	Mode mode;
	float penalty_time;
	float good_conditions_time;
	float penalty_reduction_accumulator;
};

// ----------------------------------------------

int main(int argc, char* argv[])
{
	// parse command line

	enum Mode
	{
		Client,
		Server
	};

	Mode mode = Server;
	Address address;

	// TODO: Retrieving additional command line arguments
	if (argc >= 2)
	{
		int a, b, c, d;
#pragma warning(suppress : 4996) 
		if (sscanf(argv[1], "%d.%d.%d.%d", &a, &b, &c, &d))
		{
			mode = Client;
			address = Address(a, b, c, d, ServerPort);

			// Check if the filename was provided
			if (argc >= 3)
			{
				const char* filename = argv[2];
				FILE* fp = fopen(filename, "rb");
				if (!fp)
				{
					printf("Error: Could not open file %s\n", filename);
					return 1;
				}

				printf("Selected file for transfer: %s\n", filename);
			}
			else
			{
				printf("Error: Missing filename\n");
				printf("Usage: %s <ip_address> <filename>\n", argv[0]);
				return 1;
			}
		}
	}

	// initialize

	if (!InitializeSockets())
	{
		printf("failed to initialize sockets\n");
		return 1;
	}

	ReliableConnection connection(ProtocolId, TimeOut);

	const int port = mode == Server ? ServerPort : ClientPort;

	if (!connection.Start(port))
	{
		printf("could not start connection on port %d\n", port);
		return 1;
	}

	if (mode == Client)
		// TODO: Retrieving the file from disk
		connection.Connect(address);
	else
		connection.Listen();

	bool connected = false;
	float sendAccumulator = 0.0f;
	float statsAccumulator = 0.0f;

	auto transferStartTime = std::chrono::high_resolution_clock::now();
	bool transferStarted = false;
	bool transferDone = false;

	FlowControl flowControl;

	FileSlices fileSlices;
	bool fileLoaded = false;

	while (true)
	{
		// update flow control

		if (connection.IsConnected())
			flowControl.Update(DeltaTime, connection.GetReliabilitySystem().GetRoundTripTime() * 1000.0f);

		const float sendRate = flowControl.GetSendRate();

		// detect changes in connection state

		if (mode == Server && connected && !connection.IsConnected())
		{
			flowControl.Reset();
			printf("reset flow control\n");
			connected = false;
		}

		if (!connected && connection.IsConnected())
		{
			printf("client connected to server\n");
			connected = true;
			// A1: Breaking the file in pieces to send
			if (mode == Client)
			{
				fileLoaded = fileSlices.Load("./Smiley.png");
			}
		}

		if (!connected && connection.ConnectFailed())
		{
			printf("connection failed\n");
			break;
		}
		// TODO: Sending file metadata
		
		// TODO: Breaking the file in pieces to send
		
		// send and receive packets

		sendAccumulator += DeltaTime;

		while (sendAccumulator > 1.0f / sendRate)
		{
			// A1: Sending the pieces
			unsigned char packet[PacketSize];
			memset(packet, 0, sizeof(packet));
			static int n = 0;
			//sprintf_s((char*)packet, PacketSize, "Hello World %d\n", ++n);
			if (mode == Client && fileLoaded)
			{
				static bool metaSent = false;
				if (metaSent == false)
				{
					printf("Sending %s, %lld bytes, %lld in total slices.\n", fileSlices.GetMeta()->filename, fileSlices.GetMeta()->fileSize, fileSlices.GetMeta()->totalSlices);
					memcpy(packet, fileSlices.GetMeta(), PacketSize);
					metaSent = true;
				}
				else
				{
					if (n < fileSlices.GetTotal())
					{
						printf("Sending %lld/%lld\n", fileSlices.GetSlice(n)->id + 1, fileSlices.GetMeta()->totalSlices);
						memcpy(packet, fileSlices.GetSlice(n++), PacketSize);
					}
				}
			}
			connection.SendPacket(packet, sizeof(packet));
			sendAccumulator -= 1.0f / sendRate;
		}

		while (true)
		{
			unsigned char packet[256];
			// TODO: Receiving the file metadata

			int bytes_read = connection.ReceivePacket(packet, sizeof(packet));
			// TODO: Receiving the file pieces

			// TODO: Writing the pieces out to disk

			if (bytes_read == 0)
				break;
			//printf("%s", packet);
			if (mode == Server)
			{
				if (!fileSlices.IsRead())
				{
					printf("Receiving!\n");
					fileSlices.Deserialize(packet);

					// Record the start time of receiving
					if (!transferStarted) {
						transferStartTime = std::chrono::high_resolution_clock::now();
						transferStarted = true;
					}
				}
				else
				{
					static bool saved = false;
					if (!saved && fileSlices.Verify())
					{
						auto transferEndTime = std::chrono::high_resolution_clock::now();
						auto transferDuration = std::chrono::duration_cast<std::chrono::milliseconds>(transferEndTime - transferStartTime);

						double transferSeconds = transferDuration.count() / 1000.0;
						// Calculate file size in bits
						double fileBits = fileSlices.GetMeta()->fileSize * 8.0;
						// Calculate transfer speed megabits per second
						double transferSpeedMbps = (fileBits / 1000000.0) / transferSeconds;

						printf("Transfer completed!\n");
						printf("Time taken: %.3f seconds\n", transferSeconds);
						printf("Speed: %.2f Mbps\n", transferSpeedMbps);

						fileSlices.Save();
						saved = true;
					}
				}
			}
		}

		// show packets that were acked this frame

#ifdef SHOW_ACKS
		unsigned int* acks = NULL;
		int ack_count = 0;
		connection.GetReliabilitySystem().GetAcks(&acks, ack_count);
		if (ack_count > 0)
		{
			printf("acks: %d", acks[0]);
			for (int i = 1; i < ack_count; ++i)
				printf(",%d", acks[i]);
			printf("\n");
		}
#endif

		// update connection

		connection.Update(DeltaTime);

		// show connection stats

		statsAccumulator += DeltaTime;

		while (statsAccumulator >= 0.25f && connection.IsConnected())
		{
			float rtt = connection.GetReliabilitySystem().GetRoundTripTime();

			unsigned int sent_packets = connection.GetReliabilitySystem().GetSentPackets();
			unsigned int acked_packets = connection.GetReliabilitySystem().GetAckedPackets();
			unsigned int lost_packets = connection.GetReliabilitySystem().GetLostPackets();

			float sent_bandwidth = connection.GetReliabilitySystem().GetSentBandwidth();
			float acked_bandwidth = connection.GetReliabilitySystem().GetAckedBandwidth();

			printf("rtt %.1fms, sent %d, acked %d, lost %d (%.1f%%), sent bandwidth = %.1fkbps, acked bandwidth = %.1fkbps\n",
				rtt * 1000.0f, sent_packets, acked_packets, lost_packets,
				sent_packets > 0.0f ? (float)lost_packets / (float)sent_packets * 100.0f : 0.0f,
				sent_bandwidth, acked_bandwidth);

			statsAccumulator -= 0.25f;
		}

		// TODO: Verifying the file integrity

		net::wait(DeltaTime);
	}

	ShutdownSockets();

	return 0;
}
