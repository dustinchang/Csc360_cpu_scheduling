/*
 * cpusched.c
 *
 * Skeleton code for solution to A#3, CSC 360, Summer 2014
 *
 * Prepared by: Michael Zastre (University of Victoria) 2014
 *
 * Dustin Chang
 *
 * How to execute piping gentasks into simsched:
 *      Use ctrl-D to end for input
 *      ./gentasks <num_tasks> <random seed> | ./simsched -a PS
 *      ./gentasks <num_tasks> <random seed> | ./simsched -a STRIDE -q <num>
 *      ./gentasks <num_tasks> <random seed> | ./simsched -a RR -q <num>
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#define MAX_LINE_LENGTH 100
#define FCFS 0
#define PS   1
#define RR 2
#define STRIDE 3
#define PRIORITY_LEVELS 4

typedef struct Task_t {
    int   arrival_time;
    float length;
    int   priority;

    float finish_time;
    int   schedulings;
    float cpu_cycles;
    int done;           /*added to keep track of if that task is finished; 0=not done, 1=done*/
    float meter;        /*added to keep track of stride meter for each task*/
    int meter_nemerator; /*---Formula == m = mn/ml---*/
    float meter_length; /*added to keep track of denominator of meter*/
    struct Task_t *next;
} task_t;

typedef struct node {   /*Used as a queue to use the array's at what index they are*/
    int task_num;
    struct node *next;
} Node;

/*
 * Some function prototypes.
 */
void read_task_data(void);
void init_simulation_data(int);
void first_come_first_serve(void);
void stride_scheduling(int);
void priority_scheduling(void);
void rr_scheduling(int);
void run_simulation(int, int);
void compute_and_print_stats(void);

/*
 * Some global vars.
 */
int num_tasks = 0;
task_t *tasks = NULL;
Node *head = NULL;    /*head&tail used for Round_Robin*/
Node *tail = NULL;
/*--- GMT/GML = GM ---*/
float global_meter = 0.0;       /*Always starts at 0/global_meter_length so = 0.0 anyway*/
float global_meter_track = 0; /*The global numerator*/
float global_meter_length = 0.0;

bool isEmpty() {
    return head == NULL;
}

void push(int t) {
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

int pop() {
	if(head == NULL) {
		printf("Error: Nothing to pop\n");
		exit(1);
	}
	Node *tmp = head;
	head = head->next;
	int p = tmp->task_num;
	free(tmp);
	return p;
}


void read_task_data()
{
    int max_tasks = 2;
    int  in_task_num, in_task_arrival, in_task_priority;
    float in_task_length;

    assert( tasks == NULL );

    tasks = (task_t *)malloc(sizeof(task_t) * max_tasks);
    if (tasks == NULL) {
        fprintf(stderr, "error: malloc failure in read_task_data()\n");
        exit(1);
    }

    num_tasks = 0;

    /* Given the format of the input is strictly formatted,
     * we can used fscanf .
     */
    while (!feof(stdin)) {
        fscanf(stdin, "%d %d %f %d\n", &in_task_num,
            &in_task_arrival, &in_task_length, &in_task_priority);
        assert(num_tasks == in_task_num);
        tasks[num_tasks].arrival_time = in_task_arrival;
        tasks[num_tasks].length       = in_task_length;
        tasks[num_tasks].priority     = in_task_priority;

        num_tasks++;
        if (num_tasks >= max_tasks) {
            max_tasks *= 2;
            tasks = (task_t *)realloc(tasks, sizeof(task_t) * max_tasks);
            if (tasks == NULL) {
                fprintf(stderr, "error: malloc failure in read_task_data()\n");
                exit(1);
            }
        }
    }
}


void init_simulation_data(int algorithm)
{
    int i;
    global_meter_length = 0.0;

    for (i = 0; i < num_tasks; i++) {
        tasks[i].finish_time = 0.0;
        tasks[i].schedulings = 0;
        tasks[i].cpu_cycles = 0.0;
        tasks[i].meter_nemerator = 0;   /*Added to use in formula*/
        tasks[i].done = 0;       /*Added to struct to keep track if task was finished*/
        tasks[i].meter = 0.0;
        if(algorithm == 3) {
        	global_meter_length += tasks[i].length;     /*Only used for Stride scheduling*/
        	tasks[i].meter_length = tasks[i].length;	/*Only used for Stride scheduling*/
        }
    }
}


void first_come_first_serve()
{
    int current_task = 0;
    int current_tick = 0;

    for (;;) {
        current_tick++;

        if (current_task >= num_tasks) {
            break;
        }

        /*
         * Is there even a job here???
         */
        if (tasks[current_task].arrival_time > current_tick-1) {
            continue;
        }

        tasks[current_task].cpu_cycles += 1.0;

        if (tasks[current_task].cpu_cycles >= tasks[current_task].length) {
            float quantum_fragment = tasks[current_task].cpu_cycles -
                tasks[current_task].length;
            //printf("%f\n", quantum_fragment);
            tasks[current_task].cpu_cycles = tasks[current_task].length;
            //printf("%f\n", tasks[current_task].cpu_cycles);
            tasks[current_task].finish_time = current_tick - quantum_fragment;
            //printf("%f\n", tasks[current_task].finish_time);
            tasks[current_task].schedulings = 1;
            current_task++;
            if (current_task > num_tasks) {
                break;
            }
            tasks[current_task].cpu_cycles += quantum_fragment;
        }
    }
}


void stride_scheduling(int quantum)
{
    printf("STRIDE SCHEDULING\n");
    int current_task = 0;
    int current_tick = 0;
    int j, minMeterTask;
    float minMeter, quantum_fragment;
    int quantum_flag = 0;
    int scheduling_tracker = -1;
    int quantum_tracker = quantum;
    float meter_nemerator = 0.0;
    for(;;) {   /*A process with the smallest value on its meter gets the chance to run in Stride*/
        minMeter = 20.0;   /*Should never be this high, acts as a Safeguard*/
        current_tick++;

        if(current_task >= num_tasks) {		/*Check if there are any tasks*/
        	break;
        }
        if(tasks[current_task].arrival_time > current_tick-1) {
        	continue;
        }

        for(j=0; j<num_tasks; j++) {    /*Just initializing the meters of incoming tasks*/
            if(tasks[j].arrival_time == current_tick-1) {   /*current_tick-1 cause tick is ahead of what place is going to be used by the cpu; hence the arr spot of a task*/
                tasks[j].meter = global_meter;
            }
        }

        for(j=0; j<num_tasks; j++) {
        	if(tasks[j].arrival_time < current_tick && tasks[j].done != 1 && quantum_tracker == quantum) {
        		if(tasks[j].meter < minMeter) {
        			minMeter = tasks[j].meter;
        			minMeterTask = j;
        		}
        	}
            if(quantum_tracker != quantum) {
                break;
            }
        }/*End of for(j)*/

        if(minMeterTask != scheduling_tracker) {
          tasks[minMeterTask].schedulings += 1;
        }
        scheduling_tracker = minMeterTask;

        if(quantum_flag == 1) {
          tasks[minMeterTask].cpu_cycles += quantum_fragment;
          /*When adding quantum_frag to next task, need to also increment it's meter*/
          meter_nemerator = tasks[minMeterTask].meter*tasks[minMeterTask].meter_length;
          meter_nemerator += quantum_fragment;
          tasks[minMeterTask].meter = meter_nemerator/tasks[minMeterTask].meter_length;
          /*Need to also increment global_meter*/
          global_meter_track += quantum_fragment;
          global_meter = global_meter_track/global_meter_length;
          quantum_flag = 0;
        }

        tasks[minMeterTask].cpu_cycles += 1.0;

        if(tasks[minMeterTask].cpu_cycles >= tasks[minMeterTask].length) {
            quantum_fragment = tasks[minMeterTask].cpu_cycles - tasks[minMeterTask].length;
            quantum_flag = 1;
        }
        /*Incrementing task meter*/
        meter_nemerator = tasks[minMeterTask].meter*tasks[minMeterTask].meter_length;
        meter_nemerator += 1.0;
        tasks[minMeterTask].meter = meter_nemerator/tasks[minMeterTask].meter_length;
        if(tasks[minMeterTask].meter >= 1.0 && tasks[minMeterTask].cpu_cycles >= tasks[minMeterTask].length) {
            tasks[minMeterTask].meter = 1.0;
        }
        /*Incrementing global meter*/
        if(quantum_flag == 1) {
            global_meter_track += (1.0 - quantum_fragment);
        } else {
            global_meter_track += 1.0;
        }
        global_meter = global_meter_track/global_meter_length;

        quantum_tracker -= 1;
        if(quantum_tracker == 0 || quantum_flag == 1) {
            quantum_tracker = quantum;
        }        

        if(tasks[minMeterTask].cpu_cycles >= tasks[minMeterTask].length) {
            tasks[minMeterTask].cpu_cycles = tasks[minMeterTask].length;
            tasks[minMeterTask].finish_time = current_tick - quantum_fragment;
            tasks[minMeterTask].done = 1;
            current_task++;
            if(current_task >= num_tasks) {
                break;
            }
            quantum_flag = 1;   /*Just as a safeguard*/
        }
    }/*End of for(;;)*/
}/*End of stride_scheduling()*/


void priority_scheduling()
{
    printf("PRIORITY SCHEDULING\n");
    int current_task = 0;
    int current_tick = 0;
    int i;  /*For for loop checking which has lowest priority*/
    int minPriTask;
    int minPri;
    float quantum_fragment; /*Need Global of this, is needed for the if(quantum_flag==1)*/
    int quantum_flag = 0;
    int scheduling_tracker = -1;

    for(;;) {
      minPri = 10;   /*To reinitialize for each loop*/
      current_tick++;

      if(current_task >= num_tasks) {
        printf("Shouldn't get here\n");
        break;
      }
      if(tasks[current_task].arrival_time > current_tick-1) {  /*To see if there is even a job to do*/
        continue;
      }/*Might not need this function though*/

      for(i=0; i<num_tasks; i++) {   /*To check which has lowest priority*/
        if(tasks[i].arrival_time < current_tick && tasks[i].done != 1) {
          if(tasks[i].priority < minPri) {
            minPri = tasks[i].priority;
            minPriTask = i;
          }
        }
      }

      if(minPriTask != scheduling_tracker) {
        tasks[minPriTask].schedulings += 1;
      }
      scheduling_tracker = minPriTask;

      if(quantum_flag == 1) {
        tasks[minPriTask].cpu_cycles += quantum_fragment;
        quantum_flag = 0;
      }
      tasks[minPriTask].cpu_cycles += 1.0;

      if(tasks[minPriTask].cpu_cycles >= tasks[minPriTask].length) {
        quantum_fragment = tasks[minPriTask].cpu_cycles - tasks[minPriTask].length;
        tasks[minPriTask].cpu_cycles = tasks[minPriTask].length;
        tasks[minPriTask].finish_time = current_tick - quantum_fragment;
        tasks[minPriTask].done = 1;   /*This task is now complete so should skip above for loop*/
        current_task++;
        if(current_task >= num_tasks) {
          break;
        }
        quantum_flag = 1;
      }
    }/*End of for(;;)*/
}/*End of priority_scheduling()*/


void rr_scheduling(int quantum)
{
    printf("RR SCHEDULING\n");
    int current_task = 0;
    int current_tick = 0;
    int k;
    float quantum_fragment;
    int quantum_flag = 0;
    int scheduling_tracker = -1;
    int quantum_tracker = quantum;
    int numHead;

    for(;;) {
        current_tick++;

        if(current_task >= num_tasks) {	/*Safeguard*/
            break;
        }
        if(tasks[current_task].arrival_time > current_tick-1) {
        	continue;
        }

        for(k=0; k<num_tasks; k++) {
        	if(tasks[k].arrival_time < current_tick && tasks[k].done != 1) {
        		push(k);	/*Where k is the task that is being pushed on queue*/
        		tasks[k].done = 1;	/*Use done as node k is pushed onto queue*/
        	}
        }

        if(quantum_tracker == quantum) {
        	numHead = pop();	/*numHead is the task# of the head node*/
        }

        if(numHead != scheduling_tracker) {
        	tasks[numHead].schedulings += 1;
        }
        scheduling_tracker = numHead;

        if(quantum_flag == 1) {		/*Used when a previous task just ended*/
        	tasks[numHead].cpu_cycles += quantum_fragment;
        	quantum_flag = 0;
        }
        tasks[numHead].cpu_cycles += 1.0;

        quantum_tracker -= 1;
        if(quantum_tracker == 0) {
        	quantum_tracker = quantum;
        }

        if(tasks[numHead].cpu_cycles >= tasks[numHead].length) {
        	quantum_fragment = tasks[numHead].cpu_cycles - tasks[numHead].length;
        	tasks[numHead].cpu_cycles = tasks[numHead].length;
        	tasks[numHead].finish_time = current_tick - quantum_fragment;
        	current_task++;
        	if(current_task >= num_tasks){
        		break;
        	}
        	quantum_tracker = quantum;	/*In here in case the previous task doesn't use all quantum*/
        	quantum_flag = 1;
        } else {	/*If the task is smaller than it's length and not done, re-push it*/
        	if(quantum_tracker == quantum) {
        		push(numHead);
        	}
        }
    }/*End of for(;;)*/
}/*End of rr_scheduling*/


void run_simulation(int algorithm, int quantum)
{
    switch(algorithm) {
        case STRIDE:
            stride_scheduling(quantum);
            break;
        case PS:
            priority_scheduling();
            break;
        case RR:
            rr_scheduling(quantum);
            break;
        case FCFS:
        default:
            first_come_first_serve();
            break;
    }
}


void compute_and_print_stats()
{
    int tasks_at_level[PRIORITY_LEVELS] = {0,};
    float response_at_level[PRIORITY_LEVELS] = {0.0, };
    int scheduling_events = 0;
    int i;

    for (i = 0; i < num_tasks; i++) {
        tasks_at_level[tasks[i].priority]++;
        response_at_level[tasks[i].priority] +=
            tasks[i].finish_time - (tasks[i].arrival_time * 1.0);
        scheduling_events += tasks[i].schedulings;

        printf("Task %3d: cpu time (%4.1f), response time (%4.1f), waiting (%4.1f), schedulings (%5d)\n",
            i, tasks[i].length,
            tasks[i].finish_time - tasks[i].arrival_time,
            tasks[i].finish_time - tasks[i].arrival_time - tasks[i].cpu_cycles,
            tasks[i].schedulings);

    }

    printf("\n");

    if (num_tasks > 0) {
        for (i = 0; i < PRIORITY_LEVELS; i++) {
            if (tasks_at_level[i] == 0) {
                response_at_level[i] = 0.0;
            } else {
                response_at_level[i] /= tasks_at_level[i];
            }
            printf("Priority level %d: average response time (%4.1f)\n",
                i, response_at_level[i]);
        }
    }

    printf ("Total number of scheduling events: %d\n", scheduling_events);
}


int main(int argc, char *argv[])
{
    int i = 0;
    int algorithm = FCFS;
    int quantum = 1;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-q") == 0) {
            i++;
            quantum = atoi(argv[i]);
        } else if (strcmp(argv[i], "-a") == 0) {
            i++;
            if (strcmp(argv[i], "FCFS") == 0) {
                algorithm = FCFS;
            } else if (strcmp(argv[i], "PS") == 0) {
                algorithm = PS;
            } else if (strcmp(argv[i], "RR") == 0) {
                algorithm = RR;
            } else if (strcmp(argv[i], "STRIDE") == 0) {
                algorithm = STRIDE;
            }
        }
    }

    read_task_data();

    if (num_tasks == 0) {
        fprintf(stderr,"%s: no tasks for the simulation\n", argv[0]);
        exit(1);
    }

    init_simulation_data(algorithm);
    run_simulation(algorithm, quantum);
    compute_and_print_stats();

    exit(0);
}
