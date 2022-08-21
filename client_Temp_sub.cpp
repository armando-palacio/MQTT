#include "client.hpp"
#include "proto.h"
#include <signal.h>
#include <thread>

////////////////////////////////////////////////////////////////////////////////////////////////////
//Este cliente tiene la tarea de subscribirse al topic "Temp" y recibir las publicaciones de 
//  temperatura.  
////////////////////////////////////////////////////////////////////////////////////////////////////

bool DISCONNECTED = 0;

void catch_int(int sig_num){
    DISCONNECTED = 1;
}

void recv_routine(Client* c);

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
    string client_id = "client1";

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

    // envio mensaje de suscripcion a "Temp"
    uint16_t packet_id = 12;
    SUBSCRIBE_Msg msg3(packet_id, (list<string>) {topic});
    if(sendMsg(client->get_sockfd(), msg3) == -1){
        close(client->get_sockfd());
        error("Error al enviar SUBSCRIBE");
    }
    
    // recibo mensaje de suscripcion aceptada
    SUBACK_Msg msg4;
    if(recvMsg(client->get_sockfd(), msg4) == -1){
        close(client->get_sockfd());
        error("Error al recibir SUBACK");
    }
    msg4.checkReturnCode();

    // recibo mensajes de publicacion del topic "Temp"
    thread(recv_routine, client).detach();
    while(!DISCONNECTED);

    // envía un disconnect
    DISCONNECT_Msg msg7;
    if(sendMsg(client->get_sockfd(), msg7) == -1){
        close(client->get_sockfd());
        error("Error al enviar DISCONNECT");
    }
    print("\nDesconectado del broker!");
    
    close(client->get_sockfd());
    return 0;
}


void recv_routine(Client* c){
    PUBLISH_Msg msg5;

    while(!DISCONNECTED){
        if(recvMsg(c->get_sockfd(), msg5) < 1){
            close(c->get_sockfd());
            error("Error al recibir PUBLISH");
        }
        print("Temperatura: " + msg5.getMessage());
    }
}

// g++ client_Temp_sub.cpp proto.cpp -o client_Temp_sub -lpthread