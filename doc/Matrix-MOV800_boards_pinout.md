# Matrix MOW800 Reverse Engineering / Pinbelegung

## Mainboard / Motertreiber-Board
3x MC33035DW 	Brushless DC Motor Controller
3x MC33039 		Closed loop speed control adapter
9x FDD8424H 	Dual MOSFET
74HC4052		Perimer Schleife Anbindung
TPS3840 (ZA80)	Spannungsüberwachung 4x
s
---------------
## CPU-Board
STM32F103ZET6	main CPU (72MHz, 512KB Flash, 64KB SRAM, 2x I2C, 5x USART)
MMA8452Q		3-axis, 12-bit/8-bit digital accelerometer
BUZZER

### USB_USART Stecker
1 +5V
2 CPU-138 (BOOT0) 
3 CPU-25  (NRST = RESET) 
4 +3.3V (VDD)
5 CPU-102 (PA10 = USART1-RxD)
6 CPU-101 (PA9  = USART1-TxD)
7 GND

### SWD Stecker (Programmierung CPU)
1 +3.3V (VDD)
2 CPU-105 (PA13 = SWDIO)
3 CPU-109 (PA14 = SWCLK)
4 CPU-25 (NRST = RESET)
5 GND

### J1 Stecker (zum Main-Board)
| Stecker PIN | Verbindung auf dem CPU-Board | Verbindung auf dem Mainboard |
|-------------|------------------------------|------------------------------|
| 1  (GND)    |   |   |
| 2  (GND)    |   |   |
| 3  (GND)    |   |   |
| 4  (+3,3V)  |   |   |
| 5  (ON/OFF) | Data-Stecker-1 1kOhm | Power on |
| 6  (+3,3V)  |   |   |
| 7  (CHECK)  | CPU-65 (PE12) |	Diode D31     |
| 8	 (+5V)    |   |   |
| 9	 (CK_W)   | CPU-45 (PC5)  |	(Regensensor) |
| 10 (+5V)    |   |   |
| 11 (MC12)	  |	CPU-44 (PC4)  |	(Brücke zu pin 12) (Strommessung Mähmotor) |
| 12 (MC1)	  |	CPU-43 (PA7)  |	(Brücke zu pin 11) |
| 13 (SPDR)   |	CPU-35 (PA1)  |	(Motor R speed)    |
| 14 (RR12)	  | CPU-42 (PA6)  |	(Brücke zu pin 16) (Strommessung Motor R) |
| 15 (CK_V)   |	CPU-37 (PA3)  |	(Messung der Batteriespannung) |
| 16 (RR1)    |	CPU-29 (PC3)  |	(Brücke zu pin 14) |
| 17 (DIRR0)  |	CPU-15 (PF5)  |	(Motor R direction) |
| 18 (RL12)	  |	CPU-28 	(PC2) |	(Brücke zu pin 20) (Strommessung Motor L) |
| 19 (C_ADY3) |	CPU-21 (PF9)  |	PWM Motor rechts (Eigenbelegung) |
| 20 (RL1)	  |	CPU-27 (PC1)  |	(Brücke zu pin 18) |
| 21 (ADDR1)  |	CPU-115 (PD1) |	PWM Mähmotor (Eigenbelegung) |
| 22 (CHARGE_1) | CPU-26 (PC0) | (Ladestrommessung) |

### Data Stecker (zu LCD Platiene)
1 	J1-Stecker-5 (ON/OFF) 1kOhm
2	CPU-70	(PB11) 	1kOhm (Pin auf meiner komischerweise Platine unterbrocen)
3	CPU-69	(PB10)	1kOhm

### J2 Stecker (zum Main-Board)
| Stecker PIN       | Verbindung auf dem CPU-Board | Verbindung auf dem Mainboard |
|-------------------|---|---|
| 1	 (IROUT)        | Infrearet-Stecker-2 |
| 2	 (-)	        | CPU-73 (PB12) | (Motor L+R Bremse aus ?) |
| 3	 (IR RECIVE)    | Infrearet-Stecker-3 |
| 4	 (-)		    | CPU-74 (PB13) | (Motor L+R ON/OFF) |
| 5	 (CK-RF)        | CPU-139 (PB8) |  |
| 6	 (-)		    | CPU-75 (PB14) |  |				(Perimeter Schlefen Soensor select ?) |
| 7	 (C_ADY)	    | CPU-18 (PF6) | (Perimeter Schlefen Soensor analog signal ?) |
| 8	 (-)		    | CPU-76 (PB15) | (Perimeter Schlefen Soensor select ?) |
| 9	 (C_ADY1)	    | CPU-19 (PF7) |  |				 
| 10 (C_ADY2)	    | CPU-20 (PF8) | PWM Motor links (Eigenbelegung) |
| 11 (XV1)		    | CPU-85 (PD14) | (Brücke zu pin 12) | Spannungsversorgung über Ladestation ist AN |
| 12 (XV)		    | CPU-86 (PD15)	| (Brücke zu pin 11) |
| 13 (FLAT_SENSOR1) | CPU-58 (PE7)	| (Haube Anhebesensor L+R 2) |
| 14 (CNCRUNE)	    | CPU-7	(PC13)  | (Mähmotor ON/OFF ?) |
| 15 (FLAT_SENSOR)	| CPU-79 (PD10)	| (Haube Anhebesensor L+R 1) |
| 16 (CNRUNE)		| CPU-112 (PC11) | UART5 !!! |
| 17 (DIRL0)		| CPU-98 (PC8)	 | (Motor L direction) |
| 18 (SPDL)			| CPU-34 (PA0)	 | (Motor L speed ?) |
| 19 (CHECKM1)		| CPU-118 (PD4)	 | (alle Motoren Bremse aus ?) |
| 20 (TRK)			| CPU-113 (PC12) | (Mähmotor Bremse aus ?) |
| 21 (CNBRK)		| CPU-111 (PC10) |  |
| 22 (+5V)          |                |  |


### HMC5883L Stecker
1 GND
2 CPU-116 (PD2)
3 CPU-117 (PD3)
4 +3,3V

### Dub-Stecker
1 GND (PE2)
2 CPU-1

### MMA8452Q Chip
SDA CPU-137  (PB7 = SDA)
SCL CPU-136  (PB6 = SCL)

### Crash Stecker
| Stecker Pin | Verbindung auf dem CPU-Board | Verbindung auf der Crash Platine |
|-------------|------------------------------|---|
| 1 | CPU-36 (PA2) | CPU-16 (PA6) über 10k Wiederstand
| 2 | VDD +3.3V | VDD +3.3V
| 3 | CPU-77 (PD8) | HAL rechts
| 4 | CPU-78 (PD9) | HAL links
| 5 | CPU-47 (PB1) | CPU-26 (PB13) SPI2-SCK
| 6 | CPU-46 (PB0) | CPU-27 (PB14) SPI2-MISO
| 7 | GND | GND

### CPU
89 (PG4)  BUZZER

### BUZZER
CPU-89  (PG4)


---------------
## Crash Board
CPU STM32F103C86

### J1 Stecker
1 +3,3V
2 CPU-34 (PA13 = SWDIO)
3 CPU-37 (PA14 = SWCLK)
4 GND

### CPU 
40 (PB4)  LED

