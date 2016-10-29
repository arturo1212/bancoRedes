// CLIENTE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <time.h>
#define MAX_NOMBR 256

void getTime(char *buffer){
time_t timer;
    struct tm* tm_info;
    time(&timer);
    tm_info = localtime(&timer);
    strftime(buffer, 26, "%Y/%m/%d %H:%M", tm_info);
}

int readline(FILE *f, char *buffer){
   char c; 
   int i;

   memset(buffer, 0, MAX_NOMBR);

   for (i = 0; i <MAX_NOMBR; i++)
   {   
      int c = fgetc(f);

      if (!feof(f) && c!='\n') 
      {  

        buffer[i] = c;
      }   
      else
      {   
         //fprintf(stderr, "read_line(): recv returned %d\n", c);
         return -1; 
      }   
   }
   return -1; 
}

void procesar_argumentos(char* srvr, char* port,char* op,char* monto, char* id,char* argv[]){
	for(int i = 1;i<sizeof(argv)-1;i+=2){
		if(argv[i][1]=='d'){
			sprintf(srvr,"%s",argv[i+1]);

		}else if(argv[i][1]=='p'){
			sprintf(port,"%s",argv[i+1]);

		}else if (argv[i][1]=='c'){
			*op = argv[i+1][1];

		}else if (argv[i][1]=='m'){
			sprintf(monto,"%s",argv[i+1]);

		}else if (argv[i][1]=='i'){
			sprintf(id,"%s",argv[i+1]);

		}else{
			fprintf(stderr, "Opcion %s\n invalida.", argv[i]);
			fprintf(stderr, "Uso: bsb_cli -d <ip servidor> -p <puerto> -c <operacion> \
							 -m <monto> -i <identificador>\n" );
			exit(1);
		}
	}
}

int main(int argc, char* argv[]){

	// Declaracion de Variables
	FILE *fp;
	char *nombre,fecha[256],tipo,monto[5],*ipsrvrstr,*puertostr;
	char buffer[1024], id[20], msj[1024], linea[1024];
	
	int clientSocket, total, puerto;	// Socket del cliente

	struct sockaddr_in serverAddr;
	struct timeval tv;
	socklen_t addr_size;

	//Definimos el timeout
	tv.tv_sec = 10;
	tv.tv_usec = 0;

	if (argc != 11){ //Si los argumentos estan incompletos mostrar el uso correcto
		fprintf(stderr, "Uso: bsb_cli -d <ip servidor> -p <puerto> -c <operacion> \
							 -m <monto> -i <identificador>\n" );
		exit(1);
	}else{
		procesar_argumentos(ipsrvrstr,puertostr,&tipo,monto,id,argv);

	}

/*--------------------------Establecer la conexion------------------------*/
	// Creacion del socket.
	if ( (clientSocket = socket(PF_INET, SOCK_STREAM, 0)) == 0){
        perror("Error creando el socket");
        exit(EXIT_FAILURE);		
	}

	// Definimos el timeout en los sockets
	setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	setsockopt(clientSocket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
	//  Colocar los datos del servidor
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(8888);
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

/*------------------------- Conexion con el servidor ---------------------*/

	// Conectarse al servidor. 
	addr_size = sizeof serverAddr;
	if (connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size) < 0){
        perror("Error conectando al servidor");
        exit(EXIT_FAILURE);			
	}
	// Esperar respuesta del servidor.
	memset(buffer, '\0', sizeof buffer);	// Limpiar el buffer
	recv(clientSocket, buffer, 1024, 0);	// Recibir el mensaje.
	printf("Mensaje: %s\n",buffer);					// Mostrar el mensaje.

/*---------------------------------  Transaccion --------------------------*/
	

	// Abrimos el archivo 
	fp = fopen("cajeroV.txt", "w+");

	// Verificar si tengo nombre
	readline(fp,linea);
	if(linea[0]!='\0'){
		sprintf(nombre,"%s",linea);
		readline(fp,linea);
		total = atoi(linea);
	}
	else{
		nombre = "-";
		total = 80000;
	}
	getTime(fecha);
	printf("%s\n",fecha);
	sprintf(msj,"%s|%s|%s|%s|%s|",nombre,fecha,id,tipo,monto);

	printf("Enviare el mensaje");
	send(clientSocket, msj,strlen(msj),0);
	memset(buffer, '\0', sizeof buffer);		// Limpiar el buffer
	if (recv(clientSocket, buffer, 1024, 0)<0){	// Esperar respuesta del servidor 
        perror("No se recibe respuesta");		
        exit(EXIT_FAILURE);			
	}	


  return 0;
}
