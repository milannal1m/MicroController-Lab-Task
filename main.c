#include <stdio.h>
#include <stdint.h>
#include <reg.h>

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

  *TIMG_T0CONFIG_REG &= ~(1 << 9);
  *TIMG_T0CONFIG_REG |= (1 << 30);

	*TIMG_T0CONFIG_REG |= (80 << 13);                    // Sets the Divider of the Timer to 5
	
  *TIMG_T0LOADLO_REG &= 0;

	*TIMG_T0CONFIG_REG |= (1 << 31);                   //Enables the Timer

}

long readSensor()
{
	/*
    Reads the Echo Sensor
    */

	config_timer();

  long time = 0;                                     // Variables to save the time at the rising and falling edge of the Echo signal
  long time2 = 0;
  long timediff = 0;

	*GPIO_OUT_REG |= (1 << 2);                          // Set GPIO2 to HIGH
	delay(10);                                     		// Wait 10us
	*GPIO_OUT_REG &= ~(1 << 2);                         // Set GPIO2 to LOW

	while(!(*GPIO_IN_REG & (1<<3))){                    // While the Echo is LOW
    }
  
  *TIMG_T0UPDATE_REG |= (1 << 31);                    // Allows the Counter of the Timer to be read
	while(!(*TIMG_T0UPDATE_REG == 0)){}
  time = *TIMG_T0LO_REG;

	while(*GPIO_IN_REG & (1<<3)){                       // While the Echo is HIGH
	}

	*TIMG_T0UPDATE_REG |= (1 << 31);                    // Allows the Counter of the Timer to be read
	while(!(*TIMG_T0UPDATE_REG == 0)){}
  time2 = *TIMG_T0LO_REG;                             // Saves the time at the falling edge of the Echo signal

  timediff = time2-time;

  long distance = timediff/58;                         // Calculates the distance in cm

}

/* Beispiel für Umrechnung ohne Gleitkomma-Arithmetik (Scale Optional)

int calculate_scaled(int variable) {
    // Skalierungsfaktor für höhere Genauigkeit
    const int scale = 1000;
    return (50 * variable * scale) / (127 * scale);
}

*/

void loop()
{
	//
}




void app_main(void)
{
	
    *GPIO_ENABLE_REG |= 0b110;  // enable GPIO1 output

	for (int i = 0; i < 15; i++) {
    	pixels[i] = 150;
	}

	for (int i = 15; i < LEDS*3; i++) {
    	pixels[i] = 0;
	}

	while(1){
		readSensor();
	}
	
	//display();

}

// *gpio_out_reg &= ~(1 << 2); AUS
// *gpio_out_reg |= (1 << 2);  AN