#include "client.hpp"
#include "proto.h"


int main(int argc, char *argv[])
{
    if (argc != 3) {
       fprintf(stderr,"usage %s hostname/addr, topic \n", argv[0]);
       exit(0);
    }
    char* hostname = argv[1];
    list<string> topic; topic.push_back(argv[2]);

    // defino el tiempo de vida del cliente en segundos y el cliente id
    uint16_t keep_alive = 5;
    string client_id = "client0";

    // creo el cliente y lo conecto
    Client* client = new Client(keep_alive, client_id);
    client->get_server_addr(hostname);
    client->CONNECT();

    // envio el mensaje de conexion
    CONNECT_Msg msg (client->get_keep_alive(), client->get_client_id());
    if(sendMsg(client->get_sockfd(), msg) == -1)
        error("Error al enviar CONNECT");

    // recibo el mensaje de conexion aceptada
    CONNACK_Msg msg2(0x00);
    if(recvMsg(client->get_sockfd(), msg2) == -1)
        error("Conección rechazada!");
    print("Conexión aceptada!");

    // envio mensaje de suscripcion
    uint16_t packet_id = 12;
    SUBSCRIBE_Msg msg3(packet_id, topic);
    if(sendMsg(client->get_sockfd(), msg3) == -1)
        error("Error al enviar SUBSCRIBE");

    // recibo mensaje de suscripcion aceptada
    SUBACK_Msg msg4;
    if(recvMsg(client->get_sockfd(), msg4) == -1)
        error("Error al recibir SUBACK");
    msg4.checkReturnCode();

    // recibo mensaje de publicacion
    PUBLISH_Msg msg5;
    if(recvMsg(client->get_sockfd(), msg5) == -1)
        error("Error al recibir PUBLISH");
    print(msg5.getMessage());

    // while(1);
    
    // // envio mensaje de desconexion
    // DISCONNECT_Msg msg6;
    // if(sendMsg(client->get_sockfd(), msg6) == -1)
    //     error("Error al enviar DISCONNECT");

    // cierro la conexion
    close(client->get_sockfd());
    return 0;
}

// g++ client0.cpp proto.cpp -o client0
