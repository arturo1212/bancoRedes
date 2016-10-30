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
  
#define MAXc 10

typedef struct Cajeros{
    int total,nombre;
}cajero;

void inicializar (cajero C[]){
    int i = 0;
    for (;i<MAXc;i++){
        C[i].total = 80000;
        C[i].nombre = 0;        
    }
}

int get_index_of(int name, cajero C[]){
    int i = 0;
    for (;i>MAXc;i++){
        if(C[i].nombre == name){
            return i;
        }
    }
    return -1;
}

int get_max_client(cajero C[]){
    int i = 0,max = 0;
    for (;i>MAXc;i++){
        if(max < C[i].nombre ){
            max = C[i].nombre;
        }
    }
    return max;
}

void procesar_transaccion(char *buffer,cajero C[],int sckt_fd, char *depotfile, char * retirfile){
    char nombre[20],fecha[16]/*o 17*/,id[20],tipoc,
         *tipor = "Retiro",
         *tipod = "Deposito";
    char buffer2[20];
    int i,max, nombreint,monto;
    FILE *fd_diario,*fd_deposito,*fd_retiro;
    sscanf(buffer,"%s|%s|%s|%c|%d|",nombre,fecha,id,&tipoc,&monto);
    printf("Estoy procesando una peticion\n");
    puts(buffer);
    //RECORDAR QUE LOS ARCHIVOS DE AQUI SON PARAMENTROS DE LLAMADA
    //A EXCEPCION DEL DEIARIO
    if((fd_diario = fopen("logDiario.txt", "a+") )== NULL){//Ver Cambiar nombre por dia
        perror("Error abriendo log diario.");
        exit(1);
    }
    if(nombre[0]=='-'){
        puts("Asignando Nombre.");
        max = get_max_client(C);
        nombreint = max + 1;
        sprintf(buffer2,"%d",nombreint);
        printf("Nuevo nombre %s\n",buffer2 );
        send(sckt_fd,buffer2,strlen(buffer2),0);
        puts("Nombre enviado");
    }else{
        sscanf(nombre,"%d",&nombreint);
    }
    printf("Tipo de trans: %c\n",tipoc);;
    if (tipoc == 'r'){ //Retiros
        puts("Efectuando retiro");
        i = get_index_of(nombreint,C);
        C[i].nombre = nombreint;
        if(C[i].total > 5000){
            //Enviamos mensaje de confirmacion
            if (send(sckt_fd,"y",strlen("y"),0) != strlen("y")){
                perror("Fallo en envio de confirmacion retiro.");
                exit(1);
            }
            if((fd_retiro = fopen(retirfile, "a+") )== NULL){
                perror("Error abriendo log deposito.");
                exit(1);
            }
            //Escribimos en la bitacora
            fprintf(fd_diario,"%s por el usurio %s de monto %d el %s en el cajero %d\n",tipor,id,monto,fecha,C[i].nombre);
            fprintf(fd_retiro,"%s por el usurio %s de monto %d el %s en el cajero %d\n",tipor,id,monto,fecha,C[i].nombre);
            fclose(fd_retiro);
            //Actualizamos total
            C[i].total -= monto;
        }else{ // NO HAY RIAL
            if (send(sckt_fd,"n",strlen("y"),0) != strlen("n")){
                perror("Fallo en envio de negacion retiro.");
                exit(1);
            }
            //Negado y mandamos rial (?)
        }
    }else if (tipoc == 'd'){//Deposito
        i = get_index_of(nombreint,C);
        C[i].nombre = nombreint;
        if (send(sckt_fd,"y",strlen("y"),0) != strlen("y")){
                perror("Fallo en envio de confirmacion deposito.");
                exit(1);
            }
        if((fd_deposito = fopen(depotfile, "a+") )== NULL){
            perror("Error abriendo log deposito.");
            exit(1);
        }
        fprintf(fd_diario,"%s por el usuario %s de monto %d el %s en el cajero %d\n",tipod,id,monto,fecha,C[i].nombre);
        fprintf(fd_deposito,"%s por el usuario %s de monto %d el %s en el cajero %d\n",tipod,id,monto,fecha,C[i].nombre);
        C[i].total += monto;
        fclose(fd_deposito);
    }
    fclose(fd_diario);
    //readline(fp,linea);
    //if(linea[0]!='\0'){
    //    sprintf(nombre,"%s",linea);
        //readline(fp,linea);
    //}
}

void procesarArgumentos(char* argv[],char *port, char *depotfile, char *retirfile){
    for(int i = 1;i<=5;i+=2){
        if(argv[i][1]=='l'){
            sprintf(port,"%s",argv[i+1]);
        }
        else if((argv[i][1]=='i')){
            sprintf(depotfile,"%s",argv[i+1]);           
        }            

        else if(argv[i][1]=='o'){
            sprintf(retirfile,"%s",argv[i+1]);
        }
    }
}

int main(int argc , char *argv[])
{
    /*---------------------- Declaracion de Variables -------------------*/
    //ARREGLO DE CLIENTES
    cajero clientes[MAXc];

    // ENTEROS
    int opt = 1;
    int masterS , addrlen , new_socket, PORT;
    int clientS[MAXc];
    int max_sd, activity, i , valread , sd;

    // STRINGS Y CARACTERES 
    char buffer[1025], depotfile[1025], retirfile[1025], port[10];
    char *msj = "Conexion Satisfactoria. \n";

    // DESCRIPTORES.
    fd_set readfds;

    // SOCKETS
    struct sockaddr_in address;

    /*-------------------------- Argumentos ------------------------------*/
    if (argc != 7){
        fprintf(stderr, "Uso: bsb_srvr -l <puerto> -i <bitacora depositos> -c <bitacora retiro>\n");
        exit(1);
    }
    procesarArgumentos(argv, port, depotfile, retirfile);
    PORT = atoi(port); 
    puts("Pase por aqui");


    /*----------------------- Acondicionar el entorno --------------------*/
    for (i = 0; i < MAXc; i++){
        clientS[i] = 0;
    }
      
    // SOCKET:Creacion de socket maestro
    if( (masterS = socket(AF_INET , SOCK_STREAM , 0)) == 0) {
        perror("Error en socket.");
        exit(EXIT_FAILURE);
    }
    // Propiedades socket maestro
    if( setsockopt(masterS, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ){
        perror("Error en config del socket.");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;           // Familia
    address.sin_addr.s_addr = INADDR_ANY;   // Todas mis interfaces.
    address.sin_port = htons(PORT);         // Puerto especificado.
      
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
    
    /*----------------------------- Monitorear --------------------------*/
    while(1){
        FD_ZERO(&readfds);          // Limpiar el conjunto de sockets
        FD_SET(masterS, &readfds);  // Agregar socket maestro
        max_sd = masterS;           // Actualizar descriptor mas grande

        for ( i = 0 ; i < MAXc ; i++){          
            sd = clientS[i];                    // Tomar descriptor del socket
            if(sd > 0){FD_SET( sd , &readfds);} // Agregar sockets activos (Clientes)
            if(sd > max_sd){max_sd = sd;}       // Actualizar max fd
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
                             NULL , NULL , NULL);
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
            printf("Vamo a procesar");
            procesar_transaccion(buffer,clientes,sd,depotfile,retirfile);
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
                printf("Hubo actividad");
                if ((valread = read( sd , buffer, 1024)) == 0){
                    close( sd );
                    clientS[i] = 0;
                }
                else
                {   
                    procesar_transaccion(buffer,clientes,sd,depotfile,retirfile);
                    //buffer[valread] = '\0';
                    //puts(buffer);
                    //send(sd , buffer , strlen(buffer) , 0 );
                }
            }
        }
    }
      
    return 0;
} 
