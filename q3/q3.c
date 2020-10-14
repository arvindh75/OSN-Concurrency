#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>

#define MAX_N 1001

pthread_mutex_t per_lock[MAX_N];
pthread_mutex_t stage_lock[MAX_N];

char per_name[50][MAX_N];
char per_inst[MAX_N];
int per_time[MAX_N];
int per_status[MAX_N];
int per_coid[MAX_N];
int per_stage[MAX_N];

int stage_typ[MAX_N];
int stage_per[MAX_N];
int stage_status[MAX_N];

pthread_t per_thr[MAX_N];

sem_t a_sem;
sem_t e_sem;
sem_t c_sem;
sem_t join_sem;

int k,a,e,c,t1,t2,t;

typedef struct st{
    int id;
}st;
    
st per_struct[MAX_N];

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

void reset() {
    printf("\033[0m");
}

void* acoustic(void* input) {
    int id = ((struct st*)input)->id;
    int i;
    struct timespec tim;
    clock_gettime(CLOCK_REALTIME, &tim);
    tim.tv_sec += t;
    int ret = sem_timedwait(&a_sem, &tim);
    pthread_mutex_lock(&per_lock[id]);
    if(ret == -1) {
        if(per_status[id] == 0) {
            per_status[id] = 2;
            red();
            printf("\n%s %c has left because of impatience\n", per_name[id], per_inst[id]);
            reset();
        }
        pthread_mutex_unlock(&per_lock[id]);
        return NULL;
    }
    if(per_status[id] == 1) {
        sem_post(&a_sem);
        pthread_mutex_unlock(&per_lock[id]);
        return NULL;
    }
    if(per_status[id] == 2) {
        sem_post(&a_sem);
        pthread_mutex_unlock(&per_lock[id]);
        return NULL;
    }
    for(i=0;i<a+e;i++) {
        pthread_mutex_lock(&stage_lock[i]);
        if(stage_status[i] == 0) {
            if(stage_typ[i] == 0) {
                stage_per[i] = id;
                per_status[id] = 1;
                per_stage[id] = i;
                if(per_inst[id] == 's') {
                    stage_status[i] = 2;
                }
                else {
                    stage_status[i] = 1;
                    sem_post(&join_sem);
                }
                pthread_mutex_unlock(&stage_lock[i]);
                break;
            }
        }
        pthread_mutex_unlock(&stage_lock[i]);
    }
    pthread_mutex_unlock(&per_lock[id]);
    srand(time(0));
    double ti = (rand()*(t2-t1))/RAND_MAX + t1;
    yellow();
    printf("\n%s performing %c at Acoustic stage for %d sec\n", per_name[id], per_inst[id], (int)ti);
    reset();
    sleep((int)ti);
    printf("\n%s performing %c at Acoustic stage for 1st\n", per_name[id], per_inst[id]);
    int co_per=1;
    pthread_mutex_lock(&per_lock[id]);
    pthread_mutex_lock(&stage_lock[per_stage[id]]);
    if(per_coid[id] != -1) {
        pthread_mutex_unlock(&stage_lock[per_stage[id]]);
        pthread_mutex_unlock(&per_lock[id]);
        sleep(2);
        pthread_mutex_lock(&per_lock[id]);
        pthread_mutex_lock(&stage_lock[per_stage[id]]);
        co_per=2;
    }
    yellow();
    printf("\n%s performing %c at Acoustic stage for 2nd\n", per_name[id], per_inst[id]);
    reset();
    stage_per[per_stage[id]]=-1;
    stage_status[per_stage[id]]=0;
    sem_post(&a_sem);
    if(per_coid[id] == -1) {
        if(per_inst[id] != 's')
            sem_wait(&join_sem);
    }
    per_status[id]=2;
    green();
    printf("\n%s performance at Acoustic stage ended\n", per_name[id]);
    reset();
    if(per_coid[id] != -1) {
        pthread_mutex_lock(&per_lock[per_coid[id]]);
        per_status[per_coid[id]]=2;
        green();
        printf("\n%s performance at Electric stage ended\n", per_name[per_coid[id]]);
        reset();
        pthread_mutex_unlock(&per_lock[per_coid[id]]);
    }
    pthread_mutex_unlock(&stage_lock[per_stage[id]]);
    pthread_mutex_unlock(&per_lock[id]);
    for(int j=0;j<co_per;j++) {
        sem_wait(&c_sem);
        if(j==0) {
            magenta();
            printf("\n%s collecting T-shirt\n", per_name[id]);
            reset();
        }
        else {
            magenta();
            printf("\n%s collecting T-shirt\n", per_name[per_coid[id]]);
            reset();
        }
        sem_post(&c_sem);
    }
    return NULL;
}

void* electric(void* input) {
    int id = ((struct st*)input)->id;
    int i;
    struct timespec tim;
    clock_gettime(CLOCK_REALTIME, &tim);
    tim.tv_sec += t;
    int ret = sem_timedwait(&e_sem, &tim);
    pthread_mutex_lock(&per_lock[id]);
    if(ret == -1) {
        if(per_status[id] == 0) {
            per_status[id] = 2;
            red();
            printf("\n%s %c has left because of impatience\n", per_name[id], per_inst[id]);
            reset();
        }
        pthread_mutex_unlock(&per_lock[id]);
        return NULL;
    }
    if(per_status[id] == 1) {
        sem_post(&e_sem);
        pthread_mutex_unlock(&per_lock[id]);
        return NULL;
    }
    if(per_status[id] == 2) {
        sem_post(&e_sem);
        pthread_mutex_unlock(&per_lock[id]);
        return NULL;
    }
    for(i=0;i<a+e;i++) {
        pthread_mutex_lock(&stage_lock[i]);
        if(stage_status[i] == 0) {
            if(stage_typ[i] == 1) {
                stage_per[i] = id;
                per_status[id] = 1;
                per_stage[id] = i;
                if(per_inst[id] == 's') {
                    stage_status[i] = 2;
                }
                else {
                    stage_status[i] = 1;
                    sem_post(&join_sem);
                }
                pthread_mutex_unlock(&stage_lock[i]);
                break;
            }
        }
        pthread_mutex_unlock(&stage_lock[i]);
    }
    pthread_mutex_unlock(&per_lock[id]);
    srand(time(0));
    double ti = (rand()*(t2-t1))/RAND_MAX + t1;
    yellow();
    printf("\n%s performing %c at Electric stage for %d sec\n", per_name[id], per_inst[id], (int)ti);
    reset();
    sleep((int)ti);
    int co_per=1;
    pthread_mutex_lock(&per_lock[id]);
    pthread_mutex_lock(&stage_lock[per_stage[id]]);
    if(per_coid[id] != -1) {
        pthread_mutex_unlock(&stage_lock[per_stage[id]]);
        pthread_mutex_unlock(&per_lock[id]);
        sleep(2);
        pthread_mutex_lock(&per_lock[id]);
        pthread_mutex_lock(&stage_lock[per_stage[id]]);
        co_per=2;
    }
    stage_per[per_stage[id]]=-1;
    stage_status[per_stage[id]]=0;
    sem_post(&e_sem);
    if(per_coid[id] == -1) {
        if(per_inst[id] != 's')
            sem_wait(&join_sem);
    }
    per_status[id]=2;
    green();
    printf("\n%s performance at Electric stage ended\n", per_name[id]);
    reset();
    if(per_coid[id] != -1) {
        pthread_mutex_lock(&per_lock[per_coid[id]]);
        per_status[per_coid[id]]=2;
        green();
        printf("\n%s performance at Electric stage ended\n", per_name[per_coid[id]]);
        reset();
        pthread_mutex_unlock(&per_lock[per_coid[id]]);
    }
    pthread_mutex_unlock(&stage_lock[per_stage[id]]);
    pthread_mutex_unlock(&per_lock[id]);
    for(int j=0;j<co_per;j++) {
        sem_wait(&c_sem);
        if(j==0) {
            magenta();
            printf("\n%s collecting T-shirt\n", per_name[id]);
            reset();
        }
        else {
            magenta();
            printf("\n%s collecting T-shirt\n", per_name[per_coid[id]]);
            reset();
        }
        sem_post(&c_sem);
    }
    return NULL;
}

void* join_per(void* input) {
    int id = ((struct st*)input)->id;
    int i;
    struct timespec tim;
    clock_gettime(CLOCK_REALTIME, &tim);
    tim.tv_sec += t;
    int ret = sem_timedwait(&join_sem, &tim);
    pthread_mutex_lock(&per_lock[id]);
    if(ret == -1) {
        if(per_status[id] == 0) {
            per_status[id] = 2;
            red();
            printf("\n%s %c has left because of impatience\n", per_name[id], per_inst[id]);
            reset();
        }
        pthread_mutex_unlock(&per_lock[id]);
        return NULL;
    }
    if(per_status[id] == 1) {
        sem_post(&join_sem);
        pthread_mutex_unlock(&per_lock[id]);
        return NULL;
    }
    if(per_status[id] == 2) {
        sem_post(&join_sem);
        pthread_mutex_unlock(&per_lock[id]);
        return NULL;
    }
    for(i=0;i<a+e;i++) {
        pthread_mutex_lock(&stage_lock[i]);
        if(stage_status[i] == 1) {
            per_stage[id] = i;
            per_status[id] = 1;
            stage_status[i] = 2;
            per_coid[stage_per[i]] = id;
            cyan();
            printf("\n%s joined %s's performance, performance extended by 2 secs\n", per_name[id], per_name[stage_per[id]]);
            reset();
            pthread_mutex_unlock(&stage_lock[i]);
            break;
        }
        pthread_mutex_unlock(&stage_lock[i]);
    }
    pthread_mutex_unlock(&per_lock[id]);
    return NULL;
}

void* performer(void* input) {
    int id = ((struct st*)input)->id;
    green();
    printf("\n%s %c has arrived\n", per_name[id], per_inst[id]);
    reset();
    pthread_t a;
    pthread_t e;
    pthread_t join;
    if(per_inst[id] == 'p' || per_inst[id] == 'g') {
        pthread_create(&a, NULL, acoustic, &per_struct[id]);
        pthread_create(&e, NULL, electric, &per_struct[id]);
        pthread_join(a, NULL);
        pthread_join(e, NULL);
    }
    else if(per_inst[id] == 'b') {
        pthread_create(&e, NULL, electric, &per_struct[id]);
        pthread_join(e, NULL);
    }
    else if(per_inst[id] == 'v') {
        pthread_create(&a, NULL, acoustic, &per_struct[id]);
        pthread_join(a, NULL);
    }
    else if(per_inst[id] == 's') {
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
    sem_init(&a_sem,0,a);
    sem_init(&e_sem,0,e);
    sem_init(&c_sem,0,c);
    sem_init(&join_sem,0,0);
    for(i=0;i<k;i++) {
        scanf("%s %c %d", name, &per_inst[i], &per_time[i]);
        strcpy(per_name[i], name);
        per_status[i] = 0;
        per_coid[i]=-1;
        per_stage[i]=-1;
        pthread_mutex_init(&per_lock[i], NULL);
    }
    for(i=0;i<a+e;i++) {
        if(i<e)
            stage_typ[i]=1;
        else
            stage_typ[i]=0;
        stage_per[i]=-1;
        stage_status[i]=0;
        pthread_mutex_init(&stage_lock[i], NULL);
    }
    blue();
    printf("\nBeginning Simulation\n");
    reset();
    for(i=0;i<k;i++) {
        per_struct[i].id=i;
        pthread_create(&per_thr[i], NULL, performer, &per_struct[i]);
    }
    for(i=0;i<k;i++) {
        pthread_join(per_thr[i], NULL);
    }
    blue();
    printf("\nSimulation over\n");
    reset();
    for(i=0;i<k;i++) {
        pthread_mutex_destroy(&per_lock[i]);
    }
    for(i=0;i<a+e;i++) {
        pthread_mutex_destroy(&stage_lock[i]);
    }
    sem_destroy(&a_sem);
    sem_destroy(&e_sem);
    sem_destroy(&c_sem);
    sem_destroy(&join_sem);
    return 0;
}
