//build as x64 with c++14
#define ASIO_STANDALONE

#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include "asio.hpp"
#include "json.hpp"
#include "StringParser.hpp"

struct ServerInfo {
	std::string IP;
	std::string JSON;
};

std::vector<ServerInfo> ServerData;
std::mutex ServerDataGuard;
std::mutex LogGuard;
std::mutex FileGuard;
std::vector<std::string> EndPoints;
std::vector<bool> TestedEndPoints;

void CheckForServer(size_t socketNum, size_t PortionSize, bool CacheThread) {
	asio::io_context SearchContext;
	asio::ip::tcp::socket SearchSocket(SearchContext);

	//test all ips given by thread allocator
	for (size_t i = 0; i < PortionSize; i++) {
		asio::error_code Error;

		//create socket
		asio::ip::tcp::endpoint ServerAddress(asio::ip::make_address(EndPoints.at(PortionSize * socketNum + i)), 25565);
		TestedEndPoints.at(i) = true;

		if (CacheThread) {
			FileGuard.lock();
			std::ofstream IndexCache("SearchIndex.txt", std::ofstream::out | std::ofstream::trunc);
			if (IndexCache.is_open()) {
				IndexCache << std::to_string(i);
			}
			FileGuard.unlock();
		}
		SearchSocket.connect(ServerAddress, Error);

		if (Error) {
			LogGuard.lock();
			std::cout << "Failed to connect to: " << EndPoints.at(PortionSize * socketNum + i) << '\n';
			LogGuard.unlock();
		}
		else {
			std::string IpString = EndPoints.at(PortionSize * socketNum + i);

			uint8_t RequestSize = 1 + 1 + IpString.size() + 2 + 1 + 1;
			size_t PacketSize = RequestSize + 1;
			uint8_t* PacketData = new uint8_t[PacketSize];

			//write data packet for server info request https://wiki.vg/Server_List_Ping
			PacketData[0] = RequestSize;
			PacketData[1] = 0x00;
			PacketData[2] = 0x00;
			PacketData[3] = IpString.size();

			for (int i = 0; i < IpString.size(); i++) {
				PacketData[4 + i] = IpString.at(i);
			}

			PacketData[4 + IpString.size()] = 0xDD;
			PacketData[5 + IpString.size()] = 0x63;

			PacketData[6 + IpString.size()] = 0x01;

			uint8_t StatusRequest[2] = { 1, 0 };

			//send request to ip
			SearchSocket.write_some(asio::buffer(PacketData, PacketSize), Error);
			SearchSocket.write_some(asio::buffer(StatusRequest, 2), Error);

			//get responce if there is one
			std::vector<char> Buffer;
			Buffer.resize(20000);
			SearchSocket.wait(SearchSocket.wait_read);

			for (;;) {
				if (SearchSocket.available()) {
					SearchSocket.read_some(asio::buffer(Buffer), Error);
				}
				else {
					break;
				}
			}

			//parse json info
			std::string JSON = "";

			//responses start with null bytes for some reason so check first five bytes
			for (int a = 5; a < Buffer.size(); a++) {

				//\0 marks the end of a buffer
				if (Buffer.at(a) != '\0') {
					JSON.push_back(Buffer.at(a));
				}
				else {
					SearchSocket.close();

					//do this dumb hack to get rid of server favico data if there is any
					int PlayerLocation = JSON.find("\"version\"");
					if (PlayerLocation >= 0) {

						JSON.insert(PlayerLocation - 1, "}");

						for (int b = PlayerLocation; b < JSON.size(); b++) {
							JSON.at(b) = '\0';
						}

						JSON.shrink_to_fit();
					}

					//if the ip sent a json a responce
					if (JSON.size() > 0) {
						LogGuard.lock();
						std::cout << "Server found!" << '\n';
						LogGuard.unlock();

						//create new server data entry
						ServerDataGuard.lock();
						ServerInfo NewServerInfo;
						NewServerInfo.IP = IpString;
						NewServerInfo.JSON = JSON;
						ServerData.push_back(NewServerInfo);
						ServerDataGuard.unlock();

						//append data to output file
						std::ofstream DataBaseStream("FoundServers.txt", std::ios_base::app);

						if (DataBaseStream.is_open()) {
							DataBaseStream << IpString << '\n';
							DataBaseStream << JSON << '\n';
							DataBaseStream.close();
						}
					}
					else {
						LogGuard.lock();
						std::cout << "false connection" << '\n';
						LogGuard.unlock();
					}
					break;
				}
			}
		}
	}

	return;
}

void HttpServer(const std::vector<ServerInfo>* JSONData) {
	//bind http server socket to port 80
	asio::io_context ServerContext;
	asio::ip::tcp::acceptor ServerAcceptor(ServerContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 80));
	asio::ip::tcp::socket ClientSocket(ServerContext);

	for (;;) {
		try {
			ServerAcceptor.accept(ClientSocket);

			std::vector<char> ReadBuffer;
			ReadBuffer.resize(2000);

			asio::error_code Error;
			ClientSocket.wait(ClientSocket.wait_read);

			//wait for a request
			for (;;) {
				if (ClientSocket.available()) {
					ClientSocket.read_some(asio::buffer(ReadBuffer, ReadBuffer.size()), Error);
				}
				else {
					break;
				}
			}

			std::string ReadString = ReadBuffer.data();
			StringParser RequestParser(&ReadString);
			RequestParser.AddIgnoreChar('\r');
			RequestParser.IgnoreCharsEnabled = true;
			RequestParser.SeperateString('\n', false);

			std::vector<std::string> ProcessedRequest;

			for (int i = 0; i < RequestParser.ChunkedString->size(); i++) {
				StringParser LineParser(&RequestParser.ChunkedString->at(i));
				LineParser.SeperateString(' ', false);
				for (int a = 0; a < LineParser.ChunkedString->size(); a++) {
					ProcessedRequest.push_back(LineParser.ChunkedString->at(a));
				}
			}

			//handle request
			if (ProcessedRequest.at(0) == "GET") {
				//send JSONData table through html
				std::string Response =
					"HTTP/1.1 200 OK\n"
					"Accept-Ranges: bytes\n"
					"Content-Length: ";

				char* HtmlContent;
				//open template html page
				std::ifstream FileStream("List.html", std::ifstream::binary);

				if (FileStream.is_open()) {
					int FileLength = 0;
					FileStream.seekg(0, FileStream.end);
					FileLength = FileStream.tellg();
					FileStream.seekg(0, FileStream.beg);

					HtmlContent = new char[FileLength + 1];
					FileStream.read(HtmlContent, FileLength);
					HtmlContent[FileLength] = '\0';
					FileStream.close();

					std::string HtmlString = HtmlContent;
					//Response += HtmlContent;

					//generate html objects to be inserted into response 
					//instert JSON list data at the end of </tr>
					size_t EndOfFirstTableElement = HtmlString.find("</tr>");

					//prase json file

					size_t HtmlStringWriteIndex = EndOfFirstTableElement + 6;

					//generate html page
					ServerDataGuard.lock();
					for (int i = 0; i < JSONData->size(); i++) {
						try {
							nlohmann::json Json = nlohmann::json::parse(JSONData->at(i).JSON);
							std::string ServerText = Json["description"]["text"];
							int PlayersOnline = Json["players"]["online"];
							int PlayersMax = Json["players"]["max"];

							HtmlString.insert(HtmlStringWriteIndex, "<tr>\n");
							HtmlStringWriteIndex += 5;

							//insert json data
							HtmlString.insert(HtmlStringWriteIndex, "<th>" + JSONData->at(i).IP + "</th>\n");
							HtmlStringWriteIndex += 4 + 6 + JSONData->at(i).IP.size();
							HtmlString.insert(HtmlStringWriteIndex, "<th>" + ServerText + "</th>\n");
							HtmlStringWriteIndex += 4 + 6 + ServerText.size();
							HtmlString.insert(HtmlStringWriteIndex, "<th>" + std::to_string(PlayersOnline) + " / " + std::to_string(PlayersMax) + "</th>\n");
							HtmlStringWriteIndex += 4 + std::to_string(PlayersOnline).size() + 3 + std::to_string(PlayersMax).size() + 6;
							HtmlString.insert(HtmlStringWriteIndex, "</tr>");
							HtmlStringWriteIndex += 5;
						}
						catch (std::exception e) {
							//insert Error data
							HtmlString.insert(HtmlStringWriteIndex, "<th>" + JSONData->at(i).IP + "</th>\n");
							HtmlStringWriteIndex += 4 + 6 + JSONData->at(i).IP.size();
							HtmlString.insert(HtmlStringWriteIndex, "<th>Error</th>\n");
							HtmlStringWriteIndex += 4 + 6 + 5;
							HtmlString.insert(HtmlStringWriteIndex, "<th>Error</th>\n");
							HtmlStringWriteIndex += 4 + 5 + 6;
							HtmlString.insert(HtmlStringWriteIndex, "</tr>");
							HtmlStringWriteIndex += 5;
						}

					}
					ServerDataGuard.unlock();

					//create response packet
					Response += std::to_string(HtmlString.size());
					Response += '\n';
					Response += "Content-Type: text/html\n\n\n";
					Response += HtmlString;

					//send response
					ClientSocket.send(asio::buffer(Response.c_str(), Response.size()));
					ClientSocket.close();

					delete[] HtmlContent;
				}
			}
		}
		catch (std::exception e) {

		}

	}

	return;
}

//arguments: ./ProgramName [number of sockets] [ip range file]
int main(int argc, char** argv) {
	if (argc != 3) {
		std::cout << "Usage: ./Program [number of sockets] [ip range file name]" << '\n';
		return 1;
	}

	if (std::stoi(argv[1]) <= 0) {
		std::cout << "Socket count must be greater than zero" << '\n';
		return 1;
	}

	//open ip search file
	std::ifstream IpRangeFileStream(argv[2], std::ifstream::binary);
	if (!IpRangeFileStream.is_open()) {
		std::cout << "Could not open file: " << argv[2] << '\n';
	}

	//start http server on port 80
	std::thread HttpServer_(HttpServer, &ServerData);

	//configure search sockets
	size_t NumberOfSockets = 1;
	try {
		NumberOfSockets = std::stoi(argv[1]) + 1;
	}
	catch (std::exception e) {
		std::cout << "Error: invalid socket number" << '\n';
		return 0;
	}

	char* IpRanges;
	//load ip ranges from range file in format 0.0.0.0-255.255.255.255
	int IpRangesLength = 0;
	IpRangeFileStream.seekg(0, IpRangeFileStream.end);
	IpRangesLength = IpRangeFileStream.tellg();
	IpRangeFileStream.seekg(0, IpRangeFileStream.beg);

	IpRanges = new char[IpRangesLength + 1];
	IpRanges[IpRangesLength] = '\0';

	IpRangeFileStream.read(IpRanges, IpRangesLength);
	std::string IpRangeString = IpRanges;
	delete[] IpRanges;
	StringParser IpRangeParser(&IpRangeString);
	IpRangeParser.SeperateString('\n', false);

	std::cout << "creating endpoints...." << '\n';
	//create endpoints and load them to memory
	//yes i know i should not do this but making the program run faster is a good exscuse

	//parse range file and create endpoints
	for (int i = 0; i < IpRangeParser.ChunkedString->size(); i++) {
		StringParser RangeParser(&IpRangeParser.ChunkedString->at(i));
		RangeParser.AddIgnoreChar('\r');
		RangeParser.IgnoreCharsEnabled = true;
		RangeParser.SeperateString('-', false);

		StringParser BegIpParser(&RangeParser.ChunkedString->at(0));
		BegIpParser.SeperateString('.', false);
		StringParser EndIpParser(&RangeParser.ChunkedString->at(1));
		EndIpParser.SeperateString('.', false);

		std::string PrevIp = RangeParser.ChunkedString->at(0);
		while (PrevIp != RangeParser.ChunkedString->at(1)) {
			std::string NewIp = "";
			EndPoints.push_back(PrevIp);

			StringParser IpParser(&PrevIp);
			IpParser.SeperateString('.', false);

			int IpNumCodes[4];
			IpNumCodes[0] = std::stoi(IpParser.ChunkedString->at(0));
			IpNumCodes[1] = std::stoi(IpParser.ChunkedString->at(1));
			IpNumCodes[2] = std::stoi(IpParser.ChunkedString->at(2));
			IpNumCodes[3] = std::stoi(IpParser.ChunkedString->at(3));

			if (IpNumCodes[3] == 255) {
				if (IpNumCodes[2] == 255) {
					if (IpNumCodes[1] == 255) {
						if (IpNumCodes[0] == 255) {
							std::cout << "Error Ip Limit Reached!!!" << '\n';
							return 2;
						}
						else {
							IpNumCodes[0]++;
						}

						IpNumCodes[1] = 0;
					}
					else {
						IpNumCodes[1]++;
					}

					IpNumCodes[2] = 0;
				}
				else {
					IpNumCodes[2]++;
				}

				IpNumCodes[3] = 0;
			}
			else {
				IpNumCodes[3]++;
			}

			std::string IpNumCodesS[4];
			IpNumCodesS[0] = std::to_string(IpNumCodes[0]);
			IpNumCodesS[1] = std::to_string(IpNumCodes[1]);
			IpNumCodesS[2] = std::to_string(IpNumCodes[2]);
			IpNumCodesS[3] = std::to_string(IpNumCodes[3]);

			PrevIp = IpNumCodesS[0] + '.' + IpNumCodesS[1] + '.' + IpNumCodesS[2] + '.' + IpNumCodesS[3];
		}
		EndPoints.push_back(PrevIp);


	}
	std::cout << "Done!" << std::endl;
	TestedEndPoints.resize(EndPoints.size());
	for (int i = 0; i < TestedEndPoints.size(); i++) {
		TestedEndPoints.at(i) = false;
	}

	//create allocate, run search sockets
	size_t Remander = 0;
	size_t SocketPortion = 0;

	//devide endpoints amoung threads
	SocketPortion = EndPoints.size() / NumberOfSockets - 1;
	//the remainder gets put on an extra thread
	Remander = EndPoints.size() % NumberOfSockets - 1;

	std::vector<std::thread> SearchThreads;

	//start threads
	if (SocketPortion > 0) {
		for (size_t i = 0; i < NumberOfSockets - 1; i++) {
			if (i == 0) {
				//set the first thread to log it's current progress in testing it's list of endpoints
				SearchThreads.push_back(std::thread(CheckForServer, i, SocketPortion, true));
			}
			else {
				SearchThreads.push_back(std::thread(CheckForServer, i, SocketPortion, false));
			}
		}
	}
	else {
		std::cout << "Error: Too many threads" << '\n';
		return 0;
	}

	std::thread RemanderThread;
	//check for underflow of a remainder of zero minus one
	if (Remander != (size_t)0 - 1) {
		RemanderThread = std::thread(CheckForServer, NumberOfSockets - 1, Remander, false);
	}

	//wait for threads to finish their work
	for (int i = 0; i < NumberOfSockets - 1; i++) {
		SearchThreads.at(i).join();
	}

	if (RemanderThread.joinable()) {
		RemanderThread.join();
	}
	std::cout << "Done searching" << '\n';

	//remember that the http server is still running
	return 0;
}