#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

int var=8;
int stu_trial[3];
int stu_cnt;

float zone_prob[2];
int zone_slot[2];
int zone_vac[2];

int cnt = 3*3;

int com_batch[1];
int com_num[1];
int com_prob[1];

pthread_mutex_t mutex;

void red() {
    printf("\033[1;31m");
}

void blue() {
    printf("\033[1;34m");
}

void cyan() {
    printf("\033[1;36m");
}

void yellow() {
    printf("\033[1;33m");
}

void green() {
    printf("\033[1;32m");
}

void reset() {
    printf("\033[0m");
}

typedef struct stu{
    int id;
}stu;

void* produce(void* inp) {
    sleep(rand()%4+2);
    com_batch[0] = rand()%5+1;
    com_num[0] = rand()%11+10;
    for(int i=0; i<2;i++) {
        if(com_batch[0] != 0) {
            if(zone_vac[i] == 0) {
                zone_vac[i] = com_num[0];
            }
            com_batch[0]--;
        }
    }
    return NULL;
}

void* vaccinate(void* inp) {
    stu* input = (stu*) inp;
    sleep(2);
    if(stu_trial[input->id] == 0) {
        return NULL;
    }
    if(var == 0) {
        printf("Vaccine stock over!\n");
        stu_trial[input->id]=0;
        return NULL;
    }
    stu_cnt++;
    //pthread_mutex_lock(&mutex);
    var--;
    if(var > 0) {
        printf("Student %d has arrived for his %d round of vaccination\n", input->id, 4-stu_trial[input->id]);
        //sleep(1);
        if((double)rand() / (double)RAND_MAX < zone_prob[0]) {
            green();
            printf("Student %d vaccination succeeded\n\n", input->id);
            reset();
            stu_trial[input->id]=0;
        }
        else {
            red();
            printf("Student %d vaccination failed\n\n", input->id);
            reset();
            stu_trial[input->id]--;
        }
    }
    else {
        printf("Vaccine stock over!\n");
    }
    stu_cnt --;
    //pthread_mutex_unlock(&mutex);
    return NULL;
}

int main() {
    pthread_t tid1;
    pthread_t tid2;
    pthread_t tid3;
    pthread_t ids[3];
    stu* s[3];
    stu_trial[0]=3;
    stu_trial[1]=3;
    stu_trial[2]=3;
    zone_prob[0]=0.3;
    zone_slot[0]=2;
    int slt_cnt=0;

    pthread_mutex_init(&mutex, NULL);

    while(cnt > 0) {
        stu_cnt=0;
        slt_cnt=0;
        for(int i=0;i<3;i++) {
            if(stu_trial[i] > 0 && slt_cnt < zone_slot[0]) {
                slt_cnt++;
                s[i] = (stu*)malloc(sizeof(stu));
                s[i]->id=i;
                pthread_create(&ids[i], NULL, vaccinate, (void*)s[i]);
                //pthread_join(ids[i], NULL);
            }
            //pthread_join(tid3, NULL);
        }
        cnt = stu_trial[0] + stu_trial[1] + stu_trial[2];
    }

    pthread_mutex_destroy(&mutex);

    printf("Vaccines left: %d\n", var);
    return 0;
}
