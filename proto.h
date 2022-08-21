#include <stdint.h>
#include <arpa/inet.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <list>

#define TYPE_MASK 0xF0 // 11110000
#define FLAGS_MASK 0x0F // 00001111
#define BUFFER_SIZE 256000  // tamaño máximo del buffer

#define CONNECT_flag     0
#define CONNACK_flag     0
#define PUBACK_flag      0
#define PUBREC_flag      0
#define PUBREL_flag      2
#define PUBCOMP_flag     0
#define SUBSCRIBE_flag   2   // Flags del Header Fijo para cada tipo de paquete
#define SUBACK_flag      0
#define UNSUBSCRIBE_flag 2
#define UNSUBACK_flag    0
#define PINGREQ_flag     0
#define PINGRESP_flag    0
#define DISCONNECT_flag  0

using namespace std;

typedef enum Contr_Pack_Types{ // Tipos de paquetes
    RESERVED,
    CONNECT,
    CONNACK,
    PUBLISH,
    PUBACK,
    PUBREC,
    PUBREL,
    PUBCOMP,
    SUBSCRIBE,
    SUBACK,
    UNSUBSCRIBE,
    UNSUBACK,
    PINGREQ,
    PINGRESP,
    DISCONNECT
} Type;

void inline error(const char *msg){ // Imprime un mensaje de error y termina el programa
    perror(msg);
    exit(0);
}

void inline print_v(uint8_t* buffer, int len){  // imprime un buffer de longitud len
    for (int i = 0; i < len; i++)
        cout << (int)buffer[i] << " ";
    cout<<endl;
}

template <typename Msg> void inline print(const Msg& msg){  // imprime un mensaje genérico
    cout<<msg<<endl;
}

int inline get_rem_len(const int &sockfd, uint8_t* buffer, uint32_t& rem_len){    // agarra del buffer los Bytes codificados del remaining length y devuelve la cantidad de Bytes codificados
    int multiplier = 1, value = 0, i = 0, j = 0;
    uint8_t encodedByte;
    do{
        while(j<1){ // intenta hasta que se reciba 1 byte
            int len = read(sockfd, &buffer[i], 1);
            if(len == -1)
                error("ERROR recibiendo del socket " + sockfd);
            j+=1;
        }
        
        encodedByte = buffer[i];
        value += (encodedByte & 127)*multiplier;
        multiplier *= 128;
        if(multiplier > 128*128*128)
            return -1;
        i++;
        j=0;
    } while ((encodedByte & 128) != 0);
    
    rem_len = value;
    return i;
}

int inline recvBuffer(const int &sockfd, uint8_t* buffer){  // recibe del sockfd el packete y devuelve un buffer
    int i = 0;
    uint32_t rem_len;

    //recibimos el primer byte del fixed header
    while(i<1){ // intenta hasta que se reciba 1 byte
        int len = read(sockfd, &buffer[i], 1);
        if(len == -1){
            perror("ERROR recibiendo del socket " + sockfd);
            return -1;
        }
        i+=len; //si lee 0 sigue leyendo;
    }

    // verificamos que sea un tipo de paquete valido
    uint8_t type = (uint8_t)(buffer[i-1]>>4);
    if(type==0 || type>14){
        perror("Tipo de paquete incorrecto!");
        return -1;
    }

    //obtenemos la cantidad de bytes que ocupa el campo remaing len y el valor 
    i += get_rem_len(sockfd, &buffer[i], rem_len);

    // recibimos el resto del paquete, rem_len bytes
    int j = 0;
    while(j<rem_len){
        int len = read(sockfd, &buffer[i+j], rem_len - j);
        if(len == -1){
            perror("ERROR recibiendo del socket " + sockfd);
            return -1;
        }
        j+=len; //si lee 0 sigue leyendo;
    }
    return i+j;
}

template <typename T> int inline recvMsg(const int &sockfd, T& msg){  // recibe por el sockfd un buffer y lo guarda en msg
    uint8_t buffer[BUFFER_SIZE]={};
    int len = recvBuffer(sockfd, buffer);
    if(len == -1)
        return len;
    return msg.unpack(buffer);
}
template<typename T> int inline sendMsg(const int &sockfd, const T& msg){
    //armo un buffer de tamaño BUFFER_SIZE = 256 MB y lo limpio
    uint8_t buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);

    //guardo el largo del paquete a enviar
    int largo = msg.pack(buffer);

    int n=0;
    do{
        //Leo del socket largo bytes y los guardo en el buffer
        int len = write(sockfd, &buffer[n], largo-n);
        if(len == -1){
            perror("ERROR enviando por el socket " + sockfd);
            return -1; 
        }
        n += len;
    }while(largo>n); // si leo menos que el largo, vuelvo a leer el restante y 
                     //lo guardo en la posición del buffer donde me quedé
    return n;
}

class Header{
    protected:
        Type type;
        uint8_t flags;
        int rem_len;
    
    public:
        Header() = default;
        virtual ~Header() = default;

        //Funciones para armar el buffer a partir de los parametros
        void setType(uint8_t* buffer) const;
        void setHFlags(uint8_t* buffer) const;
        int setRemLength(uint8_t* buffer) const;
    
        //Funciones para setear parametros desde el buffer
        Type getType(uint8_t* buffer);
        int getHFlags(uint8_t* buffer);
        int getRemLength(uint8_t* buffer);

        //Arma el mensaje en buffer
        virtual int pack (uint8_t * buffer) const{ return 0;}

        //Desarma el mensaje contenido en buffer. Chequea errores. Devuelve la cantidad de bytes del mensaje si no hay errores sino -1 
        virtual int unpack (uint8_t * buffer) const{ return 0;}
};

class CONNECT_Msg:public Header{
    private:
        //Variable header
        uint8_t protocol_name[6];        //6 bytes
        uint8_t protocol_level;          //1 byte
        uint8_t connect_flags;           //1 byte
        uint16_t keep_alive;             //2 bytes

        //Payload
        uint16_t ID_len;            //2 bytes. Valor max 23.
        string client_ID;           //ID_len bytes
        uint16_t will_topic_len;    //2 bytes.
        string will_topic;          //will_topic_len bytes
        uint16_t will_message_len;  //2 bytes.
        string will_message;        //will_message_len bytes

        //User Name Flag, Password Flag y Will QoS, deben fijarse a '0' en esta implementación. 
        bool will_retain;  // 1 bit
        bool will_flag;   // 1 bit


    public:
        CONNECT_Msg(){};
        CONNECT_Msg(uint16_t keep_alive_, string client_ID_);
        CONNECT_Msg(uint16_t keep_alive_, string client_ID_, string will_topic_, string will_message_, bool retain_);
        ~CONNECT_Msg(){};

        int pack (uint8_t * buffer) const;  //Arma el mensaje en buffer
        int unpack (uint8_t * buffer);  //Desarma el mensaje contenido en buffer. Chequea errores. Devuelve la cantidad de bytes del mensaje si no hay errores sino -1

        int check_protocol_name_level (uint8_t* buffer) const;  // chequea que el protocolo y la verión sean las correctas
        int set_header (uint8_t* buffer) const;  //Arma el header en buffer
        int get_header (uint8_t* buffer);  //Desarma el header contenido en buffer. Chequea errores. Devuelve la cantidad de bytes del header si no hay errores sino -1

        int set_payload(uint8_t* buffer) const;  //Arma el payload en buffer
        int get_payload(uint8_t* buffer);  //Desarma el payload contenido en buffer. Chequea errores. Devuelve la cantidad de bytes del payload si no hay errores sino -1

        bool is_will_retain() const;  //Devuelve el valor del bit de will retain
        bool is_will_flag() const;  //Devuelve el valor del bit de will flag

        uint16_t get_keep_alive() const;  //Devuelve el valor del keep alive

        string get_client_ID() const;  //Devuelve el valor del client ID
        string get_will_topic() const;  //Devuelve el valor del will topic
        string get_will_message() const;  //Devuelve el valor del will message

        void show_buffer();  //Imprime el buffer en pantalla
};

class CONNACK_Msg:public Header{
    //Esta clase no tiene payload
    private:
        uint8_t return_code;
        uint8_t session_present;

    public:
        CONNACK_Msg(){};
        CONNACK_Msg(uint8_t return_code_);
        ~CONNACK_Msg(){};

        int pack (uint8_t * buffer) const;  //Arma el mensaje en buffer
        int unpack (uint8_t * buffer);  ///Desarma el mensaje contenido en buffer. Chequea errores. Devuelve la cantidad de bytes del mensaje si no hay errores sino -1

        void show_buffer();  //Imprime el buffer en pantalla
};

class PUBLISH_Msg:public Header{
    private:
        //Variable header
        uint16_t Topic_len;  //2 bytes
        string Topic_name;   //Topic_len bytes
        bool retain;       //1 bit

        //Payload
        uint16_t Message_len; // no se incluye en el buffer
        string Message;   //(rem_len-2-Topic_len) bytes

    public:
        PUBLISH_Msg(){};
        PUBLISH_Msg(string topic_, string message_, bool retain_);
        ~PUBLISH_Msg(){};

        bool isRetain() const;  //Devuelve el valor del bit de retain

        int pack (uint8_t * buffer) const;  //Arma el mensaje en buffer
        int unpack (uint8_t * buffer);  //Desarma el mensaje contenido en buffer. Chequea errores. Devuelve la cantidad de bytes del mensaje si no hay errores sino -1

        string getTopic() const;  //Devuelve el valor del topic
        string getMessage() const;  //Devuelve el valor del message

        void show_buffer();  //Imprime el buffer en pantalla
};

class SUBSCRIBE_Msg:public Header{
    private:
        // Variable Header
        uint16_t packet_ID;

        // Payload
        list<string> Topic_name;

    public:
        SUBSCRIBE_Msg(){};
        SUBSCRIBE_Msg(uint16_t packet_ID_, list<string> topic_name_);
        ~SUBSCRIBE_Msg(){};

        int pack (uint8_t * buffer) const; //Arma el mensaje en buffer
        int unpack (uint8_t * buffer); //Desarma el mensaje contenido en buffer. Chequea errores. Devuelve la cantidad de bytes del mensaje si no hay errores sino -1

        uint16_t getPacketID() const; //Devuelve el valor del packet ID
        list<string>& getTopics() const; //Devuelve la lista de topics

        void show_buffer(); //Imprime el buffer en pantalla
};

class SUBACK_Msg:public Header{
    private:
        // Variable Header
        uint16_t packet_ID; //2 bytes

        // Payload
        list<uint8_t> return_code;
    
    public:
        SUBACK_Msg(){};
        SUBACK_Msg(uint16_t packet_ID_, list<uint8_t> return_code_);
        ~SUBACK_Msg(){};

        int pack (uint8_t * buffer) const; //Arma el mensaje en buffer
        int unpack (uint8_t * buffer); //Desarma el mensaje contenido en buffer. Chequea errores. Devuelve la cantidad de bytes del mensaje si no hay errores sino -1

        void checkReturnCode() const; //Chequea el return
        void show_buffer(); //Imprime el buffer en pantalla
};

class UNSUBSCRIBE_Msg:public Header{
    private:
        // Variable Header
        uint16_t packet_ID;

        // Payload
        list<string> Topic_name;
        
    public:
        UNSUBSCRIBE_Msg(){};
        UNSUBSCRIBE_Msg(uint16_t packet_ID_, list<string> topic_name_);
        ~UNSUBSCRIBE_Msg(){};

        int pack (uint8_t * buffer) const; 
        int unpack (uint8_t * buffer);

        uint16_t getPacketID() const;  //Devuelve el valor del packet ID
        list<string>& getTopics() const;  //Devuelve la lista de topics

        void show_buffer();  //Imprime el buffer en pantalla
};

class UNSUBACK_Msg:public Header{
    private:
        // Variable Header
        uint16_t packet_ID; //2 bytes

    public:
        UNSUBACK_Msg(){};
        UNSUBACK_Msg(uint16_t packet_ID_);
        ~UNSUBACK_Msg(){};

        int pack (uint8_t * buffer) const;
        int unpack (uint8_t * buffer);

        void show_buffer();
};

class PINGREQ_Msg:public Header{
    public:
        PINGREQ_Msg();
        ~PINGREQ_Msg(){};

        int pack (uint8_t * buffer) const;
        int unpack (uint8_t * buffer);

        void show_buffer();
};

class PINGRESP_Msg:public Header{
    public:
        PINGRESP_Msg();
        ~PINGRESP_Msg(){};

        int pack (uint8_t * buffer) const;
        int unpack (uint8_t * buffer);

        void show_buffer();
};

class DISCONNECT_Msg:public Header{
    public:
        DISCONNECT_Msg();
        ~DISCONNECT_Msg(){};

        int pack (uint8_t * buffer) const;
        int unpack (uint8_t * buffer);

        void show_buffer();
};