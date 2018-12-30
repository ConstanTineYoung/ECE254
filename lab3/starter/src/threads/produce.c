#define _XOPEN_SOURCE 500 

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <math.h>
#include <sys/time.h>
#include <time.h>

/* Type Definition */
typedef struct {
    int id;
    int value;
} task;

typedef struct node {
    task* task;
    struct node* next;
} node;


/* Global Variables */
int total_tasks;
int active_tasks;
int remaining_tasks = 0;
int task_id_counter = 0;
pthread_mutex_t mutex;
sem_t empty_list;
sem_t done;
node* list_head;
int total_tasks;
int buffer_size;
int producer_num;
int consumer_num;

struct timeval tv;
double g_time[2];


/* Function Prototypes for pthreads */
void* producer( void* );
void* consumer( void * );


/* Function Declaration */
task* take_task();
void check_root( task* todo, int consumer_id );
void post_tasks( int producer_id, int value );
void consumer_cleanup( void* arg);



/*Main Function*/
int main( int argc, char** argv ) {
    gettimeofday(&tv, NULL);
    g_time[0] = (tv.tv_sec) + tv.tv_usec/1000000.;


    if ( argc != 5 ) {
        printf( "Wrong arguments\n");
        return -1;
    }
	
    /* Init global variables here */
    list_head = NULL;
    total_tasks = atoi( argv[1] );
    buffer_size = atoi( argv[2] );
    producer_num = atoi( argv[3] );
    consumer_num = atoi( argv[4] );

    pthread_t P[producer_num];
    pthread_t C[consumer_num];
    
    active_tasks = 0;
    remaining_tasks = total_tasks;
    pthread_mutex_init( &mutex, NULL );
    sem_init( &done, 0, 0 );
    sem_init( &empty_list, 0, 0 );
  
  
    /* Launch threads here */
    for( int i = 0; i < producer_num; i++ ){
    	int *prod_id = malloc(sizeof(int));
	*prod_id = i;
        pthread_create(&P[i], NULL, producer, prod_id);
    }

    for( int i = 0; i < consumer_num; i++ ) {
        int *cons_id = malloc(sizeof(int));
        *cons_id = i;
        pthread_create(&C[i], NULL, consumer, cons_id);
    }
   
    /* Wait for Producers to be done */
    printf("Before join\n");
    for( int i = 0; i < producer_num; i++) {
	pthread_join(P[i], NULL);
    }
    
    /* Cancel the Consumers When done */
    sem_wait(&done);
    for( int i = 0; i < consumer_num; i++) {
       pthread_cancel(C[i]);
    } 

    /* Cleanup Global Variables here */
    sem_destroy( &empty_list );
    sem_destroy( &done);
    pthread_mutex_destroy( &mutex );

    /* Count on Time */
    gettimeofday(&tv, NULL);
    g_time[1] = (tv.tv_sec) + tv.tv_usec/1000000.;

    printf("System execution time: %.6lf seconds\n", \
        g_time[1] - g_time[0]);
    return 0;
}

/*Producer Function*/
void* producer( void * arg ) {

    int *producer_id = (int *)arg;
    while(1){
	for (int j = *producer_id; j < total_tasks; j += producer_num){
	    pthread_mutex_lock(&mutex);
	    //producers produce numbers one by one
		
	    if (active_tasks <= buffer_size) {
		post_tasks(*producer_id, j);
	    } else {
    		pthread_mutex_unlock(&mutex);
    		sem_wait(&empty_list);
    		pthread_mutex_lock(&mutex);
    		post_tasks(*producer_id, j);
	    }
	    //when buffer has a vacancy just post the new number
	    //else wait for consumers to make vacancy
		
	    pthread_mutex_unlock(&mutex);
	}
	free(arg);
	pthread_exit(0);
    }
}

/*Consumer Function*/
void* consumer( void * ignore ) {
    int consumer_id = *((int *)ignore);
    task* todo;
    pthread_cleanup_push( consumer_cleanup, ignore );
    //cleanup handler
    //automatically called when thread is canceled
	
    while( 1 ) {
        pthread_testcancel();
	//test whether it is cancelled
	    
        pthread_mutex_lock( &mutex );
	if (active_tasks > 0) {
	    todo = take_task();
	    check_root(todo, consumer_id);
            printf("rem %d\n", remaining_tasks);
	    if (remaining_tasks == 0) {
	        sem_post(&done);
            }
            active_tasks--;
	}
	//take numbers away if the buffer is not empty
	    
        if (active_tasks < buffer_size ) {
            sem_post(&empty_list);
        }
	//inform the producers about vacancy 
	    
        pthread_mutex_unlock( &mutex );
	
    }
    pthread_cleanup_pop( 0 ) ;
}

void consumer_cleanup( void* arg ) {
    free(arg);
    //clean up the malloc
}


void post_tasks( int producer_id, int value ) {
    task* t = malloc( sizeof( task ));
    t->id = ++task_id_counter;
    t->value = value;
    node* n = malloc( sizeof( node ));
    n->task = t;
    n->next = list_head;
    list_head = n;
    active_tasks++;
    //post numbers to the buffer list
}

void check_root( task* todo, int consumer_id ) {
     int received = todo -> value; 
     int root = sqrt(todo -> value);
     if ((root * root) == received) {
         printf("%d %d %d\n", consumer_id, received, root);
     }
     //check whether the number is a square root
     free( todo );
}

task* take_task( ) {
    if (list_head == NULL) {
        exit( -1 );
    }
    node* head = list_head;
    task* t = head->task;
    list_head = list_head->next;
    free( head );
    remaining_tasks--;
    return t;
    //take the number from the buffer list
}


