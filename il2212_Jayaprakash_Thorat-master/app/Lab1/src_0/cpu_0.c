#include <stdio.h>
#include "system.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"
#include "io.h"
#include "altera_avalon_performance_counter.h"
#include <altera_avalon_fifo.h>
#include <altera_avalon_fifo_regs.h>
#include <altera_avalon_fifo_util.h>

#define SECTION_1 1
#define SECTION_2 2
#define SECTION_3 3

/*
 * Mutex API
 */
#include "altera_avalon_mutex.h"

/*
 * LUT for seven segment display
 */

static int b2sLUT[] = {0x3F, //0
                 0x06, //1
                 0x5B, //2
                 0x4F, //3
                 0x66, //4
                 0x6D, //5
                 0x7D, //6
                 0x07, //7
                 0x7F, //8
                 0x67, //9
                 0x40, //-
};

/*
 * convert int to seven segment display format
 */
int int2seven(int inval){
    return b2sLUT[inval];
}

/*
 * Take the input from user and store in shared memory to get the calculation done from other CPUs. And, read the results from FIFO and display on BCD
 */
void Calculator()
{
alt_mutex_dev *mutex_calc = NULL ;
int cpu_id = ALT_CPU_CPU_ID_VALUE;
int operation = 0;
int num1= 0;
int num2 = 0;
int choice = 0;
int result = 0;
 int tmp = 0;
 int out= 0;
 int out_high = 0;
 int out_low = 0;
 int out_sign = 0;


// Initialising FIFO
altera_avalon_fifo_init(FIFO_0_IN_CSR_BASE,0,2,10);
altera_avalon_fifo_init(FIFO_0_OUT_CSR_BASE,0,2,10);


/* Reset Performance Counter */
  PERF_RESET(PERFORMANCE_COUNTER_0_BASE);  

  /* Start Measuring */
  PERF_START_MEASURING (PERFORMANCE_COUNTER_0_BASE);

  /* Section 1  - Mutex lock  */
  PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_1);

altera_avalon_mutex_lock (altera_avalon_mutex_open( "/dev/mutex_0"), 1);
altera_avalon_mutex_lock (altera_avalon_mutex_open( "/dev/mutex_1"), 1);
altera_avalon_mutex_lock (altera_avalon_mutex_open( "/dev/mutex_2"), 1);
altera_avalon_mutex_lock (altera_avalon_mutex_open( "/dev/mutex_3"), 1);
altera_avalon_mutex_lock (altera_avalon_mutex_open( "/dev/mutex_4"), 1);

  PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_1);

/* Section 2  - Take input -> store in shared memory -> mutex unlock*/

  PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_2);

alt_putstr("\nEnter number1: ");
num1 = alt_getchar(stdin);
alt_putchar(num1);
num1-= '0';

alt_putstr("\nEnter number2: ");
num2 = alt_getchar(stdin);
alt_putchar(num2);
num2 -= '0';

/* Writing input to the shared memory*/

      IOWR_32DIRECT(SHARED_ONCHIP_BASE, 0x00, num1);
      IOWR_32DIRECT(SHARED_ONCHIP_BASE, 0x04, num2);

alt_putstr("\nPress push buttons your choice of operation (1: leftmost and 4: rightmost) : 1.Addition  2. Subtraction 3. Multiplication 4. Division :");

//Waiting for input from push buttons

while(IORD_ALTERA_AVALON_PIO_DATA(BUTTONS_BASE) == 0)
{


}


operation =  IORD_ALTERA_AVALON_PIO_DATA(BUTTONS_BASE);
 
switch(operation)
       {
           case 8 : 
		 alt_putstr("\n Waiting for cpu_1 to perform addition");
                  mutex_calc = altera_avalon_mutex_open( "/dev/mutex_0");
                  altera_avalon_mutex_unlock (mutex_calc);
           break;

           case 4 : 
		 alt_putstr("\n Waiting for cpu_2 to perform subtraction");
                   mutex_calc = altera_avalon_mutex_open( "/dev/mutex_1");
                    altera_avalon_mutex_unlock (mutex_calc);
           break;

           case 2 : 
	         alt_putstr("\n Waiting for cpu_3 to perform multiplication");
                 mutex_calc = altera_avalon_mutex_open( "/dev/mutex_2");                   
                 altera_avalon_mutex_unlock (mutex_calc);
           break;

           case 1 : 
		 alt_putstr("\n Waiting for cpu_4 to perform division");
                  mutex_calc = altera_avalon_mutex_open( "/dev/mutex_3");
                  altera_avalon_mutex_unlock (mutex_calc);
           break;

          default:  alt_putstr ("Enter a valid choice!!");
           break;

      }
  PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_2);

  /* Section 3 Lock the mutex to read the result from FIFO and display on Seven segment display */

  PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_3);
 

while( altera_avalon_mutex_trylock(mutex_calc, 1) != 0)
{

}
     
while (altera_avalon_fifo_read_status(FIFO_0_OUT_CSR_BASE,ALTERA_AVALON_FIFO_STATUS_E_MSK)!=2)                        // this is to check if fifo is  not empty
		{
			 result = altera_avalon_fifo_read_fifo(FIFO_0_OUT_BASE,FIFO_0_OUT_CSR_BASE);                                                     // this is to pop the content of FIFO
		}

 printf("\nThe result is : %d", result);

// Print result on seven segment display
tmp = result;
if (tmp<0)
{
     out_sign = int2seven(10);
     tmp *= -1;
}
else{
     out_sign = int2seven(0);
  }

out_high = int2seven(tmp / 10);
  out_low = int2seven(tmp - (tmp/10) * 10);
  
  out = int2seven(0) << 24 |
            out_sign << 16 |
            out_high << 8 |
            out_low;

  IOWR_ALTERA_AVALON_PIO_DATA( HEX3_HEX0_BASE, out);

alt_printf("\n  Do you want to continue? 1. Yes 2. No");

choice = alt_getchar(stdin);	
alt_putchar(choice);
choice -= '0';
if(choice == 1)
{
      	Calculator();
}
else	
  {

alt_putstr("\nThank you for using Calculator!");

}
PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_3);

  /* End Measuring */
  PERF_STOP_MEASURING(PERFORMANCE_COUNTER_0_BASE);

}

int main()
{

alt_printf("\nHello from CPU %d  (--instance number may be different).\n", 
          ALT_CPU_CPU_ID_VALUE);

// Calling function to perform simple calculator
Calculator();

  /* Print measurement report */
  perf_print_formatted_report
	 (PERFORMANCE_COUNTER_0_BASE,            
	  ALT_CPU_FREQ,                 // defined in "system.h"
	  3,                            // How many sections to print
	  "Lock_ALL_Mutex", "Write_in_Shared_Memory", "Read_From_FIFO_Peripherals");    // Display-name of section(s).

  return 0;
}
