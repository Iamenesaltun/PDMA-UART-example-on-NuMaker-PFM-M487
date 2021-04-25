# PDMA UART example on NuMaker-PFM-M487
 On this project i made an UART example with PDMA on NuMaker-PFM-M487. There is a counter on main function and PDMA sends current value of this counter via UART1. 
 On every end of PDMA task, an interrupt occurs. On this interrupt, a flag with name of FLAG_ISLEM_BITTI turns to high. Then i start PDMA task again after checking this flag.
 Also 2 LEDs with name of D6(RED) and D7(YELLOW) toggle on main function. 
 
 You will need an external USB to TTL (ex. FT232) to see UART1 values. You have to connect RX of FT232 to D10(on NU6 connector) on NuMaker-PFM-M487. D10 is TX of UART1. 
 Also dont forget to connect GND of this two boards. 

After being sure about your connections, open Docklight or PuTTy or whatever you use serial port monitor. On this example i use Docklight.
You have to open two different windows to see UART0 and UART1 values. 

UART1 outputs : 

![image](https://user-images.githubusercontent.com/83173161/116000202-e1e02380-a5f7-11eb-910b-0e64c442be26.png)


UART0 outputs :

![image](https://user-images.githubusercontent.com/83173161/116000281-34214480-a5f8-11eb-8b95-04903c432e1c.png)




You can freely ask your questions to me. Have fun :)
