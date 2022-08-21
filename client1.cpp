#include "client.hpp"
#include "proto.h"


int main(int argc, char *argv[])
{
    if (argc != 3) {
       fprintf(stderr,"usage %s hostname/addr, topic\n", argv[0]);
       exit(0);
    }
    char* hostname = argv[1];
    list<string> topic = {argv[2]};

    // defino el tiempo de vida del cliente en segundos y el cliente id
    uint16_t keep_alive = 60;
    string client_id = "client_1";

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
    uint16_t packet_id = 13;
    SUBSCRIBE_Msg msg3(packet_id, topic);
    if(sendMsg(client->get_sockfd(), msg3) == -1)
        error("Error al enviar SUBSCRIBE");

    // recibo mensaje de suscripcion aceptada
    SUBACK_Msg msg4;
    if(recvMsg(client->get_sockfd(), msg4) == -1)
        error("Error al recibir SUBACK");
    msg4.checkReturnCode();

    // envio mensaje de publicacion
    PUBLISH_Msg msg5(topic.front(), "Hola mundo", 1);
    if(sendMsg(client->get_sockfd(), msg5) == -1)
        error("Error al enviar PUBLISH");
    print("Mensaje enviado!");

    // uint8_t buffer[1024];
    // while(recvMsg(client->get_sockfd(), buffer) != -1);
    
    // envio mensaje de desconexion
    // DISCONNECT_Msg msg6;
    // if(sendMsg(client->get_sockfd(), msg6) == -1)
    //     error("Error al enviar DISCONNECT");
    // print("mensaje disconnect enviado!");

    // cierro la conexion
    close(client->get_sockfd());
    return 0;
}

// g++ client1.cpp proto.cpp -o client1
