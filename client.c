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
#include <netdb.h>

#define   MAX_NOMBR 256
#ifndef   NI_MAXHOST
#define   NI_MAXHOST 1025
#endif

void getTime(char *buffer){
	time_t timer;
    struct tm* tm_info;
    time(&timer);
    tm_info = localtime(&timer);
    strftime(buffer, 26, "%Y/%m/%d-%H:%M", tm_info);
}

int numRetiros(char * filename,char* id){
  int  result = 0;
  char buffer[10];
  FILE *fp;
  fp = fopen(filename, "a+");
  while(fscanf(fp,"%s\n", buffer) > 0){
      if(strcmp(buffer,id)==0){
        result += 1;
      }    
  }
  fclose(fp);
  return result;
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

int resolvDir(char* dir, char* ret){
    struct addrinfo* result;
    struct addrinfo* res;
    int status;
    int error;
    if ( (status = getaddrinfo(dir, NULL /*HACE FALTA PUERTO*/, NULL, &result)) != 0){
        fprintf(stderr, "Error getaddrinfo() : %s\n",gai_strerror(status) );
        exit(EXIT_FAILURE);
        return -1;
    } 
    for (res = result; res != NULL; res = res->ai_next) {   
        char hostname[NI_MAXHOST];
        error = getnameinfo(res->ai_addr, res->ai_addrlen, hostname, NI_MAXHOST, NULL, 0, 0); 
        if (error != 0) {
            fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(error));
            continue;
        }
        if (*hostname != '\0')
            sprintf(ret,"%s",hostname);
            return 0;
    }   
}

void writeLine(char * filename,char* id){
	int  result = 0;
  	char buffer[10];
  	FILE *fp;
	fp = fopen(filename, "a+");
	fprintf(fp, "%s\n", id);
	fclose(fp);
}


void procesar_argumentos(char* srvr, char* port,char* op,char* monto, char* id,char* argv[]){
	for(int i = 1;i<=9;i+=2){
		if(argv[i][1]=='d'){
			sprintf(srvr,"%s",argv[i+1]);
		}else if(argv[i][1]=='p'){
			sprintf(port,"%s",argv[i+1]);

		}else if (argv[i][1]=='c'){
			sprintf(op,"%s",argv[i+1]);

		}else if (argv[i][1]=='m' && strlen(argv[i+1]) <= 5){
			sprintf(monto,"%s",argv[i+1]);

		}else if (argv[i][1]=='i' && strlen(argv[i+1]) <= 20){
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
	/*---------------------- Declaracion de Variables -------------------------*/
	/* STRINGS Y CARACTERES
	  	ipsrvrstr : ip o nombre del servidor
		puertostr : puerto del servidor
		tipo	  : tipo de transaccion
		monto	  : monto de la transaccion
		id        : id del usuario
		nombreReal: ip definitiva
	*/
	char ipsrvrstr[256],puertostr[256];
	char tipo[5], monto[5],id[20], nombreReal[256],fecha[256];
	char buffer[1024], msj[1024], linea[1024];	
	char nombre[256];
	char *localhost = "127.0.0.1";
	// SOCKETS
	struct sockaddr_in serverAddr;
	struct timeval tv;
	socklen_t addr_size;

	// TIMEOUT
	tv.tv_sec = 10;
	tv.tv_usec = 0;
	
	// ARCHIVOS
	FILE *fp, *retiros;

	// ENTEROS
	int clientSocket, total, puerto,PORT;	// Socket del cliente

	/*------------------------------ VERIFICACIONES ---------------------------*/
	// Numero de argumentos
	if (argc != 11){
		fprintf(stderr, "Uso: bsb_cli -d <ip servidor> -p <puerto> -c <operacion> \
							 -m <monto> -i <identificador>\n" );
		exit(1);
	}
	// Obtener argumentos
	procesar_argumentos(ipsrvrstr,puertostr,tipo,monto,id,argv);
	if(strcmp(ipsrvrstr,localhost)){
		resolvDir(ipsrvrstr, nombreReal);		
	}
	else{
		sprintf(nombreReal,"%s",localhost);
	}

	
	// FECHA
	getTime(fecha);

	// RANGOS
	printf("TIPO: %s\n", tipo);
	/*
	if( strcmp(tipo, "r") && strcmp(tipo, "d")){
		printf("Opcion Incorrecta.");
		exit(0);			
	}
	*/
	if(atoi(monto)> 3000 || atoi(monto)<=0){
		printf("Monto Invalido.");
		exit(0);	
	}

	// Ver, si es un retiro, si es el cuarto.
	if(numRetiros("retiros.txt",id) >= 3 && tipo[0] == 'r'){
		printf("No puede realizar mas retiros por hoy.");
		exit(0);	
	}
	// Si es un retiro, agregarlo a la lista.

	if(tipo[0] == 'r'){
		writeLine("retiros.txt",id);
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
	serverAddr.sin_port = htons(atoi(puertostr));								// Cambiar puerto tambien
	serverAddr.sin_addr.s_addr = inet_addr(nombreReal);						// CAMBIAR AQUI
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

	/*------------------------- Conexion con el servidor ---------------------*/

	// Conectarse al servidor. 
	addr_size = sizeof serverAddr;
	if (connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size) < 0){
        perror("Error conectando al servidor");
        exit(EXIT_FAILURE);			
	}
	/*------------------------------ Revision de nombre ----------------------*/
	// Abrimos el archivo 
	fp = fopen("cajeroV.txt", "a+");
	readline(fp,linea);
	if(linea[0]!='\0'){
		printf("TIPO: %s\n", tipo);
		sprintf(nombre,"%s",linea);
		//readline(fp,linea);
	}
	else{
		sprintf(nombre,"0");
	}
	fclose(fp);
	/*---------------------------------  Transaccion --------------------------*/
	sprintf(msj,"%s | %s | %s | %s | %s",nombre,fecha,id,tipo,monto);  // Creacion del mensaje
	send(clientSocket, msj,strlen(msj),0);						// Enviar mensaje
	// Esperar respuesta del servidor 	
	// Esperar respuesta del servidor.
	if (nombre[0]=='0'){
		memset(buffer, '\0', sizeof buffer);			// Limpiar el buffer
		puts("No tengo nombre");
		if (recv(clientSocket, buffer, 1024, 0)<0){			// Recibir el mensaje.
			perror("No se recibe respuesta");		
        	exit(EXIT_FAILURE);
    	}
    	puts("Ya tengo nombre");
		sprintf(nombre,"%s",buffer);
		writeLine("cajeroV.txt",nombre);
	}
	printf("Mi nombre es: %s\n", nombre );
	
	//Esperar respuesta del servidor
	if (recv(clientSocket, buffer, 1024, 0)<0){	
        perror("No se recibe respuesta");		
        exit(EXIT_FAILURE);			
	}
	printf("Mensaje: %s\n",buffer);			

	// Mostrar al usuario la respuesta del servidor.
	if (buffer[0] == 'y'){
		printf("Transaccion realizada con exito!\n");
		printf("Fecha: %s\n Operacion: %s\n ID: %s\n Monto: %s\n",
				 fecha, tipo, id, monto );
	}	
	else{
		printf("No hay dinero disponible actualmente.");
	}

  return 0;
}
