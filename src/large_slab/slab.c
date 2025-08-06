#include <stdio.h>
#include <stdbool.h>
#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"
#include "slab.h"


bool get_bit(char num, int i){
	return ((num & (1 << i)) != 0);
}
char set_bit(char num, int i){
	return num | (1 << i);
}
char clear_bit(char num, int i){
	return num & (~(1 << i));
}

unsigned int nextPower(unsigned int n)
{
	unsigned count  = 0;
	if(n <= 16 && n > 0)
		return 0;
	n--;
	while(n > 0)
	{
		n>>=1;
		count ++;
	}
	return count - 4;
}

struct {
	struct spinlock lock;
	struct slab slab[NSLAB];
} stable;


void bitmap_set(char *bitmap, int bit_index){
	int byte_idx = bit_index / 8;
	int bit_idx = bit_index % 8;
	bitmap[byte_idx] = set_bit(bitmap[byte_idx], bit_idx);
}

void bitmap_clear(char *bitmap, int bit_index) {
    int byte_idx = bit_index / 8;
    int bit_idx = bit_index % 8;
    bitmap[byte_idx] = clear_bit(bitmap[byte_idx], bit_idx);
}

bool bitmap_test(char *bitmap, int bit_index) {
    int byte_idx = bit_index / 8;
    int bit_idx = bit_index % 8;
    return get_bit(bitmap[byte_idx], bit_idx);
}



void cover_page(struct slab *s)
{
	int write_idx = 0;
	int objs = s->num_objects_per_page;
	for(int m = 0 ; m < s->num_pages ; m++)
	{
		if(s->page[m] == 0)
		{
				write_idx = m;
			break;
		}
	}
	int r = write_idx;
	int check_byte = (objs + 7) / 8;
	for(int read_idx = r ; read_idx < s->num_pages ; read_idx++)
	{
		if(s->page[read_idx] == 0)
		{	
			continue;
		}
		if(write_idx!=read_idx)
		{
	
			s->page[write_idx] = s->page[read_idx];	
			if(objs % 8 == 0){
			memmove(&s->bitmap[write_idx * check_byte], &s->bitmap[read_idx * check_byte], check_byte);
			memset(&s->bitmap[read_idx * check_byte], 0, check_byte);
			s->page[read_idx] = 0;
			}
			else
			{
				for(int i = 0 ; i < objs ; i++){
	   				int del_bit = read_idx * objs + i;
       					int str_bit = write_idx * objs + i;
					if(bitmap_test(s->bitmap, del_bit))
					{
						bitmap_set(s->bitmap, str_bit);
					}
					else
					{
						bitmap_clear(s->bitmap, str_bit);
					}
					bitmap_clear(s->bitmap, del_bit);
				}
				s->page[read_idx] = 0;
			}
		}	
		write_idx ++;
	}
	
	int ans = 0;
	for(int i = 0 ; i < s->num_pages ; i++)
		if(s->page[i] != 0)
		{
			ans ++;	
		}

	s->num_pages =ans;
	if(s->num_pages == 0)
	{
		s->page[0] = kalloc();
		memset(s->page[0], 0, PGSIZE);
		s->num_pages = 1;
		s->num_free_objects +=PGSIZE / s->size;
	}
}

void slabinit(){
	initlock(&stable.lock, "slab lock");
	acquire(&stable.lock);
	int slab_size[8] = {16, 32, 64, 128, 256, 512, 1024, 2048};
	
	struct slab *s;

	for(int i = 0 ; i <NSLAB ; i++){
		s = &stable.slab[i];
		s->size = slab_size[i];
		s->num_pages = 1;
		s->num_free_objects = PGSIZE / slab_size[i];
		s->num_used_objects = 0;

		if((s->bitmap = (char *)kalloc())==0)
			panic("slabinit: no mem");
		memset(s->bitmap, 0, PGSIZE);
		
		s->num_objects_per_page = PGSIZE / slab_size[i];
		for(int j = 0 ; j < MAX_PAGES_PER_SLAB ; j++)
			s->page[j] = 0;

		s->page[0] = kalloc();

	        if (s->page[0] == 0)
			panic("slabinit: no page mem");
		
	}
	release(&stable.lock);
}


char *kmalloc(int size){
	if(size <= 0 || size > PGSIZE * MAX_PAGES_PER_SLAB)
		return 0;
	
	acquire(&stable.lock);
	
	if(size > MAX_OBJ_SIZE)
	{
		int page_num = (size + PGSIZE - 1) / PGSIZE ;
		release(&stable.lock);
		return kalloc_large(page_num);
	}
	
	
	int slab_num = nextPower(size);
	struct slab *s = &stable.slab[slab_num];
	if(s->bitmap == 0){
		s->bitmap = (char *) kalloc();
		if(s->bitmap == 0)
		{
			release(&stable.lock);
			return 0;
		}

		memset(s->bitmap, 0, PGSIZE);
	}
	if(s->num_free_objects == 0 || s->page[0] == 0)
	{
		if(s->num_pages < MAX_PAGES_PER_SLAB)
		{
			s->page[s->num_pages] = kalloc();
			if(s->page[s->num_pages] == 0)
			{
				release(&stable.lock);
				return 0;
			}
			s->num_pages++;
			s->num_free_objects = s->num_objects_per_page;
		}
		else
		{
			release(&stable.lock);
			return 0;
		}
	}

	int check_total = s->num_pages * s->num_objects_per_page;
	for(int obj = 0 ; obj < check_total ; obj++)
	{
		int check_page = obj / s->num_objects_per_page;
		
		if(s->page[check_page] == 0)
		{
			obj = obj + s->num_objects_per_page - 1;
			continue;
		}
		
		if(bitmap_test(s->bitmap, obj))
			continue;
		
		int offset = (obj % s->num_objects_per_page) * s->size;
		bitmap_set(s->bitmap, obj);
		s->num_free_objects--;
		s->num_used_objects++;
		char *addr = (char *)s->page[check_page] + offset;
		release(&stable.lock);
		return addr;
	}
	release(&stable.lock);
	return 0;
}

void kmfree(char *addr, int size){
	if(size <= 0 || size > PGSIZE * MAX_PAGES_PER_SLAB)
		return ;

	acquire(&stable.lock);
	if(size > MAX_OBJ_SIZE)
	{
		int free_num = (size + PGSIZE - 1) / PGSIZE;
		for(int i = 0 ; i< free_num ; i++)
    			kfree(addr + i * PGSIZE);
		release(&stable.lock);
		return;
	}
	int slab_num = nextPower(size);
	struct slab *s = &stable.slab[slab_num];
	if(s == 0)
	{
		release(&stable.lock);
		return ;
	}
	int page_idx = -1;
	int obj_idx = -1;
	for(int i = 0  ; i < s->num_pages ; i++)
	{
		if(addr >= (char *)s->page[i] && addr < (char*)s->page[i] + PGSIZE)
		{
			page_idx = i;
			obj_idx = (addr - (char *)s->page[i]) / s->size;
			break;
		}
	}
	if(page_idx == -1 || obj_idx == -1)
	{
		release(&stable.lock);
		return ;
	}
	int obj_num = page_idx * s->num_objects_per_page + obj_idx;
	if(!bitmap_test(s->bitmap, obj_num))
	{
		release(&stable.lock);
		return ;
	}
	bitmap_clear(s->bitmap, obj_num);
	s->num_used_objects--;
	s->num_free_objects++;
        for(int i = 0 ; i < s->num_pages ; i++)
        {
                if(s->page[i])
                {
                        int obj_idx = i * s->num_objects_per_page;
                        int obj_idx_end = obj_idx + s->num_objects_per_page;
                        int byte_start = obj_idx / 8;
                        int byte_end = (obj_idx_end + 7) / 8;
                        int page_use = 0;
                        if(s->size == 1024 || s->size == 2048)
                        {
                                int empty = 1;
                                for(int j = 0 ; j<s->num_objects_per_page ; j++)
                                {
                                        int bit = i * s->num_objects_per_page + j;
                                        if(bitmap_test(s->bitmap, bit)){
                                                empty = 0;
                                                break;
                                        }
                                }
                                if(empty == 1 && s->num_pages > 1)
                                {
                                        kfree(s->page[i]);
                                        s->page[i]  = 0;
                                        s->num_free_objects -= s-> num_objects_per_page;
                                        continue;
                                }
                        }
                        else{
                                for(int j = byte_start ; j<byte_end ; j++)
                                {
                                        if(s->bitmap[j] !=0)
                                        {
                                                page_use = 1;
                                                break;
                                        }
                                }

                                if(!page_use && i >= 0)
                                {
                                        kfree(s->page[i]);
                                        s->page[i] = 0;
                                        s->num_free_objects -= s->num_objects_per_page;
                                }
                        }
                }
        }
        cover_page(s);
        release(&stable.lock);
        return ;	

}

/* Helper functions */
void slabdump(){
	cprintf("__slabdump__\n");

	struct slab *s;

	cprintf("size\tnum_pages\tused_objects\tfree_objects\n");

	for(s = stable.slab; s < &stable.slab[NSLAB]; s++){
		cprintf("%d\t%d\t\t%d\t\t%d\n", 
			s->size, s->num_pages, s->num_used_objects, s->num_free_objects);
	}
}

int numobj_slab(int slabid)
{
	return stable.slab[slabid].num_used_objects;
}

int numpage_slab(int slabid)
{
	return stable.slab[slabid].num_pages;
}
