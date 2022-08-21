#include "broker.h"

Broker::Broker(){
    this->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(this->sockfd <0){
        perror("Error opening serverSocket!");
        exit(0);
    }
    bzero((char*)&(this->server_addr), sizeof(this->server_addr));
    int val = 1; 
    setsockopt(this->sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    this->server_addr.sin_family = AF_INET;
    this->server_addr.sin_addr.s_addr = INADDR_ANY;
    this->server_addr.sin_port = htons(MQTT_PORT);

    if(bind(this->sockfd, (struct sockaddr *) &(this->server_addr), sizeof(this->server_addr) ) < 0){
        perror("Error en binding!");
        exit(0);
    }

    if(listen(this->sockfd, MAX_CLIENTS) <0){
        perror("Error en listening!");
        exit(0);
    }
    else{
        this->start_broker();
        cout<<"Broker listening!\n";
    }
}

Broker::~Broker(){
    close(this->sockfd);
}

int Broker::get_server_socket() const{
    return this->sockfd;
}

void Broker::add_client_to_server(Client * c){
    unique_lock<mutex> lock_list(this->mutex_list_active_clients);
    this->list_active_clients.push_back(c);
    lock_list.unlock();
}

int Broker::get_number_of_clients() const{
    return (this->list_active_clients).size();
}

void Broker::add_client_to_topics(const list<string> &topics, Client* c){
    map<string, unordered_set<Client*>>::iterator it, it_end;

    unique_lock<mutex> lock_sub(this->mutex_subscriptions);

    for(auto topic : topics){
        it = this->subscriptions.find(topic);
        it_end = this->subscriptions.end();

        if(!find_topic(c->topics, topic))
            c->topics.push_back(topic);

        if(it == it_end){
            this->subscriptions[topic].insert(c);
        }
        else{
            (this->subscriptions.find(topic))->second.insert(c);
        }
    }
    lock_sub.unlock();
}

void Broker::remove_client_from_server(Client* c){
    unique_lock<mutex> lock_list(this->mutex_list_active_clients);
    this->list_active_clients.remove(c);
    lock_list.unlock();

    if (c->topics.size() > 0)
        this->remove_client_from_topics(c->topics, c);

    close(c->sockfd);
    delete c;
}

void Broker::remove_client_from_topics(list<string> &topics, Client* c){
    map<string, unordered_set<Client*>>::iterator it, it_end;

    unique_lock<mutex> lock_sub(this->mutex_subscriptions);

    for(auto topic : topics){
        it = this->subscriptions.find(topic);
        it_end = this->subscriptions.end();

        if(it != it_end){
            (this->subscriptions.find(topic))->second.erase(c);
            cout<<c->sockfd<<": Cliente \""<<c->client_id<<"\" eliminado de topic \""<<topic<<"\"\n";

            if(it->second.size() == 0){
                this->subscriptions.erase(it);
                cout<<"\n**Topic \""<<topic<<"\" eliminado!**\n\n";
            }
        }
    }
    lock_sub.unlock();
}

void Broker::get_clients_by_topic(const string &topic, list<Client*> &clients){
    map<string, unordered_set<Client*>>::iterator it, it_end;

    unique_lock<mutex> lock_sub(this->mutex_subscriptions);

    it = this->subscriptions.find(topic);
    it_end = this->subscriptions.end();

    lock_sub.unlock();

    if(it != it_end)
        clients = list<Client*> (it->second.begin(), it->second.end());
}

bool Broker::check_client_ID(const string &client_ID){
    for(auto c : this->list_active_clients){
        if(c->client_id == client_ID){
            this->remove_client_from_server(c);
            return true;
        }
    }
    return false;
}

void Broker::new_publish(PUBLISH_Msg* msg){
    if(msg->isRetain()){
        PUBLISH_Msg* pub = new PUBLISH_Msg(msg->getTopic(),msg->getMessage(), 1); //copia el mensaje
        unique_lock<mutex> lock_ret(this->mutex_retain_msg);
        this->retain_msg[msg->getTopic()] = pub;
        lock_ret.unlock();
    }

    this->queue_msg.push(*msg);
}

void Broker::publish_will_msg(Client* c){
    PUBLISH_Msg* msg = new PUBLISH_Msg(c->will_topic, c->will_message, c->will_retain);
    this->new_publish(msg);
}

PUBLISH_Msg Broker::get_publish(){
    return this->queue_msg.pop();
}

PUBLISH_Msg Broker::get_retain_msg_by_topic(const string &topic){
    map <string, PUBLISH_Msg*>::iterator it, it_end;

    unique_lock<mutex> lock_ret(this->mutex_retain_msg);
    it = this->retain_msg.find(topic);
    it_end = this->retain_msg.end();
    lock_ret.unlock();

    if(it != it_end)
        return *(it->second);
    return PUBLISH_Msg();
}

bool Broker::is_running() const{
    return this->running;
}

void Broker::start_broker(){
    this->running = true;
}

void Broker::stop_broker(){
    this->running = false;
}

void Broker::show_suscriptions(){
    map<string, unordered_set<Client*>>::iterator it, it_end;

    unique_lock<mutex> lock_sub(this->mutex_subscriptions);

    for(it = this->subscriptions.begin(); it != this->subscriptions.end(); it++){
        cout<<"'"<<it->first<<"' -> {";
        for(auto c : it->second){
            cout<<c->sockfd<<", "; // se imprimen los sockets de los clientes suscritos
        }
        cout<<"}\n";
    }
    lock_sub.unlock();
}

void Broker::show_retain_msg(){
    map<string, PUBLISH_Msg*>::iterator it, it_end;

    unique_lock<mutex> lock_ret(this->mutex_retain_msg);

    for(it = this->retain_msg.begin(); it != this->retain_msg.end(); it++){
        cout<<"'"<<it->first<<"' -> "<<it->second->getMessage()<<"\n";
    }

    lock_ret.unlock();
}

void client_handler(Broker* br, Client* c){  //funcion que se encarga de manejar los mensajes del cliente
    cout<<"\nRunning handler for client_socket: "<< c->sockfd <<"\n";

    // Potocolo Connect
    CONNECT_Msg connect_msg;
    int e = recvMsg(c->sockfd, connect_msg);
    if (e<=0){
        close(c->sockfd);
        return;
    }
    c->client_id = connect_msg.get_client_ID();

    if(br->check_client_ID(c->client_id))  // Si el cliente ya existe, lo elimina y cierra el socket
        cout<<c->sockfd<<": Cliente \""<<c->client_id<<"\" ya existe, se elimina el existente!\n";

    if(connect_msg.is_will_flag()){
        c->will_topic = connect_msg.get_will_topic();
        c->will_message = connect_msg.get_will_message();
        c->will_retain = connect_msg.is_will_retain();
    }

    br->add_client_to_server(c);
    cout<<c->sockfd<<": CONNECT_Msg recibido de \""<<c->client_id<<"\"\n";

    // Si todo está correcto enviamos un CONNACK_Msg
    CONNACK_Msg connack_msg(0x00);  // 0 = ACCEPTED
    if(sendMsg(c->sockfd, connack_msg)<=0){
        cout<<"Error al enviar CONNACK_Msg\n";
        close(c->sockfd);
        return;
    }
    cout<<c->sockfd<<": CONNACK_Msg enviado!\n";

    uint8_t buffer[BUFFER_SIZE];

    while(1){
        bzero(buffer, BUFFER_SIZE);
        while(!recvBuffer(c->sockfd, buffer));        

        switch ((Type)(buffer[0]>>4)){
            case PUBLISH:{
                PUBLISH_Msg* publish_msg = new PUBLISH_Msg();
                if(publish_msg->unpack(buffer)<0){
                    if(c->will_topic.size())
                        br->publish_will_msg(c);
                    goto end;
                }
                br->new_publish(publish_msg);

                cout<<c->sockfd<<": PUBLISH_Msg recibido de \""<<c->client_id<<"\"\n";
                if(publish_msg->isRetain())
                    cout<<c->sockfd<<": Mensaje retenido en topic: \""<<publish_msg->getTopic()<<"\"\n";
                break;
            }

            case SUBSCRIBE:{
                SUBSCRIBE_Msg* subscribe_msg = new SUBSCRIBE_Msg();
                if(subscribe_msg->unpack(buffer)<0){
                    if(c->will_topic.size())
                        br->publish_will_msg(c);
                    goto end;
                }
                br->add_client_to_topics(subscribe_msg->getTopics(),c);
                cout<<c->sockfd<<": SUBSCRIBE_Msg recibido de \""<<c->client_id<<"\"\n";

                // Enviamos un SUBACK_Msg
                int n = subscribe_msg->getTopics().size();
                list<uint8_t> return_code(n,0); // 0 = QoS 0
                SUBACK_Msg suback_msg(subscribe_msg->getPacketID(), return_code); 
                if(sendMsg(c->sockfd, suback_msg)<0){
                    if(c->will_topic.size())
                        br->publish_will_msg(c);
                    goto end;
                }
                cout<<c->sockfd<<": SUBACK_Msg enviado!\n";

                // Si existen mensajes retenidos en estos topics, lo enviamos al cliente
                for(auto topic : subscribe_msg->getTopics()){
                    PUBLISH_Msg msg = br->get_retain_msg_by_topic(topic);

                    if(msg.getTopic().size()){
                        if(sendMsg(c->sockfd, msg) == -1)
                            goto end;
                        cout<<c->sockfd<<": Mensaje retenido en topic \""<<topic<<"\" enviado!\n";
                    }
                }
                br->show_suscriptions();
                break;
            }

            case UNSUBSCRIBE:{
                UNSUBSCRIBE_Msg* unsubscribe_msg = new UNSUBSCRIBE_Msg();
                if(unsubscribe_msg->unpack(buffer)<0){
                    if(c->will_topic.size())
                        br->publish_will_msg(c);
                    goto end;
                }
                cout<<c->sockfd<<": UNSUBSCRIBE_Msg recibido de \""<<c->client_id<<"\"\n";
                br->remove_client_from_topics(unsubscribe_msg->getTopics(),c);

                // Enviamos un UNSUBACK_Msg
                UNSUBACK_Msg unsuback_msg(unsubscribe_msg->getPacketID());
                if(sendMsg(c->sockfd, unsuback_msg))
                    goto end;
                cout<<c->sockfd<<": UNSUBACK_Msg enviado!\n";
                break;
            }

            case PINGREQ:{
                PINGREQ_Msg* pingreq_msg = new PINGREQ_Msg();
                if(pingreq_msg->unpack(buffer)<0){
                    if(c->will_topic.size())
                        br->publish_will_msg(c);
                    goto end;
                }
                cout<<c->sockfd<<": PINGREQ_Msg recibido de \""<<c->client_id<<"\"\n";

                // Enviamos un PINGRESP_Msg
                PINGRESP_Msg pingresp_msg;
                if(sendMsg(c->sockfd, pingresp_msg))
                    goto end;
                cout<<c->sockfd<<": PINGRESP_Msg enviado!\n";
                break;
            }

            case DISCONNECT:{
                DISCONNECT_Msg* disconnect_msg = new DISCONNECT_Msg();
                if(disconnect_msg->unpack(buffer)<0){
                    if(c->will_topic.size())
                        br->publish_will_msg(c);
                    goto end;
                }
                cout<<c->sockfd<<": DISCONNECT_Msg recibido de \""<<c->client_id<<"\"\n";
                goto end;
            }

            default:{
                cout<<c->sockfd<<": Mensaje desconocido, cerrando conexión!\n";
                if(c->will_topic.size())
                        br->publish_will_msg(c);
                goto end;
            }
        }
    }
    end:
    cout<<c->sockfd<<": Cliente \""<<c->client_id<<"\" eliminado de la lista de clientes activos!\n";
    cout<<c->sockfd<<": Conexión cerrada!\n";
    br->remove_client_from_server(c); 
    return;
}



void accept_client_handler(Broker* br){  // Función encargada de aceptar nuevas conexiones y asignar un hilo para cada una
    cout<<"Running accept_clients_handler!\n";
    while (1){
        struct sockaddr_in client_addr;
        socklen_t clientLen = sizeof(client_addr);

        int clientSocket = accept(br->get_server_socket(), (struct sockaddr *) &client_addr, &clientLen);
        if(clientSocket <0){
            perror("Error en accept!");
            close(clientSocket);
        }

        // Si hay lugar para un nuevo cliente, creo el thread que lo maneja.
        if(br->get_number_of_clients() < MAX_CLIENTS){
            Client *c = new Client;
            c->sockfd = clientSocket;

            thread client_handler_thr(client_handler, br, c);
            client_handler_thr.detach();
        }
        else{
            cout<<"\nEl broker está manejando el máximo numero de clientes que puede!\n";
            close(clientSocket);
        }
    }
}

void consumer(Broker* br){  // Función encargada de agarrar los mensajes de la cola y enviarselos a los clientes suscritos a ese topic
    cout<<"Running consumer!\n";
    while(1){
        PUBLISH_Msg msg = br->get_publish();
        list<Client*> clients;
        br->get_clients_by_topic(msg.getTopic(), clients);
        for(auto c : clients){
            if(sendMsg(c->sockfd, msg) == -1){
                cout<<"Error al enviar mensaje al cliente "<<c->client_id<<"\n";
                br->remove_client_from_server(c);
            }
        }
    }
}