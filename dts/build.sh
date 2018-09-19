dtc -W no-unit_address_vs_reg -@ -I dts -O dtb -o spi-gpio-cs.dtbo spi-gpio-cs-overlay-joon.dts           
#dtc -W no-unit_address_vs_reg -@ -I dts -O dtb -o reset-gpio-overlay.dtbo reset-gpio-overlay.dts 
sudo cp spi-gpio-cs.dtbo /boot/overlays 
sudo cp reset-gpio-overlay.dtbo /boot/overlays 
