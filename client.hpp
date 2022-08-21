#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <string>
#include <stdint.h>
#include <arpa/inet.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>

#define MQTT_PORT 1883 // puerto por defecto para MQTT

using namespace std;

void error(const char *msg){ // Imprime un mensaje de error y termina el programa
    perror(msg);
    exit(0);
}

class Client{  // Clase cliente.
    private:
        int sockfd; 
        struct sockaddr_in serv_addr;
        string client_id;
        uint16_t keep_alive;
    public:
        Client(uint16_t keep_alive, string client_id);
        ~Client();
        int get_sockfd() const; // devuelve el descriptor del socket
        string get_client_id() const; // devuelve el client_id
        uint16_t get_keep_alive() const; // devuelve el keep_alive
        void get_server_addr(char* hostname); // obtiene la dirección del servidor
        void CONNECT(); // realiza una petición de conexión

};

Client::Client(uint16_t keep_alive, string client_id){
    this->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");
    this->client_id = client_id;
    this->keep_alive = keep_alive;
}

int Client::get_sockfd() const{
    return this->sockfd;
}

string Client::get_client_id() const{
    return this->client_id;
}

uint16_t Client::get_keep_alive() const{
    return this->keep_alive;
}

void Client::get_server_addr(char* hostname){
    struct hostent *server = gethostbyname(hostname); 
    if(server == NULL){
        fprintf(stderr,"ERROR, no such host\n"); 
        exit(0);
    }
    bzero((char *) &(this->serv_addr), sizeof(this->serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&(this->serv_addr).sin_addr.s_addr, server->h_length);
    this->serv_addr.sin_port = htons(MQTT_PORT);
}

void Client::CONNECT(){
    if(connect(this->sockfd, (struct sockaddr *) &(this->serv_addr), sizeof(this->serv_addr)) < 0)
        error("ERROR connecting");
}
