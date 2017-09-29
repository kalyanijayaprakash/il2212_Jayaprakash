#include <stdio.h>
#include "altera_avalon_performance_counter.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"
#include "system.h"
#include "io.h"
//#include "images.h"
#include "images_alt.h"
#include <stdlib.h>
#include <alt_types.h>
#include <altera_avalon_fifo.h>
#include <altera_avalon_fifo_regs.h>
#include <altera_avalon_fifo_util.h>

#define PERFORMANCE 0
#define DEBUG 1
#define SECTION_1 1

extern void delay (int millisec);

#if PERFORMANCE == 1
unsigned char* img_arrays[4] = {circle32x32, rectangle32x32, circle32x32,rectangle32x32};
int array_size =4;
unsigned char* offset= 0x102408;
unsigned char* offset1= 0x102810;
unsigned char* offset2= 0x102C18;
unsigned char* offset3= 0x103020;
unsigned char* offset4= 0x103428;

#else if DEBUG == 1
unsigned char* img_arrays[12] = {circle20x20, circle24x24, circle32x22,circle32x32, circle40x28, circle40x40,rectangle20x20, rectangle24x24, 
				rectangle32x22, rectangle32x32, rectangle40x28, rectangle40x40 };
int array_size =12;

unsigned char* offset= (unsigned char*)0x102648;
unsigned char* offset1= (unsigned char*)0x102C90;
#endif

int count= 0;
int counter =420;

unsigned char* shared = (unsigned char*) SHARED_ONCHIP_BASE;

/*
 * Function to convert RGB images stored in SRAM to gray scale and store gray scale image to shared memory
 */
void sram_shared(unsigned char* base, unsigned char* shared1)
{
alt_u32 x;
alt_u64 *shared_word;
alt_u8 size_x = *base++;
alt_u8 size_y = *base++;
alt_u32 index = 0;
alt_u8 grayscale_img[ size_x*size_y];

#if DEBUG ==1
printf("\nProcessing image:  %d x %d", size_x,size_y);

#endif
	
*base++;  // omit max color value

// convert to grayscale

#if PERFORMANCE == 1
for(x = 64; x > 0; x--)
	{
grayscale_img[index] = (*base++ + *base++ + *base++)>>1;
grayscale_img[index+1] = (*base++ + *base++ + *base++)>>1;
grayscale_img[index+2] = (*base++ + *base++ + *base++)>>1;
grayscale_img[index+3] = (*base++ + *base++ + *base++)>>1;
grayscale_img[index+4] = (*base++ + *base++ + *base++)>>1;
grayscale_img[index+5] = (*base++ + *base++ + *base++)>>1;
grayscale_img[index+6] = (*base++ + *base++ + *base++)>>1;
grayscale_img[index+7] = (*base++ + *base++ + *base++)>>1;
grayscale_img[index+8] = (*base++ + *base++ + *base++)>>1;
grayscale_img[index+9] = (*base++ + *base++ + *base++)>>1;
grayscale_img[index+10] = (*base++ + *base++ + *base++)>>1;
grayscale_img[index+11] = (*base++ + *base++ + *base++)>>1;
grayscale_img[index+12] = (*base++ + *base++ + *base++)>>1;
grayscale_img[index+13] = (*base++ + *base++ + *base++)>>1;
grayscale_img[index+14] = (*base++ + *base++ + *base++)>>1;
grayscale_img[index+15] = (*base++ + *base++ + *base++)>>1;

index=index+16;
}


// start storing values in shared memory
*shared1++  = 32;
*shared1++ = 32;
shared1=shared1+6;

// create a 64 bit pointer to point to the shared memory
shared_word = (alt_u64*)shared1;


index =0;

for(x = 16; x >0; x--)
	{
*shared_word++ = *((alt_u64*)(grayscale_img+index));
*shared_word++ = *((alt_u64*)(grayscale_img+index+8));
*shared_word++ = *((alt_u64*)(grayscale_img+index+16));
*shared_word++ = *((alt_u64*)(grayscale_img+index+24));
*shared_word++ = *((alt_u64*)(grayscale_img+index+32));
*shared_word++ = *((alt_u64*)(grayscale_img+index+40));
*shared_word++ = *((alt_u64*)(grayscale_img+index+48));
*shared_word++ = *((alt_u64*)(grayscale_img+index+56));
index = index+64;

}

#else if DEBUG == 1

for(x = 0; x < size_x*size_y; x++)
	{
grayscale_img[index] = (*base++ + *base++ + *base++)>>1;

index++;
}

index =0;

// start storing values in shared memory
*shared1++  = size_x;
*shared1++ = size_y;
shared1=shared1+6;

// create a 64 bit pointer to point to the shared memory
shared_word = (alt_u64*)shared1;

for(index = 0; index < size_x*size_y; index+=8)
	{
*(shared_word) = *((alt_u64*)(grayscale_img+index));
shared_word++;

}
#endif

}

/*
 * Function to read the processed image from shared memory
 */
void read_from_shared( unsigned char* shared)
{
alt_u8 size_x = *shared++;
alt_u8 size_y = *shared++;
alt_u64 *shared_word;
alt_u8 q=0,j=0;
alt_u8 ascii_img[size_x*size_y] __attribute__ ((aligned(8)));

// Align bytes
shared = shared+6;

// create a 64 bit pointer to point to the shared memory
shared_word = (alt_u64*) shared;


#if PERFORMANCE == 1
// Read processed image from shared memory
for( j=8; j>0; j--)
	{
	*(((alt_u64*)(ascii_img))+q) = *(shared_word+q);
	*(((alt_u64*)(ascii_img))+q+1) = *(shared_word+q+1);
	*(((alt_u64*)(ascii_img))+q+2) = *(shared_word+q+2);
	*(((alt_u64*)(ascii_img))+q+3) = *(shared_word+q+3);
	q=q+4;

	}
#else if DEBUG == 1
for(q =0; q <(size_y*size_x)>>3; q++)
{

*(((alt_u64*)(ascii_img))+q) = *(shared_word+q);

}

#endif

// update a count after each image is processed
counter--;


print_ascii(ascii_img, size_x, size_y);
}

/*
 * Function to convert image to ASCII image and print on console
 */
void print_ascii(alt_u8* img, alt_u8 size_x, alt_u8 size_y)
{
alt_u16 i,j;
alt_u32 img_size = size_x*size_y;
unsigned char symbols[16] = {32, 46, 59, 42, 115, 116, 73, 40, 106, 117, 86, 119, 98, 82, 103, 64};

// Comment this for-loop for printing ASCII
for(i = 0; i < img_size; i++)
{
img[i] = symbols[img[i]%16];
}

// Uncomment this for-loop for printing ASCII
/*for(i = 0; i < size_y; i++)
	{
	for(j=0; j<size_x;j++)
		{
		printf("%c",symbols[img[j+(i*size_x)]%16]);
		}
	printf("\n");
	}*/
}


/*
 * Function to monitor FIFO and get the address of processed image
 */
void read_image()
{
alt_u8 i = 0,n;
#if PERFORMANCE == 1
n =6;
#else if DEBUG == 1
n= 3;
#endif
for(i=0; i<n; i++)
	{
	while (altera_avalon_fifo_read_status(FIFO_1_OUT_CSR_BASE,ALTERA_AVALON_FIFO_STATUS_E_MSK) == 2)                        // this is to check if fifo is  not empty
		{
           	                           
	        }
          // Get the address from FIFO of processed image and read

	read_from_shared(altera_avalon_fifo_read_fifo(FIFO_1_OUT_BASE,FIFO_1_OUT_CSR_BASE));  
	}
}

#if PERFORMANCE
/*
 * Entry point to start storing 6 gray scale images in shared memory
 */
void store_image()
{
sram_shared(img_arrays[(count)%array_size], shared);
altera_avalon_fifo_write_fifo(FIFO_0_IN_BASE,FIFO_0_IN_CSR_BASE, shared);

sram_shared(img_arrays[(count+1)%array_size], offset);
altera_avalon_fifo_write_fifo(FIFO_0_IN_BASE,FIFO_0_IN_CSR_BASE,offset);

sram_shared(img_arrays[(count+2)%array_size], offset1);
altera_avalon_fifo_write_fifo(FIFO_0_IN_BASE,FIFO_0_IN_CSR_BASE, offset1);

sram_shared(img_arrays[(count+3)%array_size], offset2);
altera_avalon_fifo_write_fifo(FIFO_0_IN_BASE,FIFO_0_IN_CSR_BASE, offset2);

sram_shared(img_arrays[(count+4)%array_size], offset3);
altera_avalon_fifo_write_fifo(FIFO_0_IN_BASE,FIFO_0_IN_CSR_BASE, offset3);

sram_shared(img_arrays[(count+5)%array_size], offset4);
altera_avalon_fifo_write_fifo(FIFO_0_IN_BASE,FIFO_0_IN_CSR_BASE, offset4);

// Start reading processed images
read_image();

}
#endif

#if DEBUG
/*
 * Entry point to start storing 6 gray scale images in shared memory
 */
void store_image_debug()
{
sram_shared(img_arrays[(count)%array_size], shared);
altera_avalon_fifo_write_fifo(FIFO_0_IN_BASE,FIFO_0_IN_CSR_BASE, shared);

sram_shared(img_arrays[(count+1)%array_size], offset);
altera_avalon_fifo_write_fifo(FIFO_0_IN_BASE,FIFO_0_IN_CSR_BASE,offset);


sram_shared(img_arrays[(count+2)%array_size], offset1);
altera_avalon_fifo_write_fifo(FIFO_0_IN_BASE,FIFO_0_IN_CSR_BASE, offset1);

count = count+3;

// Start reading processed images
read_image();

}

#endif



int main(void) {
	
// Initialising FIFO
altera_avalon_fifo_init(FIFO_0_IN_CSR_BASE,0,3,10);
altera_avalon_fifo_init(FIFO_0_OUT_CSR_BASE,0,3,10);

// Initialising FIFO
altera_avalon_fifo_init(FIFO_1_IN_CSR_BASE,0,3,10);
altera_avalon_fifo_init(FIFO_1_OUT_CSR_BASE,0,3,10);

int fifo_element,i;

alt_printf("Waiting for all CPUs to be ready...\n");
	// Empty FIFOs from previously stored elements
while ( altera_avalon_fifo_read_status( FIFO_0_OUT_CSR_BASE, ALTERA_AVALON_FIFO_STATUS_E_MSK )!= 0x02 ) // Read FIFO while it is not empty
	{
		fifo_element = altera_avalon_fifo_read_fifo( FIFO_0_OUT_BASE, FIFO_0_OUT_CSR_BASE );
	}
while ( altera_avalon_fifo_read_status( FIFO_1_OUT_CSR_BASE, ALTERA_AVALON_FIFO_STATUS_E_MSK )!= 0x02 ) // Read FIFO while it is not empty
	{
		fifo_element = altera_avalon_fifo_read_fifo( FIFO_1_OUT_BASE, FIFO_1_OUT_CSR_BASE );
	}
	// Wait until ALL four CPUs have written to the FIFO. WARNING: This depends on the "almostempty" initialization value which is set to 3. With this value, when 4 elements have been written to the FIFO, the "almost empty" flag will be cleared

while ( altera_avalon_fifo_read_status( FIFO_1_OUT_CSR_BASE, ALTERA_AVALON_FIFO_STATUS_AE_MSK ) == 0x08 )
	{
		 
	}
	// Read all messages to double check they are from cpu_1 through cpu_4
	for( i = 0; i < 4; i++ ) // Read ONLY four messages. With the while we can read ALL messages until FIFO buffer is empty
	{
		fifo_element = altera_avalon_fifo_read_fifo( FIFO_1_OUT_BASE, FIFO_1_OUT_CSR_BASE );
		switch( fifo_element )
		{
			case 0x1:	alt_printf( "cpu_1 is ready!!!\n" ); break;
			case 0x2:	alt_printf( "cpu_2 is ready!!!\n" ); break;
			case 0x3:	alt_printf( "cpu_3 is ready!!!\n" ); break;
			case 0x4:	alt_printf( "cpu_4 is ready!!!\n" ); break;
			default:	alt_printf( "Something weird happened!!! Message received was: %x\n", fifo_element ); break;

		}
	}
alt_printf("ALL CPUs are ready!!!\n");


#if PERFORMANCE == 1

// Initialize and start the performance counter
PERF_RESET(PERFORMANCE_COUNTER_0_BASE);
PERF_START_MEASURING (PERFORMANCE_COUNTER_0_BASE);
PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_1);		
while (count<70)
	{
		/* Measurement here */
		store_image();
		count++;
	}

PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_1);

/* Print report */
perf_print_formatted_report
(PERFORMANCE_COUNTER_0_BASE,            
ALT_CPU_FREQ,        // defined in "system.h"
1,                   // How many sections to print
"Section 1"       // Display-name of section(s).
); 


#else if DEBUG==1

while(1)
{
		// Initialize and start the performance counter
		PERF_RESET(PERFORMANCE_COUNTER_0_BASE);
		PERF_START_MEASURING (PERFORMANCE_COUNTER_0_BASE);
		PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_1);

		store_image_debug();

		PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_1);
		/* Print report */
		perf_print_formatted_report
		(PERFORMANCE_COUNTER_0_BASE,            
		ALT_CPU_FREQ,        // defined in "system.h"
		1,                   // How many sections to print
		"Section 1"      // Display-name of section(s).
		);  
    
                delay(500);
}
#endif

		  
printf("\nTotal images processed: %d", 420-counter);  
 return 0;
}
