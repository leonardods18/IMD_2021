## Kernel driver

### Descripción

Para poder escribir el driver haciendo uso de las API del núcleo i2c, se utiliza, al igual que en el ejemplo de clase, una estructura privada, la cual contiene la estructura que caracteriza el dispositivo i2c y la estructura miscdevice.

```c
/* Private device structure */
struct mpu9250_dev {
	struct i2c_client *client;
	struct miscdevice mse_miscdevice;
	char name[9]; /* msedrvXX */
};
```

Dentro de la función lcd_probe se hace un devm_kzalloc() para generar dinámicamente una instancia de dicha estructura. A esta instancia se le asigna la estructura i2c_client() del dispositivo y se cargan todos los datos necesarios dentro de la estructura miscdevice (nombre del driver, número minor y la estructura de file-operations). La memoria ocupada por la instancia de lcd_dev() es liberada por la función lcd_remove() al momento de remover el módulo.

#### File opearation WRITE

Hace uso de la API i2c_master_send():

```c
/* Funcion para escribir el dispositivo /dev/mseXX utilizando write() en espacio usuario */
static ssize_t lcd_write(struct file *file, const char __user *buffer, size_t len, loff_t *offset)  {

    struct lcd_dev *mse; /* Puntero a la estructura privada del device */
    int wr_stat;
    int error_count = 0;

    /* pr_info("INFO: mpu9250_write() fue invocada.\n"); */

    /* A partir del puntero a file obtengo la estructura completa del device */
    mse = container_of(file->private_data, struct mpu9250_dev, mse_miscdevice);

    error_count = copy_from_user(message ,buffer, len);
    if (error_count != 0) {
        pr_err("ERROR: Error al ejecutar copy_from_user(). Return value: %d\n", error_count);
        return -1;
    }

    wr_stat = i2c_master_send(mse->client,message,len);
    if (wr_stat < 0) {
        pr_err("ERROR: Error al ejecutar i2c_master_send(). Return value: %d\n", wr_stat);
        return wr_stat;
    }

    return wr_stat;
}
```

#### File opearation READ

Hace uso de las APIs i2c_master_send() e i2c_master_recv():

```c
/* Funcion para leer el dispositivo /dev/mseXX utilizando read() en espacio usuario */
static ssize_t lcd_read(struct file *file, char __user *userbuf, size_t count, loff_t *ppos)  {

    struct lcd_dev *mse; /* Puntero a la estructura privada del device */
    int rd_stat;
    int error_count = 0;

    /* pr_info("INFO: lcd_read() fue invocada.\n"); */

    /* A partir del puntero a file obtengo la estructura completa del device */
    mse = container_of(file->private_data, struct mpu9250_dev, mse_miscdevice);

    rd_stat = i2c_master_recv(mse->client, message, count);
    if (rd_stat < 0) {
        pr_err("ERROR: Error al ejecutar i2c_master_recv(). Return value: %d\n", rd_stat);
        return rd_stat;
    }

    error_count = copy_to_user(userbuf, message, count);
    if (error_count != 0) {
        pr_err("ERROR: Error al ejecutar copy_to_user(). Return value: %d\n", error_count);
        return -1;
    }

    return rd_stat;
}
```

### Compilación

1. Abrir una terminal y correr los siguientes comandos:

```
export PATH=$PATH:~/leonardo/ISO_II/toolchain/arm-mse-linux-gnueabihf/bin
export CROSS_COMPILE=arm-linux-
export ARCH=arm
```

2. Ir hasta la carpeta donde se encuentra el driver (esta carpeta)

3. Ejecutar el comando "make".

4. El módulo a cargar es "mylcd.c"

### Insert and remove

Para insertar el módulo:
1. Dentro de la consola de la SBC navegar hasta el directorio donde se encuentra el módulo compilado
2. Ejecutar: "insmod mylcd.ko"
3. Debe observarse el mensaje de que el módulo fue cargado correctamente, el nombre del device y el número minor asignado.

Para remover el módulo:
1. Ejecutar: "rmmod mylcd.ko"
2. Debe observarse un mensaje que anuncia que el módulo fue removido correctamente.

