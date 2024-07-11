#include <iostream>
// serverM.cpp
// main server
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>


#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <cstring>
#include <vector>
#include <sstream>

#include <signal.h>
#include <sys/time.h>

#define UDP_PORT 34975 // 34000 + xxx
#define UDP "34975"
#define TCP_PORT 35975 // 35000 + xxx
#define TCP "35975"

#define SERVER_C_PORT "31975"
#define SERVER_RTH_PORT "32975"
#define SERVER_EEB_PORT "33975"

#define BUF_LEN 1024
int udpSocket, clientSocket;
 sockaddr_in udpServerAddr;
int c_port, eeb_port, rth_port;
std::string username;
bool guest = false;
std::string verifyCredentials(std::string username, std::string password);
std::vector<std::string> extractRequest(const char buf[], int fd);
std::string roomRequest(std::string request, std::string room);
void processRequest(const char buffer[], int clientSocket){
    // std::cout << "[DEBUG] Inside of processRequest()\n";
    std::vector<std::string> vec = extractRequest(buffer, clientSocket);
    std::string requestType = vec[3];
    std::string room = vec[0].substr(0,3); // i.e EEB
    if (!guest){ //MEMBER
        // std::cout << "[DEBUG] Processing request as a member\n";
        if (requestType == "Availability" || requestType == "availability"){ //process availability request
            std::cout << "The main server has received the availability request on Room " << vec[0]
             << " at time " << vec[2] << " on " << vec[1] << " from " << username << " over port " << TCP_PORT << ".\n";
            if (room == "EEB"){
                std::cout << "The main server has sent a request to Server EEB.\n";
                std::string avail_result = roomRequest(buffer, room);
                const char * buff = avail_result.c_str();
                if (send(clientSocket, buff, strlen(buff), 0) == -1 ){
                    perror("send failed");
                }
                std::cout << avail_result << ": The main server sent the availability information to the client.\n";
            }
            else if(room == "RTH"){
                std::cout << "The main server has sent a request to Server RTH.\n";
                std::string avail_result = roomRequest(buffer, room) ; 
                const char * buff = avail_result.c_str();
                if (send(clientSocket, buff, strlen(buff), 0) == -1 ){
                    perror("send failed");
                }
                std::cout << avail_result<< ": The main server sent the availability information to the client.\n";
            }
        }
        else if(requestType == "Reservation" || requestType == "reservation"){ //process reservation request
            std::cout << "The main server has received a reservation request on Room " << vec[0]
             << " at time " << vec[2] << " on " << vec[1] << " from " << username << " over port " << TCP_PORT << ".\n";
            if (vec[0].substr(0, 3) == "EEB"){
                std::cout << "The main server has sent a request to Server EEB.\n";
                std::string avail_result = roomRequest(buffer, room);
                const char * buff = avail_result.c_str();
                if (send(clientSocket, buff, strlen(buff), 0) == -1 ){
                    perror("send failed");
                }
                std::cout << avail_result << ": The main server sent the reservation information to the client.\n";             
            }
            else if (vec[0].substr(0,3) == "RTH"){
                std::cout << "The main server has sent a request to Server RTH.\n";
                std::string avail_result = roomRequest(buffer, room) ; 
                const char * buff = avail_result.c_str();
                if (send(clientSocket, buff, strlen(buff), 0) == -1 ){
                    perror("send failed");
                }
                std::cout << avail_result <<  ": The main server sent the reservation information to the client.\n"; 
            }

        }
    }
    else{ //GUEST
        std::cout << "[DEBUG] Processing request as a guest\n";
        if (requestType == "Reservation" || requestType == "reservation"){ //error on reservation request
            std::cout << "The main server received the reservation request on Room" << vec[0] << " at " <<
                vec[2] << " on " << vec[1] << " from " << username << " using TCP over port " << TCP_PORT << ".\n";
            std::cout << "Permission denied. " << username << " cannot make a reservation.\n";
            std::string rtn = "error";
            const char*  buff = rtn.c_str();
            int res;
            if (send(clientSocket, buff, strlen(buff), 0) == -1 ){
                perror("send failed");
            }
            std::cout << "The main server sent the error message to the client.\n";
        }
        else{ //AVAILABILITY
            std::cout << "The main server has received the availability request on Room " << vec[0]
             << " at time " << vec[2] << " on " << vec[1] << " from " << username << " over port " << TCP_PORT << ".\n";
            std::string room = vec[0].substr(0,3);
            if (room == "EEB"){
                std::cout << "The main server has sent a request to Server EEB.\n";
                std::string avail_result = roomRequest(buffer, room);
                const char * buff = avail_result.c_str();
                if (send(clientSocket, buff, strlen(buff), 0) == -1 ){
                    perror("send failed");
                }
                std::cout << "The main server sent the availability information to the client.\n";
            }
            else if(room == "RTH"){
                std::cout << "The main server has sent a request to Server RTH.\n";
                std::string avail_result = roomRequest(buffer, room) ; 
                const char * buff = avail_result.c_str();
                if (send(clientSocket, buff, strlen(buff), 0) == -1 ){
                    perror("send failed");
                }
                std::cout << "The main server sent the availability information to the client.\n";
            }
        }
    }
}
void response(const char buf[], int clientSocket){
    std::string msg = std::string(buf);
    std::string delim = " ";
    size_t pos = msg.find(delim);
	std::string function = msg.substr(0, pos);
	msg.erase(0, pos+delim.length());
	std::string input = msg;

    size_t commaPosition = input.find(',');
    if (commaPosition != std::string::npos) {//MEMBER
        username = input.substr(0, commaPosition);
        // std::string username = input.substr(0, commaPosition);
        std::string password = input.substr(commaPosition + 1);
	    std::cout << "The main server has received the authentication for " << username << " using TCP over port " << TCP_PORT << ".\n";
        
        // send credentials to serverC for authentication
        std::string rtnValue = verifyCredentials(username, password);
        buf = rtnValue.c_str();
        //Upon sending an authentication response to the client
        if (send(clientSocket, buf, strlen(buf), 0) == -1 ){
            perror("send failed");
        }
        std::cout << "The main server sent the authentication result to the client.\n";
    } 
    else{ //GUEST
        guest = true;
        username = input;
        std::cout << "The main server has received the guest request for " << input << 
                    " using TCP over port " << TCP_PORT << ".\n";
        std::cout << "The main server accepts " << input << " as a guest.\n";
        std::string guest = "guest";
        buf = guest.c_str();
        if (send(clientSocket, buf, strlen(buf), 0) == -1 ){
            perror("send failed");
        }
        std::cout << "The main server sent the guest response to the client.\n";
    }
}
std::string verifyCredentials(std::string username, std::string password){
    // std::cout << "[DEBUG] Inside verifyCredentials()\n";
    int sockfd, sockfdC, byteNum, error;
	char buffer2[BUF_LEN];
	struct addrinfo serv_info, *servinfo, *p;
	struct addrinfo serv_infoC, *servinfoC, *pC;
	struct sockaddr_storage curr_addr;
	socklen_t addr_len;

    // Sets server classification
	memset(&serv_infoC, 0, sizeof serv_infoC);
	serv_infoC.ai_family = AF_INET;
	serv_infoC.ai_socktype = SOCK_DGRAM;

    // Gets server information from main
	if(( error = getaddrinfo("localhost", SERVER_C_PORT, &serv_infoC, &servinfoC) ) != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
		exit(1);
	}
    // Find usable process for client port
	for(pC = servinfoC; pC != NULL; pC = pC->ai_next){
		if((sockfdC = socket(pC->ai_family, pC->ai_socktype, pC->ai_protocol)) == -1){
			perror("socket failed");
			continue;
		}
		break;
	}

    // Check if valid connection
	if((p == NULL) || (pC == NULL)){
		fprintf(stderr, "invalid connection\n");
		exit(1);
	}
    // std::cout << "About to send credentials to Server C\n";
    // Send encrypted credentials to ServerC
	std::string credentials = username + "," + password;
    // std::cout << credentials <<std::endl;
	const char* msg = credentials.c_str();
	if ((byteNum = sendto(sockfdC, msg, strlen(msg), 0, pC->ai_addr, pC->ai_addrlen)) == -1) {
        std::cout << "ERROR\n";
		perror("send failed");
		exit(1);
	}
    std::cout << "The main server forwarded the authentication for " << username << " using UDP over port " << UDP << ".\n";
    std::string msg2 = "";
    addr_len = sizeof curr_addr;
    bzero(buffer2, BUF_LEN);
    if((byteNum = recvfrom(udpSocket, buffer2, BUF_LEN, 0, (struct sockaddr*)&curr_addr, &addr_len)) == -1){
        // std::cout << "HELLO\n";
        perror("receive failed");
        exit(1);
    }
    msg2 = std::string(buffer2);
    // Upon receiving the response from Server C
    std::cout << "The main server has received authentication result for " 
        << username << " using UDP over port " << UDP_PORT << ".\n";

    freeaddrinfo(servinfoC);
    close(sockfdC);
    return buffer2;
}

void handleTCPConnection(int clientSocket){
    // receive info and according sends
    char buffer[BUF_LEN];
    int byteNum;
    while(true)
    {
        bzero(buffer, BUF_LEN);
        if((byteNum = recv(clientSocket, buffer, BUF_LEN-1, 0)) == -1){
            perror("recv failed");
            exit(1);
        }
        if (byteNum == 0){
            std::cout << "[DEBUG] Client has closed the connection.\n";
            break;
        }
        std::string room = std::string(buffer).substr(0,3);
        if (room == "EEB" || room == "RTH"){
            // std::cout << "Avail/reser request.\n";
            std::cout << std::string(buffer) << std::endl;
            processRequest(buffer,clientSocket );
        }
        else{
            response(buffer, clientSocket);
        }
    }
    // close(clientSocket);
}
void* startUDPServer(void* arg) {
    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    // int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in udpServerAddr;
    memset(&udpServerAddr, 0, sizeof(udpServerAddr));
    udpServerAddr.sin_family = AF_INET;
    udpServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    udpServerAddr.sin_port = htons(UDP_PORT);

    if (bind(udpSocket, (struct sockaddr*)&udpServerAddr, sizeof(udpServerAddr)) < 0) {
        perror("UDP Bind failed. Error");
        exit(EXIT_FAILURE);
    }

    char buffer[BUF_LEN];
    sockaddr_in udpClientAddr;
    socklen_t udpClientAddrLen = sizeof(udpClientAddr);
    // std::cout << "The main server is up and running." << std::endl;
    
    for (int i =0; i < 3; i++){
        int received = recvfrom(udpSocket, buffer, BUF_LEN, 0, (struct sockaddr*)&udpClientAddr, &udpClientAddrLen);
        if (received > 0) {
            buffer[received] = '\0';
            std::string srvr = std::string(buffer).substr(2, 6);
            std::string txt = "The main server has received the notification from Server ";
            if (buffer[0] == 'C'){
                std::cout << txt << "C using UDP over port " << srvr << ".\n";
            }
            else if (buffer[0] == 'E'){
                std::cout << txt << "EEB using UDP over port " << srvr << ".\n";
            }
            else if (buffer[0] == 'R'){
                std::cout << txt << "RTH using UDP over port " << srvr << ".\n";
            }
        }
    }
    while (true) {
    }
    // close(udpSocket);
    return NULL;
}

void startTCPServer() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(TCP_PORT);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed. Error");
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, 10) < 0) {
        perror("Listen failed. Error");
        exit(EXIT_FAILURE);
    }
    // std::cout << "The main server is up and running." << std::endl;

    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket < 0) {
            perror("Accept failed");
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) { // This is the child process
            close(serverSocket); // Child doesn't need the listener socket
            handleTCPConnection(clientSocket);
            exit(0);
        } else if (pid > 0) { // Parent process
            close(clientSocket); // Parent doesn't need this specific client socket
        } else {
            perror("Fork failed");
        }
    }
    // close(clientSocket);
}
// Main server to start TCP/UDP servers and handle multi-threading
int main(int argc, char *argv[] ) {
    signal(SIGCHLD, SIG_IGN);
   
    std::cout << "The main server is up and running." << std::endl;
    pthread_t udpThread;
    pthread_create(&udpThread, NULL, startUDPServer, NULL);
    pthread_detach(udpThread);

    startTCPServer();
    return 0;
}
std::string roomRequest(std::string request, std::string room){
    int sockfdEEB, sockfdRTH, byteNum, error;
	char buffer[BUF_LEN];
	struct addrinfo serv_infoEEB, *servinfoEEB, *pEEB;
	struct addrinfo serv_infoRTH, *servinfoRTH, *pRTH;
	struct sockaddr_storage curr_addr;
	socklen_t addr_len;

    // Sets server classification
	memset(&serv_infoEEB, 0, sizeof serv_infoEEB);
	serv_infoEEB.ai_family = AF_INET;
	serv_infoEEB.ai_socktype = SOCK_DGRAM;

    memset(&serv_infoRTH, 0, sizeof serv_infoRTH);
	serv_infoRTH.ai_family = AF_INET;
	serv_infoRTH.ai_socktype = SOCK_DGRAM;

    if(( error = getaddrinfo("localhost", SERVER_EEB_PORT, &serv_infoEEB, &servinfoEEB) ) != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        exit(1);
    }   
    if(( error = getaddrinfo("localhost", SERVER_RTH_PORT, &serv_infoRTH, &servinfoRTH) ) != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        exit(1);
    }
    for(pEEB = servinfoEEB; pEEB != NULL; pEEB = pEEB->ai_next){
		if((sockfdEEB = socket(pEEB->ai_family, pEEB->ai_socktype, pEEB->ai_protocol)) == -1){
			perror("socket failed");
			continue;
		}
		break;
	}
	for(pRTH = servinfoRTH; pRTH != NULL; pRTH = pRTH->ai_next){
		if((sockfdRTH = socket(pRTH->ai_family, pRTH->ai_socktype, pRTH->ai_protocol)) == -1){
			perror("socket failed");
			continue;
		}
		break;
	}
    // Check if valid connection
	if((pEEB == NULL) || (pRTH == NULL)){
		fprintf(stderr, "invalid connection\n");
		exit(1);
	}
    const char* msg = request.c_str();
    std::cout <<"Room: " << room << "\n";
    if (room == "EEB"){
        if ((byteNum = sendto(sockfdEEB, msg, strlen(msg), 0, pEEB->ai_addr, pEEB->ai_addrlen)) == -1) {
            std::cout << "ERROR\n";
            perror("send failed");
            exit(1);
        }
        // bzero(buffer, BUF_LEN);
        // if((byteNum = recvfrom(udpSocket, buffer, BUF_LEN, 0, (struct sockaddr*)&curr_addr, &addr_len)) == -1){
        //     // std::cout << "HELLO\n";
        //     perror("receive failed");
        //     exit(1);
        // }
        std::cout << "in_progress: The main server received the response from Server EEB using UDP over port " << UDP << ".\n";
        // std::cout << "[DEBUG] "<< std::string(buffer) << std::endl;
        // std::cout << std::string(buffer);        
    }
    else if (room == "RTH"){
        if ((byteNum = sendto(sockfdRTH, msg, strlen(msg), 0, pRTH->ai_addr, pRTH->ai_addrlen)) == -1) {
            std::cout << "ERROR\n";
            perror("send failed");
            exit(1);
        }
        // bzero(buffer, BUF_LEN);
        // if((byteNum = recvfrom(udpSocket, buffer, BUF_LEN, 0, (struct sockaddr*)&curr_addr, &addr_len)) == -1){
        //     // std::cout << "HELLO\n";
        //     perror("receive failed");
        //     exit(1);
        // }
        std::cout << "in_progress: The main server received the response from Server RTH using UDP over port " << UDP << ".\n";
        // std::cout << "[DEBUG] "<< std::string(buffer) << std::endl;
        // std::cout << std::string(buffer); 
    }
    freeaddrinfo(servinfoEEB);
    freeaddrinfo(servinfoRTH);
    close(sockfdEEB);
    close(sockfdRTH);
    return "in_progress";

}
std::vector<std::string> extractRequest(const char buf[], int fd){
    std::string msg = std::string(buf);
    // std::string delim = " ";
    // size_t pos = msg.find(delim);
	// std::string function = msg.substr(0, pos);
	// msg.erase(0, pos+delim.length());
	std::string input = msg;

    std::string room, day, time, type;
    std::vector<std::string> res;
    // Create a stringstream object
    std::stringstream ss(input);

    // Extract the room, time, and type strings
    std::getline(ss, room, ',');
    std::getline(ss, day, ',');
    std::getline(ss, time, ',');
    std::getline(ss, type, ',');
    res.push_back(room);
    res.push_back(day);
    res.push_back(time);
    res.push_back(type);
    return res;
}
