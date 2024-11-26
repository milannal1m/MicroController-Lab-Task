

#include <stdio.h>
#include <stdint.h>
#include <esp32c3_reg.h>
#include "esp_intr_alloc.h"

#define LEDS 16
uint8_t pixels[LEDS*3]; // Komponenten für Grün, Rot und Blau je LED


volatile uint32_t* gpio_out_reg = (volatile uint32_t*) GPIO_OUT_REG;
volatile uint32_t* gpio_in_reg = (volatile uint32_t*) GPIO_IN_REG;

void delay(uint32_t us)
{
	//delay of us* microseconds

	asm volatile(
		"1: \n\t"
		"beq %0,zero,3f \n\t"
		"li t0,38 \n\t"
		"2: \n\t"
		"addi t0,t0,-1 \n\t"
		"bne t0, zero, 2b \n\t"
		"addi %0,%0,-1 \n\t"
		"j 1b \n\t"
		"3: \n\t"
		:
		: "r"(us)
		:"t0"
	);

}

void display()
{
	//Displays pixels onto the LED

	int factor = 1;

	for(int i = 0; i<LEDS*3; i++){


		for (int n = 7; n >= 0; n--) {
			// Überprüfen, ob das i-te Bit gesetzt ist
			if (pixels[i] & (1 << n)) {
				factor = 2;
			}else{
				factor = 1;
			}

			asm volatile (

				//GPIO2 HIGH

				"li t0,2						\n\t"
				"lw t1, gpio_out_reg			\n\t"
				"or t2,t1,t0 					\n\t" // *gpio_out_reg |= (1 << 1)
				"sw t2, 0(t1)					\n\t"

				//Warteschleife HIGH
				"li t0, 10                  	\n\t" //t0 = 720
				"mul t2,t0,%1					\n\t" //t2 = 720*factor
				"add t3,t2,zero 				\n\t" // t3 = t2

				"1:                  			\n\t"
				"addi t2,t2,-1                  \n\t" // t2--
				"bne t2,zero,1b                 \n\t" // if t2 != 0 -> 1

				//GPIO2 LOW
				"li t0,2						\n\t"
				"lw t1,gpio_out_reg				\n\t"
				"not t0,t0 						\n\t"
				"add t2,t1,t0 					\n\t" // *gpio_out_reg &= ~(1 << 1)
				"sw t2, 0(t1)					\n\t" 

				//Wartschleife LOW
				"li t4, 39					\n\t" // t4 = 3000
				"sub t3,t4,t3					\n\t" // t3 = 3000 - 720*factor

				"2:                  			\n\t"
				"addi t3,t3,-1                  \n\t" // t3--
				"bne t3,zero,2b                 \n\t" // if t3 != 0 -> 2

				:
				:    
				"r" (gpio_out_reg), 	// %0: Adresse von gpio_out_reg
				"r" (factor)           // %1: Adresse von factor 
				: "t0", "t1", "t2","t3","t4"
			);
		}

	}
	delay(50);

	

}

void config_timer()
{
	volatile uint32_t* timg_t0_config_reg = (volatile uint32_t*) TIMG_T0CONFIG_REG;
	volatile uint32_t* timg_t0alarmlo_reg = (volatile uint32_t*) TIMG_T0ALARMLO_REG;

	*timg_t0_config_reg |= (5 << 16); // DIVIDER
	*timg_t0_config_reg |= (1 << 10); // ALARMEN

	*timg_t0alarmlo_reg = 1048576;

	*timg_t0_config_reg |= (1 << 31);

}

void readSensor()
{
	// Reads the Echo Sensor

	config_timer();

	*gpio_out_reg |= (1 << 2);
	delay(1000000);
	*gpio_out_reg &= ~(1 << 2);

	*TIMG_T0UPDATE_REG |= (1 << 31);

	long time1 = 0;

	while(*gpio_in_reg & (1<<3)){
		if(time1 != 0){
			time1 = *TIMG_T0LO_REG;
		}
	}

	*TIMG_T0UPDATE_REG |= (1 << 31);
	long time2 = *TIMG_T0LO_REG;

	long timediff = time2-time1;

	printf("%li\n\n",timediff);
}

void loop()
{
	//
}




void app_main(void)
{
	
    volatile uint32_t* gpio_enable_reg = (volatile uint32_t*) GPIO_ENABLE_REG;
	
    *gpio_enable_reg |= 0b110;  // enable GPIO1 output

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
