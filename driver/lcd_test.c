#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>




int main(void)
{
int my_dev;
my_dev = open("/dev/mse00", 0);
if (my_dev < 0) {
perror("no hay dispositivo I2C: /dev/mse00.");
}
else {
ioctl(my_dev, 100, 110); /* cmd = 100, arg = 110. */
close(my_dev);
};
return 0;
}
