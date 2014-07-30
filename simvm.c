/*
 * Skeleton code for CSC 360, Summer 2014,  Assignment #3.
 *
 * Prepared by: Michael Zastre (University of Victoria) 2014
 *
 *	To run:
 *	./simvm --file=/home/dustin/Documents/csc360/Assignments/Assign3/
 		csc360_a3_cpu_scheduling/goal_b/trace05.out
 		--framesize=12 --numframes=256 --scheme=fifo --progress
 *	Same but can use other framesizes, numframes, and scheme=lru
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>

#define SCHEME_NONE 0
#define SCHEME_FIFO 1
#define SCHEME_LRU  2
#define SCHEME_OPTIMAL 3

int page_replacement_scheme = SCHEME_NONE;

#define TRUE 1
#define FALSE 0
#define PROGRESS_BAR_WIDTH 30
#define MAX_LINE_LEN 100
/*Added for debugging; use false=0, true=1*/
#define debug 0
#define debug2 0
#define debug3 0

int size_of_frame = 0;  /* power of 2 */
int size_of_memory = 0; /* number of frames */

int initialize(void);
int finalize(void);
int output_report(void);
long resolve_address(long, int);
void error_resolve_address(long, int);

int page_faults = 0;
int mem_refs    = 0;
int swap_outs   = 0;
int swap_ins    = 0;
int lru_tick	= 0;	/*Added for lru scheme*/

struct page_table_entry *page_table = NULL;

struct page_table_entry {
	long page_num;
	int dirty;
	int free;
	int lru_timer;	/*Added for lru scheme*/
};

typedef struct node {   /*Used as a queue to see which frames are used first*/
    long task_num;
    struct node *next;
} Node;
Node *head = NULL;
Node *tail = NULL;
bool isEmpty() {
    return head == NULL;
}
void push(long t) {
	Node *newNode = (Node*) malloc (sizeof(Node));
	newNode->task_num = t;
	newNode->next = NULL;
	if(isEmpty()) {
		head = newNode;
		tail = newNode;
	} else {
		tail->next = newNode;
		tail = newNode;
	}
}/*End of push()*/
long pop() {
	if(head == NULL) {
		printf("Error: Nothing to pop\n");
		exit(1);
	}
	Node *tmp = head;
	head = head->next;
	long p = tmp->task_num;
	free(tmp);
	return p;
}/*End of pop()*/

int lru(long page) {	/*Used a time function to deal with lru*/
	int k;
	int least_recent_index = 0;	/*Used for the index of the lru page*/
	int least_recent_used = 0;	/*Used as just the lru*/
	for(k=0; k<size_of_memory; k++) {
		if(least_recent_used < page_table[k].lru_timer) {
			least_recent_used = page_table[k].lru_timer;
			least_recent_index = k;
		}
	}
	page_table[least_recent_index].page_num = page;
	page_table[least_recent_index].lru_timer = 0;	/*Resetting lru_timer*/
	return least_recent_index;
}/*End of lru()*/



long resolve_address(long logical, int memwrite)
{
	int i;
	long page, frame, frame_pop;	/*Added frame_pop for fifo*/
	long offset;
	long mask = 0;
	long effective;

	/* Get the page and offset */
	page = (logical >> size_of_frame);

	for (i=0; i<size_of_frame; i++) {
		mask = mask << 1;
		mask |= 1;
	}
	offset = logical & mask;

	/* Find page in the inverted page table. */
	frame = -1;
	for ( i = 0; i < size_of_memory; i++ ) {
		if (!page_table[i].free && page_table[i].page_num == page) {
			frame = i;
			/*Adding LRU*/
			lru_tick = (lru_tick+1)%size_of_memory;
			if(lru_tick == 0) {	/*If goes over mem size, then increment all lru_timer's since they were used*/
				int l;
				for(l=0; l<size_of_memory; l++) {
					page_table[l].lru_timer ++;
				}
			}
			page_table[i].lru_timer = 0;	/*Now using this frame so timer back to zero*/
			/*Finsih Adding LRU*/
			break;
		}
	}

	/* If frame is not -1, then we can successfully resolve the
	 * address and return the result. */
	if (frame != -1) {
		effective = (frame << size_of_frame) | offset;
		return effective;
	}

	/* If we reach this point, there was a page fault. Find
	 * a free frame. */
	page_faults++;

	for ( i = 0; i < size_of_memory; i++) {
		if (page_table[i].free) {
			frame = i;
			break;
		}
	}

	/* If we found a free frame, then patch up the
	 * page table entry and compute the effective
	 * address. Otherwise return -1.
	 */
	if (frame != -1) {
		page_table[frame].page_num = page;
		page_table[i].free = FALSE;
		swap_ins++;
		/*Added for lru*/
		lru_tick = (lru_tick+1)%size_of_memory;
		if(lru_tick == 0 && page_replacement_scheme == SCHEME_LRU) {	/*If goes over mem size, then increment all lru_timer's since they were used*/
			int l;
			for(l=0; l<size_of_memory; l++) {
				page_table[l].lru_timer++;
			}
		}
		page_table[i].lru_timer = 0;	/*Now timer back to 0 since it's being used*/
		/*End added for lru*/
		effective = (frame << size_of_frame) | offset;
		push(frame);		/*For FIFO*/
		if(memwrite == 1) {	/*If there is a write W then swap out increment*/
			swap_outs++;
		}
		return effective;
	} else {
		lru_tick = (lru_tick+1)%size_of_memory;
		if(lru_tick == 0 && page_replacement_scheme == SCHEME_LRU) {	/*If goes over mem size, then increment all lru_timer's since they were used*/
			int l;
			for(l=0; l<size_of_memory; l++) {
				page_table[l].lru_timer++;
			}
		}
		if(page_replacement_scheme == SCHEME_FIFO) {
			frame_pop = pop();
			push(frame_pop);
			page_table[frame_pop].page_num = page;
			page_table[i].free = FALSE;		/*Not free goes back onto queue*/
			frame = frame_pop;
		} else if(page_replacement_scheme == SCHEME_LRU) {
			frame = lru(page);
		}
		swap_ins++;
		if(memwrite == 1) {
			swap_outs++;
		}
		effective = (frame << size_of_frame) | offset;
		return effective;
	}
}

void display_progress(int percent)
{
	int to_date = PROGRESS_BAR_WIDTH * percent / 100;
	static int last_to_date = 0;
	int i;

	if (last_to_date < to_date) {
		last_to_date = to_date;
	} else {
		return;
	}

	printf("Progress [");
	for (i=0; i<to_date; i++) {
		printf(".");
	}
	for (; i<PROGRESS_BAR_WIDTH; i++) {
		printf(" ");
	}
	printf("] %3d%%", percent);
	printf("\r");
	fflush(stdout);
}

int initialize()
{
	int i;

	page_table = (struct page_table_entry *)malloc(sizeof(struct page_table_entry) *
		size_of_memory);

	if (page_table == NULL) {
		fprintf(stderr, "Simulator error: cannot allocate memory for page table.\n");
		exit(1);
	}

	for (i=0; i<size_of_memory; i++) {
		page_table[i].free = TRUE;
	}

    return -1;
}

int finalize()
{
	/*Just freeing head & tail*/
	free(head);
	free(tail);
    return -1;
}

void error_resolve_address(long a, int l)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "Simulator error: cannot resolve address 0x%lx at line %d\n",
		a, l);
	exit(1);
}

int output_report()
{
	printf("\n");
	printf("Memory references: %d\n", mem_refs);
	printf("Page faults: %d\n", page_faults);
	printf("Swap ins: %d\n", swap_ins);
	printf("Swap outs: %d\n", swap_outs);

    return -1;
}

int main(int argc, char **argv)
{
	int i;
	char *infile_name = NULL;
	struct stat infile_stat;
	FILE *infile = NULL;
	int infile_size = 0;
	char *s;
	int show_progress = FALSE;
	char buffer[MAX_LINE_LEN], is_write_c;
	long addr_inst, addr_operand;
	int  is_write;
	int line_num = 0;

	for (i=1; i < argc; i++) {
		if (strncmp(argv[i], "--scheme=", 9) == 0) {
			s = strstr(argv[i], "=") + 1;
			if (strcmp(s, "fifo") == 0) {
				page_replacement_scheme = SCHEME_FIFO;
			} else if (strcmp(s, "lru") == 0) {
				page_replacement_scheme = SCHEME_LRU;
			} else if (strcmp(s, "optimal") == 0) {
				page_replacement_scheme = SCHEME_OPTIMAL;
			}
		} else if (strncmp(argv[i], "--file=", 7) == 0) {
			infile_name = strstr(argv[i], "=") + 1;
		} else if (strncmp(argv[i], "--framesize=", 12) == 0) {
			s = strstr(argv[i], "=") + 1;
			size_of_frame = atoi(s);
		} else if (strncmp(argv[i], "--numframes=", 12) == 0) {
			s = strstr(argv[i], "=") + 1;
			size_of_memory = atoi(s);
		} else if (strcmp(argv[i], "--progress") == 0) {
			show_progress = TRUE;
		}
	}

	if (infile_name == NULL) {
		infile = stdin;
	} else if (stat(infile_name, &infile_stat) == 0) {
		infile_size = (int)(infile_stat.st_size);
		infile = fopen(infile_name, "r");  /* If this fails, infile will be null */
	}

	if (page_replacement_scheme == SCHEME_NONE || size_of_frame <= 0 ||
	    size_of_memory <= 0 || infile == NULL)
	{
		fprintf(stderr, "usage: %s --framesize=<m> --numframes=<n>", argv[0]);
		fprintf(stderr, " --scheme={fifo|lru|optimal} [--file=<filename>]\n");
		exit(1);
	}

	initialize();

	while (fgets(buffer, MAX_LINE_LEN-1, infile)) {
		line_num++;

		if (strstr(buffer, ":")) {
			sscanf(buffer, "%lx: %c %lx", &addr_inst, &is_write_c, &addr_operand);

		 	if (is_write_c == 'R') {
				is_write = FALSE;
			} else if (is_write_c == 'W') {
				is_write = TRUE;
			}  else {
				fprintf(stderr, "Simulator error: unknown memory operation at line %d\n",
					line_num);
				exit(1);
			}

			if (resolve_address(addr_inst, FALSE) == -1) {
				error_resolve_address(addr_inst, line_num);
			}
			if (resolve_address(addr_operand, is_write) == -1) {
				error_resolve_address(addr_operand, line_num);
			}
			mem_refs += 2;
		} else {
			sscanf(buffer, "%lx", &addr_inst);
			if (resolve_address(addr_inst, FALSE) == -1) {
				error_resolve_address(addr_inst, line_num);
			}
			mem_refs++;
		}

		if (show_progress) {
			display_progress(ftell(infile) * 100 / infile_size);
		}
	}

	finalize();
	output_report();

	fclose(infile);

    exit(0);
}
