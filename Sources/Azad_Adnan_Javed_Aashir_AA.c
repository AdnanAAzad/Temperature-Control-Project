/*
 MCO 455 TEMPERATURE CONTROL PROJECT
 Date: 24.10.2019
 Authors: Adnan Azad, Aashir Javed 
 Program Description: This program that uses a DE1 board to interface with a temperature sensor that sends and 
 receives binary/HEX data values to indicate temperature changes which is displayed in the windows form application  
 as well as the 7-Segment displays on the board. Alternatively, temperature can be simulated using the on-board  
 buttons which is then updated accordingly in the program as well as 7-Segment displays. All of this is done live with  no wait time or latency.
 
 */
#include <hidef.h> /* for EnableInterrupts macro */
#include "derivative.h" /* include peripheral declarations */
#include "E:\library_de1.h"

// GLOBAL VARIABLES
volatile unsigned char SETPOINT_VALUE;    // Set Point Variable
volatile unsigned char Temp;              // internal temp variable; is not read from tp8
volatile unsigned char Temp_read;         // variable used to read temp from TP8

// Global indicator variables used as indecators for heating/cooling/no change in the system
volatile unsigned char heater_indicator;       // indicates if heating is on [1] or off [0]
volatile unsigned char cool_indicator;         // indicates if cooling is on [1] or off[0]
volatile unsigned char fan_indicator;          // indicates if fan in active [1] or inactive [0]

// counter variables: used to waste time for temperature operations
volatile unsigned char countcool;   // counter used for cooling
volatile unsigned char countheat;   // counter used for heating

// HEX VALUES FOR 7 SEG DISPLAYS: has digits 0-F for 7 segment display INDICATED VIA INDEX
unsigned static const segs[]={
	0x02,0x9f,0x24,0x0c,0x99,0x48,0x41,0x1f,
	0x00,0x18,0x10,0xc1,0x62,0x85,0x61,0x71 };

// FUNCTION PROTOTYPES
void CoolerOn(void);            // display cooler on + turn on led
void CoolerOff(void);           // display cooler off + turn off led
void FanOn(void);               // displays fan is on
void FanOff(void);              // displays fan is off
void HeaterOn(void);            // display heater in on + turn on led
void HeaterOff(void);           // display heater is off + turn off led
void UP(void);                  // increases setpoint and display
void DOWN(void);                // decrease setpoint and display
void InitializeFunction (void); // executes functions: scr_clear(),ClearLCD(),FormatPC(),FormatLCD(),DisplaySetPoint().SetTemp(),GetTemp(), and Display().

// Condition Functions: changes indicator variables and calls appropriate functions for selected state
void CoolerCondition(void);     // assigns conditions associated with cooling
void HeaterCondition(void);     // assigns conditions associated with heating 
void SameCondition(void);       // assigns conditions associated with temp and setpoint being the same 

// display functions
void DisplaySetPoint(void);     // updates all displays regarding the setpoint
void FormatLCD(void);           // Formats LCD and populates it with needed text for project
void FormatPC(void);            // Formats PC hyperterminal and populates it with needed text for project
void StateDisplay(void);        // displays to PC, LCD, LEDs and ON/OFF states for for Fan/Cooler/Heater
void Display(void);             // calls: DisplayTemp(), StateDisplay();
void DisplayTemp(void);         // Displays temperature obtained from tp 8 onto pc, LCD, and right two 7 segment displays

// get and set functions 
void SetTemp(void);             // set function used to set tp10 value
void GetTemp(void);             // Used as a get function to obtain temp value from tp8
void ClearLCD(void);            // clears LCD

// MAIN
void main(void) {
	// Initialize Variables
	SETPOINT_VALUE=0x014;     // assign set point to be 20 in decimal
	Temp=0x014;               // assign temp to be 20 in decimal
	Temp_read;                
	
	// set all indicator variables to zero
	heater_indicator=0x00; 
	cool_indicator=0x00;
	fan_indicator=0x00;

	// set all counters to be zero
	countcool = 0x00;
	countheat = 0x00;

	SOPT_COPE = 0;          // SOPT_COPE: disable computer operating prop.
	LCD_ROWS = 4; 			// set rows for 4X16 LCD display
	LCD_COLS = 16; 			// set columns for 4X16 LCD display
	devices_init();         // initialize devices	
	InitializeFunction();   // Function call: used to initialize the system
	
	for(;;){
		// Check for Button press
		if ((!(SW_KEY3)) && (hex2dec(SETPOINT_VALUE) > 0x10)){       // if most left button is pressed, decrease setpoint while setpoint is greater than 10
			do{
				DOWN();                                              // function call DOWN().
			}while((!(SW_KEY3)) && (hex2dec(SETPOINT_VALUE) > 0x10));
		} 
		else if ((!(SW_KEY2)) && (hex2dec(SETPOINT_VALUE)<0x35) ){        // if second most left push button is pressed, increase setpoint while setpoint is less than 35
			do{
				UP();                                                // function call UP().
			}while((!(SW_KEY2)) && (hex2dec(SETPOINT_VALUE)<0x35) ); // Loop while SP is less than Max value.
		}
		// Temperature operations
		if ( SETPOINT_VALUE > Temp_read ){                      // system heating condition
			/*
			 * This condition Heats the system by having countheat go up to 10 to use up 1 second.
			 * Then increases Temp variable and sends Temp value to TP10 using SetTemp().
			 * GetTemp() is then used to obtain the data from TP8 and stores that value to Temp_read variable
			 * Next, the conditions associated with a heating system is set to the indicator variables using HeaterCondition().
			 * Finally the changes in the system is displayed on the LCD, PC, and right two 7 Segment Displays.
			 * */
			countheat++;
			if (countheat>0x0A){
				countheat=0x00;          // reset counter
				Temp++;                  // increase internal temp value
				SetTemp();               // send setpoint value to tp10
				GetTemp();               // retrieve temp value using new variable: Temp_read
				HeaterCondition();       // change conditions to heating
				Display();               
			}
		}
		else if ( SETPOINT_VALUE < Temp_read) { // system cooling condition
			/*
			 * This condition Cools the system by having countcool go up to 10 to use up 1 second.
			 * Then increases Temp variable and sends Temp value to TP10 using SetTemp().
			 * GetTemp() is then used to obtain the data from TP8 and stores that value to Temp_read variable
			 * Next, the conditions associated with a cooling system is set to the indicator variables using CoolerCondition().
			 * Finally the changes in the system is displayed on the LCD, PC, and right two 7 Segment Displays.
			 * */
			countcool++;            // increment counter for cooling condition by 0x01
			if (countcool>0x0A){
				countcool=0x00;     // reset counter
				Temp--;             // decrease internal temp value
				SetTemp();          // send setpoint value to tp10
				GetTemp();          // retrieve temp value using new variable: Temp_read
				CoolerCondition();  // change conditions
				Display();          // Display changes
			}
		}
		else if (Temp_read == SETPOINT_VALUE ){ // Temperature and Set point are same.
			/*
			 * This condition assess the temperature and setpoint value to be the same.
			 * Then increases Temp variable and sends Temp value to TP10 using SetTemp().
			 * GetTemp() is then used to obtain the data from TP8 and stores that value to Temp_read variable
			 * Next, the conditions associated with an equilibrium  system is set to the indicator variables using SameCondition().
			 * Finally the changes in the system is displayed on the LCD, PC, and right two 7 Segment Displays.
			 * */
			SameCondition();        // Change conditions
			Display();              // display changes in the system
		}
		          
		delay_milli(100);           // delay 0.1 second
	}; // END OF: for(;;)
}

// FUNCTION DEFINITIONS
void SetTemp(){
	// This function is used to Send value stored in Temp to TP10
	analog_write_int(1,((Temp)<<4));         // write to TP10 with 4 bits shifted to the left
}
void GetTemp(){
	/*
	 * This get function is used to obtain temp value from tp8 and stores it into
	 * Temp_read variable that is used for displaying.
	 * */
	Temp_read = (analog_read_int(1))>>4;     // reads temp from tp8 and shifts 4 bits to right
}
void DisplayTemp(){
	/*
	 * This function is used to display the temperature reading from TP8
	 * to all peripherals.
	 * */
	// PC (HyperTerminal) Display
 	scr_setcursor(6,48);          		    //sets PC screen cursor to (6,48)
	scr_write(hex2dec(Temp_read)); 			// writes the temperature

	// LCD Display
	lcd_setcursor(1,12);                    // Set cursor to 1,13 on LCD
	lcd_write(hex2dec(Temp_read));          // Write Temp_read value to cursor location

	// Display on right two 7 segment displays
	HEX1= segs[hex2dec(Temp_read) >> 4];    // Obtain first digit of two-digit value and display on HEX 1
	HEX0= segs[hex2dec(Temp_read) & 0x0f];  // Obtain second digit of two-digit value and display on Hex 0
	StateDisplay();                         // Function call: StateDisplay()
}
void DisplaySetPoint(){
	/*
	 * This function displays the Setpoint and/or update the
	 * setpoint to all peripherals.
	 * */
	// display setpoint on left 2 7 segment displays
	HEX3 = segs[hex2dec(SETPOINT_VALUE) >> 4];     // Obtain first digit of two-digit value and display on HEX 3
	HEX2 = segs[hex2dec(SETPOINT_VALUE) & 0x0f];   // Obtain second digit of two-digit value and display on Hex 2
	// PC
	scr_setcursor(6,26);                           //sets PC screen cursor to (6,26)
	scr_write(hex2dec(SETPOINT_VALUE));            // writes the setpoint 
	// LCD
	lcd_setcursor(1,3);                            // set LCD cursor to 1st row, 5th column
	lcd_write(hex2dec(SETPOINT_VALUE));            // setpoint value at cursor location
}
void ClearLCD(){
	/*
	 * This function is used to clear the LCD screen by printing empty strings in every row of the LCD.
	 * */
	lcd_setcursor(0,0);              // set cursor to row 0
	lcd_print("                 ");  // print 16-bit long empty string
	lcd_setcursor(1,0);              // set cursor to row 1
	lcd_print("                 ");  // print 16-bit long empty string
	lcd_setcursor(2,0);              // set cursor to row 2
	lcd_print("                 ");  // print 16-bit long empty string
	lcd_setcursor(3,0);              // set cursor to row 3
	lcd_print("                 ");  // print 16-bit long empty string
}
void StateDisplay(){
	/*
	 * This function uses indicator variables to decide which aspect of the system is on or off.
	 * Aspects of system include: Fan, Cooler, and Heater.
	 * Once decided, it then calls the appropriate function to execute code that makes the changes.
	 * */
	// heater state
	if (heater_indicator == 0x01){           // If heater indicator is ON [1]
		HeaterOn();                       // call HeaterOn()
	}
	else if (heater_indicator == 0x00){      // If heater indicator is OFF[0]
		HeaterOff();                      // call HeaterOff()
	}
	// cooler state
	if(cool_indicator==0x01){                // If cooler indicator is ON [1]
		CoolerOn();                       // call CoolerOn()
	}
	else if(cool_indicator==0x00){           // If cooler indicator is OFF [0]
		CoolerOff();                      // call CoolerOff()
	}
	// fan state
	if(fan_indicator==0x01){                 // If fan indicator is ON [1]
		FanOn();                          // Call FanOn()
	}
	else if(fan_indicator==0x00){            // If fan indicator is OFF [0]
		FanOff();                         // Call FanOff()
	}
}
void Display(){
	/*
	 * This function calls the major display functions that updates PC, LCD, and 7 Segment Displays.
	 * */
	DisplayTemp();   // Call DisplayTemp()
	StateDisplay();  // Call StateDisplay()
}
void DOWN(void){
	/*
	 * This function is used to decrease the setpoint value and display it on all peripherals.
	 * This function also wastes 0.2 seconds.  
	 * */
	SETPOINT_VALUE--;  // decrement setpoint by 1
	DisplaySetPoint(); // used to display SP to all peripherals
	delay_milli(200);  // wait 0.2 seconds
}
void UP(void){
	/*
	 * This function is used to increase the setpoint value and display it on all peripherals.
	 * This function also wastes 0.2 seconds.  
	 * */
	SETPOINT_VALUE++;  // increment setpoint by 1
	DisplaySetPoint(); // display the setpoint to all peripherals
	delay_milli(200);  // wait 0.2 seconds
}
void HeaterCondition(){
	/*
	 * This function assigns conditions for associated with heating the system
	 * by changing the indicator variables.
	 * */
	fan_indicator=0x01;     // assign 1 for indicating fan is on
	heater_indicator=0x01;  // assign heater 1 for indicating heater on
	cool_indicator=0x00;    // assign 0 for cooling for indicating cooler off
}
void CoolerCondition(){
	/*
	 * This function assigns conditions for associated with cooling the system
	 * by changing the indicator variables.
	 * */
	fan_indicator=0x01;     // assign 1 for indicating fan is on
	heater_indicator=0x00;  // assign heater 0 for indicating heater off
	cool_indicator=0x01;    // assign 1 for cooling for indicating cooler on
}
void SameCondition(){
	/*
	 * This function assigns conditions for associated the system 
	 * being in equilibrium by changing the indicator variables.
	 * */
	fan_indicator=0x00;     // assign 0 for indicating fan is off
	heater_indicator=0x00;  // assign heater 0 for indicating heater off
	cool_indicator=0x00;    // assign 0 for cooling for indicating cooler off
}
void CoolerOn(void){
	/* 
	 * This function Updates the LCD and PC condition of
	 * the cooler to be 'ON' as well as turn on the
	 * third green LED from the left [Green LED 5]
	 * Turns on third from the left GREEN LED
	 * Display Cooler ON on LCD and PC screen.
	 * */
	// LCD Display
	lcd_setcursor(3,12); // set cursor to 3,12
	lcd_print("ON ");    // print ON
	// PC Display
	scr_setcursor(9,55); //sets PC screen cursor to (9,55)
	scr_print("ON ");	 //prints message on PC screen
	// LED
	LEDG_LED5=1;         // turn on green LED5
} 
void CoolerOff(void){
	/*
	 * Turns off 3rd from left green led
	 * displays cooler off on LCD and PC screen
	 * and make needed hardware changes 
	 * */
	// LCD Display
	lcd_setcursor(3,12);  // set cursor to 3,12
	lcd_print("OFF");     // print OFF
	// PC Display
	scr_setcursor(9,55);  // sets PC screen cursor to (9,55)
	scr_print("OFF");	  // prints message off PC screen
	// LED 
	LEDG_LED5=0;          // turn off green LED5
}
void HeaterOn(void){
	/* 
	 * This function displays ON status for the heater on
	 * the LCD as well as the PC screen and turns ON the right
	 * most red LED to indicate the heater is on. 
	 * */
	// LCD Display
	lcd_setcursor(3,0);   // sets LCD cursor to (3,1)
	lcd_print("ON ");     // print ON on LCD
	// PC Display
	scr_setcursor(9,20);  //changes the PC screen cursor to (9,20)
	scr_print("ON ");     //prints ON PC screen 
	// LED 
	LEDR_LED0=1;          // set right most led to HIGH
}
void HeaterOff(void){
	/* 
	 * This function is used to set OFF status for the
	 * heater on the LCD and PC displays as well as
	 * turn off the right most LED to indicate the heater
	 * is OFF.
	 * */
	// LCD Display
	lcd_setcursor(3,0);   // Set LCD cursor to 3,0
	lcd_print("OFF");     // print OFF at cursor
	// PC Display
	scr_setcursor(9,20);  //changes the PC screen cursor to (9,20)
	scr_print("OFF");     //prints message on PC screen 
	// LED
	LEDR_LED0=0;          // Turn off very right red LED
}

void FanOn(void){
	/*
	 * This function sets the on/off state for 
	 * */
	// LCD Display
	lcd_setcursor(3,6);   // 
	lcd_print("ON ");
	//PC Display
	scr_setcursor(9,36);//sets PC screen cursor to (9,36)
	scr_print("ON ");//prints message on PC screen 
	// Hardware Indication
	LEDG_LED7=1;
}
void FanOff(void){
	/*
	 * Display Fan off on LCD and PC screen
	 * As well as Display Fan hardware indication
	 * */
	// LCD Display
	lcd_setcursor(3,6);
	lcd_print("OFF");
	//PC Display
	scr_setcursor(9,36);//sets PC screen cursor to (9,36)
	scr_print("OFF");//prints message on PC screen 
	// Hardware Indication
	LEDG_LED7=0;
}
void FormatPC(){
	/*
	 * This function prints system messages
	 * on the PC screen that do not change.
	 * */
	scr_setcursor(1,21);                               // sets PC screen cursor to (1,20)
	scr_print("TEMPERATURE CONTROL SYSTEM PROGRAM");   // prints on screen
	scr_setcursor(3,34);                               // sets PC screen cursor to (3,34)
	scr_print("LOCAL SYSTEM");                         // prints local system on PC screen
	scr_setcursor(5,20);							   // sets PC screen cursor to (5,20)
	scr_print("SETPOINT TEMPERATURE");				   // prints on Pc screen 
	scr_setcursor(5,44);					           // sets PC screen cursor to (5,44)
	scr_print("TEMPERATURE READING ");                 // prints on PC screen	
	scr_setcursor(6,28);                               // sets PC screen cursor to (6,28)
	scr_print(" deg C");                                 // writes the degree symbol
	scr_setcursor(6,50);                               // sets PC screen cursor to (6,50)
	scr_print(" deg C");                                 // writes the degree symbol
	scr_setcursor(8,15);                               //sets PC screen cursor to (8,215)
	scr_print("HEATING SYSTEM");                       // prints message on PC screen 
	scr_setcursor(8,36);                               // sets PC screen cursor to (8,236)
	scr_print("FAN");                                  // prints message on PC screen 	
	scr_setcursor(8,50);                               // sets PC screen cursor to (8,50)
	scr_print("COOLING SYSTEM");                       // prints message on PC screen
}
void FormatLCD(){
	// top row TEMPERATURE CONTROL SYSTEM
	lcd_setcursor(0,0);
	lcd_print("TEMP");
	lcd_setcursor(0,5);
	lcd_print("Ctrl");
	lcd_setcursor(0,10);
	lcd_print("System");
	
	// second row DISPLAY SETPOITN AND TEMP
	lcd_setcursor(1,0);
	lcd_print("SP");

	lcd_setcursor(1,5);
	lcd_writech(0xDF); // degree symbol
	lcd_setcursor(1,6);
	lcd_print("C");

	lcd_setcursor(1,8);
	lcd_print("Tmp");
	
	lcd_setcursor(1,14);
	lcd_writech(0xDF); // degree symbol
	lcd_setcursor(1,15);
	lcd_print("C");    
	// 3rd row
	lcd_setcursor(2,0);
	lcd_print("HTR");
	lcd_setcursor(2,6);
	lcd_print("FAN");
	lcd_setcursor(2,12);
	lcd_print("Cool");
}
void InitializeFunction (){
	/*
	 * This function is used to call functions: SetPoint(), ClearLCD(), SetTemp(), GetTemp(), and Display().
	 * */
	scr_clear(); // clears PC screen
	ClearLCD(); // clear LCD
	FormatPC(); // format pc screen with requirements
	FormatLCD(); // format LCD with requirements
	DisplaySetPoint(); // display setpoint on all peripherals
	SetTemp();
	GetTemp();
	Temp_read=0x014;
	Display(); // display all
}
