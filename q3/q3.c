#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>

#define MAX_N 1001 //Maximum number of performers

pthread_mutex_t per_lock[MAX_N]; //Array of locks for each performer
pthread_mutex_t stage_lock[MAX_N]; //Array of locks for each stage

char per_name[50][MAX_N]; //Performer name
char per_inst[MAX_N]; //Performer Instrument
int per_time[MAX_N]; //Performer arrival time
int per_status[MAX_N]; //Performer Status
int per_coid[MAX_N]; //Performer's Co-performer ID
int per_stage[MAX_N]; //Performer's assigned stage ID

int stage_typ[MAX_N]; //Stage type
int stage_per[MAX_N]; //Stage's current performer's ID
int stage_status[MAX_N]; //Stage status

pthread_t per_thr[MAX_N]; //Array of performer threads

sem_t a_sem; //Acoustic stage semaphore
sem_t e_sem; //Electric stage semaphore
sem_t c_sem; //Co-ordinator semaphore
sem_t join_sem; //Joinable stages semaphore

int k,a,e,c,t1,t2,t; //Global variables for input

typedef struct st{ //Struct to pass ID
    int id;
}st;
    
st per_struct[MAX_N]; //Create a structure for each performer

//Colors
void red() {
    printf("\033[0;31m");
}

void blue() {
    printf("\033[0;34m");
}

void cyan() {
    printf("\033[0;36m");
}

void yellow() {
    printf("\033[0;33m");
}

void green() {
    printf("\033[0;32m");
}

void magenta() {
    printf("\033[0;35m");
}

void reset() { //Resets the color back
    printf("\033[0m");
}

void* tshirt(void* input) {
    int id = ((struct st*)input)->id;
    sem_wait(&c_sem);
    magenta();
    printf("\n%s collecting T-shirt\n", per_name[id]);
    reset();
    sem_post(&c_sem);
    return NULL;
}

void* acoustic(void* input) { //Function for assigning Acoustic stage
    int id = ((struct st*)input)->id;
    int i;
    struct timespec tim;
    clock_gettime(CLOCK_REALTIME, &tim);
    tim.tv_sec += t;
    int ret = sem_timedwait(&a_sem, &tim); //Timed semaphore wait (To take care of arrival time)
    pthread_mutex_lock(&per_lock[id]);
    if(ret == -1) { //Time up before a free slot is encountered
        if(per_status[id] == 0) { //If stage not already allocated
            per_status[id] = 2; //The performer leaves as he grew impatient
            red();
            printf("\n%s %c has left because of impatience\n", per_name[id], per_inst[id]);
            reset();
        }
        pthread_mutex_unlock(&per_lock[id]);
        return NULL; //Return as the performer left
    }
    if(per_status[id] == 1) { //If a stage is already allocated to the performer by some other thread
        sem_post(&a_sem); //Increase the semaphore value
        pthread_mutex_unlock(&per_lock[id]);
        return NULL; //Return as the current thread lost the race to allocate
    }
    if(per_status[id] == 2) { //If the performer has already left
        sem_post(&a_sem); //Increase the semaphore value
        pthread_mutex_unlock(&per_lock[id]);
        return NULL; //Return as the performer has left in some other thread already
    }
    for(i=0;i<a+e;i++) { //Loop through all stages
        pthread_mutex_lock(&stage_lock[i]);
        if(stage_status[i] == 0) { //If the stage is Free
            if(stage_typ[i] == 0) { //If the stage is Acoustic
                //Allocate the selected stage
                stage_per[i] = id; //Assign stage's performer id as the current performer's ID
                per_status[id] = 1; //Make the status of the performer as "Stage allocated"
                per_stage[id] = i; //Store the stage ID in perfomer's stage ID
                if(per_inst[id] == 's') { //If its a singer
                    stage_status[i] = 2; //Make stage status "Not joinable" as two singers cannot collaborate
                }
                else {
                    stage_status[i] = 1; //Make stage status "Joinable"
                    sem_post(&join_sem); //Increase "Joinable Stages" semaphore
                }
                pthread_mutex_unlock(&stage_lock[i]);
                break; //Stage is allocated so break
            }
        }
        pthread_mutex_unlock(&stage_lock[i]);
    }
    pthread_mutex_unlock(&per_lock[id]);
    srand(time(0));
    double ti = (rand()*(t2-t1))/RAND_MAX + t1; //Choose a random time between t1 and t2
    yellow();
    printf("\n%s performing %c at Acoustic stage for %d sec\n", per_name[id], per_inst[id], (int)ti);
    reset();
    sleep((int)ti); //Sleep for that random time
    int co_per=1; //Count the total number of performers performing with the selected performer
    pthread_mutex_lock(&per_lock[id]);
    pthread_mutex_lock(&stage_lock[per_stage[id]]);
    if(per_coid[id] != -1) { //If there is a co-performer
        pthread_mutex_unlock(&stage_lock[per_stage[id]]);
        pthread_mutex_unlock(&per_lock[id]);
        sleep(2); //Sleep for extra 2 secs
        pthread_mutex_lock(&per_lock[id]);
        pthread_mutex_lock(&stage_lock[per_stage[id]]);
        co_per=2; //Increase the count of performers
    }
    stage_per[per_stage[id]]=-1; //Reset the Stage as the performance is over
    stage_status[per_stage[id]]=0; // Reset the Stage as the performance is over
    sem_post(&a_sem); //Increase ACoustice stages semaphore as the stage is free now
    if(per_coid[id] == -1) { //If there is a co-performer
        if(per_inst[id] != 's') //If the current performer is not a singer
            sem_wait(&join_sem); //Decrease the "Joinable Stages" semaphore as we increased it earlier in line 109
    }
    per_status[id]=2; //Performer leaves as the performance ends
    green();
    printf("\n%s performance at Acoustic stage ended\n", per_name[id]);
    reset();
    if(per_coid[id] != -1) { //If there is a co-performer
        pthread_mutex_lock(&per_lock[per_coid[id]]);
        per_status[per_coid[id]]=2; //Co-perfomer leaves
        green();
        printf("\n%s performance at Acoustic stage ended\n", per_name[per_coid[id]]);
        reset();
        pthread_mutex_unlock(&per_lock[per_coid[id]]);
    }
    pthread_mutex_unlock(&stage_lock[per_stage[id]]);
    pthread_mutex_unlock(&per_lock[id]);
    
    pthread_t c_t[2];
    for(int j=0;j<co_per;j++) { //Loop through number of performer (either 1 or 2)
        if(j==0)
            pthread_create(&c_t[j], NULL, tshirt, &per_struct[id]);
        else
            pthread_create(&c_t[j], NULL, tshirt, &per_struct[per_coid[id]]);
    }
    for(int j=0;j<co_per;j++) { //Loop through number of performer (either 1 or 2)
        pthread_join(c_t[j], NULL);
    }
    
    return NULL; //Return as stage was used and reset
}

void* electric(void* input) { //Function for assigning Electric stage
    int id = ((struct st*)input)->id;
    int i;
    struct timespec tim;
    clock_gettime(CLOCK_REALTIME, &tim);
    tim.tv_sec += t;
    int ret = sem_timedwait(&e_sem, &tim); //Timed semaphore wait (To take care of arrival time)
    pthread_mutex_lock(&per_lock[id]);
    if(ret == -1) { //Time up before a free slot is encountered
        if(per_status[id] == 0) { //If stage not already allocated
            per_status[id] = 2; //The performer leaves as he grew impatient
            red();
            printf("\n%s %c has left because of impatience\n", per_name[id], per_inst[id]);
            reset();
        }
        pthread_mutex_unlock(&per_lock[id]);
        return NULL; //Return as the performer left
    }
    if(per_status[id] == 1) { //If a stage is already allocated to the performer by some other thread
        sem_post(&e_sem); //Increase the semaphore value
        pthread_mutex_unlock(&per_lock[id]);
        return NULL; //Return as the current thread lost the race to allocate
    }
    if(per_status[id] == 2) { //If the performer has already left
        sem_post(&e_sem); //Increase the semaphore value
        pthread_mutex_unlock(&per_lock[id]);
        return NULL; //Return as the performer has left in some other thread already
    }
    for(i=0;i<a+e;i++) { //Loop through all stages
        pthread_mutex_lock(&stage_lock[i]);
        if(stage_status[i] == 0) { //If the stage is Free
            if(stage_typ[i] == 0) { //If the stage is Acoustic
                //Allocate the selected stage
                stage_per[i] = id; //Assign stage's performer id as the current performer's ID
                per_status[id] = 1; //Make the status of the performer as "Stage allocated"
                per_stage[id] = i; //Store the stage ID in perfomer's stage ID
                if(per_inst[id] == 's') { //If its a singer
                    stage_status[i] = 2; //Make stage status "Not joinable" as two singers cannot collaborate
                }
                else {
                    stage_status[i] = 1; //Make stage status "Joinable"
                    sem_post(&join_sem); //Increase "Joinable Stages" semaphore
                }
                pthread_mutex_unlock(&stage_lock[i]);
                break; //Stage is allocated so break
            }
        }
        pthread_mutex_unlock(&stage_lock[i]);
    }
    pthread_mutex_unlock(&per_lock[id]);
    srand(time(0));
    double ti = (rand()*(t2-t1))/RAND_MAX + t1; //Choose a random time between t1 and t2
    yellow();
    printf("\n%s performing %c at Electric stage for %d sec\n", per_name[id], per_inst[id], (int)ti);
    reset();
    sleep((int)ti); //Sleep for that random time
    int co_per=1; //Count the total number of performers performing with the selected performer
    pthread_mutex_lock(&per_lock[id]);
    pthread_mutex_lock(&stage_lock[per_stage[id]]);
    if(per_coid[id] != -1) { //If there is a co-performer
        pthread_mutex_unlock(&stage_lock[per_stage[id]]);
        pthread_mutex_unlock(&per_lock[id]);
        sleep(2); //Sleep for extra 2 secs
        pthread_mutex_lock(&per_lock[id]);
        pthread_mutex_lock(&stage_lock[per_stage[id]]);
        co_per=2; //Increase the count of performers
    }
    stage_per[per_stage[id]]=-1; //Reset the Stage as the performance is over
    stage_status[per_stage[id]]=0; // Reset the Stage as the performance is over
    sem_post(&e_sem); //Increase ACoustice stages semaphore as the stage is free now
    if(per_coid[id] == -1) { //If there is a co-performer
        if(per_inst[id] != 's') //If the current performer is not a singer
            sem_wait(&join_sem); //Decrease the "Joinable Stages" semaphore as we increased it earlier in line 109
    }
    per_status[id]=2; //Performer leaves as the performance ends
    green();
    printf("\n%s performance at Electric stage ended\n", per_name[id]);
    reset();
    if(per_coid[id] != -1) { //If there is a co-performer
        pthread_mutex_lock(&per_lock[per_coid[id]]);
        per_status[per_coid[id]]=2; //Co-perfomer leaves
        green();
        printf("\n%s performance at Electric stage ended\n", per_name[per_coid[id]]);
        reset();
        pthread_mutex_unlock(&per_lock[per_coid[id]]);
    }
    pthread_mutex_unlock(&stage_lock[per_stage[id]]);
    pthread_mutex_unlock(&per_lock[id]);
    pthread_t c_t[2];
    for(int j=0;j<co_per;j++) { //Loop through number of performer (either 1 or 2)
        if(j==0)
            pthread_create(&c_t[j], NULL, tshirt, &per_struct[id]);
        else
            pthread_create(&c_t[j], NULL, tshirt, &per_struct[per_coid[id]]);    
    }
    for(int j=0;j<co_per;j++) { //Loop through number of performer (either 1 or 2)
        pthread_join(c_t[j], NULL);
    }
    return NULL; //Return as stage was used and reset
}

void* join_per(void* input) { //Thread to allocate co-performable stages
    int id = ((struct st*)input)->id;
    int i;
    struct timespec tim;
    clock_gettime(CLOCK_REALTIME, &tim);
    tim.tv_sec += t;
    int ret = sem_timedwait(&join_sem, &tim); //To simulate impatience of performers
    pthread_mutex_lock(&per_lock[id]);
    if(ret == -1) { //If time has run out
        if(per_status[id] == 0) { //If the performer has not assigned stages
            per_status[id] = 2; //Performer leaves
            red();
            printf("\n%s %c has left because of impatience\n", per_name[id], per_inst[id]);
            reset();
        }
        pthread_mutex_unlock(&per_lock[id]);
        return NULL; //Return as performer left
    }
    if(per_status[id] == 1) { //If the performer has a stage already assigned, this thread has llost the race to allocate
        sem_post(&join_sem); //Replenish "Joinable Stages" semaphore
        pthread_mutex_unlock(&per_lock[id]);
        return NULL; //Return as the thread lost the race
    }
    if(per_status[id] == 2) { //If the performer has already left
        sem_post(&join_sem); //Replenish "Joinable Stages" semaphore
        pthread_mutex_unlock(&per_lock[id]);
        return NULL; //Return as the performer already left
    }
    for(i=0;i<a+e;i++) { //Loop through all stages
        pthread_mutex_lock(&stage_lock[i]);
        if(stage_status[i] == 1) { //If the stage is joinable
            //Allocate the stage to the performer
            per_stage[id] = i; //Store stage ID in performer details
            per_status[id] = 1; //Make performer status as "Stage assigned"
            stage_status[i] = 2; //Make stage status as "Not joinable" as there are already two people allocated
            per_coid[stage_per[i]] = id; //Store the current performer's ID in the stage's main performer's co-performer ID
            cyan();
            printf("\n%s joined %s's performance, performance extended by 2 secs\n", per_name[id], per_name[stage_per[id]]);
            reset();
            pthread_mutex_unlock(&stage_lock[i]);
            break; //Break as we allocated a stage
        }
        pthread_mutex_unlock(&stage_lock[i]);
    }
    pthread_mutex_unlock(&per_lock[id]);
    return NULL; //Return as we allocated a stage
}

void* performer(void* input) {
    int id = ((struct st*)input)->id;
    green();
    printf("\n%s %c has arrived\n", per_name[id], per_inst[id]);
    reset();
    pthread_t a; //Thread for Acoustic stages
    pthread_t e; //Thread for Electric stages
    pthread_t join; //Thread for "Joinable stages"
    if(per_inst[id] == 'p' || per_inst[id] == 'g') { //If Piano or Guitar
        //Can allocate Acoustic or Electric stage
        pthread_create(&a, NULL, acoustic, &per_struct[id]);
        pthread_create(&e, NULL, electric, &per_struct[id]);
        pthread_join(a, NULL);
        pthread_join(e, NULL);
    }
    else if(per_inst[id] == 'b') { //If Bass
        //Can allocate only Electric stage
        pthread_create(&e, NULL, electric, &per_struct[id]);
        pthread_join(e, NULL);
    }
    else if(per_inst[id] == 'v') { //If Violin
        //Can allocate only Acoustic stage
        pthread_create(&a, NULL, acoustic, &per_struct[id]);
        pthread_join(a, NULL);
    }
    else if(per_inst[id] == 's') { //If singer
        //Can allocate Acoustic stage or Electric stage or Join an allocated stage as co-performer
        pthread_create(&a, NULL, acoustic, &per_struct[id]);
        pthread_create(&e, NULL, electric, &per_struct[id]);
        pthread_create(&join, NULL, join_per, &per_struct[id]);
        pthread_join(a, NULL);
        pthread_join(e, NULL);
        pthread_join(join, NULL);
    }
    return NULL;
}

int main() {
    int i;
    char name[50];
    scanf("%d %d %d %d %d %d %d", &k, &a, &e, &c, &t1, &t2, &t);
    if((k==0) || (a== 0 && e == 0 ) || (c==0) || (t1>t2)) {
        printf("Wrong inputs\n");
        printf("Simulation Over.\n");
    }
    //Initialize the semaphores with corresponding values
    sem_init(&a_sem,0,a);
    sem_init(&e_sem,0,e);
    sem_init(&c_sem,0,c);
    sem_init(&join_sem,0,0);
    for(i=0;i<k;i++) { //Loop through number of performers, Take input and store their details 
        scanf("%s %c %d", name, &per_inst[i], &per_time[i]);
        strcpy(per_name[i], name);
        per_status[i] = 0;
        per_coid[i]=-1;
        per_stage[i]=-1;
        pthread_mutex_init(&per_lock[i], NULL); //Initialize locks
    }
    for(i=0;i<a+e;i++) { //Loop through the number stages and mark them as either Acoustic or Electric and reset the stage details
        if(i<e)
            stage_typ[i]=1;
        else
            stage_typ[i]=0;
        stage_per[i]=-1;
        stage_status[i]=0;
        pthread_mutex_init(&stage_lock[i], NULL);//Initialize locks
    }
    blue();
    printf("\nBeginning Simulation\n"); //Simulation begins
    reset();
    for(i=0;i<k;i++) { //Loop through number of performers and create a thread for each
        per_struct[i].id=i;
        pthread_create(&per_thr[i], NULL, performer, &per_struct[i]);
    }
    for(i=0;i<k;i++) { //Loop through all performer threads and join them back
        pthread_join(per_thr[i], NULL);
    }
    blue();
    printf("\nSimulation over\n"); //Simulation ends
    reset();
    for(i=0;i<k;i++) { //Destroy all performer locks
        pthread_mutex_destroy(&per_lock[i]);
    }
    for(i=0;i<a+e;i++) { //Destroy all stage locks
        pthread_mutex_destroy(&stage_lock[i]);
    }
    //Destroy the semaphores
    sem_destroy(&a_sem);
    sem_destroy(&e_sem);
    sem_destroy(&c_sem);
    sem_destroy(&join_sem);
    return 0;
}
