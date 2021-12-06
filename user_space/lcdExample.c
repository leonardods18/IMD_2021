/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "mylcd.h"

#define BUFFER_LENGTH 81               ///< The buffer length (crude but fine)

static char receive[BUFFER_LENGTH];     ///< The receive buffer from the LKM
char buffer[3];

int main()
{
    int ret, fd;
    char stringToSend[BUFFER_LENGTH] =  "Hola Leonardo       ";
    char stringToSend2[BUFFER_LENGTH] = "IMD 6ta Cohorte     ";
    char stringToSend3[BUFFER_LENGTH] = "2021                ";
    char stringToSend4[BUFFER_LENGTH] = "Tucuman";
    char ch;

    printf("Conectando con el dispositivo...\n");
    /* Abrimos el dispositivo */
    fd = open("/dev/LCD_HD44780", O_RDWR);             
    if (fd < 0)
    {
        perror("No hay dispositivo...");
        return errno;
    }

    /* Escribimos en el dispositivo */
    ret = write(fd, stringToSend, strlen(stringToSend)); 

    if (ret < 0)
    {
        perror("Falla al escribir el mensaje.");
        return errno;
    }
	
    printf("Leyendo del dispositvo...\n");
    /* Leemos 3 caracteresd el dispositivo */
    ret = read(fd, receive, 3);       
    if (ret < 0){
        perror("Falla al leer el dispositivo.");
        return errno;
    }
    

    printf("Leyendo el dispositivo...\n");
    /* Leemos 5 caracteresd el dispositivo */
    ret = read(fd, receive, 5);       
    if (ret < 0){
        perror("Falla al leer el dispositivo.");
        return errno;
   }
   
    
    /* Menu */
    while(1)
    {
        printf("<IMD 6ta cohorte>\n");
	printf("Elija una opción:\n");
	printf("------------------------\n");
        printf("1: Saludo:\n");
        printf("2: Materia:\n");
        printf("3: año:\n");
        printf("4: Provincia:\n");
        printf("5: Lectura pos\n");
        printf("6: Reset\n");   
        printf("7: Apago pantalla\n");     
        printf("e: Salir\n");
	printf("------------------------\n");
        scanf("%c", &ch);    
	
        /* Ejecutamos la opcion enviada por el usuario */
        switch(ch)
        {
            case '1':
               
		ret = write(fd, stringToSend, strlen(stringToSend)); 

            break;
          
	   case '2':
               
		ret = write(fd, stringToSend2, strlen(stringToSend2)); 

            break;
            
	   case '3':
               
		ret = write(fd, stringToSend3, strlen(stringToSend3)); 

            break;

   
	   case '4':
               
		ret = write(fd, stringToSend4, strlen(stringToSend4)); 

            break;



 
            case '5':
                if(0 > ioctl(fd, LCD_IOCTL_GETPOSITION, buffer))
                {
                    perror("Failed to ioctl\n");
                    return errno;
                }
                else
                {
                    printf("Ultima posicion -->  %d %d\n\n", buffer[0], buffer[1]);
                }
            break;

            case '6':
                buffer[0] = '1';

                if(0 > ioctl(fd, LCD_IOCTL_RESET, buffer))
                {
                    perror("Failed to ioctl\n");
                    return errno;
                }
            break;               

            case '7':
                buffer[0] = '0';
                if(0 > ioctl(fd, LCD_IOCTL_SETBACKLIGHT, buffer))
                {
                    perror("Failed to ioctl\n");
                    return errno;
                }
            break;
       
            case 'e':
                printf("End of the program\n\n");
                close(fd); 
                return 0;
            break;
        }

    }

    return 0;
}
/*==================[end of file]============================================*/
