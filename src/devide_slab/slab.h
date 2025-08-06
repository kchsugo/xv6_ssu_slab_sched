#define NSLAB 8 // {16, 32, 64, 128, 256, 512, 1024, 2048}
#define MAX_PAGES_PER_SLAB 100
#define MAX_OBJ_SIZE 2048

struct slab {
	int size;
	int num_pages;
	int num_free_objects;
	int num_used_objects;
	int num_objects_per_page;
	char *bitmap;
	char *page[MAX_PAGES_PER_SLAB];
	int large_count;
	char *next_data_addr;
};

void slabdump();
