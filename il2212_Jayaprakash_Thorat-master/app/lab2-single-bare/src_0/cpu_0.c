#include <stdio.h>
#include "altera_avalon_performance_counter.h"
#include "includes.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"
#include "sys/alt_alarm.h"
#include "system.h"
#include "io.h"
#include "images_alt.h"
#include <stdlib.h>
#include <alt_types.h>

#define PERFORMANCE 0
#define DEBUG 1

#define SECTION_1 1

extern void delay (int millisec);


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
 * Function to peform sobel edge detection
 */
void sobelFilter( alt_u8* compact_img, alt_u32 width, alt_u32 height ) 
{
alt_u8 i=0;
alt_u8 j=0;
alt_u32 img_size = width*height;
alt_u8 sobel_img[img_size];
alt_32 g_x = 0;
alt_32 g_y = 0;

for(i=1;i< height-1;i++)
 {
	for(j=1; j<width-1;j++)
	{

	g_x = compact_img[((i-1)*width)+(j-1)] - compact_img[((i-1)*width)+(j+1)] + (compact_img[((i)*width)+(j-1)]<<1)-(compact_img[((i)*width)+(j+1)]<<1)+ (compact_img[((i+1)*width)+(j-1)] - 
		 compact_img[((i+1)*width)+(j+1)]);
	g_y = compact_img[((i-1)*width)+(j-1)]+ (compact_img[((i-1)*width)+(j)]<<1)+compact_img[((i-1)*width)+(j+1)]-(compact_img[((i+1)*width)+(j-1)]+ (compact_img[((i+1)*width)+(j)]<<1)+
		 compact_img[((i+1)*width)+(j+1)]);    

	if(g_x < 0) g_x = 0- g_x;

	if(g_y < 0) g_y = 0- g_y;

	sobel_img[(i*width)+j] = (g_x + g_y)>>3; 
	}

}

print_ascii( sobel_img, width ,height);

}




/*
 * Function to read RGB image from SRAM, convert to grayscale and compact grayscale image.
 */
void rgb_to_gray(unsigned char* base)
{
alt_u32 x;
alt_u8 size_x = *base++;
alt_u8 size_y = *base++;
alt_u8 grayscale_img[ size_x* size_y];
alt_u32 index = 0;
alt_u32 m =0;
alt_u8 n=0;
alt_u8 compact_index = 0;
alt_u8 compact_img[size_x/2*size_y/2];
	
	// incrementing pointer to omit max color 
	base++;

for(x = size_x*size_y; x > 0; x--)
{
	grayscale_img[index] = (*base++ + *base++ + *base++)>>1;
	index++;
}

alt_u32 offset =0;

for(n  = 0; n<size_y-1;n++)
{
 	for(m =0; m <size_x-1; m++)
	{
	compact_img[compact_index] = grayscale_img[m+offset] ;
	compact_index++;
	m++;
	}
offset = offset+ (size_x<<1);
n++;
}
sobelFilter(compact_img, size_x/2, size_y/2);
}


int main(void) {
	
	char current_image=0;
	

	#if PERFORMANCE == 1
	/* Sequence of images for measuring performance */
	char number_of_images=3;
	int count = 420;
	unsigned char* img_array[3] = {rectangle32x32, circle32x32, rectangle32x32};
	PERF_RESET(PERFORMANCE_COUNTER_0_BASE);
	PERF_START_MEASURING (PERFORMANCE_COUNTER_0_BASE);
	PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_1);
	while(count>0)
	{
		// Process image
		rgb_to_gray(img_array[current_image]);
	
		/* Increment the image pointer */
		current_image=(current_image+1) % number_of_images;
		
		count--;

	}	
		PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_1);  
		/* Print report */
		perf_print_formatted_report
		(PERFORMANCE_COUNTER_0_BASE,            
		ALT_CPU_FREQ,        // defined in "system.h"
		1,                   // How many sections to print
		"Section 1"       // Display-name of section(s).
		); 
	printf("Number of images processed: %d", 420-count);
	

	#else if DEBUG == 1
	/* Sequence of images for testing if the system functions properly */
	char number_of_images=12;
	
	unsigned char* img_array[12] = {circle20x20, circle24x24, circle32x22,circle32x32, circle40x28, circle40x40,rectangle20x20, rectangle24x24, 
				rectangle32x22, rectangle32x32, rectangle40x28, rectangle40x40 };
	while (1)
	{ 
		PERF_RESET(PERFORMANCE_COUNTER_0_BASE);
		PERF_START_MEASURING (PERFORMANCE_COUNTER_0_BASE);
		PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_1);
		
		// Process image
		rgb_to_gray(img_array[current_image]);
		
		PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_1);  

		/* Print report */
		perf_print_formatted_report
		(PERFORMANCE_COUNTER_0_BASE,            
		ALT_CPU_FREQ,        // defined in "system.h"
		1,                   // How many sections to print
		"Section 1"        // Display-name of section(s).
		);  

		/* Increment the image pointer */
		current_image=(current_image+1) % number_of_images;
             
		// Delay to observe the output
     		delay(500);
	}
	#endif
   
  return 0;
}
