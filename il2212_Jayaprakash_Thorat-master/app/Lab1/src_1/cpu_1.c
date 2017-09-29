#include <stdio.h>
#include "system.h"
#include "io.h"
#include <altera_avalon_fifo.h>
#include <altera_avalon_fifo_regs.h>
#include <altera_avalon_fifo_util.h>

#define TRUE 1

/*
 * Mutex API
 */
#include "altera_avalon_mutex.h"



int main()
{

alt_mutex_dev *mutex_add;
int cpu_id = ALT_CPU_CPU_ID_VALUE;

mutex_add = altera_avalon_mutex_open( "/dev/mutex_0");

while(1)
{
alt_printf("\ncpu_1: Waiting for mutex");
while( altera_avalon_mutex_trylock(mutex_add, 1) != 0)
{

}
alt_printf("\n cpu_1: mutex unlocked");
	
alt_printf("\nThe given numbers are: %x\t%x  and the sum is stored in shared memory", IORD_32DIRECT(SHARED_ONCHIP_BASE, 0x00), IORD_32DIRECT(SHARED_ONCHIP_BASE, 0x04));

 /* writing  the addition to FIFO */

altera_avalon_fifo_write_fifo(FIFO_0_IN_BASE,FIFO_0_IN_CSR_BASE, IORD_32DIRECT(SHARED_ONCHIP_BASE, 0x00) +  IORD_32DIRECT(SHARED_ONCHIP_BASE, 0x04)); 
	
 altera_avalon_mutex_unlock (mutex_add);
}

  return 0;
}
