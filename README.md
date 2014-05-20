ecwmbus
=======

A wireless EnergyCam Setup with a Windows PC to monitor the energy usage

FAST EnergyCam: The quick and inexpensive way to turn your conventional meter into a smart metering device (http://www.fastforward.ag/eng/index_eng.html)

Hardware:

  *   FAST EnergyCam RF and FAST USB Communication Interface to configure the Energycam prior to installation
	(available at http://umwelt-messtechnik.de/index.php?cat=c91_Smart-Grid-Komponenten-Smart-Grid-Komponenten.html)

  * wireless M-Bus USB Stick (2 manufacturers are supported) 
  	- IMST IM871A-USB Stick ( available at http://www.tekmodul.de/index.php?id=shop-wireless_m-bus_oms_module or 
          http://webshop.imst.de/funkmodule/im871a-usb-wireless-mbus-usb-adapter-868-mhz.html)
	- AMBER Wireless M-Bus USB Adapter (http://amber-wireless.de/406-1-AMB8465-M.html)


Software:

  *  Visual Studio C++ 2010 Express can be used to compile the application.


Usage:

  *  Display commandline options
  
```shell
..\bin>ecwmbus /h  
   Commandline options:
   ecwmbus /c:3 /m:T /i
   /c:3 : COM3
   /m:T : T2 mode
   /i    : show detailed infos
```

  *  Connect to a stick on COM 3 using T2 Mode
  
```shell
..\bin>ecwmbus /c:3 /m:T /i 
```


Features:

  * The commandline application shows you all received wireless M-Bus packages. 
  * You can add meters that are watched. The received values of these are printed on the screen.
