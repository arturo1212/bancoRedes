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

int resolvDir(char* dir, char* ret){
    struct addrinfo* result;
    struct addrinfo* res;
    int error;
    if (getaddrinfo(dir, NULL, NULL, &result) != 0){
        perror("no pude resolver");
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


void procesar_argumentos(char* srvr, char* port,char* op,char* monto, char* id,char* argv[]){
	for(int i = 1;i<=9;i+=2){
		printf("%d %s\n", i, argv[i]);
		if(argv[i][1]=='d'){
			printf("Entre en el if\n");
			srvr = (char *)malloc(sizeof(argv[i+1]));
			sprintf(srvr,"%s",argv[i+1]);
			printf("%s\n", srvr);

		}else if(argv[i][1]=='p'){
			port = (char *)malloc(sizeof(argv[i+1]));
			sprintf(port,"%s",argv[i+1]);

		}else if (argv[i][1]=='c'){
			*op = argv[i+1][1];

		}else if (argv[i][1]=='m'){
			monto = (char *)malloc(sizeof(argv[i+1]));
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
	/*---------------------- Declaracion de Variables -------------------------*/
	/* STRINGS Y CARACTERES
	  	ipsrvrstr : ip o nombre del servidor
		puertostr : puerto del servidor
		tipo	  : tipo de transaccion
		monto	  : monto de la transaccion
		id        : id del usuario
		nombreReal: ip definitiva
	*/
	char *ipsrvrstr,*puertostr;
	char tipo, monto[5],id[20], nombreReal[256],fecha[256];
	char buffer[1024], msj[1024], linea[1024];	
	char *nombre;

	// SOCKETS
	struct sockaddr_in serverAddr;
	struct timeval tv;
	socklen_t addr_size;

	// TIMEOUT
	tv.tv_sec = 10;
	tv.tv_usec = 0;
	
	// ARCHIVOS
	FILE *fp;

	// ENTEROS
	int clientSocket, total, puerto;	// Socket del cliente

	/*------------------------------ VERIFICACIONES ---------------------------*/
	// Numero correcto de argumentos
	if (argc != 11){
		fprintf(stderr, "Uso: bsb_cli -d <ip servidor> -p <puerto> -c <operacion> \
							 -m <monto> -i <identificador>\n" );
		exit(1);
	}
	// Obtener los argumentos del terminal, la ip del servidor y la fecha
	procesar_argumentos(ipsrvrstr,puertostr,&tipo,monto,id,argv);
	resolvDir(ipsrvrstr, nombreReal);
	getTime(fecha);


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
	serverAddr.sin_port = htons(8888);								// Cambiar puerto tambien
	serverAddr.sin_addr.s_addr = INADDR_ANY;						// CAMBIAR AQUI
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

	/*------------------------- Conexion con el servidor ---------------------*/

	// Conectarse al servidor. 
	addr_size = sizeof serverAddr;
	if (connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size) < 0){
        perror("Error conectando al servidor");
        exit(EXIT_FAILURE);			
	}

	// Esperar respuesta del servidor.
	memset(buffer, '\0', sizeof buffer);			// Limpiar el buffer
	recv(clientSocket, buffer, 1024, 0);			// Recibir el mensaje.
	printf("Mensaje: %s\n",buffer);					// Mostrar el mensaje.

	
	/*------------------------------ Revision de nombre ----------------------*/
	// Abrimos el archivo 
	fp = fopen("cajeroV.txt", "w+");
	readline(fp,linea);
	if(linea[0]!='\0'){
		sprintf(nombre,"%s",linea);
		//readline(fp,linea);
	}
	else{
		nombre = "-";
	}
	printf("Mi nombre es: %s\n", nombre );

	/*---------------------------------  Transaccion --------------------------*/
	memset(buffer, '\0', sizeof buffer);						// Limpieza del buffer
	sprintf(msj,"%s|%s|%s|%d|%s|",nombre,fecha,id,tipo,monto);  // Creacion del mensaje
	send(clientSocket, msj,strlen(msj),0);						// Enviar mensaje
	printf("Esperando respuesta del servidor...\n");			// Mensaje al Usuario.
	// Esperar respuesta del servidor 	
	if (recv(clientSocket, buffer, 1024, 0)<0){	
        perror("No se recibe respuesta");		
        exit(EXIT_FAILURE);			
	}
	// Mostrar al usuario la respuesta del servidor.
	if (buffer[1] == 'y'){
		printf("Transaccion realizada con exito!\n");
		//mostrar mensaje con cosas que hacen falta.
	}	
	else{
		printf("No hay dinero disponible actualmente.");
	}

  return 0;
}
