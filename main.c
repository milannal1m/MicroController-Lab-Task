#include <stdio.h>
#include <stdint.h>
#include <esp32c3_reg.h>
#include "esp_intr_alloc.h"
#include "calibrate_adc.h"

#define LEDS 16
uint8_t pixels[LEDS*3]; 									// 3 color channels per LED

volatile uint32_t* gpio_enable_reg = (volatile uint32_t*) GPIO_ENABLE_REG;
volatile uint32_t* gpio_out_reg = (volatile uint32_t*) GPIO_OUT_REG;
volatile uint32_t* gpio_in_reg = (volatile uint32_t*) GPIO_IN_REG;


void delay(uint32_t us)
{
    /*
        Waits us microseconds.
    */

	asm volatile
	(
		"1:                                     \n\t"
		"beq %0,zero,3f                         \n\t"   	// if us == 0 -> 3
		"li t0,38                               \n\t"   	// t0 = 38
		"2:                                     \n\t"
		"addi t0,t0,-1                          \n\t"   	// t0--
		"bne t0, zero, 2b                       \n\t"   	// if t0 != 0 -> 2
		"addi %0,%0,-1                          \n\t"   	// us--
		"j 1b                                   \n\t"   	// jump to 1
		"3:                                     \n\t"
		:
		: "r"(us)                                       	// %0: adress of us
		:"t0"
	);

}

void display(int port, int number){

	/*
		Writes every bit of a number of the pixel data to GPIO1.
        Writes HIGH to GPIO1 for 600ns if the bit is 1.
        Writes LOW to GPIO1 for 300ns if the bit is 0.
        After writing a Bit waits 650ns, if the bit was 1 and waits 950ns if the bit was 0.
	*/

	int factor = 1;

	for (int n = 7; n >= 0; n--)                    		// For every bit of every number
	{                                        
		if (number & (1 << n))                   			// If bit n of number i is 1
		{	
			factor = 2;                             		// Set factor to 2 else 1
		}else
		{                                  
			factor = 1;
		}

		asm volatile 
		(

			//GPIO1 HIGH
			"lw t1, gpio_out_reg			\n\t"   		// t1 = *gpio_out_reg
			"or t2,t1,%1 					\n\t"   		// *gpio_out_reg |= (2)
			"sw t2, 0(t1)					\n\t"   		// store gpio_out_reg

			//Warteschleife HIGH
			"li t0, 10                  	\n\t"   		// t0 = 10
			"mul t2,t0,%0					\n\t"   		// t2 = 10*factor
			"add t3,t2,zero 				\n\t"   		// t3 = t2

			"1:                  			\n\t"
			"addi t2,t2,-1                  \n\t"   		// t2--
			"bne t2,zero,1b                 \n\t"   		// if t2 != 0 -> 1

			//GPIO1 LOW
			"lw t1,gpio_out_reg				\n\t"   		// t1 = *gpio_out_reg
			"not t0,%1 						\n\t"   		// t0 = ~t0
			"and t2,t1,t0 					\n\t"   		// *gpio_out_reg &= ~(2)
			"sw t2, 0(t1)					\n\t"   		// store gpio_out_reg

			//Wartschleife LOW
			"li t4, 39					    \n\t"   		// t4 = 39
			"sub t3,t4,t3					\n\t"   		// t3 = 39 - 10*factor

			"2:                  			\n\t"
			"addi t3,t3,-1                  \n\t"   		// t3--
			"bne t3,zero,2b                 \n\t"   		// if t3 != 0 -> 2

			:
			:    
			"r" (factor), "r"(port)                  		// %0: adress of factor %1: adress of port
			: "t0", "t1", "t2","t3","t4"
		);
	}
}

void displayLEDs()
{
	/*
		Displays every channel of every LED onto the LED Strip.
		Display every channel of the first LED onto the onboard LED of the ESP32-C3.
        In the end waits for 50us to set the LED strip.
    */

	for(int i = 0; i<LEDS*3; i++)                       	// For every channel of every LED
	{
		display((1 << 1),pixels[i]);						// Display the channel onto the LED strip
	}
	delay(50); 

	display((1 << 8),pixels[0]);							// Display every channel of the first LED
	display((1 << 8),pixels[1]);							// Onto the onboard LED of the ESP32-C3
	display((1 << 8),pixels[2]);

	delay(50);                                         		// Wait 50us 
}

void config_timer()
{
    /*
    Configures the Timer
    */

  	*TIMG_T0CONFIG_REG &= ~(1 << 9);						// Enables the APB Clock
  	*TIMG_T0CONFIG_REG |= (1 << 30);						// Enables the timer to increment

	*TIMG_T0CONFIG_REG |= (80 << 13);                   	// Sets the divider of the timer
	
  	*TIMG_T0LOADLO_REG &= 0;								// Initializes the timer value with 0
	*TIMG_T0CONFIG_REG |= (1 << 31);                   		// Enables the timer

}

long readSensor()
{
	/*
    Reads the Echo Sensor by using a Timer to measure the time between the rising and falling edge of the Echo signal.
	The Echo Signal is started by setting the Trigger Signal to HIGH for 10us.
	The Echo Signal is connected to GPIO3 and the Trigger Signal is connected to GPIO2.
	The time between the rising and falling edge of the Echo signal is converted to cm and returned.
    */

	long time = 0;                                     		// Variables to save the time at the rising 
	long time2 = 0;									   		// and falling edge of the Echo signal
	long timediff = 0;							   			// Variable to save the difference between time and time2

	*gpio_out_reg|= (1 << 2);                          		// Sets the Trigger to HIGH
	delay(10);                                     			// Wait 10us
	*gpio_out_reg &= ~(1 << 2);                         	// Sets the Trigger to LOW

	while(!(*gpio_in_reg & (1<<3))){}                   	// Waits while the Echo is LOW
  
  	*TIMG_T0UPDATE_REG |= (1 << 31);                    	// Allows the Counter of the Timer to be read
	while(!(*TIMG_T0UPDATE_REG == 0)){}						// Waits until the Timer is updated
  	time = *TIMG_T0LO_REG;									// Reads the time at the rising edge of the Echo signal

	while(*gpio_in_reg & (1<<3)){}                    		// Waits while the Echo is HIGH

	*TIMG_T0UPDATE_REG |= (1 << 31);                    	// Allows the Counter of the Timer to be read
	while(!(*TIMG_T0UPDATE_REG == 0)){}  			 		// Waits until the Timer is updated
  	time2 = *TIMG_T0LO_REG;                             	// Saves the time at the falling edge of the Echo signal

  	timediff = time2-time;									// Calculates the difference between time and time2

  	long distance = timediff/58;                        	// Calculates the distance in cm
	return distance;

}

int getDistance()
{
	/*
		Calculates and returns the mean of 5 measurements of the Sensor.
	*/

	int n = 4;
	int measurement[n];
	int mean = 0;

	for(int i = 0; i<n; i++)
	{
		measurement[i] = readSensor();						
		mean += measurement[i];								
	}

	return mean/n;

}

void setPixel(long distance)
{
	/*
	Sets the pixels of the LED Strip.
	Sets 1 LED if an object is 5cm away and 16 LEDs if an object is 50cm away.
	Also sets the color of the LEDs to fade from green, if the object is close, to yellow, orange and red as the object gets closer.
	*/

	int numberOfLEDS = 0;

	if(distance < 51 && distance > 4) 						// If the distance is between 5 and 50 cm
	{
		numberOfLEDS = (50-distance)/3 +1;       			// Calculates the number of LEDs to light up
	}
	else if(distance < 5) 									// If the distance is less than 5 cm
	{
		numberOfLEDS = 16;									// Sets 16 LEDs
	}

	int green = ((16-numberOfLEDS)*10000)/588;     			// Calculates the green Channel to go from 255-0 as an object gets closer
	int red = ((numberOfLEDS-1)*10000)/588;  				// Calculates the red Channel to go from 0-255 as an object gets closer

	for (int i = 0; i < ((numberOfLEDS-1)*3); i+=3)
	{
		pixels[i] = green;									// Sets the green Channel 
		pixels[i+1] = red;									// Sets the red Channel 
		pixels[i+2] = 0;									// Sets the blue Channel 
	}

	pixels[(numberOfLEDS-1)*3] = (green*(3-distance%3))/3;	// Sets the color Channel of the last LED to make the color fade
	pixels[(numberOfLEDS-1)*3+1] = (red*(3-distance%3))/3;	// as an object moves away 
	pixels[(numberOfLEDS-1)*3+2] = 0;						

	for(int i = numberOfLEDS*3; i<LEDS*3; i++)  			// Sets unused LEDs to 0
	{	
		pixels[i] = 0;									
	}
	
}

void adc_init(){

	/*
		Initiates the ADC of the ESP-32C3
	*/

	*IO_MUX_GPIO0_REG &= ~(0b11 << 7); 						//Disables pulldown and pullup for GPIO0

	*APB_SARADC_CTRL_REG |= 1;								// Selects software to start SAR ADC
	*APB_SARADC_CTRL_REG |= (1 << 1);						// Starts the SAR ADC by software

	*APB_SARADC_ONETIME_SAMPLE_REG |= (0b11 << 23); 		// Configures the attenuation for one-time sampling
	*APB_SARADC_ONETIME_SAMPLE_REG |= (1 << 31); 			// Enables SAR ADC1 one-time sampling
	*APB_SARADC_ONETIME_SAMPLE_REG &= ~(0b1111 << 25); 		// Configures the channel for one-time sampling

	*APB_SARADC_INT_ENA_REG |= (1 << 31); 

	adc_calibrate(1); 										// Calibrates the adc

}

uint32_t adc_read(){

	/*
		Reads the ADC value and returns it.
	*/

	int adcValue = 9999; 									// Initial Value 

	*APB_SARADC_ONETIME_SAMPLE_REG |= (1<< 29);				// Starts the ADC
	for (int n = 0; n < 1000; n++) {}						// Waits for the ADC to finish
	*APB_SARADC_ONETIME_SAMPLE_REG &= ~(1<< 29);			// Stops the ADC

	while(((*APB_SARADC_INT_RAW_REG & (1<<31)) >> 31)!=1){}	// Waits for the ADC to finish

	adcValue = *APB_SARADC_1_DATA_STATUS_REG & 0xFFFF; 		// Reads the ADC value
	return adcValue;
}

void setBrightness()
{
	/*
		Sets the brightness of the LED Strip.
	*/

	adc_init();
	int value = adc_read();

	value = (value-19)/4 + 100; 							// Maps the ADC value to a value between 100 and 1100

	for(int i = 0; i<LEDS*3; i++)							// For every channel of every LED
	{							
		int color = (pixels[i]*value)/1000; 				// Sets the brightness of the color by multiplying the color with 0,1-1,1
		if(color < 256) 									// If the new color doesn't exceed 255
		{
			pixels[i] = color;								// Set the new color
		}
	}
}



void loop()
{
	/*
	Main Loop
	*/

	long distance = getDistance();                       	// Reads the Sensor
	setPixel(distance);                                 	// Sets the Pixels
	setBrightness();										// Sets the Brightness
	displayLEDs();                                          // Displays the Pixels
	delay(200000);                                     		// Waits 0.2s
}

void app_main(void)
{
	
    *gpio_enable_reg |= 0b100000110; 						// enable GPIO1 and GPIO2 output
	config_timer();										 	// Configures the Timer

	while(1)
	{
		loop();
	}
}
