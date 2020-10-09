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
    printf("\nAll the vaccines prepared by Pharmaceutical Company %d are emptied. Resuming production now.\n", 0);
    com_batch[0] = rand()%5+1;
    com_num[0] = rand()%11+10;
    printf("\nPharmaceutical Company %d is preparing %d batches of vaccines which have success probability %d\n", 0, com_batch[0], com_prob[0]);
    sleep(rand()%4+2);
    printf("\nPharmaceutical Company %d has prepared %d batches of vaccines which have success probability %d. Waiting for all the vaccines to be used to resume production\n", 0, com_batch[0], com_prob[0]);
    for(int i=0;i<2;i++) {
        if(com_batch[0] != 0) {
            if(zone_vac[i] == 0) {
                printf("\nPharmaceutical Company %d is delivering a vaccine batch to Vaccination Zone %d which has success probability %d\n", 0, i, com_prob[0]);
                sleep(2);
                zone_vac[i] = com_num[0];
                printf("\nPharmaceutical Company %d has delivered vaccines to Vaccination zone %d, resuming vaccinations now\n", 0, i);
            }
            com_batch[0]--;
        }
    }
    return NULL;
}

void* vaccinate(void* inp) {
    stu* input = (stu*) inp;
    printf("Vaccination Zone %d entering Vaccination Phase\n", 0);
    sleep(2);
    if(stu_trial[input->id] == 0) {
        printf("\nEXIT1\n");
        return NULL;
    }
    if(var == 0) {
        printf("\nEXIT2\n");
        printf("\nVaccine stock over!\n");
        stu_trial[input->id]=0;
        return NULL;
    }
    stu_cnt++;
    pthread_mutex_lock(&mutex);
    var--;
    if(var > 0) {
        printf("Student %d on Vaccination zone %d waiting to be vaccinated\n", input->id, 0);
        //sleep(1);
        if((double)rand() / (double)RAND_MAX < zone_prob[0]) {
            green();
            printf("Student %d has tested positive for antibodies\n\n", input->id);
            reset();
            stu_trial[input->id]=0;
        }
        else {
            red();
            printf("Student %d has tested negative for antibodies\n\n", input->id);
            reset();
            stu_trial[input->id]--;
        }
    }
    else {
        printf("Vaccine stock over!\n");
    }
    stu_cnt --;
    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main() {
    pthread_t ids[3];
    stu* s[3];
    stu_trial[0]=3;
    stu_trial[1]=3;
    stu_trial[2]=3;
    zone_prob[0]=0.3;
    zone_slot[0]=2;
    int slt_cnt=0;
    int stu_assign[3];

    pthread_mutex_init(&mutex, NULL);

    while(cnt > 0) {
        for(int i=0;i<3;i++) {
            stu_assign[i]=-1;
        }
        stu_cnt=0;
        slt_cnt=0;
        for(int i=0;i<3;i++) {
            if(stu_trial[i] > 0) {
                printf("Student %d has arrived for his %d round of vaccination\n", i, 4-stu_trial[i]);
                if(slt_cnt < zone_slot[0]) {
                    slt_cnt++;
                    s[i] = (stu*)malloc(sizeof(stu));
                    s[i]->id=i;
                    stu_assign[i]=1;
                    pthread_create(&ids[i], NULL, vaccinate, (void*)s[i]);
                    //pthread_join(ids[i], NULL);
                }
            }
            //pthread_join(NULL, NULL);
        }
        for(int i=0;i<3;i++) {
            if(stu_assign[i] !=-1) {
                pthread_join(ids[i], NULL);
            }
        }
        
        cnt = stu_trial[0] + stu_trial[1] + stu_trial[2];
    }

    pthread_mutex_destroy(&mutex);

    printf("Vaccines left: %d\n", var);
    printf("\nSimulation Over.\n");
    return 0;
}
