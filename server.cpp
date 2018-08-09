#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <thread>
#include <mutex>
#include <vector>
#include <map>
#include <set>

#include <string.h>

using namespace std;

mutex locker;
set<string> clients;
map<int,string> fromSockToClient;
map<string,int> fromClientToSock;
map<string,string> fromNameToRoom;
map<string,set<string>> room_users;

void serverThread(int sock);
vector<string> split(string msg);
vector<string> splitcomma(string msg);
string findText(string msg);

int main( int argc, char *argv[] ) {
   int sockfd, newsockfd, portno;
   socklen_t clilen;
   struct sockaddr_in serv_addr, cli_addr;
   int n;

   /* Initialize socket structure */
   bzero((char *) &serv_addr, sizeof(serv_addr));
   portno = 5000;

   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(portno);

   clilen = sizeof(cli_addr);

   sockfd = socket(AF_INET, SOCK_STREAM, 0);

   if(sockfd < 0) {
      perror("ERROR opening socket");
      exit(1);
   }

   int tr = 1;
   if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
   }

   if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      perror("ERROR on binding");
      exit(1);
   }

   listen(sockfd, 5);

   while (1) {
      newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

      if(newsockfd < 0) {
         perror("ERROR on accept");
         exit(1);
      }

      thread* t = new thread(serverThread, newsockfd);
   }

   return 0;
}

void serverThread(int sock) {
   char buffer[256];
   int n;
   while(1) {
      bzero(buffer, 256);
      n = recv(sock, buffer, 256, 0);
      string msg = "";
      int i = 0;
      while(i < 256 && buffer[i]) {
         msg += buffer[i];
         i++;
      }
      if(n < 0) {
         printf("recv() error\n");
         return;
      }
      else if(n == 0) {
         string name = fromSockToClient[sock];
         string room = fromNameToRoom[name];
         room_users[room].erase(name);
         clients.erase(name);
         fromNameToRoom.erase(name);
         fromSockToClient.erase(sock);
         fromClientToSock.erase(name);
         printf("%s is disconnected from room #%s\n", name.c_str(), room.c_str());
         return;
      }
      else {
         string type = msg.substr(0, 4);
         string content = msg.substr(5);
         string response = "";
         vector<string> word = split(content);
         if(type == "HELO") {
            locker.lock();
            string name = word[0];
            string room = word[1];
            string new_name = name;
            int cnt = 2;
            while(fromNameToRoom.count(new_name)) {
               new_name = name + "-" + to_string(cnt);
               cnt++;
            }
            name = new_name;
            if(!room_users.count(room)) {
               set<string> new_set;
               new_set.insert(name);
               room_users[room] = new_set;
            }
            else {
               room_users[room].insert(name);
            }
            fromSockToClient[sock] = name;
            fromNameToRoom[name] = room;
            fromClientToSock[name] = sock;
            clients.insert(name);
            for(auto user : room_users[room]) {
               response = "MSGE\n";
               if(user == name) {
                  response += "Hello " + name + "! This is room #" + room;
               }
               else {
                  response += name + " joined room #" + room;
               }
               int sk = fromClientToSock[user];
               send(sk, response.c_str(), response.size(), 0);
               usleep(1000);
            }
            locker.unlock();
         }
         else if(type == "LIST") {
            locker.lock();
            int i = 1;
            string room = fromNameToRoom[fromSockToClient[sock]];
            response = "MSGE\n";
            response += "This is list of users in room #" + room + "\n";
            for(auto user : room_users[room]) {
               response += to_string(i) + ". " + user + "\n";
               i++;
            }
            send(sock, response.c_str(), response.size(), 0);
            usleep(1000);
            locker.unlock();
         }
         else if(type == "JOIN") {
            locker.lock();
            string name = fromSockToClient[sock];
            string room = word[0];
            string old_room = fromNameToRoom[name];
            room_users[old_room].erase(name);
            for(auto user : room_users[old_room]) {
               response = "MSGE\n";
               response += name + " is disconnected from the room #" + room;
               int sk = fromClientToSock[user];
               send(sk, response.c_str(), response.size(), 0);
               usleep(1000);
            }
            if(!room_users.count(room)) {
               set<string> new_set;
               new_set.insert(name);
               room_users[room] = new_set;
            }
            else {
               room_users[room].insert(name);
            }
            fromNameToRoom[name] = room;
            for(auto user : room_users[room]) {
               response = "MSGE\n";
               if(user == name) {
                  response += "Hello " + name + "! This is room #" + room;
               }
               else {
                  response += name + " joined room " + room;
               }
               int sk = fromClientToSock[user];
               send(sk, response.c_str(), response.size(), 0);
               usleep(1000);
            }
            locker.unlock();
         }
         else if(type == "SEND") {
            locker.lock();
            string name = fromSockToClient[sock];
            string room = fromNameToRoom[name];
            vector<string> receivers = splitcomma(content);
            string txt = findText(content);
            if(receivers[0] == "All") {
               for(auto user : room_users[room]) {
                  if(user != name) {
                     int sk = fromClientToSock[user];
                     response = "MSGE\n";
                     response += name + " : " + txt;
                     send(sk, response.c_str(), response.size(), 0);
                     usleep(1000);
                  }
               }
            }
            else {
               vector<string> unfound;
               for(auto user : receivers) {
                  if(!room_users[room].count(user)) {
                     unfound.push_back(user);
                  }
                  else {
                     int sk = fromClientToSock[user];
                     response = "MSGE\n";
                     response += name + " : " + txt;
                     send(sk, response.c_str(), response.size(), 0);
                     usleep(1000);
                  }
               }
               if(unfound.size() > 0) {
                  response = "MSGE\n";
                  response += "There is no such user : ";
                  for(auto user : unfound) {
                     response += user + ", ";
                  }
                  response.pop_back();
                  response.pop_back();
                  send(sock, response.c_str(), response.size(), 0);
                  usleep(1000);
               }
            }
            locker.unlock();
         }
         else if(type == "QUIT") {
            locker.lock();
            string name = fromSockToClient[sock];
            string room = fromNameToRoom[name];
            for(auto user : room_users[room]) {
               if(user == name) {
                  response = "QUIT\n";
                  response += "Good bye " + name;
               }
               else {
                  response = "MSGE\n";
                  response += name + " is disconnected from the room #" + room;
               }
               int sk = fromClientToSock[user];
               send(sk, response.c_str(), response.size(), 0);
               usleep(1000);
            }
            room_users[room].erase(name);
            locker.unlock();
         }
      }
   }
}

vector<string> split(string msg) {
    vector<string> ans;
    int p = 0;
    while(p < msg.size()) {
        string word = "";
        while(p < msg.size() && msg[p] == ' ') {
            p++;
        }
        if(p == msg.size()) break;
        while(p < msg.size() && msg[p] != ' ') {
            word += msg[p];
            p++;
        }
        ans.push_back(word);
    }
    return ans;
}

vector<string> splitcomma(string msg) {
    vector<string> ans;
    int p = 0;
    while(p < msg.size()) {
        string word = "";
        while(p < msg.size() && (msg[p] == ' ' || msg[p] == ',')) {
            p++;
        }
        if(p == msg.size() || msg[p] == ':') break;
        while(p < msg.size() && msg[p] != ' ' && msg[p] != ',') {
            word += msg[p];
            p++;
        }
        ans.push_back(word);
    }
    return ans;
}

string findText(string msg) {
   int pos = msg.find(":");
   return msg.substr(pos + 2);
}
