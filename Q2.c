#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_N 1000

pthread_mutex_t com_lock[MAX_N];
pthread_mutex_t zone_lock[MAX_N];
pthread_t com_thr[MAX_N];
pthread_t zone_thr[MAX_N];
pthread_t stu_thr[MAX_N];

int com_batches[MAX_N];
int com_vac[MAX_N];
double com_prob[MAX_N];
int com_left[MAX_N];

int stu_trail[MAX_N];
int stu_vac[MAX_N];

double zone_prob[MAX_N];
int zone_vacleft[MAX_N];
int zone_slots[MAX_N];
int zone_prog[MAX_N];
int zone_over_slots[MAX_N];

typedef struct st{
    int id;
}st;

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

int n,m,o,stu,waiting;

void* company(void* inp) {
    int id = ((struct st*)inp)->id;
    int vac,bat;
    while(stu > 0) {
        srand(time(NULL));
        vac = rand()%11+10;
        bat = rand()%5+1;
        printf("\nPharmaceutical Company %d is preparing %d batches of vaccines which have success probability %lf\n", id, bat, com_prob[id]);
        sleep(rand()%4+2);
        printf("\nPharmaceutical Company %d has prepared %d batches of vaccines which have success probability %lf. Waiting for all the vaccines to be used to resume production\n", id, bat, com_prob[id]);
        com_batches[id]=bat;
        com_vac[id]=vac;
        com_left[id]=bat;
        while(com_left[id] > 0) {
            if(stu==0)
                return NULL;
        }
        printf("\nAll the vaccines prepared by Pharmaceutical Company %d are emptied. Resuming production now.\n", id);
    }
    return NULL;
}

int min(int a, int b, int c) {
    if(a<=c && a<=b) {
        return a;
    }
    if(b<=c && b<=a) {
        return b;
    }
    return c;
}

int assign_slot(int id) {
    int temp;
    while((temp = waiting) <= 0) {
        if(stu<1) {
            return -1;
        } 
    }
    zone_slots[id] = (rand()%(min(zone_vacleft[id], 8, temp))) +1;
    int temp2= zone_slots[id];
    printf("\nVaccination Zone %d is ready to vaccinate with %d slots\n", id, temp2);
    zone_prog[id]=0;
    while(zone_slots[id] && waiting) {
    }
    zone_over_slots[id] = temp2-zone_slots[id];
    printf("\nVaccination Zone %d entering Vaccination Phase\n", id);
    zone_prog[id]=1;
    while(zone_over_slots[id]) {
        if(stu==0)
            return -1;
    }
    return 0;
}

void* zone(void* inp) {
    int id = ((struct st*)inp)->id;
    int it=0, x;
    while(stu > 0) {
        zone_prog[id]=0;
        while(1) {
            pthread_mutex_lock(&com_lock[it]);
            if(com_batches[it] > 0) {
                com_batches[it]--;
                pthread_mutex_unlock(&com_lock[it]);
                zone_prob[id] = com_prob[it];
                zone_vacleft[id] = com_vac[it];
                printf("\nPharmaceutical Company %d is delivering a vaccine batch to Vaccination Zone %d which has success probability %lf\n", it, id, com_prob[it]);
                sleep(1); //DIFF
                printf("\nPharmaceutical Company %d has delivered vaccines to Vaccination zone %d, resuming vaccinations now\n", it, id);
                break;
            }
            pthread_mutex_unlock(&com_lock[it]);
            it++;
            it = it%n;
        }
        //NEW
        /*
        int waiters;
slot_dec:
        while((waiters==waiting)<=0) {
            if(stu==0)
                return NULL;
        }
        int k = (rand()%(min(zone_vacleft[id], 8, waiters))) +1;
        zone_slots[id]=k;
        printf("\nVaccination Zone %d is ready to vaccinate with %d slots\n", id, k);
        zone_prog[id]=0;
        while(zone_over_slots[id] > 0 && waiting > 0) {
        }
        zone_over_slots[id] = k-zone_slots[id];
        printf("\nVaccination Zone %d entering Vaccination Phase\n", id);
        zone_prog[id]=1;
        while(zone_over_slots[id]) {
            if(stu==0)
                return NULL;
        }
        if(zone_vacleft[id]>0)
            goto slot_dec;
        //NEW
        */
        x = assign_slot(id);
        
        if(x==-1) {
            return NULL;
        }
        while(zone_vacleft[id]) {
            x = assign_slot(id);
            if(x==-1) {
                return NULL;
            }
        }
        zone_prog[id]=0;
        zone_slots[id]=0;
        printf("\nVaccination Zone %d has run out of vaccines\n", id);
        com_left[it]--;
    }
    return NULL;
}

int waitzone(int id) {
    int it=0;
    printf("\nStudent %d is waiting to be allocated a slot on a Vaccination Zone\n", id);
    waiting++;
    while(1) {
        //printf("\n\nHERE %d %d %d\n\n",it,zone_slots[it], zone_prog[it]);
        pthread_mutex_lock(&zone_lock[it]);
        if(zone_slots[it] > 0 && (zone_prog[it] == 0)) {
            zone_slots[it]--;
            waiting--;
            pthread_mutex_unlock(&zone_lock[it]);
            printf("\nStudent %d assigned a slot on the Vaccination Zone %d and waiting to be vaccinated\n", id, it);
            while(zone_prog[it] == 0) {
            }
            zone_vacleft[it]--;
            zone_over_slots[it]--;
            printf("\nStudent %d on Vaccination Zone %d has been vaccinated which has success probability %lf\n", id, it, zone_prob[it]);
            if((double)rand() / (double)RAND_MAX < zone_prob[it]) {
                green();
                printf("\nStudent %d has tested positive for antibodies.\n\033[0m", id);
                stu_vac[id]=1;
                return 1;
            }
            else {
                red();
                printf("\nStudent %d has tested negative for antibodies.\n\033[0m", id);
                stu_vac[id]=0;
                stu_trail[id]++;
                return 1;
            }
        }
        pthread_mutex_unlock(&zone_lock[it]);
        it = it+1;
        it = it%m;
    }
    return 0;
}

void* student(void* inp) {
    sleep(rand()%4+2);
    int id = ((struct st*)inp)->id;
    int ret;
    while(stu_vac[id]!=1 && stu_trail[id]<4) {
        printf("\nStudent %d has arrived for his round %d of Vaccination\n", id, stu_trail[id]);
        ret = waitzone(id);
    }
    if(stu_vac[id]!=1) {
        printf("\nStudent %d is sent back home.\n", id);
    }
    stu--;
    return NULL;
}

int main() {
    scanf("%d %d %d", &n, &m, &o);
    stu=o;
    waiting=0;
    int i;
    for(i=0;i<n;i++) {
        scanf("%lf", &com_prob[i]);
    }
    st stu_struct[o];
    st com_struct[n];
    st zone_struct[m];
    //st* s1 = (st*)malloc(sizeof(st));
    for(i=0;i<n;i++) {
        com_vac[i]=0;
        com_left[i]=0;
        com_struct[i].id=i;
        pthread_mutex_init(&com_lock[i], NULL);
        pthread_create(&com_thr[i], NULL, company, &com_struct[i]);
    }
    for(i=0;i<m;i++) {
        zone_prob[i]=0;
        zone_prog[i]=0;
        zone_slots[i]=0;
        zone_vacleft[i]=0;
        zone_struct[i].id=i;
        pthread_mutex_init(&zone_lock[i], NULL);
        pthread_create(&zone_thr[i], NULL, zone, &zone_struct[i]);
    }
    for(i=0;i<o;i++) {
        stu_vac[i]=0;
        stu_trail[i]=1;
        stu_struct[i].id=i;
        pthread_create(&stu_thr[i], NULL, student, &stu_struct[i]);
    }
    for(i=0;i<n;i++) {
        pthread_join(com_thr[i], NULL);
    }
    for(i=0;i<m;i++) {
        pthread_join(zone_thr[i], NULL);
    }
    for(i=0;i<o;i++) {
        pthread_join(stu_thr[i], NULL);
    }
    printf("\nSimulation Over.\n");
    return 0;
}
