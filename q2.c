#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

int var=8;
int stu_trial[3];
float zone_prob[1];
int zone_slot[1];
int stu_cnt;

int cnt = 3*3;

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

void* vaccinate(void* inp) {
    stu* input = (stu*) inp;
    sleep(2);
    if(stu_trial[input->id] == 0) {
        return NULL;
    }
    stu_cnt++;
    //printf("STU_CNT: %d\n", stu_cnt);
    if(stu_cnt < 2) {
        pthread_mutex_lock(&mutex);
        //printf("\nhere1\n");
        var--;
        if(var > 0) {
            printf("%d got vaccinated\n", input->id);
            if((double)rand() / (double)RAND_MAX < zone_prob[0]) {
                green();
                printf("%d vaccination succeeded\n\n", input->id);
                reset();
                stu_trial[input->id]=0;
            }
            else {
                red();
                printf("%d vaccination failed\n\n", input->id);
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
    else {
        var--;
        //printf("\nhere2\n");
        if(var > 0) {
            printf("%d got vaccinated\n", input->id);
            if((double)rand() / (double)RAND_MAX < zone_prob[0]) {
                green();
                printf("%d vaccination succeeded\n\n", input->id);
                reset();
                stu_trial[input->id]=0;
            }
            else {
                red();
                printf("%d vaccination failed\n\n", input->id);
                reset();
                stu_trial[input->id]--;
            }
        }
        else {
            printf("Vaccine stock over!\n");
        }
        stu_cnt --;
        return NULL;
    }
}

int main() {
    pthread_t tid1;
    pthread_t tid2;
    pthread_t tid3;
    pthread_t ids[3];
    stu_trial[0]=3;
    stu_trial[1]=3;
    stu_trial[2]=3;
    zone_prob[0]=0.75;
    zone_slot[0]=2;

    pthread_mutex_init(&mutex, NULL);

    while(cnt > 0) {
        stu_cnt=0;
        stu* s1 = (stu*)malloc(sizeof(stu));
        stu* s2 = (stu*)malloc(sizeof(stu));
        stu* s3 = (stu*)malloc(sizeof(stu));
        s1->id=0;
        s2->id=1;
        s3->id=2;

        pthread_create(&tid1, NULL, vaccinate, (void*)s1);
        pthread_create(&tid2, NULL, vaccinate, (void*)s2);
        pthread_create(&tid3, NULL, vaccinate, (void*)s3);

        pthread_join(tid1, NULL);
        pthread_join(tid2, NULL);
        pthread_join(tid3, NULL);
        cnt = stu_trial[0] + stu_trial[1] + stu_trial[2];
    }

    pthread_mutex_destroy(&mutex);

    printf("Vaccines left: %d\n", var);
    return 0;
}

