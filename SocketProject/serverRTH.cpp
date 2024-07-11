// serverRTH.cpp
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
#include <utility>
#include <climits>
#include <algorithm>


#define UDP_PORT "32975" // 33000 + xxx
#define M_PORT "34975"
#define BUF_LEN 1024
std::vector<std::string> extractRequest(const char buf[]){
    std::string msg = std::string(buf);
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
std::string checkOpenSchedule (std::map<std::string, std::vector<std::pair<std::string, std::string>>> &schedule, 
    std::string data, std::vector<std::string> arguments);
void readTxt(std::map<std::string, std::vector<std::pair<std::string, std::string>>> &schedule);
std::string checkSchedule (std::map<std::string, std::vector<std::pair<std::string, std::string>>> &schedule, std::string data);
void updateAvailability(std::map<std::string, std::vector<std::pair<std::string, std::string>>> &schedule,
    std::string, std::string, std::string);
void printRoomAvailability(std::map<std::string, std::vector<std::pair<std::string, std::string>>> &schedule){
    for (const auto& entry : schedule) {
        std::cout << "Room: " << entry.first << std::endl;
        for (const auto& schedule_entry : entry.second) {
            std::cout << "\t" << schedule_entry.first << " at " << schedule_entry.second << std::endl;
        }
    }
}
int main(){
    std::map<std::string, std::vector<std::pair<std::string, std::string>>> schedule;
    readTxt(schedule);
    // Output the map to verify the content
    std::cout << "RTH initial room availability:\n";
    for (const auto& entry : schedule) {
        std::cout << "Room: " << entry.first << std::endl;
        for (const auto& schedule_entry : entry.second) {
            std::cout << "\t" << schedule_entry.first << " at " << schedule_entry.second << std::endl;
        }
    }
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
        fprintf(stderr, "listener: failed to bind socket\n"); 
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
    int port = ntohs(sin.sin_port);
    std::cout << "The Server RTH is up and running using UDP on port " << port << ".\n";

    std::string portNum = "R,32975";
    const char* portnum = portNum.c_str();
    if ((numbytes = sendto(sockfdM, portnum, strlen(portnum), 0, pM->ai_addr, pM->ai_addrlen)) == -1) {
        perror("send failed");
        exit(1);
    }
    std::cout << "The Server RTH has informed the main server.\n";

    // receive data in while loop
    while(1){
        addr_len = sizeof their_addr;
		bzero(buf, BUF_LEN);
		if((numbytes = recvfrom(sockfd, buf, BUF_LEN-1, 0,
			(struct sockaddr*)&their_addr, &addr_len)) == -1){
			perror("receive failed");
			exit(1);
		}
        std::string recv_msg = std::string(buf);
        std::vector<std::string> vec = extractRequest(buf);
        // std::cout << recv_msg << std::endl;
        if (vec[1] == "open" && vec[2] == "open"){
            checkOpenSchedule(schedule, recv_msg, vec);
        }
        else if(vec[1] != "open" && vec[2] == "open"){
            checkOpenSchedule(schedule, recv_msg, vec);
        }
        else{
            std::string result = checkSchedule(schedule, recv_msg);
            const char * msg = result.c_str();
            // std::cout << result << " "<< std::string(msg) << std::endl;
        }
        // ERROR in sending avail/res results back to main server
        // if ((numbytes = sendto(sockfdM, msg, strlen(msg), 0, pM->ai_addr, pM->ai_addrlen)) == -1) {
		// 	perror("send failed");
		// 	exit(1);
		// }
		std::cout << "in_progress: The Server RTH finished sending the response to the main server.\n";
    }
    close(sockfd);
    close(sockfdM);
    return 0;
}
void readTxt(std::map<std::string, std::vector<std::pair<std::string, std::string>>> &schedule){
    std::ifstream file("RTH.txt");
    if (!file) {
        std::cerr << "Could not open the file!" << std::endl;
        exit(0);
    }
    // std::map<std::string, std::string> user_password_map;
    std::string line;

    while (std::getline(file, line)) {
        // Find the positions of the commas
        size_t first_comma = line.find(',');
        size_t second_comma = line.find(',', first_comma + 1);

        // Extract room, day, and time
        std::string room = line.substr(0, first_comma);
        std::string day = line.substr(first_comma + 2, second_comma - first_comma - 2);
        std::string time = line.substr(second_comma + 2);

        // Add to the map
        schedule[room].emplace_back(day, time);
    }

    file.close();
}
std::string checkSchedule (std::map<std::string, std::vector<std::pair<std::string, std::string>>> &schedule, std::string data){
    // Create a stringstream object
    std::stringstream ss(data);
    std::string room, day, time, type;
    // Extract the room, time, and type strings
    std::getline(ss, room, ',');
    std::getline(ss, day, ',');
    std::getline(ss, time, ',');
    std::getline(ss, type, ','); 
    room.erase(std::remove(room.begin(), room.end(), ' '), room.end());
    std::string result = "0";
    if (type == "Availability"){ //AVAILABILITY
        std::cout << "The Server RTH received an availability request from the main server.\n";
        // std::cout << data << std::endl;
        if (schedule.find(room) != schedule.end() ){
             for (const auto& entry : schedule[room]){
                // std::cout << entry.first << ", " << entry.second << " | " << day << "," << time << std::endl;
                if (entry.first == day && entry.second  == time){
                    std::cout << "Room " << room << " is available at " << time << " on " << day << ".\n";
                    result = "11";
                    break;
                }
            }
            if (result != "11"){
                std::cout << "Room " << room << " is not available at " << time << " on " << day << ".\n"; 
                result ="00";
            }
        }
        else{ 
            std::cout << "Not able to find the room " << room << ".\n";
            result = "22";
        }
    }
    else{ //RESERVATION
        std::cout << "The Server RTH received a reservation request from the main server.\n";
        if (schedule.find(room) != schedule.end() ){
            for (const auto& entry : schedule[room]){
                std::cout << entry.first << ", " << entry.second << " | " << day << "," << time << std::endl;
                if (entry.first == day && entry.second  == time){
                    std::cout << "Successful reservation. The status of " << room << " is updated.\n";
                    result = "11";
                    break;
                }
            }
            if (result != "11"){
                std::cout << "Cannot make a reservation. Room " << room << " is not available at " << time << " on " << day << ".\n"; 
                result ="00";
            }
        }
        else{
            std::cout << "Cannot make a reservation. Not able to find the room layout.\n";
            result = "22";
        }
        if (result == "11"){
            std::cout << "Updating room availability ...\n";
            updateAvailability(schedule, room, day, time);
            std::cout << "New availability:\n";
            printRoomAvailability(schedule);
            result =  "success";
        }
    }
    return result;
}
std::string checkOpenSchedule (std::map<std::string, std::vector<std::pair<std::string, std::string>>> &schedule, 
    std::string data, std::vector<std::string> arguments){
    std::stringstream ss(data);
    std::string room, day, time, type;
    // Extract the room, time, and type strings
    std::getline(ss, room, ',');
    std::getline(ss, day, ',');
    std::getline(ss, time, ',');
    std::getline(ss, type, ','); 
    room.erase(std::remove(room.begin(), room.end(), ' '), room.end());
    std::cout << data << std::endl;
    std::cout << arguments[1] << "," << arguments[2] << std::endl;
    if (arguments[1] == "open" && arguments[2] == "open"){
        if (arguments[3] == "Availability"){
            if (schedule.find(room) != schedule.end() ){
                std::cout << "All of the availability of the room " << room << " has been extracted.\n";
                for (const auto& entry : schedule[room]){
                    std::cout << "Room " << room << " is available at " << entry.second << " on " << entry.first << ".\n";
                }
            }
            else{
                std::cout << "Not able to find the room " << room << ".\n";
            }
        }
        else{ //RESERVATION

        }
    }
    else if (arguments[1] != "open" && arguments[2] == "open"){
        if (arguments[3] == "Availability"){
            if (schedule.find(room) != schedule.end() ){
                std::cout << "All of the availability of the room " << room << " on " << day << " has been extracted.\n";
                for (const auto& entry : schedule[room]){
                    if (entry.first == day){
                        std::cout << "Room " << room << " is available at " << entry.second << " on " << entry.first << ".\n";
                    }
                }
            }
            else{
                std::cout << "Not able to find the room " << room << ".\n";
            }
        }
        else{ //RESERVATION

        }
    }
    return "";
}
void updateAvailability(std::map<std::string, std::vector<std::pair<std::string, std::string>>> &schedule,
    std::string room, std::string day, std::string time){
      //test removing time
    std::string roomToRemove = room;
    std::string dayToRemove = day;
    std::string timeToRemove = time;
    auto it = schedule.find(roomToRemove);
    if (it != schedule.end()) {
        // Reference to the vector of pairs
        std::vector<std::pair<std::string, std::string>>& pairs = it->second;

        // Find the pair in the vector
        auto pairIt = std::find_if(pairs.begin(), pairs.end(),
                                    [&dayToRemove, &timeToRemove](const std::pair<std::string, std::string>& p) {
                                        return p.first == dayToRemove && p.second == timeToRemove;
                                    });

        // If the pair is found, remove it
        if (pairIt != pairs.end()) {
            pairs.erase(pairIt);
            std::cout << "Removed (" << dayToRemove << ", " << timeToRemove << ") from " << roomToRemove  << " from room availability.\n";
        } else {
            std::cout << "Pair not found in the room schedule." << std::endl;
        }
    }
}
