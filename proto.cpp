#include "proto.h"

using namespace std;


////////////////////////////////////////////////////////////////////////////////////////////////////
//Header
////////////////////////////////////////////////////////////////////////////////////////////////////

void Header::setType(uint8_t* buffer) const{   // Setea el tipo en el buffer
    buffer[0] = ((this->type << 4) & TYPE_MASK) | (buffer[0] & FLAGS_MASK);
}

void Header::setHFlags(uint8_t* buffer) const{   // guarda los flags en el buffer
    buffer[0] = (buffer[0] & TYPE_MASK) | (this->flags & FLAGS_MASK);
}

int Header::setRemLength(uint8_t* buffer) const{   // guarda en el buffer los Bytes codificados del remaining length y devuelve la cantidad de Bytes codificados
    int i=0;
    uint8_t encodedByte;
    int X = this->rem_len;
    do{
        encodedByte = X%128;
        X = X/128;
        if(X > 0)
            encodedByte = encodedByte | 0x80;
        buffer[i++] = encodedByte;
    }while (X>0);
    return i;
}

Type Header::getType(uint8_t* buffer){    // agarra del buffer el tipo de mensaje y lo devuelve
    this->type = (Type) ((buffer[0] & TYPE_MASK)>> 4);
    return this->type;
}

int Header::getHFlags(uint8_t* buffer){    // agarra del buffer los flags y los retorna
    this->flags = (buffer[0] & FLAGS_MASK);
    return this->flags;
}

int Header::getRemLength(uint8_t* buffer){    // agarra del buffer los Bytes codificados del remaining length y devuelve la cantidad de Bytes codificados
    int multiplier = 1, value = 0, i = 0;
    uint8_t encodedByte;
    do{
        encodedByte = buffer[i];
        value += (encodedByte & 127)*multiplier;
        multiplier *= 128;
        if(multiplier > 128*128*128){
            return -1;
        }
        i++;
    } while ((encodedByte & 128) != 0);
    
    this->rem_len = value;
    return i;
}




////////////////////////////////////////////////////////////////////////////////////////////////////
//CONNECT_Msg
////////////////////////////////////////////////////////////////////////////////////////////////////
CONNECT_Msg::CONNECT_Msg(uint16_t keep_alive_, string client_ID_){   // Constructor de la clase CONNECT_Msg
    //Fixed Header
    this->type = CONNECT;
    this->flags = CONNECT_flag;

    //Variable Header
    int i=0; 
    this->protocol_name[i++] = 0x00; //0
    this->protocol_name[i++] = 0x04; //4
    this->protocol_name[i++] = 0x4d; //M
    this->protocol_name[i++] = 0x51; //Q
    this->protocol_name[i++] = 0x54; //T
    this->protocol_name[i++] = 0x54; //T

    this->protocol_level = 0x04; //MQTT v3.1.1
    i++;

    this->connect_flags = 0x02;  //solo Clean Session
    i++;

    this->keep_alive = keep_alive_;   // tiempo en segundos que el cliente quiere mantener la conexion, si es 0, no se cierra la conexion nunca
    i += 2;

    // Pyload
    this->ID_len = client_ID_.length();
    this->client_ID = client_ID_;
    i += 2 + this->ID_len;

    this->will_flag = 0x00;
    this->will_retain = 0x00;

    // Remaining Length
    this->rem_len = i;
}

CONNECT_Msg::CONNECT_Msg(uint16_t keep_alive_, string client_ID_, string will_topic_, string will_message_, bool will_retain_){   // Constructor de la clase CONNECT_Msg
    //Fixed Header
    this->type = CONNECT;
    this->flags = CONNECT_flag;

    //Variable Header
    int i=0; 
    this->protocol_name[i++] = 0x00; //0
    this->protocol_name[i++] = 0x04; //4
    this->protocol_name[i++] = 0x4d; //M
    this->protocol_name[i++] = 0x51; //Q
    this->protocol_name[i++] = 0x54; //T
    this->protocol_name[i++] = 0x54; //T

    this->protocol_level = 0x04; //MQTT v3.1.1
    i++;

    this->connect_flags = 0x06 | (will_retain_ << 5);  //solo Will_retain, Will_Flag, Clean Session
    i++;

    this->keep_alive = keep_alive_;   // tiempo en segundos que el cliente quiere mantener la conexion, si es 0, no se cierra la conexion nunca
    i += 2;

    // Pyload
    this->ID_len = client_ID_.length();
    this->client_ID = client_ID_;
    i += 2 + this->ID_len;

    this->will_topic_len = will_topic_.length();
    this->will_topic = will_topic_;
    i += 2 + this->will_topic_len;

    this->will_message_len = will_message_.length();
    this->will_message = will_message_;
    i += 2 + this->will_message_len;

    this->will_flag = 0x01;
    this->will_retain = will_retain_;

    // Remaining Length
    this->rem_len = i;
}


int CONNECT_Msg::set_header(uint8_t* buffer) const{  // Setea el header en el buffer y devuelve la cantidad de Bytes codificados
    int i=0;
    //Fixed header
    this->setType(&buffer[i]);
    this->setHFlags(&buffer[i]);
    i += this->setRemLength(&buffer[++i]);

    //Variable header
    memcpy(&buffer[i], this->protocol_name, 6);
    i+=6;
    buffer[i++] = this->protocol_level;
    buffer[i++] = this->connect_flags;
    buffer[i++] = (uint8_t) (this->keep_alive >> 8);
    buffer[i++] = (uint8_t) (this->keep_alive);
    return i;
}

int CONNECT_Msg::check_protocol_name_level(uint8_t* buffer) const{   // Chequea que el protocol name y protocol level sean correctos
    int i = 0;
    if(buffer[i++] == 0x00 && buffer[i++] == 0x04 && buffer[i++] == 0x4d && buffer[i++] == 0x51 && buffer[i++] == 0x54 && buffer[i++] == 0x54){
        if(buffer[i] == 0x04){
            return 0;
        }
    }
    return -1;
}

int CONNECT_Msg::get_header(uint8_t* buffer) {  // Agarra del buffer el header y lo guarda en la clase.
    int i=0;
    //Fixed header
    if((this->getType(&buffer[i])) != CONNECT){   // Si el mensaje no es de tipo CONNECT retorna 0
        cout<<"El mensaje no es tipo CONNECT!"<<endl;
        return 0;
    }
    if((this->getHFlags(&buffer[i])) != CONNECT_flag){  // Si los flags no coinciden con CONNECT_flags el Servidor debe cerrar la conexión
        cout<<"Los Flags del Header Fijo no son los quedeberían!"<<endl;
        return -1;
    }
    i += this->getRemLength(&buffer[++i]);

    //Variable header
    if(this->check_protocol_name_level(&buffer[i])<0){   // Si el Protocol Name y Protocol Level no son los que deberían el Servidor debe cerrar la conexión
        cout<<"Name Protocol or Level Protocol son erroneos!"<<endl;
        return -1;
    }
    memcpy(this->protocol_name, &buffer[i], 6);
    i+=6;
    this->protocol_level = buffer[i++];

    if(buffer[i] & 0x01 != 0x00){   // Si el primer bit del Byte Connect Flag no es cero, el Servidor debe cerrar la conexión
        cout<<"El primer bit del Byte Connect Flag está reservado (0)!"<<endl;
        return -1;
    }
    this->connect_flags = buffer[i++];
    this->will_flag = (this->connect_flags & 0x04) >> 2;
    if(this->will_flag)
        this->will_retain = (this->connect_flags & 0x20) >> 5;
    else
        this->will_retain = 0;

    this->keep_alive = (uint16_t) ((buffer[i++] << 8) | buffer[i++]);
    return i;
}

int CONNECT_Msg::set_payload(uint8_t* buffer) const{   // Setea el payload en el buffer
    //Client Identifier 
    int i=0;
    buffer[i++] = (uint8_t) (this->ID_len >> 8);
    buffer[i++] = (uint8_t) (this->ID_len);
    memcpy(&buffer[i], this->client_ID.c_str(), this->ID_len);
    i+=this->ID_len;

    //Will Topic and Will Message
    if(this->will_flag){
        buffer[i++] = (uint8_t) (this->will_topic_len >> 8);
        buffer[i++] = (uint8_t) (this->will_topic_len);
        memcpy(&buffer[i], this->will_topic.c_str(), this->will_topic_len);
        i+=this->will_topic_len;

        buffer[i++] = (uint8_t) (this->will_message_len >> 8);
        buffer[i++] = (uint8_t) (this->will_message_len);
        memcpy(&buffer[i], this->will_message.c_str(), this->will_message_len);
        i+=this->will_message_len;
    }
    return i;
}

int CONNECT_Msg::get_payload(uint8_t* buffer){   // Agarra del buffer el payload y lo guarda en la clase
    //Client Identifier
    int i=0;
    this->ID_len = (uint16_t) (buffer[i++] << 8 | buffer[i++]);

    if(this->ID_len == 0 && (this->connect_flags & 0x02) == 0x00){   // Si el Client ID es 0 y el bit Clean_session es 0, el Servidor debe cerrar la conexión
        cout<<"Para usar Client_ID = 0 el bit Clean_Session debe ser '1'"<<endl;
        return -1;
    }

    string id ((char*)&buffer[i], this->ID_len);
    this->client_ID = id;
    i+=this->ID_len;

    //Will Topic and Will Message
    if(this->will_flag){
        this->will_topic_len = (uint16_t) (buffer[i++] << 8 | buffer[i++]);
        string topic ((char*)&buffer[i], this->will_topic_len);
        this->will_topic = topic;
        i+=this->will_topic_len;

        this->will_message_len = (uint16_t) (buffer[i++] << 8 | buffer[i++]);
        string message ((char*)&buffer[i], this->will_message_len);
        this->will_message = message;
        i+=this->will_message_len;
    }
    return i;
}

int CONNECT_Msg::pack(uint8_t* buffer) const{   // Pone todo en el buffer y devuelve el tamaño del mensaje
    int i=0;
    i+=this->set_header(&buffer[i]);
    i+=this->set_payload(&buffer[i]);
    return i;
}

int CONNECT_Msg::unpack(uint8_t* buffer){   // Agarra del buffer el mensaje y lo guarda en la clase
    int i=0;
    int j = this->get_header(&buffer[i]);
    if(j == -1) return -1;
    i+=j;
    j = this->get_payload(&buffer[i]);
    if(j == -1) return -1;
    return i+j;
}

bool CONNECT_Msg::is_will_retain() const{
    return this->will_retain;
}

bool CONNECT_Msg::is_will_flag() const{
    return this->will_flag;
}

uint16_t CONNECT_Msg::get_keep_alive() const{
    return this->keep_alive;
}

string CONNECT_Msg::get_client_ID() const{
    return this->client_ID;
}

string CONNECT_Msg::get_will_topic() const{
    return this->will_topic;
}

string CONNECT_Msg::get_will_message() const{
    return this->will_message;
}

void CONNECT_Msg::show_buffer(){
    uint8_t buffer[BUFFER_SIZE];
    int l = this->pack(buffer);
    print_v(buffer,l);
}




////////////////////////////////////////////////////////////////////////////////////////////////////
//CONNACK_Msg
////////////////////////////////////////////////////////////////////////////////////////////////////
CONNACK_Msg::CONNACK_Msg(uint8_t return_code_){
    // Fixed Header
    this->type = CONNACK;
    this->flags = CONNACK_flag;
    this->rem_len = 2;

    // Variable Header
    this->session_present = 0;
    this->return_code = return_code_;
}


int CONNACK_Msg::pack(uint8_t* buffer) const{
    int i=0;
    //Fixed header
    this->setType(&buffer[i]);
    this->setHFlags(&buffer[i]);
    i += this->setRemLength(&buffer[++i]);
    buffer[i++] = this->session_present;
    buffer[i++] = this->return_code;
    return i;
}

int CONNACK_Msg::unpack(uint8_t* buffer){
    int i=0;
    //Fixed header
    if((this->getType(&buffer[i])) != CONNACK){   // Si el mensaje no es de tipo CONNACK retorna 0
        cout<<"El mensaje no es tipo CONNACK!"<<endl;
        return 0;
    }
    if((this->getHFlags(&buffer[i])) != CONNACK_flag){  // Si los flags no coinciden con CONNACK_flags el Cliente debe matar la conexión
        cout<<"Los Flags del Header Fijo no son los que deberían!"<<endl;
        return -1;
    }
    i += this->getRemLength(&buffer[++i]);

    //Variable header
    if(buffer[i] != 0x00){ 
        cout<<"Este servidor no soporta Session Present"<<endl;
        return -1;
    }
    this->session_present = buffer[i++];
    if(buffer[i] != 0x00){ 
        cout<<"Connection Refused!"<<endl;
        return -1;
    }
    this->return_code = buffer[i++];
    return i;
}

void CONNACK_Msg::show_buffer(){
    uint8_t buffer[BUFFER_SIZE];
    int l = this->pack(buffer);
    print_v(buffer,l);
}




////////////////////////////////////////////////////////////////////////////////////////////////////
//PUBLISH_Msg
////////////////////////////////////////////////////////////////////////////////////////////////////
PUBLISH_Msg::PUBLISH_Msg(string topic_, string message_, bool retain_){
    // Fixed Header
    this->type = PUBLISH;
    this->flags = 0x0|retain_;

    // Variable Header
    this->Topic_len = topic_.length();
    this->Topic_name = topic_;
    this->retain = retain_;
    this->Message_len = message_.length();
    this->Message = message_;

    // Remaining Length
    this->rem_len = 2 + this->Topic_len + this->Message_len;
}

bool PUBLISH_Msg::isRetain() const{
    return this->retain;
}

int PUBLISH_Msg::pack(uint8_t* buffer) const{
    int i=0;
    //Fixed header
    this->setType(&buffer[i]);
    this->setHFlags(&buffer[i]);
    i += this->setRemLength(&buffer[++i]);

    //Variable header
    buffer[i++] = (uint8_t) (this->Topic_len >> 8);
    buffer[i++] = (uint8_t) (this->Topic_len);
    memcpy(&buffer[i], this->Topic_name.c_str(), this->Topic_len);
    i+=this->Topic_len;

    //Payload
    memcpy(&buffer[i], this->Message.c_str(), this->Message_len);
    i+=this->Message_len;
    return i;
}

int PUBLISH_Msg::unpack(uint8_t* buffer) {
    int i=0;
    //Fixed header
    if((this->getType(&buffer[i])) != PUBLISH){   // Si el mensaje no es de tipo PUBLISH retorna 0
        cout<<"El mensaje no es tipo PUBLISH!"<<endl;
        return 0;
    }
    if((this->getHFlags(&buffer[i])) > 0x1){ 
        cout<<"Este servidor no soporta QoS!"<<endl;
        return -1;
    }
    retain = (buffer[i] & 0x1);
    i += this->getRemLength(&buffer[++i]);

    //Variable header
    this->Topic_len = (uint16_t) (buffer[i++] << 8 | buffer[i++]);
    string topic ((char*)&buffer[i], this->Topic_len);
    this->Topic_name = topic;
    i+=this->Topic_len;

    //Payload
    this->Message_len = this->rem_len - 2 - this->Topic_len;
    string message ((char*)&buffer[i], this->Message_len);
    this->Message = message;
    i+=this->Message_len;
    return i;
}

string PUBLISH_Msg::getTopic() const{
    return this->Topic_name;
}

string PUBLISH_Msg::getMessage() const{
    return this->Message;
}

void PUBLISH_Msg::show_buffer(){
    uint8_t buffer[BUFFER_SIZE];
    int l = this->pack(buffer);
    print_v(buffer,l);
}




////////////////////////////////////////////////////////////////////////////////////////////////////
//SUBSCRIBE_Msg
////////////////////////////////////////////////////////////////////////////////////////////////////
SUBSCRIBE_Msg::SUBSCRIBE_Msg(uint16_t packet_ID_, list<string> topic_name_){
    // Fixed Header
    this->type = SUBSCRIBE;
    this->flags = SUBSCRIBE_flag;

    // Variable Header
    int i = 0;
    this->packet_ID = packet_ID_;
    i+=2;

    // Payload
    for(auto it = topic_name_.begin(); it != topic_name_.end(); ++it){
        this->Topic_name.push_back(*it);
        i += 2 + (*it).size() + 1;
    }

    // Remaining Length
    this->rem_len = i;
}

int SUBSCRIBE_Msg::pack(uint8_t* buffer) const{
    int i=0;
    //Fixed header
    this->setType(&buffer[i]);
    this->setHFlags(&buffer[i]);
    i += this->setRemLength(&buffer[++i]);

    //Variable header
    buffer[i++] = (uint8_t) (this->packet_ID >> 8);
    buffer[i++] = (uint8_t) (this->packet_ID);

    //Payload
    for(auto it = this->Topic_name.begin(); it != this->Topic_name.end(); ++it){
        buffer[i++] = (uint8_t) ((*it).size() >> 8);
        buffer[i++] = (uint8_t) ((*it).size());
        memcpy(&buffer[i], (*it).c_str(), (*it).size());
        i+=(*it).size();
        buffer[i++] = 0x00; //QoS = 0
    }
    return i;
}

int SUBSCRIBE_Msg::unpack(uint8_t* buffer){
    int i=0;
    //Fixed header
    if((this->getType(&buffer[i])) != SUBSCRIBE){   // Si el mensaje no es de tipo SUBSCRIBE retorna 0
        cout<<"El mensaje no es tipo SUBSCRIBE!"<<endl;
        return 0;
    }
    if((this->getHFlags(&buffer[i])) != SUBSCRIBE_flag){  // Si los flags no son los correctos el servidor debe cerrar la conexión
        cout<<"Los Flags del Header Fijo no son los que deberían!"<<endl;
        return -1;
    }
    i += this->getRemLength(&buffer[++i]);

    //Variable header
    this->packet_ID = (uint16_t) ((buffer[i++] << 8) | buffer[i++]);

    //Payload
    while(i < this->rem_len){
        int topic_len = (int) ((buffer[i++] << 8) | buffer[i++]);
        string topic ((char*)&buffer[i], topic_len);
        (this->Topic_name).push_back(topic);
        i+=topic_len;

        if(buffer[i++] > 0x03 ){   // Si el QoS es mayor a 0x03 el servidor debe cerrar la conexión
            cout<<"Bits de Qos reservados"<<endl;
            return -1;
        }
    }
    return i;
}

uint16_t SUBSCRIBE_Msg::getPacketID() const{
    return this->packet_ID;
}

list<string>& SUBSCRIBE_Msg::getTopics() const{
    list<string>& topic_name = const_cast<list<string>&>(this->Topic_name);
    return topic_name;
}

void SUBSCRIBE_Msg::show_buffer(){
    uint8_t buffer[BUFFER_SIZE];
    int l = this->pack(buffer);
    print_v(buffer,l);
}




////////////////////////////////////////////////////////////////////////////////////////////////////
//SUBACK_Msg
////////////////////////////////////////////////////////////////////////////////////////////////////
SUBACK_Msg::SUBACK_Msg(uint16_t packet_ID_, list<uint8_t> return_code_){
    // Fixed Header
    this->type = SUBACK;
    this->flags = SUBACK_flag;

    // Variable Header
    this->packet_ID = packet_ID_;

    // Payload
    this->return_code.assign(return_code_.begin(), return_code_.end());

    // Remaining Length
    this->rem_len = 2 + this->return_code.size();
}

int SUBACK_Msg::pack(uint8_t* buffer) const{
    int i=0;
    //Fixed header
    this->setType(&buffer[i]);
    this->setHFlags(&buffer[i]);
    i += this->setRemLength(&buffer[++i]);

    //Variable header
    buffer[i++] = (uint8_t) (this->packet_ID >> 8);
    buffer[i++] = (uint8_t) (this->packet_ID);

    //Payload
    for(auto it = this->return_code.begin(); it != this->return_code.end(); ++it){
        buffer[i++] = *it;
    }
    return i;
}

int SUBACK_Msg::unpack(uint8_t* buffer){
    int i=0;
    //Fixed header
    if((this->getType(&buffer[i])) != SUBACK){   // Si el mensaje no es de tipo SUBACK retorna 0
        cout<<"El mensaje no es tipo SUBACK!"<<endl;
        return 0;
    }
    if((this->getHFlags(&buffer[i])) != SUBACK_flag){  // Si los flags no son los correctos el cliente debe cerrar la conexión
        cout<<"Los Flags del Header Fijo no son los que deberían!"<<endl;
        return -1;
    }
    i += this->getRemLength(&buffer[++i]);

    //Variable header
    this->packet_ID = (uint16_t) ((buffer[i++] << 8) | buffer[i++]);

    //Payload
    for(int j=0; j<this->rem_len-2; j++){
        if((buffer[i] & 0x7C) != 0){
            cout<<"Estos codigos de retorno están reservados"<<endl;
            return -1;
        }
        this->return_code.push_back(buffer[i++]);
    }
    return i;
}

void SUBACK_Msg::checkReturnCode() const{
    for(auto it = this->return_code.begin(); it != this->return_code.end(); ++it){
        if((*it) > 0x80){
            cout<<"Topic Subscription Failure!"<<endl;
        }
        if((*it) < 0x04){
            cout<<"Topic Subscription Success!"<<endl;
        }
    }
}

void SUBACK_Msg::show_buffer(){
    uint8_t buffer[BUFFER_SIZE];
    int l = this->pack(buffer);
    print_v(buffer,l);
}




////////////////////////////////////////////////////////////////////////////////////////////////////
//UNSUBSCRIBE_Msg
////////////////////////////////////////////////////////////////////////////////////////////////////
UNSUBSCRIBE_Msg::UNSUBSCRIBE_Msg(uint16_t packet_ID_, list<string> topic_name_){
    // Fixed Header
    this->type = UNSUBSCRIBE;
    this->flags = UNSUBSCRIBE_flag;

    // Variable Header
    int i=0; 
    this->packet_ID = packet_ID_;
    i+=2;

    // Payload
    for(auto it = topic_name_.begin(); it != topic_name_.end(); ++it){
        this->Topic_name.push_back(*it);
        i += 2 + (*it).size();
    }

    // Remaining Length
    this->rem_len = i;
}

int UNSUBSCRIBE_Msg::pack(uint8_t* buffer) const{
    int i=0;
    //Fixed header
    this->setType(&buffer[i]);
    this->setHFlags(&buffer[i]);
    i += this->setRemLength(&buffer[++i]);

    //Variable header
    buffer[i++] = (uint8_t) (this->packet_ID >> 8);
    buffer[i++] = (uint8_t) (this->packet_ID);

    //Payload
    for(auto it = this->Topic_name.begin(); it != this->Topic_name.end(); ++it){
        buffer[i++] = (uint8_t) ((*it).size() >> 8);
        buffer[i++] = (uint8_t) ((*it).size());
        memcpy(&buffer[i], (*it).c_str(), (*it).size());
        i += (*it).size();
    }
    return i;
}

int UNSUBSCRIBE_Msg::unpack(uint8_t* buffer){
    int i=0;
    //Fixed header
    if((this->getType(&buffer[i])) != UNSUBSCRIBE){   // Si el mensaje no es de tipo UNSUBSCRIBE retorna 0
        cout<<"El mensaje no es tipo UNSUBSCRIBE!"<<endl;
        return 0;
    }
    if((this->getHFlags(&buffer[i])) != UNSUBSCRIBE_flag){  // Si los flags no son los correctos el server debe cerrar la conexión
        cout<<"Los Flags del Header Fijo no son los que deberían!"<<endl;
        return -1;
    }
    i += this->getRemLength(&buffer[++i]);

    //Variable header
    this->packet_ID = (uint16_t) ((buffer[i++] << 8) | buffer[i++]);

    //Payload
    for(int j=0; j<this->rem_len-2; j++){
        uint8_t topic_len = (uint8_t) ((buffer[i++] << 8) | buffer[i++]);
        string topic_name((char*)&buffer[i], topic_len);
        this->Topic_name.push_back(topic_name);
        i += topic_len;
    }
    return i;
}

uint16_t UNSUBSCRIBE_Msg::getPacketID() const{
    return this->packet_ID;
}

list<string>& UNSUBSCRIBE_Msg::getTopics() const{
    list<string>& topic_name = const_cast<list<string>&>(this->Topic_name);
    return topic_name;
}

void UNSUBSCRIBE_Msg::show_buffer(){
    uint8_t buffer[BUFFER_SIZE];
    int l = this->pack(buffer);
    print_v(buffer,l);
}




////////////////////////////////////////////////////////////////////////////////////////////////////
//UNSUBACK_Msg
////////////////////////////////////////////////////////////////////////////////////////////////////
UNSUBACK_Msg::UNSUBACK_Msg(uint16_t packet_ID_){
    // Fixed Header
    this->type = UNSUBACK;
    this->flags = UNSUBACK_flag;
    this->rem_len = 2;

    // Variable Header
    this->packet_ID = packet_ID_;
}

int UNSUBACK_Msg::pack(uint8_t* buffer) const{
    int i=0;
    //Fixed header
    this->setType(&buffer[i]);
    this->setHFlags(&buffer[i]);
    i += this->setRemLength(&buffer[++i]);

    //Variable header
    buffer[i++] = (uint8_t) (this->packet_ID >> 8);
    buffer[i++] = (uint8_t) (this->packet_ID);
    return i;   
}

int UNSUBACK_Msg::unpack(uint8_t* buffer){
    int i=0;
    //Fixed header
    if((this->getType(&buffer[i])) != UNSUBACK){   // Si el mensaje no es de tipo UNSUBACK retorna 0
        cout<<"El mensaje no es tipo UNSUBACK!"<<endl;
        return 0;
    }
    if((this->getHFlags(&buffer[i])) != UNSUBACK_flag){  // Si los flags no son los correctos el server debe cerrar la conexión
        cout<<"Los Flags del Header Fijo no son los que deberían!"<<endl;
        return -1;
    }
    i += this->getRemLength(&buffer[++i]);

    //Variable header
    this->packet_ID = (uint16_t) ((buffer[i++] << 8) | buffer[i++]);
    return i;
}

void UNSUBACK_Msg::show_buffer(){
    uint8_t buffer[BUFFER_SIZE];
    int l = this->pack(buffer);
    print_v(buffer,l);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
//PINGREQ_Msg
////////////////////////////////////////////////////////////////////////////////////////////////////
PINGREQ_Msg::PINGREQ_Msg(){   // Constructor por defecto 
    //Fixed header
    this->type = PINGREQ;
    this->flags = PINGREQ_flag;
    this->rem_len = 0;
}


int PINGREQ_Msg::pack (uint8_t * buffer) const{
    setType(&buffer[0]);
    setHFlags(&buffer[0]);   
    int length_rem = setRemLength(&buffer[1]);
    return 2;
}

int PINGREQ_Msg::unpack (uint8_t* buffer){   // Agarra del buffer el header y lo guarda en la clase
    //Fixed Header
    getType(&buffer[0]);
    if((buffer[0] & 0x0F) != PINGREQ_flag){
        cout<<"Flags de ping_req reservados!"<<endl;
        return -1;
    }
    getHFlags(&buffer[0]);
    int length_rem = getRemLength(&buffer[1]); 
    return 2;
}

void PINGREQ_Msg::show_buffer(){
    uint8_t buffer[BUFFER_SIZE];
    int l = this->pack(buffer);
    print_v(buffer,l);
}




////////////////////////////////////////////////////////////////////////////////////////////////////
//PINGRESP_Msg
////////////////////////////////////////////////////////////////////////////////////////////////////
PINGRESP_Msg::PINGRESP_Msg(){
    // Fixed Header
    this->type = PINGRESP;
    this->flags = PINGRESP_flag;
    this->rem_len = 0;
}

int PINGRESP_Msg::pack(uint8_t* buffer) const{
    int i=0;
    //Fixed header
    this->setType(&buffer[i]);
    this->setHFlags(&buffer[i]);
    i += this->setRemLength(&buffer[++i]);
    return i;
}

int PINGRESP_Msg::unpack(uint8_t* buffer){
    int i=0;
    //Fixed header
    if((this->getType(&buffer[i])) != PINGRESP){   // Si el mensaje no es de tipo PINGRESP retorna 0
        cout<<"El mensaje no es tipo PINGRESP!"<<endl;
        return 0;
    }
    if((this->getHFlags(&buffer[i])) != PINGRESP_flag){  // Si los flags no son los correctos el server debe cerrar la conexión
        cout<<"Los Flags del Header Fijo no son los que deberían!"<<endl;
        return -1;
    }
    i+=this->getRemLength(&buffer[++i]);
    return i;
}

void PINGRESP_Msg::show_buffer(){
    uint8_t buffer[BUFFER_SIZE];
    int l = this->pack(buffer);
    print_v(buffer,l);
}




////////////////////////////////////////////////////////////////////////////////////////////////////
//DISCONNECT_Msg
////////////////////////////////////////////////////////////////////////////////////////////////////
DISCONNECT_Msg::DISCONNECT_Msg(){
    // Fixed Header
    this->type = DISCONNECT;
    this->flags = DISCONNECT_flag;
    this->rem_len = 0;
}

int DISCONNECT_Msg::pack(uint8_t* buffer) const{
    int i=0;
    //Fixed header
    this->setType(&buffer[i]);
    this->setHFlags(&buffer[i]);
    i += this->setRemLength(&buffer[++i]);
    return i;
}

int DISCONNECT_Msg::unpack(uint8_t* buffer){
    int i=0;
    //Fixed header
    if((this->getType(&buffer[i])) != DISCONNECT){   // Si el mensaje no es de tipo DISCONNECT retorna 0
        cout<<"El mensaje no es tipo DISCONNECT!"<<endl;
        return 0;
    }
    if((this->getHFlags(&buffer[i])) != DISCONNECT_flag){  // Si los flags no son los correctos el server debe cerrar la conexión
        cout<<"Los Flags del Header Fijo no son los que deberían!"<<endl;
        return -1;
    }
    i+=this->getRemLength(&buffer[++i]);
    return i;
}

void DISCONNECT_Msg::show_buffer(){
    uint8_t buffer[BUFFER_SIZE];
    int l = this->pack(buffer);
    print_v(buffer,l);
}