#include "client.hpp"
#include "proto.h"
#include <signal.h>
#include <thread>

////////////////////////////////////////////////////////////////////////////////////////////////////
//Este cliente tiene la tarea de enviar una lectura de temperatura cada 1 segundo al broker MQTT
//  al topic "Temp".
////////////////////////////////////////////////////////////////////////////////////////////////////

bool DISCONNECTED = 0;

void catch_int(int sig_num){
    DISCONNECTED = 1;
}

void send_routine(Client* c, string topic);

int main(int argc, char *argv[])
{
    if (argc != 3) {
       fprintf(stderr,"usage %s hostname/addr, topic\n", argv[0]);
       exit(0);
    }
    char* hostname = argv[1];
    string topic = argv[2];

    // signals
    signal(SIGINT, catch_int);

    // defino el tiempo de vida del cliente en segundos y el cliente id
    uint16_t keep_alive = 0; 
    string client_id = "client0";

    // creo el cliente y lo conecto
    Client* client = new Client(keep_alive, client_id);
    client->get_server_addr(hostname);
    client->CONNECT();

    // envio el mensaje de conexion
    CONNECT_Msg msg (client->get_keep_alive(), client->get_client_id());
    if(sendMsg(client->get_sockfd(), msg) == -1){
        close(client->get_sockfd());
        error("Error al enviar CONNECT");
    }

    // recibo el mensaje de conexion aceptada
    CONNACK_Msg msg2(0x00);
    if(recvMsg(client->get_sockfd(), msg2) == -1){
        close(client->get_sockfd());
        error("Conección rechazada!");
    }
    print("Conexión aceptada!");

    // envío mensaje de publicacion a "topic"
    thread(send_routine, client, topic).detach();
    while(!DISCONNECTED);

    // envía un disconnect
    DISCONNECT_Msg msg6;
    if(sendMsg(client->get_sockfd(), msg6) == -1){
        close(client->get_sockfd());
        error("Error al enviar DISCONNECT");
    }
    print("\nDesconectado del broker!");
    
    close(client->get_sockfd());
    return 0;
}


void send_routine(Client* c, string topic){
    string message;
    bool retain = 0;

    while(!DISCONNECTED){ 
        message = to_string(15 + rand()%10);  
        PUBLISH_Msg msg5(topic, message, retain);

        if(sendMsg(c->get_sockfd(), msg5) == -1){
            close(c->get_sockfd());
            error("Error al enviar PUBLISH");
        }
        print("sended: " + message);

        sleep(1);
    }
}

// g++ client_Temp_pub.cpp proto.cpp -o client_Temp_pub -lpthread