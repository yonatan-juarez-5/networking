// serverC
// read in member.txt file
// ID: 9778 5289 75
#include <iostream>
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
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>
#include <unordered_map>
#include <climits>

#define UDP_PORT "31975"
#define M_PORT "34975"
#define BUF_LEN 1024
#define LOCAL_HOST "127.0.0.1"

std::map<std::string, std::string> userInfo;
std::map<std::string, std::string> unencrypted_user_info;
void readTxt(std::map<std::string, std::string> &user_password_map, std::string filename);
std::string checkCredentials(std::string);
int main(){
    
    // std::map<std::string, std::string> userInfo;
    // std::map<std::string, std::string> unencrypted_user_info;
    readTxt(userInfo, "member.txt");
    readTxt(unencrypted_user_info, "unencrypted_member.txt");
    // Output the map to verify the content
    // std::cout << "Encrypted user info:\n";
    // for (const auto& pair : userInfo) {
    //     std::cout << "Username: " << pair.first << ", Password: " << pair.second << std::endl;
    // }
    // std::cout << "Unencrypted user info:\n";
    // for (const auto& pair : unencrypted_user_info) {
    //     std::cout << "Username: " << pair.first << ", Password: " << pair.second << std::endl;
    // }
    
    int sockfd, sockfdM;
    struct addrinfo hints, *servinfo, *p; 
    struct addrinfo hintsM, *servinfoM, *pM;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr; 
    char buf[BUF_LEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to use IPv4 hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    hints.ai_socktype = SOCK_DGRAM;

    memset(&hintsM, 0, sizeof hintsM);
    hintsM.ai_family = AF_INET; // set to AF_INET to use IPv4 hints.ai_socktype = SOCK_DGRAM;
    hintsM.ai_flags = AI_PASSIVE; // use my IP
    hintsM.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(NULL, UDP_PORT, &hints, &servinfo)) != 0) { 
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv)); 
        return 1;
    }
    if ((rv = getaddrinfo(NULL, M_PORT, &hintsM, &servinfoM)) != 0) { 
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv)); 
        return 1;
    }
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) { 
                perror("listener: socket"); 
                continue;
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) { 
            close(sockfd);
            perror("listener: bind");
            continue; 
        }
        break; 
    }
    for(pM = servinfoM; pM != NULL; pM = pM->ai_next) {
        if ((sockfdM = socket(pM->ai_family, pM->ai_socktype,pM->ai_protocol)) == -1) { 
                perror("socket failed"); 
                continue;
        }
        break; 
    }
    if(p==NULL || pM == NULL){
        fprintf(stderr, "FAILED\n"); 
        return 2;
    }
    freeaddrinfo(servinfo);
    freeaddrinfo(servinfoM);

    // Extract the port number
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(sockfd, (struct sockaddr *)&sin, &len) == -1) {
        perror("getsockname");
        return 1;
    }
    // For IPv6
    int port = ntohs(sin.sin_port); 
    std::cout << "The Server C is up and running using UDP on port " << port << ".\n";

    std::string portNum = "C,31975";
    const char* portnum = portNum.c_str();
    if ((numbytes = sendto(sockfdM, portnum, strlen(portnum), 0, pM->ai_addr, pM->ai_addrlen)) == -1) {
        perror("send failed");
        exit(1);
    }
    std::cout << "The Server C has informed the main server.\n";

    // receive data in while loop
    while(1){
        // Blocks until datagram received
		addr_len = sizeof their_addr;
		bzero(buf, BUF_LEN);
		if((numbytes = recvfrom(sockfd, buf, BUF_LEN-1, 0,
			(struct sockaddr*)&their_addr, &addr_len)) == -1){
			perror("receive failed");
			exit(1);
		}
        std::string recv_msg = std::string(buf);
        std::cout << "The Server C received an authentication request from the main server.\n";
        std::cout << recv_msg << std::endl;

        //check credentials
        std::string authentication_result = checkCredentials(recv_msg);
        //send authentication result back to main server
        const char * msg = authentication_result.c_str();
         if (pM != NULL) {
            printf("[DEBUG] ai_addr: %p\n", (void*)pM->ai_addr);
            printf("[DEBUG] ai_addrlen: %d\n", pM->ai_addrlen);
        } else {
            printf("pM is NULL\n");
        }
        if ((numbytes = sendto(sockfdM, msg, strlen(msg), 0, pM->ai_addr, pM->ai_addrlen)) == -1) {
			perror("send failed");
			exit(1);
		}
		
		std::cout << "The Server C finished sending the response to the main server.\n";
    }
    
    close(sockfd);
    close(sockfdM);
        
    return 0;

}
std::string checkCredentials(std::string msg){
    size_t commaPosition = msg.find(',');
    if (commaPosition != std::string::npos) {
        std::string username = msg.substr(0, commaPosition);
        std::string password = msg.substr(commaPosition + 1);
        
        if (userInfo.find(username) != userInfo.end()){
           const std::string& storedPassword = userInfo[username];
           std::cout << storedPassword << std::endl;
            if (storedPassword == password){
                std::cout << "Successful authentication.\n";
                return "success";
            }
            else{
                std::cout << "Password does not match.\n";
                return "invalid";
            }
        }
        else{
            std::cout << "Username does not exist.\n";
            return "username";
        }
    }
    return "";
    
}
void readTxt(std::map<std::string, std::string> &user_password_map, std::string filename){
    std::ifstream infile(filename);
    if (!infile) {
        std::cerr << "Could not open the file!" << std::endl;
        exit(0);
    }
    // std::map<std::string, std::string> user_password_map;
    std::string line;

    while (std::getline(infile, line)) {
        size_t comma_pos = line.find(',');
        if (comma_pos != std::string::npos) {
            std::string username = line.substr(0, comma_pos);
            std::string password = line.substr(comma_pos + 2);
            user_password_map[username] = password;
        }
    }
    infile.close();
}