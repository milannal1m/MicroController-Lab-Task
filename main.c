#include <stdio.h>
#include <stdint.h>
#include <esp32c3_reg.h>
#include "esp_intr_alloc.h"

#define LEDS 16
uint8_t pixels[LEDS*3]; // 3  color channels per LED

void delay(uint32_t us)
{
    /*
        Waits us microseconds.
    */

	asm volatile(
		"1:                                     \n\t"
		"beq %0,zero,3f                         \n\t"   // if us == 0 -> 3
		"li t0,38                               \n\t"   // t0 = 38
		"2:                                     \n\t"
		"addi t0,t0,-1                          \n\t"   // t0--
		"bne t0, zero, 2b                       \n\t"   // if t0 != 0 -> 2
		"addi %0,%0,-1                          \n\t"   // us--
		"j 1b                                   \n\t"   // jump to 1
		"3:                                     \n\t"
		:
		: "r"(us)                                       // %0: adress of us
		:"t0"
	);

}

void display()
{
	/*
        Writes every bit of every number of the pixel data to GPIO1.
        Writes HIGH to GPIO1 for 600ns if the bit is 1.
        Writes LOW to GPIO1 for 300ns if the bit is 0.
        After Writing a Bit waits 650ns, if the bit was 1 and waits 950ns if the bit was 0.
        In the end waits for 50us to set the LED strip.
    */

	int factor = 1;

	for(int i = 0; i<LEDS*3; i++){                      // For every channel of every LED

		for (int n = 7; n >= 0; n--) {                  // For every bit of every number
			                                        
			if (pixels[i] & (1 << n)) {                 // If bit n of number i is 1
				factor = 2;                             // Set factor to 2 else 1
			}else{                                  
				factor = 1;
			}

			asm volatile (

				//GPIO2 HIGH
				"li t0,2						\n\t"   // t0 = 2
				"lw t1, gpio_out_reg			\n\t"   // t1 = *gpio_out_reg
				"or t2,t1,t0 					\n\t"   // *gpio_out_reg |= (2)
				"sw t2, 0(t1)					\n\t"   // store gpio_out_reg

				//Warteschleife HIGH
				"li t0, 10                  	\n\t"   // t0 = 10
				"mul t2,t0,%0					\n\t"   // t2 = 10*factor
				"add t3,t2,zero 				\n\t"   // t3 = t2

				"1:                  			\n\t"
				"addi t2,t2,-1                  \n\t"   // t2--
				"bne t2,zero,1b                 \n\t"   // if t2 != 0 -> 1

				//GPIO2 LOW
				"li t0,2						\n\t"   // t0 = 2
				"lw t1,gpio_out_reg				\n\t"   // t1 = *gpio_out_reg
				"not t0,t0 						\n\t"   // t0 = ~t0
				"add t2,t1,t0 					\n\t"   // *gpio_out_reg &= ~(2)
				"sw t2, 0(t1)					\n\t"   // store gpio_out_reg

				//Wartschleife LOW
				"li t4, 39					    \n\t"   // t4 = 39
				"sub t3,t4,t3					\n\t"   // t3 = 39 - 10*factor

				"2:                  			\n\t"
				"addi t3,t3,-1                  \n\t"   // t3--
				"bne t3,zero,2b                 \n\t"   // if t3 != 0 -> 2

				:
				:    
				"r" (factor)                            // %0: adress of factor
				: "t0", "t1", "t2","t3","t4"
			);
		}

	}
	delay(50);                                          // Wait 50us    

	

}

void config_timer()
{
    /*
    Configures the Timer
    */

  	*TIMG_T0CONFIG_REG &= ~(1 << 9);					// Enables the APB Clock
  	*TIMG_T0CONFIG_REG |= (1 << 30);					// Enables the Timer to Increment

	*TIMG_T0CONFIG_REG |= (80 << 13);                   // Sets the Divider of the Timer
	
  	*TIMG_T0LOADLO_REG &= 0;							// Initializes the Timer value with 0
	*TIMG_T0CONFIG_REG |= (1 << 31);                   	//Enables the Timer

}

long readSensor()
{
	/*
    Reads the Echo Sensor by using a Timer to measure the time between the rising and falling edge of the Echo signal.
	The Echo Signal is started by setting the Trigger Signal to HIGH for 10us.
	The Echo Signal is connected to GPIO3 and the Trigger Signal is connected to GPIO2.
	The time between the rising and falling edge of the Echo signal is converted to cm and returned.
    */

	long time = 0;                                     	// Variables to save the time at the rising 
	long time2 = 0;									   	// and falling edge of the Echo signal
	long timediff = 0;							   		// Variable to save the difference between time and time2

	*GPIO_OUT_REG |= (1 << 2);                          // Sets the Trigger to HIGH
	delay(10);                                     		// Wait 10us
	*GPIO_OUT_REG &= ~(1 << 2);                         // Sets the Trigger to LOW

	while(!(*GPIO_IN_REG & (1<<3))){}                   // Waits while the Echo is LOW
  
  	*TIMG_T0UPDATE_REG |= (1 << 31);                    // Allows the Counter of the Timer to be read
	while(!(*TIMG_T0UPDATE_REG == 0)){}					// Waits until the Timer is updated
  	time = *TIMG_T0LO_REG;								// Reads the time at the rising edge of the Echo signal

	while(*GPIO_IN_REG & (1<<3)){}                    	// Waits while the Echo is HIGH

	*TIMG_T0UPDATE_REG |= (1 << 31);                    // Allows the Counter of the Timer to be read
	while(!(*TIMG_T0UPDATE_REG == 0)){}  			 	// Waits until the Timer is updated
  	time2 = *TIMG_T0LO_REG;                             // Saves the time at the falling edge of the Echo signal

  	timediff = time2-time;								// Calculates the difference between time and time2

  	long distance = timediff/58;                        // Calculates the distance in cm
	return distance;

}

void setPixel(long distance)
{
	/*
	Sets the Pixels of the LED Strip.
	Sets 1 LED if an object is 5cm away and 16 LEDs if an object is 50cm away.
	Also sets the color of the LEDs to fade from green, if the object is close, to yellow, orange and red as the object gets closer.
	*/

	int numberOfLEDS = (50-distance)/3 +1;       		// Calculates the number of LEDs to light up

	int green = ((16-numberOfLEDS)*10000)/588;     		// Calculates the Green Channel to go from 255-0 as an object gets closer
	int red = ((numberOfLEDS-1)*10000)/588;  			// Calculates the Red Channel to go from 0-255 as an object gets closer

	for (int i = 0; i < numberOfLEDS*3; i=3) {
		pixels[i] = green;								// Sets the Green Channel 
		pixels[i+1] = red;								// Sets the Red Channel 
		pixels[i+2] = 0;								// Sets the Blue Channel 
	}
	
}

void loop()
{
	/*
	Main Loop
	*/

	long distance = readSensor();                       // Reads the Sensor
	setPixel(distance);                                 // Sets the Pixels
	display();                                          // Displays the Pixels
	delay(1000000);                                     // Waits 1s
}




void app_main(void)
{
	
    *GPIO_ENABLE_REG |= 0b110; 							 // enable GPIO1 and GPIO2 output
	config_timer();										 // Configures the Timer

	while(1){
		loop();
	}
}
