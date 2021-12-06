## IMD 6ta Cohorte año 2021

![WhatsApp Image 2021-12-05 at 22 21 40](https://user-images.githubusercontent.com/59117988/144772856-7cee018c-e801-4bd9-90ca-f09ba57bda22.jpeg)


## Comandos:

Setear variables de entorno:

$ export PATH=$PATH:$HOME/ISO_II/toolchain/arm-mse-linux-gnueabihf/bin
$ export CROSS_COMPILE=arm-linux-
$ export ARCH=arm


### Reinicio del servicio: 

sudo /etc/init.d/nfs-kernel-server restart

### Verificar en archivo var/exports 

/home/leonardo/ISO_II/nfsroot 192.168.0.100(rw,no_root_squash,no_subtree_check)

### Abrir terminal serial:

sudo gtkterm -p /dev/ttyUSB0 -s 115200

=> setenv ipaddr 192.168.0.100

=> setenv serverip 192.168.0.50

=> tftp 0x81000000 zImage

=> tftp 0x82000000 am335x-boneblack.dtb

=> setenv bootargs console=ttyS0,115200n8

=> setenv bootargs root=/dev/nfs rw ip=192.168.0.100 console=ttyS0,115200n8 nfsroot=192.168.0.50:/home/leonardo/ISO_II/nfsroot,nfsvers=3

=> bootz 0x81000000 – 0x82000000

Funcionamiento:


