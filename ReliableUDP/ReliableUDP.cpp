/*
	Reliability and Flow Control Example
	From "Networking for Game Programmers" - http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ctime>
#include <algorithm>

#include "fileHandler.h"
#include "Net.h"

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
	//Tracks the file transfer states
	enum TransferState {
		idle,
		sendingMetadata,
		sendingFile,
		receivingMetadata,
		receivingFile,
		completed
	} transferState = idle;

	//Variables used in sending and receiving 
	char* fileBuffer = nullptr;
	size_t fileSize = 0;
	size_t currentOffset = 0;
	FileMetadata metadata;
	char tempBuffer[PacketSize];



	// Parse command-line arguments for client/server mode and additional parameters. 
	if (argc >= 2)
	{
		int a, b, c, d;
#pragma warning(suppress : 4996)
		// Parse IP address from the first argument to set up the client mode.  
		if (sscanf_s(argv[1], "%d.%d.%d.%d", &a, &b, &c, &d))
		{
			mode = Client;
			address = Address(a, b, c, d, ServerPort);
		}
	}

	// initialize
	if (mode == Client && argc >= 3) {  // Make sure we have a filename argument
		if (loadFile(argv[2], &fileBuffer, &fileSize) == 0) {
			printf("File loaded successfully: %s (%zu bytes)\n", argv[2], fileSize);
			transferState = sendingMetadata;  // Set initial state for sending
		}
		else {
			printf("Failed to load file: %s\n", argv[2]);
			return 1;
		}
	}
	else {
		transferState = receivingMetadata;
	}
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
		connection.Connect(address);
	else
		connection.Listen();

	bool connected = false;
	float sendAccumulator = 0.0f;
	float statsAccumulator = 0.0f;

	FlowControl flowControl;
	clock_t transfer_start = clock(), transfer_end = clock();
	while (true)
	{
		// update flow control

		//Verify file integrity after receiving all chunks.
		if (connection.IsConnected())
			flowControl.Update(DeltaTime, connection.GetReliabilitySystem().GetRoundTripTime() * 1000.0f);

		const float sendRate = flowControl.GetSendRate();

		// detect changes in connection state

		// Handle connection state changes for the server.
        // Add logic to gracefully handle file transfers during connection interruptions.

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
			if (mode == Client) {
				transfer_start = clock();
			}
			
		}

		if (!connected && connection.ConnectFailed())
		{
			printf("connection failed\n");
			break;
		}
		// send and receive packets

		sendAccumulator += DeltaTime;

		// Break the file into chunks of size `PacketSize` and send each chunk.
		while (sendAccumulator > 1.0f / sendRate)
		{
			if (mode == Client ) {

				switch (transferState) {
				case idle:
				case sendingMetadata: {


					size_t totalMetadataSize = sizeof(FileMetadata);
					size_t currentMetaOffset = 0;
					while (currentMetaOffset < totalMetadataSize) {
						size_t packetSize;
						createMetadataPacket(argv[2], fileSize, computeCRC32(fileBuffer, fileSize), false, tempBuffer, &packetSize, currentMetaOffset);
						connection.SendPacket((unsigned char*)tempBuffer, packetSize);
						currentMetaOffset += packetSize;
					}
					printf("Sent metadata for file: %s\n", argv[2]);
					transferState = sendingFile;
				}
					break;

				case sendingFile:
					if (currentOffset < fileSize) {
						/*size_t packetSize = createDataPacket(fileBuffer, fileSize, currentOffset, tempBuffer, PacketSize, (currentOffset + PacketSize >= fileSize));
						connection.SendPacket((unsigned char*)tempBuffer, packetSize);
						currentOffset += packetSize;
						*/
						size_t remainingSize = fileSize - currentOffset;
						size_t chunkSize = (remainingSize < (size_t)PacketSize) ? remainingSize : (size_t)PacketSize;

						if (chunkSize > PacketSize) {
							chunkSize = PacketSize;
						}

						memcpy(tempBuffer, fileBuffer + currentOffset, chunkSize);
						connection.SendPacket((unsigned char*)tempBuffer, chunkSize);
						currentOffset += chunkSize;
						float progress = (float)currentOffset / fileSize * 100.0f;
						printf("\rSending progress: %.2f%%", progress);
						fflush(stdout);

						if (currentOffset >= fileSize) {
							transfer_end = clock();
							double duration = (double)(transfer_end - transfer_start) / CLOCKS_PER_SEC;
							double speed = calculateTransferSpeed(transfer_start, transfer_end, fileSize);
							printf("Transfer completed\n");
							printf("File size: %zu bytes\n", fileSize);
							printf("Time taken: %.2f seconds\n", duration);
							printf("Transfer speed: %.2f Mbps\n", speed);
							transferState = completed;
						}
					}
					break;
				default:
					break;
				}
			}
			sendAccumulator -= 1.0f / sendRate;
		}

		while (true)
		{
			    //1.Handle the first received packet as metadata containing file details (e.g., name, size).
			    //2. If this is the first packet, treat it as metadata.
				//3. Metadata processing: Extract file name and size to prepare for receiving file chunks.
				//4. Example: Deserialize packet data to retrieve file name and size.

			// Server-Side: Handle receiving file metadata and file chunks
			unsigned char packet[256];
			//transferState = receivingMetadata; ///Changed to make sure it goes in
			int bytesRead = connection.ReceivePacket(packet, sizeof(FileMetadata));
			if (bytesRead <= 0)
				break;
			if (mode == Server) {
				switch (transferState) {
				
				case receivingMetadata: {
					static char metadataBuffer[sizeof(FileMetadata)];
					static size_t receivedMetaOffset = 0;

					if (extractMetadataPacket((char*)packet, bytesRead, &metadata, metadataBuffer, &receivedMetaOffset)) {
						printf("Receiving file: %s (Size: %zu bytes)\n", metadata.filename, metadata.fileSize);

						fileBuffer = (char*)malloc(metadata.fileSize);
						if (!fileBuffer) {
							printf("Failed to allocate memory for file\n");
							return 1;
						}

						currentOffset = 0;
						transferState = receivingFile;
					}
				}
					break;
				case receivingFile:
					if (currentOffset + bytesRead <= metadata.fileSize) {
						memcpy(fileBuffer + currentOffset, packet, bytesRead);
						currentOffset += bytesRead;

						if (currentOffset >= metadata.fileSize) {
							transfer_end = clock();
							
							uint32_t receivedCRC = computeCRC32(fileBuffer, metadata.fileSize);

							if (receivedCRC == metadata.crc) {
								char savePath[512];
								snprintf(savePath, sizeof(savePath), "received_%s", metadata.filename);
								if (saveFile(savePath, fileBuffer, metadata.fileSize) == 0) {
									double duration = (double)(transfer_end - transfer_start) / CLOCKS_PER_SEC;
									double speed = calculateTransferSpeed(transfer_start, transfer_end, metadata.fileSize);
									printf("File received successfully\n");
									printf("Saved as: %s\n", savePath);
									printf("File received in %.2f seconds\n", duration);
									printf("Transfer speed: %.2f Mbps\n", speed);
									printf("CRC verification: PASSED\n");
								}
							}
							if (receivedCRC != metadata.crc) {
								printf("CRC verification failed!\n");
								free(fileBuffer);
								fileBuffer = nullptr;
								currentOffset = 0;
								transferState = receivingMetadata;
								continue;  // Restart loop safely
							}
						}
					}
					break;

				default:
					break;
				}
			}
		}
		// Write the received chunk to the output file.
	   // Ensure that no data is lost or corrupted during the process.

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
		
		net::wait(DeltaTime);
	}
	if (fileBuffer) {
		free(fileBuffer);
	}
	ShutdownSockets();

	return 0;
}
