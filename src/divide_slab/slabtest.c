/* *********** DO NOT MODIFY THIS SOURCE FILE ************* */

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"
#include "slab.h"

#define TESTSIZE 2048 
#define TESTSLABID 7
#define MAXTEST 200
#define MAXSLABTEST 25600
#define TEST_PG_NUM 5
/* *********** DO NOT MODIFY THIS SOURCE FILE ************* */

/* 
NOTE:
We will grade your submission based on the original slabtest() codes as follows.
Thereby, you should pass all tests in the slabtest() function as is.
Note that, you can edit the test function only for the debugging purpose. 
*/

int* t[NSLAB][MAXSLABTEST] = {};

void slabtest(){
	int counter = 1;
	int start;
	int pass;
    	int slabsize;
    	int max_numslab;
	int* tmp;

	/* NOTE: 
	 * The current version of xv6 contains an error on aligning memory segments of the kernel,
	 * especially due to the literals of this source code. 
	 * More specifically, the '.stab' section should be aligned 
	 * to locate the VMA (virtual memory address) of an multiple of 4 bytes. 
	 * You can check the VMA of .stab by executing 'objdump -h kernel' in your xv6 source dir.
	 * If the VMA is not aligned to the multiple of 4B, then adjust (add/del chars) 
	 * the literal string in the cprintf function
	 */
	cprintf("==== SLAB TEST ====\n");

	/* TIPS:
	 *	You may debug your result with 
	 * cprintf();
	 * slabdump();
	 */
	/* TEST1: Single slab alloc */
	cprintf("==== TEST1 =====\n");
	start = counter;
	t[0][0] = (int*) kmalloc (TESTSIZE); 
	*(t[0][0]) = counter;
	counter++;
	cprintf( (*(t[0][0]) == start && numobj_slab(TESTSLABID) == 1) ? "OK\n":"WRONG\n");
	kmfree ((char*) t[0][0], TESTSIZE);
	/* TEST2: Single slab alloc: the size not equal to a power of 2. */
	cprintf("==== TEST2 =====\n");
	start = counter;
	t[0][0] = (int*) kmalloc (TESTSIZE-10); 
	*(t[0][0]) = counter;
	counter++;

	cprintf( (*(t[0][0]) == start && numobj_slab(TESTSLABID) == 1) ? "OK\n":"WRONG\n");
	kmfree ((char*) t[0][0], TESTSIZE);

	/* TEST3: Multiple slabs alloc */
	cprintf("==== TEST3 =====\n");
	start = counter;
	for (int i=0; i<NSLAB; i++)
	{
		slabsize = 1 << (i+4); 
		t[i][0]	= (int*) kmalloc (slabsize); 
		for (int j=0; j<slabsize/sizeof(int); j++)
		{
			memmove (t[i][0]+j, &counter, sizeof(int));
			counter++;
		}
	}
	// CHECK 
	pass = 1;
	for (int i=0; i<NSLAB; i++)
	{
		slabsize = 1 << (i+4); 
		for (int j=0; j < slabsize/sizeof(int); j++)
		{
			if ( *(t[i][0]+j) != start )
			{
				pass = 0;
				break;
			}
			start++;
		}
	}
	for (int i=0; i<NSLAB; i++)
	{
		slabsize = 1 << (i+4); 
		kmfree((char*) t[i][0], slabsize);
	}
	for (int i=0; i<NSLAB; i++){
        	if (numpage_slab(i) !=1 || numobj_slab(i) !=0){
        		pass = 0;
        		break;
        	}
        }
	cprintf( pass ? "OK\n" : "WRONG\n");	           
	
	/* TEST4: Multiple slabs alloc2 */
	cprintf("==== TEST4 =====\n");
	start = counter;
	for (int i=0; i<NSLAB; i++)
	{
		slabsize = 1 << (i+4); 
		for (int j=0; j<MAXTEST; j++)
		{
			t[i][j]	= (int*) kmalloc (slabsize); 
			for (int k=0; k<slabsize/sizeof(int); k++)
			{
				memmove (t[i][j]+k, &counter, sizeof(int));
				counter++;
			}
		}
	}
	slabdump();
	// CHECK
	pass = 1;
	for (int i=0; i<NSLAB; i++)
	{
		slabsize = 1 << (i+4); 
		for (int j=0; j<MAXTEST; j++)
		{
			for (int k=0; k<slabsize/sizeof(int); k++)
			{

				if (*(t[i][j]+k) != start)
				{
					pass = 0;
					break;
				}
				start++;
			}
		}
	}
	for (int i=0; i<NSLAB; i++)
	{
		slabsize = 1 << (i+4); 
		for (int j=0; j<MAXTEST; j++)
			kmfree((char*) t[i][j], slabsize);
	}
	slabdump();
	// DO NOT de-alloc first page. Allocate one page for slab cache.
	for (int i=0; i<NSLAB; i++){
		if (numpage_slab(i) !=1 || numobj_slab(i) !=0){
			pass = 0;
			break;
		}
	}
	cprintf( pass ? "OK\n" : "WRONG\n");	

	/* TEST5: alloc more than the page limit (100 pages) */
	cprintf("==== TEST5 =====\n");
	start = counter;
	for (int j=0; j<MAXTEST; j++)
	{
		t[0][j]	= (int*) kmalloc (TESTSIZE);
		for (int k=0; k<TESTSIZE/sizeof(int); k++){
                        memmove (t[0][j]+k, &counter, sizeof(int));
                        counter++;
                }
        }
        tmp = (int*) kmalloc (TESTSIZE);
        cprintf( (!tmp && numobj_slab (TESTSLABID) == MAXTEST) ? "OK\n" : "WRONG\n");
	/* TEST6: realloc slabs after free */
	cprintf("==== TEST6 =====\n");
	for (int j=0; j<MAXTEST; j++)
	{
		kmfree((char*) t[0][j], TESTSIZE);
	}

	start = counter;
	for (int j=0; j<MAXTEST; j++)
	{
		t[0][j]	= (int*) kmalloc (TESTSIZE); 
		for (int k=0; k<TESTSIZE/sizeof(int); k++)
		{
			memmove (t[0][j]+k, &counter, sizeof(int));
			counter++;
		}
	}

	// CHECK 
	pass = 1;
	for (int j=0; j<MAXTEST; j++)
	{
		for (int k=0; k<TESTSIZE/sizeof(int); k++)
		{
			
			//cprintf("%d, %d, %d, %d, %p\n", j, k, *(t[0][j]+k), start, t[0][j]+k);                //YOU MAY USE THIS
			if (*(t[0][j]+k) != start)
			{
				pass = 0;
				break;
			}
			start++;
		}
	}
	//cprintf( pass ? "OK\n" : "WRONG\n");	
	for (int j=0; j<MAXTEST; j++)
		kmfree((char*) t[0][j], TESTSIZE);

	for (int i=0; i<NSLAB; i++){
        	if (numpage_slab(i) !=1 || numobj_slab(i) !=0){
        		pass = 0;
        		break;
        	}
        }
        cprintf( pass ? "OK\n" : "WRONG\n");	

	/* TEST7: de-alloc empty slab pages */
	cprintf("==== TEST7 =====\n"); 	
    	max_numslab = 0;
    	pass = 1;

	for (int i=0; i<NSLAB; i++)
    	{
	    	int page_num_before = -1;
	    	int page_num_after = -1;

		slabsize = 1 << (i+4); 
		max_numslab = PGSIZE / slabsize * MAX_PAGES_PER_SLAB;

		for (int j=0; j<max_numslab; j++)
			t[i][j] = (int*) kmalloc (slabsize);

		page_num_before = numpage_slab(i);

		for(int j=0; j<max_numslab/2; j++)
			kmfree((char*) t[i][j], slabsize);
		
		page_num_after = numpage_slab(i);
        	//cprintf("%d, %d\n", page_num_before, page_num_after);                //YOU MAY USE THIS
        	// CHECK 
		if ( (page_num_before == MAX_PAGES_PER_SLAB) && (page_num_after == MAX_PAGES_PER_SLAB/2) )
        	    continue;
        	else
        	    pass = 0;
	}
	slabdump();
	for(int i=0; i<NSLAB; i++)
    	{
		slabsize = 1 << (i+4); 
		max_numslab = PGSIZE / slabsize * MAX_PAGES_PER_SLAB;
		for(int j=max_numslab/2; j<max_numslab; j++)
        	{
			kmfree((char*) t[i][j], slabsize);
		}
	} 
	slabdump();
	
	// DO NOT de-alloc first page. Allocate one page for slab cache.
	for (int i=0; i<NSLAB; i++){
        	if (numpage_slab(i) !=1 || numobj_slab(i) !=0){
        		pass = 0;
        		break;
        	}
        }
        cprintf( pass ? "OK\n" : "WRONG\n");	

    	/* TEST8: re-alloc empty slab pages */  
	cprintf("==== TEST8 =====\n"); 
	max_numslab = PGSIZE / TESTSIZE * MAX_PAGES_PER_SLAB;
    	pass = 1;

	for(int j=0; j<max_numslab; j++)
		t[0][j] = (int*) kmalloc (TESTSIZE);
	
	for(int j=0; j<max_numslab/2; j++)
		kmfree((char*) t[0][j], TESTSIZE);

    	start = counter;

	for(int j=0; j<max_numslab/2; j++)
    	{
		t[0][j] = (int*) kmalloc (TESTSIZE);
		for (int k=0; k<TESTSIZE/sizeof(int); k++)
        	{
			memmove (t[0][j]+k, &counter, sizeof(int));
			counter++;
		}
	}

	for (int j=0; j<max_numslab/2; j++)
    	{
		for (int k=0; k<TESTSIZE/sizeof(int); k++)
        	{
			if (*(t[0][j]+k) != start)
            		{
				pass = 0;
				break;
			}
			start++;
		}
	}

	for(int j=0;j<max_numslab;j++)
		kmfree((char*) t[0][j], TESTSIZE);

	// DO NOT de-alloc first page. Allocate one page for slab cache.
        for (int i=0; i<NSLAB; i++){
        	if (numpage_slab(i) !=1 || numobj_slab(i) !=0){
        		pass = 0;
        		break;
        	}
        }
        cprintf( pass ? "OK\n" : "WRONG\n");	

	/* TEST9: free some slabs already freed */
	cprintf("==== TEST9  ====\n");  
	pass =1;

	for(int i=0; i<NSLAB; i++)
    	{
		int numobjs = 0;
		slabsize = 1 << (i+4); 

		for (int j=0; j<16; j++)
			t[i][j] = (int*)kmalloc(slabsize);
		
		for (int j=8; j<16; j++)
			kmfree((char*) t[i][j], slabsize);
		
		numobjs = numobj_slab(i);

		for (int j=10; j<12; j++)
			kmfree((char*) t[i][j], slabsize);

		if (numobjs != numobj_slab(i))
            		pass = 0;
	}
	for(int i=0; i<NSLAB; i++){
		slabsize = 1 << (i + 4);
		for(int j=0; j<16; j++) 
			kmfree((char*) t[i][j], slabsize);
	}
	// DO NOT de-alloc first page. Allocate one page for slab cache.
        for (int i=0; i<NSLAB; i++){
        	if (numpage_slab(i) !=1 || numobj_slab(i) !=0){
        		pass = 0;
        		break;
		}
        }
        cprintf( pass ? "OK\n" : "WRONG\n");	


	/* TEST10: de-alloc empty slab pages */
	cprintf("===== TEST10 =====\n"); 	
    	max_numslab = 0;
    	pass = 1;

	for (int i=0; i<NSLAB; i++)
    	{
	    	int page_num_before = -1;
	    	int page_num_after = -1;

		slabsize = 1 << (i+4); 
		max_numslab = PGSIZE / slabsize * MAX_PAGES_PER_SLAB;

		for (int j=0; j<max_numslab; j++)
			t[i][j] = (int*) kmalloc (slabsize);
		
		page_num_before = numpage_slab(i);

		for(int j=0; j<max_numslab/4; j++)
			kmfree((char*) t[i][j], slabsize);
		for(int j=max_numslab/2 ; j<max_numslab/4*3 ; j++)
			kmfree((char*)t[i][j], slabsize);

		page_num_after = numpage_slab(i);
        	// CHECK 
        	if ( (page_num_before == MAX_PAGES_PER_SLAB)
			       	&& (page_num_after == MAX_PAGES_PER_SLAB/2) )
        	    continue;
        	else
        	    pass = 0;
	}
	slabdump();

	for(int i=0; i<NSLAB; i++)
    	{
		slabsize = 1 << (i+4); 
		max_numslab = PGSIZE / slabsize * MAX_PAGES_PER_SLAB;
		for(int j=max_numslab/4; j<max_numslab/2; j++)
        	{
			kmfree((char*) t[i][j], slabsize);
		}
		for(int j=max_numslab/4*3 ; j<max_numslab ; j++)
			kmfree((char *)t[i][j], slabsize);
	} 
	
	// DO NOT de-alloc first page. Allocate one page for slab cache.
	for (int i=0; i<NSLAB; i++){
        	if (numpage_slab(i) !=1 || numobj_slab(i) !=0){
        		pass = 0;
               }
	}
	slabdump();
        cprintf( pass ? "OK\n" : "WRONG\n");	


	//new_test
	cprintf("===== TEST11 =====\n");
	start = counter;
	t[0][0] = (int*) kmalloc (8000); 
	*(t[0][0]) = counter;
	t[0][1] = (int *)kmalloc(2020);
	*(t[0][1]) = counter;
	counter++;
	slabdump();	
	cprintf( (*(t[0][0]) == start && numobj_slab(TESTSLABID) == 3) ? "OK\n":"WRONG\n");	
	kmfree((char *)t[0][0], 8000);
	kmfree((char *)t[0][1], 2020);
	for (int i=0; i<NSLAB; i++){
        	if (numpage_slab(i) !=1 || numobj_slab(i) !=0){
        		pass = 0;
        		break;
        	}
        }
	slabdump();
	cprintf( pass ? "OK\n" : "WRONG\n");	           
	
	cprintf("===== TEST END =====\n");
}
