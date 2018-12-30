
#define EPS 1.E-7


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <math.h>
#include <fcntl.h>
#include <sys/fcntl.h>


double g_time[2];
int num;
int maxmsg;
int num_p;
int num_c;
int consumer_id;
struct timeval tv;
pid_t pini;
mqd_t queue_d;

void producer(mqd_t fd, int prod_id);
void consumer(mqd_t fd, int cons_id);

int main(int argc, char *argv[]) {

    if (argc != 5) {
        printf("Usage: %s <N> <B> <P> <C>\n", argv[0]);
	exit(1);
    }

    num = atoi(argv[1]);    /* number of items to produce */
    maxmsg = atoi(argv[2]); /* buffer size                */
    num_p = atoi(argv[3]);  /* number of producers        */
    num_c = atoi(argv[4]);  /* number of consumers        */

    gettimeofday(&tv, NULL);
    g_time[0] = (tv.tv_sec) + tv.tv_usec/1000000.;
    
    //process id
    pid_t pros_pids[num_p];
    pid_t cons_pids[num_c];

    //message queue initialization & opening
    mq_unlink("/coolqueue");
    struct mq_attr attr;
    attr.mq_maxmsg = maxmsg;
    attr.mq_msgsize = sizeof(int);
    attr.mq_flags = 0;
    attr.mq_curmsgs = 0;
    queue_d = mq_open("/coolqueue", O_RDWR | O_CREAT, 0644, &attr);
    if (queue_d == -1) {
        printf("Failed to open the queue\n");
	exit(1);
    }
    printf("Open queue descriptor %d\n", queue_d);

    //fork producers
    for (int i = 0; i < num_p; i++) {
        pros_pids[i] = fork();
        if (pros_pids[i] == -1) {
            printf("Failed to fork\n");
            exit(1);
        } else if (pros_pids[i] == 0) {
            printf("Creating producer %d\n", i);
            producer(queue_d, i);
        } 
    }

    //fork consumers
    for (int i = 0; i < num_c; i++) {
        cons_pids[i] = fork();
        if (cons_pids[i] == -1) {
            printf("Failed to fork\n");
            exit(1);
        } else if (cons_pids[i] == 0) {
            printf("Creating consumer %d\n", i);
            consumer(queue_d, i);
        }
    }
   
    //wait for producers to exit
    for (int i = 0; i < num_p; i++) {
        waitpid(pros_pids[i], NULL, 0);
    }

    //kill the consumers
    int killem = -1;
    for (int i = 0; i < num_c; i++) {
        mq_send(queue_d, (char *) &killem, sizeof(int), 0);
    } 

    //wait for consumers to exit
    for (int i = 0; i < num_c; i++) {
        waitpid(cons_pids[i], NULL, 0);
    }
    
    //close the message queue
    mq_unlink("/coolqueue");
    mq_close(queue_d);

    //count on time
    gettimeofday(&tv, NULL);
    g_time[1] = (tv.tv_sec) + tv.tv_usec/1000000.;

    printf("System execution time: %.6lf seconds\n", \
            g_time[1] - g_time[0]);
    exit(0);
}

void producer(mqd_t fd, int prod_id){	
    
    //send messages & close queue
    for (int j = prod_id; j < num; j += num_p){
    	mq_send(fd, (char *)&j, sizeof(int), 1);
    }
    mq_close(fd);
    exit(0);
}

void consumer(mqd_t fd, int cons_id) {
    int received;
    int root;
    printf("Recv desc %d\n", fd);
    int rcv;    
	
    //while not be killed
    //receive messages & close queue
    while(rcv != -1) {
        mq_receive(fd, (char *)&rcv, sizeof(int), NULL);
        received = rcv;
        root = sqrt(rcv);
        if ((root * root) == received) {
            printf("%d %d %d\n", cons_id, received, root);
        }
    }
    mq_close(fd);
    exit(0);
    perror(NULL);
}
