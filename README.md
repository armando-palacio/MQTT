# MQTT
MQTT es un protocolo de mensajería estándar para el Internet de las cosas (IoT). Está diseñado como un transporte de mensajería de publicación/suscripción extremadamente ligero que es ideal para conectar dispositivos remotos con una pequeña huella de código y un ancho de banda de red mínimo. Hoy en día, MQTT se utiliza en una gran variedad de industrias, como la automoción, la fabricación, las telecomunicaciones, el petróleo y el gas, etc.

![maxresdefault](https://user-images.githubusercontent.com/66741745/185775394-2b29732c-6e9c-41bb-9944-599570253078.jpg)

## ¿Cómo utilizar el código?
Primeramente ejecutar los siguientes comandos en la consola para generar los ejecutables **broker**, **client_Temp_pub** y **client_Temp_sub** :
* g++ broker_main.cpp broker.cpp proto.cpp -o broker -lpthread
* g++ client_Temp_pub.cpp proto.cpp -o client_Temp_pub -lpthread
* g++ client_Temp_sub.cpp proto.cpp -o client_Temp_sub -lpthread


Una vez realizado el paso anterior solo queda correr el **broker** y los **clientes**. Para ejecutar el broker solo tenemos que ejecutar:
```[bash]
./broker
```

Deberían salir los siguientes mensajes en consola:
```[bash]
Broker listening!
Running accept_clients_handler!
Running consumer!
```

Ahora es momento de conectar los clientes para ello ejecutamos los siguientes comandos en consola:
```[bash]
./client_Temp_sub [ip_addr | hostname] [Topic]
```

Por ejemplo, supongamos que queremos ejecutar el broker y los clientes en la misma computadora y suscribirse al tema *Temperatura*:
```[bash]
./client_Temp_sub localhost Temperatura
```
Como resultado por parte del cliente subscripto deberíamos tener
```[bash]
Conexión aceptada!
Topic Subscription Success!
```
Por parte del cliente que publica deberíamos obtener una confirmación de conexión y rápidamente comienzará a enviar datos al broker en el tema *Temperatura*. 
```[bash]
Conexión aceptada!
sended: 18
sended: 21
sended: 22
sended: 20
sended: 18
```

El cliente que está subscrito al mismo tema rápidamente comenzará a obtener los valores de temperatura publicados por el cliente anterior
```[bash]
Temperatura: 16
Temperatura: 22
Temperatura: 20
Temperatura: 24
Temperatura: 21
```
