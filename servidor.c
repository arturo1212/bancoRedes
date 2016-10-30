/**
    Handle multiple socket connections with select and fd_set on Linux
*/
  
#include <stdio.h>
#include <string.h>   //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
  
#define PORT 8888
#define MAXc 10

typedef struct Cajeros{

}
int main(int argc , char *argv[])
{
    /*---------------------- Declaracion de Variables -------------------*/
    // ENTEROS
    int opt = 1;
    int masterS , addrlen , new_socket;
    int clientS[MAXc];
    int max_sd, activity, i , valread , sd;

    
    // STRINGS Y CARACTERES 
    char buffer[1025];
    char *msj = "Conexion Satisfactoria. \n";

    // DESCRIPTORES.
    fd_set readfds;

    // SOCKETS
    struct sockaddr_in address;   

  
    for (i = 0; i < MAXc; i++) 
    {
        clientS[i] = 0;
    }
      
    // Creacion de socket maestro
    if( (masterS = socket(AF_INET , SOCK_STREAM , 0)) == 0) {
        perror("Error en socket.");
        exit(EXIT_FAILURE);
    }
    // Propiedades socket maestro
    if( setsockopt(masterS, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ){
        perror("Error en config del socket.");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);       
      
    // BIND: Asociamos el socket a la direccion especificada. 
    if (bind(masterS, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Error en bind.");
        exit(EXIT_FAILURE);
    }
    printf("Escuchando el puerto %d \n", PORT);
     
    // LISTEN: Numero maximo de conexiones y escuchar.
    if (listen(masterS, MAXc) < 0){
        perror("Error en listen.");
        exit(EXIT_FAILURE);
    }
       
    // Guardar el largo de la direccion para otras funciones.
    addrlen = sizeof(address);
    puts("Esperando conexiones");
    
    // Algoritmo principal.
    while(1){
       /*
         Limpiamos el conjunto de sockets
         Agregamos el socket del master al conjunto
         Actualizamos el descriptor mas grande.
        */
        FD_ZERO(&readfds);
        FD_SET(masterS, &readfds);
        max_sd = masterS;
         
        /*
         Agregamos los sockets de los clientes.
            (Solo agregamos los sockets activos)
         Tambien actualizamos el descriptor mas grande.
        */
        for ( i = 0 ; i < MAXc ; i++){
            sd = clientS[i];
            if(sd > 0)
                FD_SET( sd , &readfds);
            if(sd > max_sd)
                max_sd = sd;
        }
        /*
            Despues de tener todos los sockets agregados,
            los monitoreamos hasta detectar actividad en
            alguno de ellos. Si ocurre un error en el
            monitoreo, devuelve -1.
            NOTA: Como no especificamos timeout, espera
            de manera indefinida, es decir, es bloqueante.
        */  
        activity = select( max_sd + 1 , &readfds,
                             NULL , NULL , NUL L);
        if ((activity < 0) && (errno!=EINTR)){
            printf("Error en select.");
        }
          
        // Revisar si hay una conexion nueva.
        if (FD_ISSET(masterS, &readfds)){
            if ((new_socket = accept(masterS, 
                (struct sockaddr *)&address, 
                (socklen_t*)&addrlen)) < 0){
                perror("Error en Accept");
                exit(EXIT_FAILURE);
            }
          
            // Mostrar la nueva conexion.
            printf("Nueva conexion , IP: %s , PUERTO: %d \n",
                    inet_ntoa(address.sin_addr) , 
                    ntohs(address.sin_port));
        
            // Enviar mensaje de confirmacion.
            if( send(new_socket, msj, strlen(msj), 0) != strlen(msj) )  {
                perror("Error enviando mensaje.");
            }
            // Agregar el socket a la lista.
            for (i = 0; i < MAXc; i++){
                if( clientS[i] == 0 ){
                    clientS[i] = new_socket;
                    break;
                }
            }
        }
          
        // Revisar las operaciones en otros sockets
        for (i = 0; i < MAXc; i++){
            sd = clientS[i];
              
            if (FD_ISSET( sd , &readfds)){
                if ((valread = read( sd , buffer, 1024)) == 0){
                    close( sd );
                    clientS[i] = 0;
                }
                else
                {
                    buffer[valread] = '\0';
                    puts(buffer);
                    send(sd , buffer , strlen(buffer) , 0 );
                }
            }
        }
    }
      
    return 0;
} 
