#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

int stu_trial[100];
int stu_cnt;

double zone_prob[2];
int zone_slot[2];
int zone_vac[2];
int cnt;

int num_stu, num_zone, num_com;

int com_batch[1];
int com_num[1];
double com_prob[1];

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
    srand(time(0));
    com_batch[0] = rand()%5+1;
    com_num[0] = rand()%11+10;
    com_prob[0] = (double)rand() / (double)RAND_MAX;
    printf("\nPharmaceutical Company %d is preparing %d batches of vaccines which have success probability %lf\n", 0, com_batch[0], com_prob[0]);
    sleep(rand()%4+2);
    printf("\nPharmaceutical Company %d has prepared %d batches of vaccines which have success probability %lf. Waiting for all the vaccines to be used to resume production\n", 0, com_batch[0], com_prob[0]);
    for(int i=0;i<num_zone;i++) {
        if(com_batch[0] != 0) {
            if(zone_vac[i] == 0) {
                printf("\nPharmaceutical Company %d is delivering a vaccine batch to Vaccination Zone %d which has success probability %lf\n", 0, i, com_prob[0]);
                sleep(2);
                zone_vac[i] = com_num[0];
                zone_prob[i] = com_prob[0];
                printf("\nPharmaceutical Company %d has delivered %d vaccines to Vaccination zone %d, resuming vaccinations now\n", 0, com_num[0], i);
                printf("\nVaccination Zone %d entering Vaccination Phase\n", i);
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
        printf("\nEXIT1\n");
        return NULL;
    }
    if(zone_vac[0] == 0) {
        printf("\nEXIT2\n");
        printf("\nVaccine stock over for zone %d\n", 0);
        stu_trial[input->id]=0;
        return NULL;
    }
    stu_cnt++;
    pthread_mutex_lock(&mutex);
    zone_vac[0]--;
    if(zone_vac[0] > 0) {
        printf("\nStudent %d on Vaccination zone %d waiting to be vaccinated\n", input->id, 0);
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
    stu_cnt --;
    pthread_mutex_unlock(&mutex);
    return NULL;
}

int add_trials(int n) {
    int sum=0;
    for(int i=0;i<n;i++) {
        sum += stu_trial[i];
    }
    return sum;
}

pthread_t ids[100];
stu* s[100];
int stu_assign[100];

int main() {
    zone_slot[0]=4;
    int slt_cnt=0;
    pthread_t com0;
    scanf("%d %d %d", &num_stu, &num_zone, &num_com);

    for(int i=0;i<num_stu;i++) {
        stu_trial[i]=3;
    }

    pthread_mutex_init(&mutex, NULL);

    stu* s_com = (stu*)malloc(sizeof(stu));
    s_com->id=0;
    pthread_create(&com0, NULL, produce, (void*)s_com);
    pthread_join(com0, NULL);

    cnt = add_trials(num_stu);
    while(cnt > 0) {
        for(int i=0;i<num_stu;i++) {
            stu_assign[i]=-1;
        }
        stu_cnt=0;
        slt_cnt=0;
        for(int i=0;i<num_stu;i++) {
            if(stu_trial[i] > 0) {
                printf("Student %d has arrived for his %d round of vaccination\n", i, 4-stu_trial[i]);
                if(slt_cnt < zone_slot[0]) {
                    slt_cnt++;
                    s[i] = (stu*)malloc(sizeof(stu));
                    s[i]->id=i;
                    stu_assign[i]=1;
                    pthread_create(&ids[i], NULL, vaccinate, (void*)s[i]);
                }
                else {
                    for(int k=0;k<num_stu;k++) {
                        if(stu_assign[k] !=-1) {
                            pthread_join(ids[k], NULL);
                        }
                    }
                    printf("Zone 0 is done\n");
                    return 0;
                }
            }
        }
        for(int i=0;i<num_stu;i++) {
            if(stu_assign[i] !=-1) {
                pthread_join(ids[i], NULL);
            }
        }
        cnt = add_trials(num_stu);
    }

    pthread_mutex_destroy(&mutex);

    printf("Vaccines left: %d\n", zone_vac[0]);
    printf("\nSimulation Over.\n");
    return 0;
}
