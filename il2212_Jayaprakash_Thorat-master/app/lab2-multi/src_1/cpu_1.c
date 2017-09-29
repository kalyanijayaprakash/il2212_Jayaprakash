#include <stdio.h>
#include "system.h"
#include "io.h"
#include <stdlib.h>
#include <alt_types.h>
#include <altera_avalon_fifo.h>
#include <altera_avalon_fifo_regs.h>
#include <altera_avalon_fifo_util.h>

#define PERFORMANCE 0
#define DEBUG 1
/*
 * Write the processed image back to shared memory
 */
void write_to_shared( unsigned char* image, alt_u32 size, int width)
{

unsigned char* shared = 0x103830;
alt_u64 *shared_word ;
alt_u32 i=0, j=0;

*shared++ = width;
*shared++ = size/width;
shared = shared+6;
shared_word = (alt_u64*) shared;

#if PERFORMANCE == 1
for( j=8; j>0; j--)
{
*shared_word++ = *((alt_u64*)(image+i));
*shared_word++ = *((alt_u64*)(image+i+8));
*shared_word++ = *((alt_u64*)(image+i+16));
*shared_word++ = *((alt_u64*)(image+i+24));

i=i+32;

}

#else if DEBUG==1
for( j=0; j<size; j+=8)
{
*shared_word = *((alt_u64*)(image +j));
shared_word++;

}
#endif

altera_avalon_fifo_write_fifo(FIFO_1_IN_BASE,FIFO_1_IN_CSR_BASE, 0x103830);	
}

/*
 * Function to peform sobel edge detection
 */
void sobelFilter( alt_u8* compact_img, alt_u32 width, alt_u32 height ) 
{
alt_u32 i=0;
alt_u32 j=0;
alt_u32 img_size = width*height;
alt_u8 sobel_img[img_size];
alt_32 g_x = 0;
alt_32 g_y = 0;
alt_u8 factor = 4; 
for(i=1;i< height-1;i++)

 {
	for(j=1; j<width-1;j++)
	{
	#if PERFORMANCE ==1
	g_x = compact_img[((i-1)<<factor)+(j-1)] - compact_img[((i-1)<<factor)+(j+1)] + (compact_img[((i)<<factor)+(j-1)]<<1)
              -(compact_img[((i)<<factor)+(j+1)]<<1)+ (compact_img[((i+1)<<factor)+(j-1)] -  compact_img[((i+1)<<factor)+(j+1)]);


	g_y = compact_img[((i-1)<<factor)+(j-1)]+ (compact_img[((i-1)<<factor)+(j)]<<1)+compact_img[((i-1)<<factor)+(j+1)]
              -(compact_img[((i+1)<<factor)+(j-1)]+ (compact_img[((i+1)<<factor)+(j)]<<1)+ compact_img[((i+1)<<factor)+(j+1)]); 
	
	if(g_x < 0) g_x = 0- g_x;

	if(g_y < 0) g_y = 0- g_y;
	
	sobel_img[(i<<factor)+j] = (g_x + g_y)>>3;

	#else if DEBUG == 1
	
	g_x = compact_img[((i-1)*width)+(j-1)] - compact_img[((i-1)*width)+(j+1)] + (compact_img[((i)*width)+(j-1)]<<1)-
		(compact_img[((i)*width)+(j+1)]<<1)+ (compact_img[((i+1)*width)+(j-1)] -  compact_img[((i+1)*width)+(j+1)]);


	g_y = compact_img[((i-1)*width)+(j-1)]+ (compact_img[((i-1)*width)+(j)]<<1)+compact_img[((i-1)*width)+(j+1)]-
		(compact_img[((i+1)*width)+(j-1)]+ (compact_img[((i+1)*width)+(j)]<<1)+ compact_img[((i+1)*width)+(j+1)]);   
	if(g_x < 0) g_x = 0- g_x;

	if(g_y < 0) g_y = 0- g_y;
	
	sobel_img[(i*width)+j] = (g_x + g_y)>>3; 

	#endif
	
	}

}

write_to_shared( sobel_img, img_size,width);

}	

/*
 * Function to read input image from shared memory and convert to compact image
 */
void process_img(unsigned char* shared)
{

alt_u8 size_x = *shared++;
alt_u8 size_y = *shared++;
alt_u32 m =0;
alt_u8 n=0;
alt_u64 *shared_word;
alt_u32 compact_index = 0;
alt_u8 gray_img[size_x*size_y] __attribute__ ((aligned(8)));
alt_u32 offset =0;
alt_u8 compact_img[size_x*size_y/4];

shared = shared+6;
shared_word = (alt_u64*) shared;
alt_u8 q=0;

#if PERFORMANCE == 1
for(n =64;  n>0; n--)
{

*(((alt_u64*)(gray_img))+q) = *(shared_word+q);
*(((alt_u64*)(gray_img))+q+1) = *(shared_word+q+1);
q=q+2;
}
#else if DEBUG == 1

for(q =0; q <(size_y*size_x)>>3; q++)
{

*(((alt_u64*)(gray_img))+q) = *(shared_word+q);

}
#endif

for(n  = 0; n<size_y-1;n++)
{

	for(m =0; m <size_x-1; m++)
	{
	compact_img[compact_index] = gray_img[m+offset] ;
			
	compact_index++;
	m++;
	}
offset = offset+ (size_x<<1);
n++;
}
sobelFilter(compact_img, size_x/2, size_y/2);

}


int main()
{
unsigned char* result;

// Initialising FIFO
altera_avalon_fifo_init(FIFO_0_IN_CSR_BASE,0,3,10);
altera_avalon_fifo_init(FIFO_0_OUT_CSR_BASE,0,3,10);
altera_avalon_fifo_init(FIFO_1_IN_CSR_BASE,0,3,10);
altera_avalon_fifo_init(FIFO_1_OUT_CSR_BASE,0,3,10);


// Send message to cpu_0 indicating cpu_1 is ready
altera_avalon_fifo_write_fifo( FIFO_1_IN_BASE, FIFO_1_IN_CSR_BASE, 0x1 );
	
while(1)
{

result = 0;

// Waiting for CPU_0 to write the image
while(result == 0)
{
result = altera_avalon_fifo_read_fifo(FIFO_0_OUT_BASE,FIFO_0_OUT_CSR_BASE );

}
process_img(result);
}
return 0;
}

