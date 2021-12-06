## Device tree

### Cambios hechos al archivo am335x-boneblack.dts

```
/* pinmux i2c1 */
&am33xx_pinmux {
	i2c1_pins: pinmux_i2c1_pins {
		pinctrl-single,pins = <
			AM33XX_PADCONF(AM335X_PIN_SPI0_D1, PIN_INPUT_PULLUP, MUX_MODE2) /* spi0_d1.i2c1_sda */
			AM33XX_PADCONF(AM335X_PIN_SPI0_CS0, PIN_INPUT_PULLUP, MUX_MODE2) /* spi0_cs0.i2c1_scl */
		>;
	};
};

/* habilito el bus i2c1 */
&i2c1 {
	status = "okay";
	pinctrl-names = "default";
	clock-frequency = <100000>;
	pinctrl-0 = <&i2c1_pins>;

	/* devices */
	mylcd: mylcd@27 {
		compatible = "mse,mylcd";
		reg = <0x27>; // dirección I2C del display lcd
	};
};

```

### Compilacion

1. Abrir una terminal y correr los siguientes comandos:

```
export PATH=$PATH:~/leonardo/ISO_II/toolchain/arm-mse-linux-gnueabihf/bin
export CROSS_COMPILE=arm-linux-
export ARCH=arm
```

2. Ir hasta la carpeta raiz del repositorio donde se compilo el kernel, en mi caso ~/leonardo/ISO_II/kernel/linux-stable

3. Ejecutar el siguiente comando:
```
make dtbs
```

4. El archivo dtb que hay que cargar en la placa se encuentra en: /home/leonardo/MSE/ISO_II/kernel/linux-stable/arch/arm/boot/dts/am335x-boneblack.dtb

5. Copiar el archivo al directorio fuente del servido tftp: /var/lib/tftpboot
