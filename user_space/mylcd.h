#ifndef _MY_LCD_H_
#define _MY_LCD_H_
/*==================[inclusions]=============================================*/

/*==================[macros]=================================================*/
/**
* @def LCD_IOCTL_BASE
* @brief Numero magico
*/
#define LCD_IOCTL_BASE  0xF5

/**
* @def IOCTLB
* @brief Indica que el argumento es un valor en binario
*/
#define IOCTLB (1)

/**
* @def IOCTLC
* @brief Indica que el argumento es un caracter
*/
#define IOCTLC (2)

#define LCD_IOCTL_GETCHAR           _IOR(LCD_IOCTL_BASE, IOCTLC | (0x01 << 2), char *)
#define LCD_IOCTL_SETCHAR           _IOW(LCD_IOCTL_BASE, IOCTLC | (0x01 << 2), char *)
#define LCD_IOCTL_GETPOSITION       _IOR(LCD_IOCTL_BASE, IOCTLB | (0x03 << 2), char *)
#define LCD_IOCTL_SETPOSITION       _IOW(LCD_IOCTL_BASE, IOCTLB | (0x04 << 2), char *)
#define LCD_IOCTL_RESET             _IOW(LCD_IOCTL_BASE, IOCTLC | (0x05 << 2), char *)
#define LCD_IOCTL_HOME              _IOW(LCD_IOCTL_BASE, IOCTLC | (0x06 << 2), char *)
#define LCD_IOCTL_SETBACKLIGHT      _IOW(LCD_IOCTL_BASE, IOCTLC | (0x07 << 2), char *)
#define LCD_IOCTL_GETBACKLIGHT      _IOR(LCD_IOCTL_BASE, IOCTLC | (0x07 << 2), char *)
#define LCD_IOCTL_SETCURSOR         _IOW(LCD_IOCTL_BASE, IOCTLC | (0x08 << 2), char *)
#define LCD_IOCTL_GETCURSOR         _IOR(LCD_IOCTL_BASE, IOCTLC | (0x08 << 2), char *)
#define LCD_IOCTL_SETBLINK          _IOW(LCD_IOCTL_BASE, IOCTLC | (0x09 << 2), char *)
#define LCD_IOCTL_GETBLINK          _IOR(LCD_IOCTL_BASE, IOCTLC | (0x09 << 2), char *)
#define LCD_IOCTL_CLEAR             _IOW(LCD_IOCTL_BASE, IOCTLC | (0x0C << 2), char *)
/*==================[typedef]================================================*/

/*==================[internal data declaration]==============================*/

/*==================[internal functions declaration]=========================*/

/*==================[internal data definition]===============================*/

/*==================[external data definition]===============================*/

/*==================[internal functions definition]==========================*/

/*==================[external functions definition]==========================*/

/*==================[end of file]============================================*/

#endif /* _MY_LCD_H_ */




