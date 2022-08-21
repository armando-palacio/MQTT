#include "broker.h"

using namespace std;


int main(){
    // inicializamos el broker
    Broker* Mqtt = new Broker();

    // creamos un hilo para el manejador de clientes
    thread(accept_client_handler, Mqtt).detach();

    // creamos un hilo para el consumer
    thread(consumer, Mqtt).join();

    return 0;
}

// g++ broker_main.cpp broker.cpp proto.cpp -o broker -lpthread