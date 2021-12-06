/*==================[inclusions]=============================================*/
#include "mylcd.h"
#include <linux/init.h>           
#include <linux/module.h>          
#include <linux/device.h>         
#include <linux/kernel.h>          
#include <linux/fs.h>             
#include <linux/uaccess.h>          
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/mutex.h>
/*==================[macros and definitions]=================================*/
/**
* @def LCD_ADDRESS_SIZE
* @brief Cantidad de datos del arreglo de direcciones de los tipos de topologias de LCD
*/
#define LCD_ADDRESS_SIZE    6

/**
* @def DEVICE_NAME
* @brief Nombre del dispositivo
*/
#define DEVICE_NAME "LCD_HD44780"    

/**
* @def CLASS_NAME
* @brief Nombre de la clase
*/
#define CLASS_NAME  "i2c"          

/**
* @def LCD_STRING_SIZE
* @brief Tamaño maximo de un string enviado al lcd
*/
#define LCD_STRING_SIZE 81  

/**
* @def LCD_BUFFER_SIZE
* @brief Buffer del LCD
* @note 20 columns * 4 rows + 4 chars extra
*/
#define LCD_BUFFER_SIZE 0x68 


/**
* @def lcdTopology_t
* @brief Posibles topologias del LCD
*/
typedef enum{
    LCD_TOPOLOGY_40x2 = 0
,   LCD_TOPOLOGY_20x4
,   LCD_TOPOLOGY_20x2
,   LCD_TOPOLOGY_16x4
,   LCD_TOPOLOGY_16x2
,   LCD_TOPOLOGY_8x2
,   LCD_INVALID_TOPOLOGY
}lcdTopology_t;

/**
* @def lcdPinout_t
* @brief Pinout del lcd
*/
typedef enum 
{
    LCD_PIN_RS = 0
,   LCD_PIN_RW
,   LCD_PIN_EN
,   LCD_PIN_BL          //Backlight
,   LCD_PIN_DB4
,   LCD_PIN_DB5
,   LCD_PIN_DB6
,   LCD_PIN_DB7
}lcdPinout_t;             

/**
* @def lcdCmd_t
* @brief Comandos del lcd
*/
typedef enum 
{
    LCD_CMD_CLEARDISPLAY    = 1
,   LCD_CMD_HOME            = 2    
,   LCD_CMD_ENTRYMODE       = 4
,   LCD_CMD_DISPLAYCONTROL  = 8
,   LCD_CMD_DISPLAYSHIFT    = 16
,   LCD_CMD_FUNCTIONSET     = 32
,   LCD_CMD_SETCGRAMADDR    = 64
,   LCD_CMD_SETDDRAMADDR    = 128
}lcdCmd_t;

/**
* @def lcdDisplayControl_t
* @brief Valores para comandar del display
*/
typedef enum
{
    LCD_BLINK = 1
,   LCD_CURSOR = 2
,   LCD_DISPLAY = 4
}lcdDisplayControl_t;

/**
* @def lcdCmdDisplayControl_t
* @brief Comandos del display control del lcd
*/
typedef enum
{
    LCD_CURSOR_BLINK_ON     = LCD_CMD_DISPLAYCONTROL | LCD_BLINK
,   LCD_CURSOR_BLINK_OFF    = LCD_CMD_DISPLAYCONTROL
,   LCD_CURSOR_ON           = LCD_CMD_DISPLAYCONTROL | LCD_CURSOR
,   LCD_CURSOR_OFF          = LCD_CMD_DISPLAYCONTROL
,   LCD_DISPLAY_ON          = LCD_CMD_DISPLAYCONTROL | LCD_DISPLAY
,   LCD_DISPLAY_OFF         = LCD_CMD_DISPLAYCONTROL
}lcdCmdDisplayControl_t;

/**
* @def lcdCmdFunctionSet_t
* @brief Comandos del function set del lcd
*/
typedef enum
{
    LCD_8BIT_DATA   = LCD_CMD_FUNCTIONSET | (1 << 4)
,   LCD_4BIT_DATA   = LCD_CMD_FUNCTIONSET
,   LCD_1LINE       = LCD_CMD_FUNCTIONSET
,   LCD_2LINES      = LCD_CMD_FUNCTIONSET | (1 << 3)
,   LCD_10x5FONT    = LCD_CMD_FUNCTIONSET | (1 << 2)
,   LCD_8x5FONT     = LCD_CMD_FUNCTIONSET
}lcdCmdFunctionSet_t;

/**
* @def lcdDataType_t
* @brief Tipo de datos enviados al lcd
*/
typedef enum 
{
    LCD_COMMAND_TYPE = 0        
,   LCD_DATA_TYPE             
}lcdDataType_t;


/**
* @struct lcdOrganization_t
* @brief Organizacion del lcd
*/
typedef struct
{
    u8 columns;                 /**< Cantidad total de columnas */
    u8 rows;                    /**< Cantidad total de filas */
    u8 addresses[4];            /**< Arreglo con las direcciones de las filas */
    lcdTopology_t topology;     /**< Tipo de topolgia */
}lcdOrganization_t;

/**
* @struct lcdData_t
* @brief Data de interes del driver del lcd
*/
typedef struct
{
    struct i2c_client * client;     
    struct class * lcdClass;
    struct device * lcdDevice;
    struct semaphore sem;
    int    majorNumber;
    u16    devOpenCnt;

    lcdOrganization_t org;
    u8 backlight;
    u8 cursor;
    u8 blink;
    u8 column;
    u8 row;
    u8 displaycontrol;
    u8 displayfunction;
}lcdData_t;
/*==================[internal data declaration]==============================*/
/**
* @var static const u8 lcdAddress[LCD_INVALID_TOPOLOGY][LCD_ADDRESS_SIZE]
* @brief Posibles direcciones de las distintas topologias de lcd
*/
static const u8 lcdAddress[LCD_INVALID_TOPOLOGY][LCD_ADDRESS_SIZE] = 
{
    {0x00, 0x40, 0x00, 0x00, 40, 2}                 /* LCD_TOPOLOGY_40x2 */
,   {0x00, 0x40, 0x14, 0x54, 20, 4}                 /* LCD_TOPOLOGY_20x4 */
,   {0x00, 0x40, 0x00, 0x00, 20, 2}                 /* LCD_TOPOLOGY_20x2 */
,   {0x00, 0x40, 0x10, 0x50, 16, 4}                 /* LCD_TOPOLOGY_16x4 */
,   {0x00, 0x40, 0x00, 0x00, 16, 2}                 /* LCD_TOPOLOGY_16x2 */
,   {0x00, 0x40, 0x00, 0x40, 8,  2}                 /* LCD_TOPOLOGY_8x2  */
};

/**
* @var static const struct i2c_device_id mylcd_i2c_id[]
* @brief
*/
static const struct i2c_device_id mylcd_i2c_id[] = 
{
    { "mylcd", 0 },
    { }
};

MODULE_DEVICE_TABLE(i2c, mylcd_i2c_id);

/**
* @var static const struct of_device_id mylcd_of_match[]
* @brief
*/
static const struct of_device_id mylcd_of_match[] = 
{
    { .compatible = "mse,mylcd" },
    { }
};

MODULE_DEVICE_TABLE(of, mylcd_of_match);

static DEFINE_MUTEX(g_mutex);

/**
* @var static lcdData_t lcdData
* @brief Estructura de datos con los datos de interes del driver
*/
static lcdData_t lcdData;
/*==================[internal functions declaration]=========================*/

/*==================[internal data definition]===============================*/

/*==================[external data definition]===============================*/
static int mylcd_init(void);
static void mylcd_exit(void);
/*==================[internal functions definition]==========================*/
/**
* @fn static void lcdWrite(u8 data)
* @param data : Dato a enviar al lcd
* @brief Escribe en el bus i2c un dato a enviar al lcd
* @return nada
*/
static void lcdWrite(u8 data)
{
    data |= lcdData.backlight ? (1 << LCD_PIN_BL) : 0;
    i2c_master_send(lcdData.client, &data, 1);
}

/**
* @fn static void lcdToggleEn(u8 data)
* @param data : Dato a enviar al lcd
* @brief Habilita y deshabilita el pin de enable en el envio de datos
* @return nada
*/
static void lcdToggleEn(u8 data)
{
    lcdWrite(data | (1 << LCD_PIN_EN));
    udelay(1);
    lcdWrite(data & (~(1 << LCD_PIN_EN)));
    udelay(50);
}

/*
* @fn static void lcdSend(u8 data, u8 mode)
* @param data : Dato a enviar al lcd
* @param mode : Modo(incluye bit rs, rw, backlight, entre otros)
* @brief Envia 1 byte en 2 tandas de 4 bits
* @return nada
*/
static void lcdSend(u8 data, u8 mode)
{
    u8 highNibble = data & 0xF0;
    u8 lowNibble = data << 4;

    lcdToggleEn(highNibble | mode);
    lcdToggleEn(lowNibble | mode);
}

/*
* @fn static void lcdSend(u8 data)
* @param data : Dato a enviar al lcd
* @brief Envia 1 comando al lcd
* @return nada
*/
static void lcdSendCommand(u8 data)
{
    lcdSend(data, LCD_COMMAND_TYPE);
}

/*
* @fn static void lcdWriteChar(u8 data)
* @param data : Dato a enviar al lcd
* @brief Escribe un caracter en el lcd teniendo en cuenta la posicion actual
* @return nada
*/
static void lcdWriteChar(u8 data)
{

    lcdSend(data, (1 << LCD_PIN_RS));
}

/*
* @fn static void lcdSetCursor(u8 column, u8 row)
* @param column : Columna a setear
* @param row : Fila a setear
* @brief Posiciona el curso en columna y fila dadas
* @note Si el valor es muy grande, la asignacion la hace de forma circular
* @return nada
*/
static void lcdSetCursor(u8 column, u8 row)
{
    lcdData.column = (column >= lcdData.org.columns ? 0 : column);
    lcdData.row = (row >= lcdData.org.rows ? 0 : row);
    lcdSendCommand(LCD_CMD_SETDDRAMADDR | ((lcdData.column % lcdData.org.columns) + lcdData.org.addresses[(lcdData.row % lcdData.org.rows)]));  
}

/*
* @fn static void lcdSetBacklight(u8 backlight)
* @param backlight : Flag de seteo de luz de fondo
* @brief Enciende o apaga la luz de fondo del lcd
* @return nada
*/
static void lcdSetBacklight(u8 backlight)
{
    lcdData.backlight = backlight;
    lcdWrite((lcdData.backlight ? (1 << LCD_PIN_BL) : 0));
}

/*
* @fn static void lcdCursor(u8 cursor)
* @param cursor : Flag de seteo de cursor
* @brief Enciende o apaga el cursor
* @return nada
*/
static void lcdCursor(u8 cursor)
{
    if (cursor)
        lcdData.displaycontrol |= LCD_CURSOR;
    else
        lcdData.displaycontrol &= ~LCD_CURSOR;

    lcdSendCommand(lcdData.displaycontrol);
}

/*
* @fn static void lcdBlink(u8 blink)
* @param blink : Flag de seteo de blinking del cursor
* @brief Enciende o apaga el blinking del cursor
* @return nada
*/
static void lcdBlink(u8 blink)
{
    if (blink)
        lcdData.displaycontrol |= LCD_BLINK;
    else
        lcdData.displaycontrol &= ~LCD_BLINK;

    lcdSendCommand(lcdData.displaycontrol);
}

/*
* @fn static void lcdHome(void)
* @param Ninguno
* @brief Retorna el cursor al home, que en general es el 0,0
* @return nada
*/
static void lcdHome(void)
{
    lcdData.column = 0;
    lcdData.row = 0;
    lcdSendCommand(LCD_CMD_HOME);
    mdelay(2);
}

/*
* @fn static void lcdClear(void)
* @param Ninguno
* @brief Limpia el lcd y el buffer interno
* @return nada
*/
static void lcdClear(void)
{
    lcdSendCommand(LCD_CMD_CLEARDISPLAY);
    mdelay(2);
}

/*
* @fn static void lcdDeInit(void)
* @param Ninguno
* @brief Apaga el lcd
* @return nada
*/
static void lcdDeInit(void)
{
    lcdSetBacklight(0);
    lcdClear();
    lcdSendCommand(LCD_DISPLAY_OFF | LCD_CURSOR_OFF | LCD_CURSOR_BLINK_OFF);
}

/*
* @fn static void lcdRead(u8 *pData)
* @param pData : puntero a donde almacenar el byte leido
* @brief Lee un byte desde la posicion actual del cursor de la ddram
* @return nada
* @note Usar lcdSetCursor antes para elegir que posicion leer
*/
static void lcdRead(u8 *pData)
{
    u8 data, ch = 0;
    
    /* Enviamos BL, R/W y RS en alto y toggleamos el EN */
    data = 0xf0 | (1 << LCD_PIN_BL) | (1 << LCD_PIN_RW) | (1 << LCD_PIN_RS); 
    i2c_master_send(lcdData.client, &data, 1);
    udelay(100);
    data = 0xf0 | (1 << LCD_PIN_BL) | (1 << LCD_PIN_EN) | (1 << LCD_PIN_RW) | (1 << LCD_PIN_RS); 
    i2c_master_send(lcdData.client, &data, 1);
    udelay(100);

    /* Leemos del bus i2c el high nibble */
    i2c_master_recv(lcdData.client, &ch, 1);
    *pData = ch >> 4;

    /* Repetimos la secuencia - Enviamos BL, R/W y RS en alto y toggleamos el EN */
    udelay(100);
    data = 0xf0 | (1 << LCD_PIN_BL) | (1 << LCD_PIN_RW) | (1 << LCD_PIN_RS); 
    i2c_master_send(lcdData.client, &data, 1);
    udelay(100);
    data = 0xf0 | (1 << LCD_PIN_BL) | (1 << LCD_PIN_EN) | (1 << LCD_PIN_RW) | (1 << LCD_PIN_RS); 
    i2c_master_send(lcdData.client, &data, 1);
    udelay(100);

     /* Leemos del bus i2c el low nibble */
    i2c_master_recv(lcdData.client, &ch, 1);
    *pData = (*pData << 4) | (ch >> 4 );
    udelay(100);

    /* Ponemos el EN en bajo */
    data = 0xf0 | (1 << LCD_PIN_BL) | (1 << LCD_PIN_RW) | (1 << LCD_PIN_RS); 
    i2c_master_send(lcdData.client, &data, 1);
    udelay(100);

    /* Volvemos a modo escritura - por defecto */
    data = 0x0f | (1 << LCD_PIN_BL);     
    i2c_master_send(lcdData.client, &data, 1);

}

/*
* @fn static void lcdDeInit(lcdTopology_t topology)
* @param topolgy : Topologia del lcd a iniciar
* @brief Lleva a cabo el proceso de inicializacion del lcd
* @return nada
*/
static void lcdInit(lcdTopology_t topology)
{
    /* Seteamos parametros de la topologia */
    lcdData.org.topology = topology;
    lcdData.org.columns = lcdAddress[topology][4];
    lcdData.org.rows = lcdAddress[topology][5];
    memcpy(lcdData.org.addresses, lcdAddress[topology], sizeof(lcdAddress[topology]) - 2);

    /* Display control en 0 por defecto */
    lcdData.displaycontrol = 0;

    /* LCD en 4 bits, 8x5 tamaño de caracter y 2 lineas */
    /* En un futuro se podrian agregar lcd de 1 linea - 
       No pensado en las topologias soportadas por el momento 
    */
    lcdData.displayfunction = LCD_4BIT_DATA | LCD_2LINES | LCD_8x5FONT;
    
    /* Comienzo del proceso de inicializacion - Ver datasheet hd44780 */
    /* Escribimos el estado del backlight al lcd  y esperamos */
    mdelay(50);
    lcdWrite((lcdData.backlight ? (1 << LCD_PIN_BL) : 0));
    mdelay(100);

    lcdToggleEn((1 << LCD_PIN_DB4) | (1 << LCD_PIN_DB5));
    mdelay(5);

    lcdToggleEn((1 << LCD_PIN_DB4) | (1 << LCD_PIN_DB5));
    mdelay(5);

    lcdToggleEn((1 << LCD_PIN_DB4) | (1 << LCD_PIN_DB5));
    mdelay(15);

    lcdToggleEn((1 << LCD_PIN_DB5));

    lcdSendCommand(lcdData.displayfunction);
    /* Seteamos el display control en Display on, cursor off y blinking del cursor off */
    lcdData.displaycontrol |= LCD_DISPLAY_ON | LCD_CURSOR_OFF |
               LCD_CURSOR_BLINK_OFF;
    /* Seteamos el display control con las caracteristicas anteriores */
    lcdSendCommand(lcdData.displaycontrol);

    /* Limpiamos el lcd y volvemos al principio por si las dudas */
    lcdClear();
    lcdHome();

}

/**
* @fn static u8 lcdPrint(const char *data)
* @brief Escribe un string en el lcd desde la posicion actual del cursor
* @param data : Puntero a los caracteres a escribir
* @return valor final del cursor despues de escribir 
*/
static u8 lcdPrint(const char *data)
{
    int i = 0;

    /* Mientras no lleguemos al final del lcd o de la data a enviar */
    while (i < (lcdData.org.columns * lcdData.org.rows) && data[i])
    {
        /* Si es un salto de linea o retorno de carro */
        if (data[i] == '\n' || data[i] == '\r')
        {
            /* Resetamos la columna y saltamso de fila */
            lcdData.column = 0;
            lcdData.row = (lcdData.row + 1) % lcdData.org.rows;
        } 
        /* Si es un borrar */
        else if (0x08 == data[i])   //BS == Borrar
        {
            /* Volvemos una columna para atras */
            if (0 < lcdData.column)
            {
                lcdData.column--;
            }
        }
        else
        {
            /* Si es un caracter normal, lo enviamos al lcd */
            lcdSendCommand(LCD_CMD_SETDDRAMADDR | 
                            ((lcdData.column % lcdData.org.columns) + 
                            lcdData.org.addresses[(lcdData.row % lcdData.org.rows)]));
            lcdWriteChar(data[i]);

            /* Avanzamos la columna en 1 */
            lcdData.column = (lcdData.column + 1) % lcdData.org.columns;
            /* Si la columna es 0, es porque pegamos la vuelta */
            if(0 == lcdData.column)
            {
                /* Avanzamos la fila */
                lcdData.row = (lcdData.row + 1) % lcdData.org.rows;
            }
        }
        /* Aumentamos en uno el indice para saltar al proximo caracter a enviar */
        i++;        
    }

    /* Retornamos la direccion del cursor */
    return (lcdData.column + (lcdData.row * lcdData.org.columns));
}
/*==================[external functions definition]==========================*/
/**
* @fn static ssize_t mylcd_fopread(struct file *file, char __user *buffer,
               size_t length, loff_t *offset)
* @brief Retorna al usuario la data almacenada en el buffer interno del driver
*/
static ssize_t mylcd_fopread(struct file *file, char __user *buffer,
               size_t length, loff_t *offset)
{
    u8 i = 0;
    u8 ch;
    /* Guardamos el valor actual de la columna y fila */
    u8 col = lcdData.column;
    u8 row = lcdData.row;
    if(mutex_trylock(&g_mutex))
    {   

        /* Leemos siempre desde el principio */
        lcdData.column = 0;
        lcdData.row   = 0;
        
        /* Mientras no hayamos leido la cantidad que pidio el usuario 
            y no se haya llegado al final del lcd(buffer) */
        while(i < length && i < (lcdData.org.columns * lcdData.org.rows))
        {
            lcdSetCursor(lcdData.column, lcdData.row);
            mdelay(1);
            lcdRead(&ch);
            /* Escribimos en el buffer del usuario el caracer actual */
            put_user(ch, buffer++);
            /* Fijamos el nuevo valor de la columna y fila actual */
            lcdData.column = (lcdData.column + 1) % lcdData.org.columns;
            /* Si la columna es 0, es porque pegamos la vuelta */
            if(0 == lcdData.column)
            {
                /* Avanzamos la fila */
                lcdData.row = (lcdData.row + 1) % lcdData.org.rows;
            }
            /* Avanzamos al proximo caracter */
            i++;
        }

        /* Obtenemos el offset == cantidad leida */
        i %= (lcdData.org.columns * lcdData.org.rows);

        *offset = i;

        /* Volvemos a como estabamos antes del read */
        lcdSetCursor(col, row);

        mutex_unlock(&g_mutex);
    }

    return i;

}

/**
* @fn static ssize_t mylcd_fopwrite(struct file *file, const char __user *buffer,
                              size_t length, loff_t *offset)
* @brief Escribe en el buffer interno del driver y en el lcd
*/
static ssize_t mylcd_fopwrite(struct file *file, const char __user *buffer,
                              size_t length, loff_t *offset)
{
    u8 i = 0, str[LCD_STRING_SIZE];  

    if(mutex_trylock(&g_mutex))
    {
        /* Mientras no hayamos llegado al final string o al final del lcd */
        for(i = 0; i < length && i < (lcdData.org.columns * lcdData.org.rows); i++)
        {
            /* Obtenemos el caracter siguiente */
            get_user(str[i], buffer + i);
        }

        /* Ponemos un fin de cadena */
        str[i] = 0;
        /* Enviamos al lcd */
        *offset = lcdPrint(str);

        mutex_unlock(&g_mutex);
    }

    return i;

}

static long mylcd_ioctl(struct file *file, unsigned int ioctl_num, unsigned long arg)
{
    char *buffer = (char*)arg;
    u8 ch;
    if(mutex_trylock(&g_mutex))
    {
        switch(ioctl_num)
        {
            case LCD_IOCTL_SETCHAR:

                /* Obtenemos el argumento */
                get_user(ch, buffer);
                /* Escribimos el caracter */
                lcdWriteChar(ch);
                /* Fijamos el nuevo valor de la columna y fila actual */
                lcdData.column = (lcdData.column + 1) % lcdData.org.columns;
                /* Si la columna es 0, es porque pegamos la vuelta */
                if(0 == lcdData.column)
                {
                    /* Avanzamos la fila */
                    lcdData.row = (lcdData.row + 1) % lcdData.org.rows;
                }
                lcdSetCursor(lcdData.column, lcdData.row);
                break;
            case LCD_IOCTL_GETCHAR:
                /* Leemos el ultimo caracter escrito y para ello seteamos el cursor en la ultima posicion */
                lcdSetCursor(lcdData.column - 1, lcdData.row);
                mdelay(1);
                lcdRead(&ch);
                /* Volvemos a como estabamos antes del read */
                lcdSetCursor(lcdData.column + 1, lcdData.row);
                mdelay(1);
                put_user(ch, buffer);
            break;
            case LCD_IOCTL_GETPOSITION:
                /* Obtenemos la posicion actual del cursor y se la pasamos al usuario */
                put_user(lcdData.column, buffer);
                put_user(lcdData.row, buffer + 1);
            break;
            case LCD_IOCTL_SETPOSITION:
                /* Obtenemos la columna y fila del usuario y la enviamos al lcd */
                get_user(lcdData.column, buffer);
                get_user(lcdData.row, buffer + 1);
                lcdSetCursor(lcdData.column, lcdData.row);
            break;
            case LCD_IOCTL_RESET:
                /* Si el flag enviado por el usuario es un 1, reiniciamos el lcd */
                get_user(ch, buffer);
                if ('1' == ch)
                {
                    /* Reseteamos los valores al estado por defecto */
                    lcdData.row         = 0;
                    lcdData.column      = 0;
                    lcdData.backlight   = 1;
                    lcdData.cursor      = 0;
                    lcdData.blink       = 0;
                    lcdInit(lcdData.org.topology);
                }
            break;
            case LCD_IOCTL_HOME:
                /* Si el flag enviado por el usuario es un 1, volvemos al home al lcd */
                get_user(ch, buffer);
                if ('1' == ch)
                {
                    lcdHome();
                }
            break;
            case LCD_IOCTL_GETCURSOR:
                /* Enviamos al usuario el estado del cursor, encendido o apagado */
                put_user(lcdData.cursor ? '1' : '0', buffer);
            break;
            case LCD_IOCTL_SETCURSOR:
                /* Si el flag enviado por el usuario es un 1, 
                   encendemos el cursor, caso contrario lo apagamos  */
                get_user(ch, buffer);
                lcdData.cursor = ('1' == ch);
                lcdCursor(lcdData.cursor);
            break;
            case LCD_IOCTL_GETBLINK:
                /* Enviamos al usuario el estado del blinking del cursor, encendido o apagado */
                put_user(lcdData.blink ? '1' : '0', buffer);
            break;
            case LCD_IOCTL_SETBLINK:
                /* Si el flag enviado por el usuario es un 1, 
                   encendemos el blinking del cursor, caso contrario lo apagamos  */
                get_user(ch, buffer);
                lcdData.blink = ('1' == ch);
                lcdBlink(lcdData.blink);
            break;
            case LCD_IOCTL_GETBACKLIGHT:
                /* Enviamos al usuario el estado de la luz de fondo del lcd, encendido o apagado */
                put_user(lcdData.backlight ? '1' : '0', buffer);
            break;
            case LCD_IOCTL_SETBACKLIGHT:
                /* Si el flag enviado por el usuario es un 1, 
                   encendemos la luz de fondo del lcd, caso contrario la apagamos  */
                get_user(ch, buffer);
                lcdData.backlight = ('1' == ch);
                lcdSetBacklight(lcdData.backlight);
            break;
            case LCD_IOCTL_CLEAR:
                /* Si el flag enviado por el usuario es un 1, limpiamos el lcd */
                get_user(ch, buffer);
                if ('1' == ch)
                {
                    lcdClear();
                    /* Para evitar que el comando clear me vuelva el cursor al inicio */
                    lcdSetCursor(lcdData.column, lcdData.row);
                }
            break;

            default:
                printk(KERN_INFO "Unknown IOCTL\n");
                return -ENOTTY;
            break;
        }
        mutex_unlock(&g_mutex);
    }

    return 0;
}


static int mylcd_probe(struct i2c_client *client, const struct i2c_device_id *id)
{

    pr_info("mylcd_probe!\n");
    
    /* Inicializacion del driver */
    mylcd_init();

    /* Inicializacion de la data interna del driver */
    lcdData.row = 0;
    lcdData.column = 0;
    lcdData.client = client;
    lcdData.backlight = 1;
    lcdData.cursor = 0;
    lcdData.blink = 0;
    lcdData.devOpenCnt = 0;

    /* Incializacion del lcd */
    lcdInit(LCD_TOPOLOGY_20x4);

    return 0;
}

static int mylcd_remove(struct i2c_client *client)
{

    /* Apagamos el lcd */
    lcdDeInit();

    /* Exit del driver */
    mylcd_exit();

    pr_info("mylcd_remove!\n");
    return 0;
}

static int mylcd_open(struct inode *inode, struct file *file)
{
    if(mutex_trylock(&g_mutex))
    {
        lcdData.devOpenCnt++;
        mutex_unlock(&g_mutex);
    }
    return 0;
}

static int mylcd_release(struct inode *inode, struct file *file)
{
    mutex_unlock(&g_mutex); 
    lcdData.devOpenCnt--;

    return 0;
}

static struct i2c_driver mylcd_i2c_driver = 
{
    .driver = 
    {
        .name = "mylcd",
        .of_match_table = mylcd_of_match,
    },
    .probe = mylcd_probe,
    .remove = mylcd_remove,
    .id_table = mylcd_i2c_id
};

module_i2c_driver(mylcd_i2c_driver);

static struct file_operations mylcd_fops = 
{
    .read = mylcd_fopread,
    .write = mylcd_fopwrite,
    .unlocked_ioctl = mylcd_ioctl,
    .open = mylcd_open,
    .release = mylcd_release,
};

static int mylcd_init(void)
{

    printk(KERN_INFO "LCD: Iniciando..\n");
    
    mutex_init(&g_mutex);

    // Try to dynamically allocate a major number for the device -- more difficult but worth it
    lcdData.majorNumber = register_chrdev(0, DEVICE_NAME, &mylcd_fops);
    if(0 > lcdData.majorNumber)
    {
        printk(KERN_ALERT "LCD failed to register a major number\n");
        return lcdData.majorNumber;
    }
    printk(KERN_INFO "LCD: registered correctly with major number %d\n", lcdData.majorNumber);

    // Register the device class
    lcdData.lcdClass = class_create(THIS_MODULE, CLASS_NAME);
    if(IS_ERR(lcdData.lcdClass))
    {                
        // Check for error and clean up if there is
        unregister_chrdev(lcdData.majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(lcdData.lcdClass);          // Correct way to return an error on a pointer
    }
    printk(KERN_INFO "LCD: device class registered correctly\n");

    // Register the device driver
    lcdData.lcdDevice = device_create(lcdData.lcdClass, NULL, MKDEV(lcdData.majorNumber, 0), NULL, DEVICE_NAME);
    if(IS_ERR(lcdData.lcdDevice))
    {               
        // Clean up if there is an error
        class_destroy(lcdData.lcdClass);           // Repeated code but the alternative is goto statements
        unregister_chrdev(lcdData.majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(lcdData.lcdDevice);
    }
    printk(KERN_INFO "LCD: device class created correctly\n"); // Made it! device was initialized
    return 0;
}


static void mylcd_exit(void)
{
    mutex_destroy(&g_mutex);  
    device_destroy(lcdData.lcdClass, MKDEV(lcdData.majorNumber, 0));        // remove the device
    class_unregister(lcdData.lcdClass);                                     // unregister the device class
    class_destroy(lcdData.lcdClass);                                        // remove the device class
    unregister_chrdev(lcdData.majorNumber, DEVICE_NAME);                    // unregister the major number
    pr_info("LCD: Exit\n");
}


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("HD44780 LCD Module Driver");
MODULE_AUTHOR("Esp. Ing. Del Sancio");
MODULE_VERSION("1.0");
/*==================[end of file]============================================*/
