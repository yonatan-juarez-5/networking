#include <iostream>
// client.cpp

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

#include <fstream>
#include <map>
#include <string>
#include <cstring>
#include <vector>

#define TCP_PORT  "35975"
#define BUF_LEN 1024
std::string encrypt(std::string input);
char encryptChar(char c, int position);
bool checkArguments(std::string room, std::string day, std::string time, std::string request);
int main(){
    int sockfd, byteNum, error;
	char buffer[BUF_LEN];
	struct addrinfo serv_info, *servinfo, *p;

    memset(&serv_info, 0, sizeof serv_info);
	serv_info.ai_family = AF_INET;
	serv_info.ai_socktype = SOCK_STREAM;

    if(( error = getaddrinfo("localhost", TCP_PORT, &serv_info, &servinfo) ) != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
		exit(1);
	}

    // Find usable process for client port
	for(p = servinfo; p != NULL; p = p->ai_next){
		if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
			perror("socket failed");
			continue;
		}
		if(connect(sockfd, p->ai_addr, p->ai_addrlen) == -1){
			close(sockfd);
			perror("connection failed");
			continue;
		}
		break;
	}

    if(p == NULL){
		fprintf(stderr, "invalid connection\n");
		exit(1);
	}
	std::cout << "Client is up and running.\n";
	freeaddrinfo(servinfo);

    while(true){
        memset(buffer, 0, BUF_LEN); // Clear buffer at the start of each iteration
        std::string username, password;
        std::string credentials = "";
        std::cout << "Please enter the username: ";
        std::getline(std::cin, username);
        credentials += username;
        credentials += ",";
        std::cout << "Please enter the password (“Enter” to skip for guests): ";
        std::getline(std::cin, password);
        credentials += password;


        const char* msg = encrypt(username).c_str();
        std::string enc_usrname = encrypt(username);
        if (username.empty() && password.empty()){
            std::cout << "----- Starting a new request -----\n";
        }
        else if (password.empty()) {
            //GUEST request
            std::cout << "No password entered, proceeding as a guest.\n";
            if (send(sockfd, msg, strlen(msg), 0) == -1){
                perror("send failed");
            }
            std::cout <<  username << " sent a guest request to the main server using TCP over port " << TCP_PORT << ".\n";
            if((byteNum = recv(sockfd, buffer, BUF_LEN-1, 0)) == -1){
                perror("recv failed");
                exit(1);
            }
            std::cout << "Main server response: " << std::string(buffer) << std::endl;
            std::cout << "Welcome guest " << username << "!\n";
            while(true){
                std::string room, day, time, request;
                std::cout << "Please enter the room number: ";
                std::getline(std::cin, room);
                std::cout << "Please enter the day ('Enter' to skip): ";
                std::getline(std::cin, day);
                std::cout << "Please enter the time ('Enter' to skip): ";
                std::getline(std::cin, time);

                std::cout << "Would you like to search for availability of make a request?\n";
                std::cout << "(Enter 'Availability' to search for the availability or Enter 'Reservation' to make a reservation): ";
                std::getline(std::cin, request);

                if (!checkArguments(room, day, time, request)){ //check that user enters all arguments
                        std::cout << "\n-----Starting a new request-----\n";
                }
                else{
                    std::string combinedReq = room + "," + day + "," + time + ","+ request;
                    if (request == "Availability"){
                        const char *msg = combinedReq.c_str();
                        if (send(sockfd, msg, strlen(msg), 0) == -1){
                            perror("send failed");
                        }
                        std::cout << username << " sent an availability request to the main server.\n";

                        memset(buffer, 0, BUF_LEN);
                        if((byteNum = recv(sockfd, buffer, BUF_LEN-1, 0)) == -1){
                            perror("recv failed");
                            exit(1);
                        }
                        if (strcmp(buffer, "success")== 0){
                            std::cout << "The client received the response from the main server using TCP over port " <<
                                TCP_PORT << ". The requested room is available.\n";
                            std::cout << "\n-----Start a new request-----\n";
                        }
                        else if(strcmp(buffer, "unavail")== 0){
                            std::cout << "The client received the response from the main server using TCP over port " <<
                                TCP_PORT << ". The requested room is not available.\n";
                            std::cout << "\n-----Start a new request-----\n";
                        }
                        else if(strcmp(buffer, "invalid")== 0){
                            std::cout << "The client received the response from the main server using TCP over port " <<
                                TCP_PORT << ". Not able to find room.\n";
                            std::cout << "\n-----Start a new request-----\n";
                        }
                    }
                    else if (request == "Reservation"){
                        const char *msg = combinedReq.c_str();
                        if (send(sockfd, msg, strlen(msg), 0) == -1){
                            perror("send failed");
                        }
                        std::cout << username << " sent a reservation request to the main server.\n";
                        memset(buffer, 0, BUF_LEN);
                        if((byteNum = recv(sockfd, buffer, BUF_LEN-1, 0)) == -1){
                            perror("recv failed");
                            exit(1);
                        }
                        std::cout << "Main server response: " << std::string(buffer) << std::endl;
                        std::cout << "Permission denied: Guest cannot make a reservation.\n\n";
                    }
                    else{
                        std::cout << "invalid request type.\n";
                    }
                }
            }
        }
        else{ //MEMBER request
            credentials = encrypt(username) + "," + encrypt(password);
            std::cout << "Encrypted credentials: " << credentials << std::endl;
            msg = credentials.c_str();
            if (send(sockfd, msg, strlen(msg), 0) == -1){
                perror("send failed");
            }
            std::cout << username << " sent an authentication request to the main server using TCP over port " << TCP_PORT << ".\n";
        
            //After receiving the result of the authentication request from the main server (if passed)
            memset(buffer, 0, BUF_LEN); // clear buffer before receiving data from Server M
            if((byteNum = recv(sockfd, buffer, BUF_LEN-1, 0)) == -1){
                perror("recv failed");
                exit(1);
            }
            // std::cout << "[DEBUG] " << std::string(buffer) << std::endl;
            if (strcmp(buffer, "success") == 0){
                std::cout << "Welcome member " << username << "!\n";
                while(true){

                    std::string room, day, time, request;
                    std::string optionalDay, optionalTime;
                    std::cout << "Please enter the room number: ";
                    std::getline(std::cin, room);
                    std::cout << "Please enter the day ('Enter' to skip): ";
                    std::getline(std::cin, day);
                    std::cout << "Please enter the time ('Enter' to skip): ";
                    std::getline(std::cin, time);

                    // EXTRA CREDIT for missing parameters
                    if (day.empty() && time.empty()){
                        // look for availability for all days/time
                        day="open";
                        time="open";
                        // std::cout << "look for availability for all days/time\n";
                    }
                    else if(!day.empty() && time.empty()){
                        //look for availability for all times in entered day
                        time="open";
                        // std::cout << "look for availability for all times in entered day\n";
                    }

                    std::cout << "Would you like to search for availability of make a request?\n";
                    std::cout << "(Enter 'Availability' to search for the availability or Enter 'Reservation' to make a reservation): ";
                    std::getline(std::cin, request);
                    if (request == "Reservation"){
                        if (day == "open" || day == "open"){
                            //can't make reservation request
                            
                        }
                    }
                    if (!checkArguments(room, day, time, request)){ //check that user enters all arguments
                        std::cout << "\n-----Starting a new request-----\n";
                    }
                    else{
                        std::string combinedReq = room + "," + day + "," + time + ","+ request;
                        // std::cout << combinedReq << std::endl;
                        if (request == "Availability"){
                            const char *msg = combinedReq.c_str();
                            if (send(sockfd, msg, strlen(msg), 0) == -1){
                                perror("send failed");
                            }
                            std::cout << username << " sent an availability request to the main server.\n";

                            memset(buffer, 0, BUF_LEN); // clear buffer before receiving data from Server M
                            if((byteNum = recv(sockfd, buffer, BUF_LEN-1, 0)) == -1){
                                perror("recv failed");
                                exit(1);
                            }
                            std::cout << std::string(buffer) << ": Response from main server.\n";
                            if (strcmp(buffer, "sucess")==0){
                                std::cout << "The client recieved the response from the main server using TCP over port " << TCP_PORT << 
                                    ". The requested room is available.\n";
                            }
                            else if(strcmp(buffer, "unavail") == 0){
                                std::cout << "The client recieved the response from the main server using TCP over port " << TCP_PORT << 
                                    ". The requested room is not available.\n";
                            }
                            else if (strcmp(buffer, "invalid") == 0){
                                std::cout << "The client recieved the response from the main server using TCP over port " << TCP_PORT << 
                                    ". Not able to find the room.\n";
                            }
                        }
                        else if (request == "Reservation"){
                            const char *msg = combinedReq.c_str();
                            if (send(sockfd, msg, strlen(msg), 0) == -1){
                                perror("send failed");
                            }
                            std::cout << username << " sent a reservation request to the main server.\n";
                            memset(buffer, 0, BUF_LEN); // clear buffer before receiving data from Server M
                            if((byteNum = recv(sockfd, buffer, BUF_LEN-1, 0)) == -1){
                                perror("recv failed");
                                exit(1);
                            }
                            std::cout << "Reservation result: " << std::string(buffer) << std::endl;
                        }
                        else{
                            std::cout << "invalid request type.\n";
                        }
                    }
                }
            }
            else if (strcmp(buffer, "invalid") == 0){
                std::cout << "Failed login: Password does not match.\n";
            }
            else if (strcmp(buffer, "username") == 0){
                std::cout << "Failed login: Username does not exist.\n";
            }
        }
    }
    std::cout << "----- End of request of request -----\n";
    
    close(sockfd);

    return 0;
}
std::string encrypt(std::string input){
     std::string encrypted = input;
    for (size_t i = 0; i < input.length(); ++i) {
        encrypted[i] = encryptChar(input[i], i + 1); // i + 1 to make it 1-based index
    }
    return encrypted;
}
char encryptChar(char c, int position) {
    if (isalpha(c)) {
        if (isupper(c)) {
            return 'A' + (c - 'A' + position) % 26;
        } else {
            return 'a' + (c - 'a' + position) % 26;
        }
    } else if (isdigit(c)) {
        return '0' + (c - '0' + position) % 10;
    }
    // Return the character unchanged if it's a special character
    return c;
}
bool checkArguments(std::string room, std::string day, std::string time, std::string request){
    if ((day == "open" || time == "open" ) && (request == "Reservation")){
        std::cout << "Cannot reservation request with missing request arguments (i.e missing day and/or time)\n";
        return false;
    }
    if (room.empty()){
        std::cout << "Missing room number argument.\n";
        return false;
    }
    if (day.empty()){
        std::cout << "Missing day argument.\n";
        return false;
    }
    if (time.empty() ){
        std::cout << "Missing time argument.\n";
        return false;
    }
    // if (request != "Availability"){
    //     std::cout << "Invalid request type.\n";
    //     return false;
    // }
    //  if (request != "Reservation"){
    //     std::cout << "Invalid request type.\n";
    //     return false;
    // }
    if (request.empty()){
        std::cout << "Missing request argument.\n";
        return false;
    }
    return true;
}