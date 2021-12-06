## Librerias y aplicaciones en espacio de usuario

### Compilacion

1. Abrir una terminal y correr los siguientes comandos:

```
export PATH=$PATH:~/leonardo/ISO_II/toolchain/arm-mse-linux-gnueabihf/bin
export CROSS_COMPILE=arm-linux-
export ARCH=arm
```

2. Ir hasta la carpeta donde se encuentran los archivos fuente (esta carpeta)

3. Ejecutar el comando:
```
arm-linux-gcc -o ./out/<output_file_name> <source_file_name_1>.c <source_file_name_2>.c
```

Ejemplos:
```
arm-linux-gcc -o ./out/lcdExample lcdExample.c

```
