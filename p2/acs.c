/*
* Name: Zijian Chen
*
* V ID: V00867494
*
* CSC 360 A2
*
* Date: 2018/11/05
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>


// cunstractor use to store the customer information
typedef struct customer {
    int id;
    int priority;
    int arrival_time;
    int service_time;
    double start_time;
    double end_time;
} customer;

//use to store the clerk id
typedef struct clerk {
    int id;
} clerk;

//node type use to create queue
typedef struct node_t {
    customer* customer;
    struct node_t* next;
} node_t;

//Queue type include the head and tail of the queue list.
typedef struct Queue {
    struct node_t* head;
    struct node_t* tail;
    int size;
} Queue;

//defualt the customer node
node_t* constructNode(customer* currCustomer) {
    node_t* node = (node_t*) malloc(sizeof (node_t));
    if(node == NULL) {
        perror("Error: failed on malloc Node");
        return NULL;
    }
    node->customer = currCustomer;
    node->next = NULL;
    return node;
}

//defualt the Queue list
Queue* constructQueue() {
    Queue* queue = (Queue*) malloc(sizeof (Queue));
    if(queue == NULL) {
        perror("Error: failed on malloc Queue\n");
        return NULL;
    }
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    return queue;
}

//enQueue function use to add node into the list
void enQueue(Queue* equeue, node_t* newNode) {
    if(equeue->size == 0) {
        equeue->head = newNode;
        equeue->tail = newNode;
    }else {
        equeue->tail->next = newNode;
        equeue->tail = newNode;
    }
    equeue->size++;
}

//deQueue function use to remove the peek of the queue list
void deQueue(Queue* dqueue) {
    if(dqueue->size == 0) {
        exit(-1);
    }else if(dqueue->size == 1) {
        dqueue->head = NULL;
        dqueue->tail = NULL;
        dqueue->size--;
    }else {
        dqueue->head = (dqueue->head)->next;
        dqueue->size--;
    }
}

//return 1 if the customer node is the peek of the queue else return 0
int peek(Queue* queue, customer* cust) {
    if(cust == queue->head->customer){
      return 1;
    }else{
      return 0;
    }
}

//golobal veribels use to support the list todo the rest of works
#define MAX_INPUT_SIZE 128
int totalCustomer = 0;
int countBusiness = 0; //count the total of business customers
int countEconomy = 0;
customer customerArray[MAX_INPUT_SIZE];
clerk clerkArray[4];
Queue* bQueue = NULL;
Queue* eQueue = NULL;
struct timeval init_time; //init of the time use to count the customer wait time
double overAll_wait_time;
double overAll_time_bClass;
double overAll_time_eClass;

//use to record the clerk current serving customer id
int C1_serving = 0;
int C2_serving = 0;
int C3_serving = 0;
int C4_serving = 0;

//record the current clerk id
int clerk_record = 0;

//initial the pthread and mutex and condition varibal.
pthread_t customerThreads[MAX_INPUT_SIZE];
pthread_t clerkThreads[4];

//use for clerk lock and unlock and convar wait and signal
pthread_mutex_t C1_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t C1_convar = PTHREAD_COND_INITIALIZER;
pthread_mutex_t C2_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t C2_convar = PTHREAD_COND_INITIALIZER;
pthread_mutex_t C3_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t C3_convar = PTHREAD_COND_INITIALIZER;
pthread_mutex_t C4_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t C4_convar = PTHREAD_COND_INITIALIZER;

pthread_mutex_t bQ_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t bQ_convar = PTHREAD_COND_INITIALIZER;
pthread_mutex_t eQ_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t eQ_convar = PTHREAD_COND_INITIALIZER;

pthread_mutex_t access_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t access_queue_convar = PTHREAD_COND_INITIALIZER;

pthread_mutex_t overAll_mutex = PTHREAD_MUTEX_INITIALIZER;

//use to get the time of the curr time
double get_simulation_time() {
    struct timeval curr_time;
    double curr_seconds;
    double init_seconds;

    init_seconds = (init_time.tv_sec + (double) init_time.tv_usec / 1000000);
    gettimeofday(&curr_time, NULL);
    curr_seconds = (curr_time.tv_sec + (double) curr_time.tv_usec / 1000000);

    return (curr_seconds - init_seconds);
}

//set the equeue serving id
void set_eQueue_serving(int clerk_ID) {
    if(eQueue->head != NULL) {
        if(clerk_ID == 1) {
            C1_serving = eQueue->head->customer->id;
        }else if(clerk_ID == 2) {
            C2_serving = eQueue->head->customer->id;
        }else if(clerk_ID == 3) {
            C3_serving = eQueue->head->customer->id;
        }else {
            C4_serving = eQueue->head->customer->id;
        }
    }
}

//set the bQueue serving id
void set_bQueue_serving(int clerk_ID) {
  if(bQueue->head != NULL) {
      if(clerk_ID == 1) {
          C1_serving = bQueue->head->customer->id;
      }else if(clerk_ID == 2) {
          C2_serving = bQueue->head->customer->id;
      }else if(clerk_ID == 3) {
          C3_serving = bQueue->head->customer->id;
      }else {
          C4_serving = bQueue->head->customer->id;
      }
  }
}

//set and return the current serving id
int set_clerk_serving(int clerk_ID) {
    if(clerk_ID == 1) {
        return C1_serving;
    }else if(clerk_ID == 2) {
        return C2_serving;
    }else if(clerk_ID == 3) {
        return C3_serving;
    }else {
        return C4_serving;
    }
}

//customer function
void* customer_entry(void* currCustomer) {
    customer* customer_infor = (customer*) currCustomer;
    usleep(customer_infor->arrival_time * 100000);

    printf("A customer arrives: customer ID %2d. \n", customer_infor->id);

    //case 1 if priority == 0 economy class
    if(customer_infor->priority == 0) { // 0 = economy class, 1 = business class.
        countEconomy++;
        if(pthread_mutex_lock(&eQ_mutex)) {
            perror("Error: failed on mutex lock.\n");
        }
        enQueue(eQueue, constructNode(customer_infor)); //add the node into the queue

        pthread_cond_signal(&access_queue_convar); //send signal to clerk the customer arrived

        printf("A customer enters a queue: the queue ID %1d, and length of the queue %2d. \n", 1, eQueue->size);

        customer_infor->start_time = get_simulation_time();

        //wait the clerk signal when the clerk is ready.
        if(pthread_cond_wait(&eQ_convar, &eQ_mutex)) {
            perror("Error: failed on condition wait.\n");
        }
        //record the id from the global.
        int clerk_Mark = clerk_record;

        while(customer_infor->id != set_clerk_serving(clerk_Mark)) {
            pthread_cond_wait(&eQ_convar, &eQ_mutex);
        }

        deQueue(eQueue); //remove the node from queue

        if(pthread_mutex_unlock(&eQ_mutex)) {
            perror("Error: failed on mutex unlock.\n");
        };

        customer_infor->end_time = get_simulation_time();

        // get the wait time.
        pthread_mutex_lock(&overAll_mutex);
        double i = customer_infor->end_time;
        double j = customer_infor->start_time;
        overAll_wait_time += i - j;
        overAll_time_eClass += i - j;
        pthread_mutex_unlock(&overAll_mutex);

        printf("A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d. \n", customer_infor->end_time, customer_infor->id, clerk_Mark);

        //usleep customer proccessing
        usleep(customer_infor->service_time * 100000);

        double end_serving_time = get_simulation_time();

        printf("A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d. \n", end_serving_time, customer_infor->id, clerk_Mark);

        //send back a signal to clerk cotinue pick customer
        if(clerk_Mark == 1) {
            pthread_mutex_lock(&C1_mutex);
            pthread_cond_signal(&C1_convar);
            pthread_mutex_unlock(&C1_mutex);
        }else if(clerk_Mark == 2) {
            pthread_mutex_lock(&C2_mutex);
            pthread_cond_signal(&C2_convar);
            pthread_mutex_unlock(&C2_mutex);
        }else if(clerk_Mark == 3) {
            pthread_mutex_lock(&C3_mutex);
            pthread_cond_signal(&C3_convar);
            pthread_mutex_unlock(&C3_mutex);
        }else {
            pthread_mutex_lock(&C4_mutex);
            pthread_cond_signal(&C4_convar);
            pthread_mutex_unlock(&C4_mutex);
        }

      //same as the if statement when priority is 1 business class
    }else {
        countBusiness++;
        if(pthread_mutex_lock(&bQ_mutex)) {
            perror("Error: failed on mutex lock.\n");
        }
        enQueue(bQueue, constructNode(customer_infor));

        pthread_cond_signal(&access_queue_convar);

        printf("A customer enters a queue: the queue ID %1d, and length of the queue %2d. \n", 0, bQueue->size);

        customer_infor->start_time = get_simulation_time();

        if(pthread_cond_wait(&bQ_convar, &bQ_mutex)) {
            perror("Error: failed on condition wait.\n");
        }

        int clerk_Mark = clerk_record;

        while(customer_infor->id != set_clerk_serving(clerk_Mark)) {
            pthread_cond_wait(&bQ_convar, &bQ_mutex);
        }

        deQueue(bQueue);

        if(pthread_mutex_unlock(&bQ_mutex)) {
            perror("Error: failed on mutex unlock.\n");
        }

        customer_infor->end_time = get_simulation_time();

        pthread_mutex_lock(&overAll_mutex);
        double i = customer_infor->end_time;
        double j = customer_infor->start_time;
        overAll_wait_time += i - j;
        overAll_time_bClass += i - j;
        pthread_mutex_unlock(&overAll_mutex);

        printf("A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d. \n", customer_infor->end_time, customer_infor->id, clerk_Mark);

        usleep(customer_infor->service_time * 100000);

        double end_serving_time = get_simulation_time();

        printf("A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d. \n", end_serving_time, customer_infor->id, clerk_Mark);

        if(clerk_Mark == 1) {
            pthread_mutex_lock(&C1_mutex);
            pthread_cond_signal(&C1_convar);
            pthread_mutex_unlock(&C1_mutex);
        }else if(clerk_Mark == 2) {
            pthread_mutex_lock(&C2_mutex);
            pthread_cond_signal(&C2_convar);
            pthread_mutex_unlock(&C2_mutex);
        }else if(clerk_Mark == 3) {
            pthread_mutex_lock(&C3_mutex);
            pthread_cond_signal(&C3_convar);
            pthread_mutex_unlock(&C3_mutex);
        }else {
            pthread_mutex_lock(&C4_mutex);
            pthread_cond_signal(&C4_convar);
            pthread_mutex_unlock(&C4_mutex);
        }

    }
    pthread_exit(NULL);
    return NULL;
}

//clerk function use to send signal and wait pick customer
void* clerk_pickup(void* currClerk) {
    clerk* clerk_infor = (clerk*) currClerk;
    int clerk_ID = clerk_infor->id;

    while(1) {
        //lock the condition only one clerk thread can access the condition.
        pthread_mutex_lock(&access_queue_mutex);
        while (bQueue->size == 0 && eQueue->size == 0) {
            pthread_cond_wait(&access_queue_convar, &access_queue_mutex);
        }
        pthread_mutex_unlock(&access_queue_mutex);

        //if the bQueue is not empty it has high priority than economy class.
        if(bQueue->size != 0) {
            pthread_mutex_lock(&bQ_mutex);

            clerk_record = clerk_ID;

            set_bQueue_serving(clerk_ID);

            //send the signal to business queue
            if(pthread_cond_signal(&bQ_convar)) {
                perror("Error: failed on condition signal.\n");
                exit(0);
            }

            //unlock the queue
            pthread_mutex_unlock(&bQ_mutex);

            if(clerk_ID == 1 ) {
                pthread_mutex_lock(&C1_mutex);
                pthread_cond_wait(&C1_convar, &C1_mutex);
                pthread_mutex_unlock(&C1_mutex);
            }else if(clerk_ID == 2 ) {
                pthread_mutex_lock(&C2_mutex);
                pthread_cond_wait(&C2_convar, &C2_mutex);
                pthread_mutex_unlock(&C2_mutex);
            }else if(clerk_ID == 3 ) {
                pthread_mutex_lock(&C3_mutex);
                pthread_cond_wait(&C3_convar, &C3_mutex);
                pthread_mutex_unlock(&C3_mutex);
            }else if(clerk_ID == 4 ) {
                pthread_mutex_lock(&C4_mutex);
                pthread_cond_wait(&C4_convar, &C4_mutex);
                pthread_mutex_unlock(&C4_mutex);
            }

            //if the economy queue size is not zero it goes here
        }else if(eQueue->size != 0 ) {
            pthread_mutex_lock(&eQ_mutex);

            clerk_record = clerk_ID;

            set_eQueue_serving(clerk_ID);

            if(pthread_cond_signal(&eQ_convar)) {
                perror("Error: failed on condition signal.\n");
                exit(0);
            }

            pthread_mutex_unlock(&eQ_mutex);

            if(clerk_ID == 1 ) {
                pthread_mutex_lock(&C1_mutex);
                pthread_cond_wait(&C1_convar, &C1_mutex);
                pthread_mutex_unlock(&C1_mutex);
            }else if(clerk_ID == 2 ) {
                pthread_mutex_lock(&C2_mutex);
                pthread_cond_wait(&C2_convar, &C2_mutex);
                pthread_mutex_unlock(&C2_mutex);
            }else if(clerk_ID == 3 ) {
                pthread_mutex_lock(&C3_mutex);
                pthread_cond_wait(&C3_convar, &C3_mutex);
                pthread_mutex_unlock(&C3_mutex);
            }else if(clerk_ID == 4 ) {
                pthread_mutex_lock(&C4_mutex);
                pthread_cond_wait(&C4_convar, &C4_mutex);
                pthread_mutex_unlock(&C4_mutex);
            }

        }

    }
    return NULL;
}

//use to read in the file data
void readFileIn(char* file, char fileInformation[MAX_INPUT_SIZE][MAX_INPUT_SIZE]) {
    FILE *fp = fopen(file, "r");
    if(fp != NULL) {
        int count = 0;
        while(1) {
            fgets(fileInformation[count++], MAX_INPUT_SIZE, fp);
            if(feof(fp)) break;
        }
    }else {
        perror("Error: failed on read file.\n");
    }
    fclose(fp);
}

//token the information and cast it to integer
void tokenInformation(char fileInformation[MAX_INPUT_SIZE][MAX_INPUT_SIZE], int totalCustomer) {
    int i = 1;
    for(; i<=totalCustomer; i++) {
        int j = 0;
        while(fileInformation[i][j] != '\0') {
            if(fileInformation[i][j] == ':' || fileInformation[i][j] == ',') {
                fileInformation[i][j] = ' ';
            }
            j++;
        }
        customer c = {
            atoi(&fileInformation[i][0]), //id
            atoi(&fileInformation[i][2]), //priority
            atoi(&fileInformation[i][4]), //arrival_time
            atoi(&fileInformation[i][6]), //service_time
            0.0,
            0.0
        };
        customerArray[i-1] = c;
    }
}

//main function use to proccessing all the function
int main(int argc, char* argv[]) {
    char fileInformation[MAX_INPUT_SIZE][MAX_INPUT_SIZE];
    readFileIn(argv[1], fileInformation);
    totalCustomer = atoi(fileInformation[0]);
    tokenInformation(fileInformation, totalCustomer);
    bQueue = constructQueue();
    eQueue = constructQueue();

    int f;
    for(f = 1; f <= 4; f++){
        clerk c = {f};
        clerkArray[f-1] = c;
    }

    //create the clerk threads
    int k;
    for(k = 0; k < 4; k++) {
        if(pthread_create(&clerkThreads[k], NULL, clerk_pickup, &clerkArray[k])) {
            perror("Error: failed on thread create.\n");
            exit(0);
        }
    }

    //initial the time
    gettimeofday(&init_time, NULL);

    //create the customer threads
    int i;
    for(i = 0; i < totalCustomer; i++) {
        if(pthread_create(&customerThreads[i], NULL, customer_entry, &customerArray[i])) {
            perror("Error: failed on thread creation.\n");
            exit(0);
        }
    }

    //join the customer threads back to main thread or main function
    int j;
    for(j = 0; j < totalCustomer; j++) {
        if(pthread_join(customerThreads[j], NULL)){
            perror("Error: failed on join thread.\n");
            exit(0);
        }
    }

    printf("The average waiting time for all customers in the system is: %.2f seconds. \n", overAll_wait_time/totalCustomer);
    printf("The average waiting time for all business-class customers is: %.2f seconds. \n", overAll_time_bClass/countBusiness);
    printf("The average waiting time for all economy-class customers is: %.2f seconds. \n", overAll_time_eClass/countEconomy);

    return 0;
}
