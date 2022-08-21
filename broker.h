#include <thread>
#include <map> 
#include <unordered_set>
#include <time.h>
#include "proto.h"
#include "my_queue.hpp"

using namespace std;

#define MQTT_PORT 1883 // puerto por defecto para MQTT
#define MAX_CLIENTS 10 // número máximo de clientes aceptados por el broker

struct Client{ // estructura cliente. Almacena los datos de cada cliente que se conecta.
    int sockfd; 
    list<string> topics;
    string client_id;
    string will_topic;
    string will_message;
    bool will_retain;
};

bool inline find_topic(const list<string> &topics, const string &topic){  // verifica si un topic ya existe en la lista de topics
    for (auto it = topics.begin(); it != topics.end(); ++it)
        if (*it == topic)
            return true;
    return false;
}

class Broker{
    private:
        //conexión TCP
        int sockfd;  // descriptor del socket
        struct sockaddr_in server_addr; // dirección del socket

        //operación del broker
        list<Client*>  list_active_clients;  // lista de clientes activos
        Queue<PUBLISH_Msg> queue_msg;  // cola de mensajes publicados
        map<string, unordered_set<Client*>> subscriptions;  //topic, lista de clientes suscritos
        map<string, PUBLISH_Msg*> retain_msg;   //topic, mensaje a retener. Solo se retiene un mensaje por topic.

        //control de acceso
        mutex mutex_list_active_clients; // mutex para lista de clientes activos
        mutex mutex_subscriptions; // mutex para mapa de suscripciones
        mutex mutex_retain_msg; // mutex para mapa de mensajes a retener

        bool running;
    
    public:
        Broker();
        ~Broker();

        int get_server_socket() const;  // devuelve el socket del servidor
        void add_client_to_server(Client* c);  // Si el client_ID ya existe, no se agrega el cliente y se cierra el socket.
        void remove_client_from_server(Client* c);  // elimina el cliente de la lista de clientes activos.
        int get_number_of_clients() const;  // devuelve el número de clientes activos

        void add_client_to_topics(const list<string> &topics, Client* c);  // agrega el cliente a la lista de suscripciones de cada topic
        void remove_client_from_topics(list<string> &topics, Client* c);   // elimina el cliente de la lista de suscripciones de cada topic

        void get_clients_by_topic(const string& topic, list<Client*> &clients);  //devuelve la lista de clientes suscritos a un topic
        bool check_client_ID(const string &client_ID);  // verifica si ya existe un cliente con este mismo ID. Si existe lo elimina y retorna True sino False.

        void new_publish(PUBLISH_Msg* msg);  // agrega el mensaje a la cola de mensajes.
        void publish_will_msg(Client* c);  // publica el mensaje de will del cliente.

        PUBLISH_Msg get_publish();  // obtiene el mensaje de la cola de mensajes.

        PUBLISH_Msg get_retain_msg_by_topic(const string &topic);  // obtiene el mensaje de retención por topic.

        bool is_running() const;  // indica si el broker está corriendo.
        void start_broker();  // inicia el broker.
        void stop_broker();  // pausa el broker.

        void show_suscriptions();  // muestra los clientes suscritos a cada topic.
        void show_retain_msg();  // muestra los mensajes de retención.
};


void client_handler(Broker* br, Client* c);  // manejador de clientes.
void accept_client_handler(Broker* br);  // manejador de clientes.
void consumer(Broker* br);  // consumidor de mensajes.




