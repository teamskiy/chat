#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <thread>
#include <vector>
#include <string.h>

using namespace std;

void clientThread(int sock);

int main(int argc, char *argv[]) {
   if(argc != 4) {
      fprintf(stderr, "Wrong number of arguments\n");
      exit(1);
   }

   int sockfd, portno, n;
   struct sockaddr_in serv_addr;
   struct hostent *server;

   string ggg(argv[1]);

   int ps = ggg.find(":");
   string ip = ggg.substr(0, ps);

   portno = stoi(ggg.substr(ps + 1));

   server = gethostbyname(ip.c_str());

   if (server == NULL) {
      fprintf(stderr,"ERROR, no such host\n");
      exit(0);
   }

   bzero((char *) &serv_addr, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
   serv_addr.sin_port = htons(portno);

   sockfd = socket(AF_INET, SOCK_STREAM, 0);

   int tr = 1;
   if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
   }

   if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
      fprintf(stderr, "ERROR on connection\n");
      exit(1);
   }

   string name = argv[3];
   string room = argv[2];

   string hi_msg = "HELO\n";
   hi_msg += name + " " + room;
   send(sockfd, hi_msg.c_str(), hi_msg.size(), 0);
   usleep(1000);

   thread *t = new thread(clientThread, sockfd);

   while(1) {
      string msg;
      getline(cin, msg);
      if(msg.empty()) continue;
      string word;
      if(msg.size() >= 5) word = msg.substr(0, 5);
      else word = "gg";
      string query;
      if(msg[0] == '/') {
         if(word == "/quit") {
            query = "QUIT\n";
         }
         else if(word == "/join") {
            string num = msg.substr(6);
            query = "JOIN " + num;
         }
         else if(word == "/list") {
            query = "LIST\n";
         }
         else {
            printf("No such command\n");
            continue;
         }
         send(sockfd, query.c_str(), query.size(), 0);
         usleep(1000);
      }
      else {
         query = "SEND\n" + msg;
         send(sockfd, query.c_str(), query.size(), 0);
         usleep(1000);
      }
   }

   return 0;
}

void clientThread(int sock) {
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
         exit(1);
      }
      else if(n == 0) {
         printf("Disconnect(, mezhdu nami okeany)\n");
         exit(1);
      }
      else {
         string type = msg.substr(0, 4);
         string content = msg.substr(5);
         printf("%s\n", content.c_str());
         if(type == "QUIT") {
            exit(1);
         }
      }
   }
}
