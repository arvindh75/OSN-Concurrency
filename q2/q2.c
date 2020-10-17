#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_N 1001 //Assumed maximum number of Students, Zones and Companies

pthread_mutex_t com_lock[MAX_N]; //Lock for each company
pthread_mutex_t zone_lock[MAX_N]; //Lock for each zone
pthread_t com_thr[MAX_N]; //Thread for each company
pthread_t zone_thr[MAX_N]; //Thread for each zone
pthread_t stu_thr[MAX_N]; //Thread for each student

int com_batches[MAX_N]; //Number of batches produced by each company
int com_vac[MAX_N]; //Number of vaccines in each batch produced by each company
double com_prob[MAX_N]; //Probability of success of vaccines by each company
int com_left[MAX_N]; //Number of batches left for each company

int stu_trail[MAX_N]; //Number of trails a student has gone through
int stu_vac[MAX_N]; //Works like a boolean. 1 is antibody is tested positive else 0

double zone_prob[MAX_N]; //Probability of the vaccine batch in each zone
int zone_vacleft[MAX_N]; //Vaccines left in a batch in each zone
int zone_slots[MAX_N]; //Randomly chosen slots for each zone
int zone_prev_slots[MAX_N]; //Previous slots for each zone
int zone_prog[MAX_N]; //Boolean. 1 if zone in is Vaccination phase else 0
int zone_over_slots[MAX_N]; //Number of slots that are used up in each zone
pthread_mutex_t common_lock;

typedef struct st{ //Struct to pass ID
    int id;
}st;

//Colors
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

int n,m,o,stu,waiting; //Global variables for input, waiting is the number of students waiting currently

void* company(void* inp) { //Function for company thread
    int id = ((struct st*)inp)->id; //Extracting ID
    int vac,bat;
    while(stu > 0) { //Stay on an infinite loop until the number of students left becomes zero
        srand(time(NULL));
        vac = rand()%11+10; //Random number of vaccines in each batch
        bat = rand()%5+1; //Random number of batches
        printf("\nPharmaceutical Company %d is preparing %d batches of vaccines which have success probability %lf\n", id, bat, com_prob[id]);
        sleep(rand()%4+2); //Random preparation time
        printf("\nPharmaceutical Company %d has prepared %d batches of vaccines which have success probability %lf. Waiting for all the vaccines to be used to resume production\n", id, bat, com_prob[id]);
        //Update company info
        pthread_mutex_lock(&com_lock[id]);
        com_batches[id]=bat;
        com_vac[id]=vac;
        com_left[id]=bat;
        pthread_mutex_unlock(&com_lock[id]);
        
        while(com_left[id] > 0) { //Busy wait until all the batches delivered are used up
            if(stu==0)
                return NULL; //Return if all students are done with vaccination
        }
        printf("\nAll the vaccines prepared by Pharmaceutical Company %d are emptied. Resuming production now.\n", id);
    }
    return NULL;
}

int min(int a, int b, int c) { //Find the minimum of three integers
    int num=0;
    while(a && b && c) {
        a--;
        b--;
        c--;
        num++;
    }
    return num;
}

int assign_slot(int id) { //Assign slots for each zone
    int temp;
    while((temp = waiting) <= 0) { //Wait until atleast one student is there on the waiting queue
        if(stu==0) {
            return -1;
        }
    }
    
    pthread_mutex_lock(&zone_lock[id]);
    zone_prog[id]=0; //Stop the zone if its in vaccination phase
    zone_slots[id] = (rand()%(min(zone_vacleft[id], 8, temp))) +1; //Randomly create slots for each zone
    zone_prev_slots[id] = zone_slots[id];
    printf("\nVaccination Zone %d is ready to vaccinate with %d slots\n", id, zone_prev_slots[id]);
    pthread_mutex_unlock(&zone_lock[id]);
    
    while(zone_slots[id] && waiting) { //Wait until theres atleast one student in the waiting queue
    }
    
    pthread_mutex_lock(&zone_lock[id]);
    zone_over_slots[id] = zone_prev_slots[id] - zone_slots[id]; //Update over slots
    printf("\nVaccination Zone %d entering Vaccination Phase\n", id);
    zone_prog[id]=1; //Zone enters vaccination phase
    pthread_mutex_unlock(&zone_lock[id]);
    
    while(zone_over_slots[id]) { //Wait until all the slots are assigned and over
        if(stu==0)
            return -1;
    }
    pthread_mutex_lock(&zone_lock[id]);
    zone_prog[id]=0; //Zone exits vaccination phase
    pthread_mutex_unlock(&zone_lock[id]);
    return 0;
}

void* zone(void* inp) {
    int id = ((struct st*)inp)->id;
    int it=0, x;
    while(stu > 0) { //Wait in an infinite loop until all students are done
        pthread_mutex_lock(&zone_lock[id]);
        zone_prog[id]=0; //Set the zone status as "Not vaccinating" currently
        pthread_mutex_unlock(&zone_lock[id]);

        while(stu) { //Loop through all companies infinitely
            pthread_mutex_lock(&com_lock[it]);
            if(com_batches[it] > 0) { //If a company has a deliverable batch
                //Assign a batch from the company to the current zone
                com_batches[it]--; //Decrease the batches left
                pthread_mutex_unlock(&com_lock[it]);
                
                pthread_mutex_lock(&com_lock[it]);
                pthread_mutex_lock(&zone_lock[id]);
                zone_prob[id] = com_prob[it]; //Copy zone probability as selected company's vaccines' probability
                zone_vacleft[id] = com_vac[it]; //Copy number of vaccines in zone as the number of vaccines in each batch of the selected company
                pthread_mutex_unlock(&zone_lock[id]);
                pthread_mutex_unlock(&com_lock[it]);
                
                printf("\nPharmaceutical Company %d is delivering a vaccine batch to Vaccination Zone %d which has success probability %lf\n", it, id, com_prob[it]);
                //sleep(1); //DIFF
                printf("\nPharmaceutical Company %d has delivered vaccines to Vaccination zone %d, resuming vaccinations now\n", it, id);
                break;
            }
            pthread_mutex_unlock(&com_lock[it]);
            it++;
            it = it%n; //Circular loop
        }
        
        x = assign_slot(id); //Allocate slots randomly
        if(x==-1) {
            return NULL;
        }
        while(zone_vacleft[id]) { //Allocate slots as long as they are vaccines left in the zone
            x = assign_slot(id);
            if(x==-1) {
                return NULL;
            }
        }
        
        pthread_mutex_lock(&zone_lock[id]);
        zone_prog[id]=0; //If it reaches here, the vaccination phase is over as all vaccines are use up
        zone_slots[id]=0; //Reset number of slots
        pthread_mutex_unlock(&zone_lock[id]);
        
        printf("\nVaccination Zone %d has run out of vaccines\n", id);
        
        pthread_mutex_lock(&com_lock[it]);
        com_left[it]--; //Reduce the company's number of batches live in circulation
        pthread_mutex_unlock(&com_lock[it]);
    }
    return NULL;
}

int waitzone(int id) { //Assign zones to waiting students
    int it=0;
    printf("\nStudent %d is waiting to be allocated a slot on a Vaccination Zone\n", id);
    pthread_mutex_lock(&common_lock);
    waiting++; //Increase the number of students in waiting queue
    pthread_mutex_unlock(&common_lock);
    while(stu) { //Loop through all the zones
        pthread_mutex_lock(&zone_lock[it]);
        if(zone_slots[it] > 0 && (zone_prog[it] == 0)) { //If the zone atleast 1 slot and is not in vaccination phase
            zone_slots[it]--; //Decrease the slots in the zone
            pthread_mutex_lock(&common_lock);
            waiting--; //Remove the student from the waiting queue
            pthread_mutex_unlock(&common_lock);
            pthread_mutex_unlock(&zone_lock[it]);
            
            printf("\nStudent %d assigned a slot on the Vaccination Zone %d and waiting to be vaccinated\n", id, it);
            
            while(zone_prog[it] == 0) { //Wait till the zone starts vaccination phase
            }
            
            printf("\nStudent %d on Vaccination Zone %d has been vaccinated which has success probability %lf\n", id, it, zone_prob[it]);
            
            pthread_mutex_lock(&zone_lock[it]);
            zone_vacleft[it]--; //Decrease the vaccines left in the zone
            zone_over_slots[it]--; //Decrease the left out slots
            pthread_mutex_unlock(&zone_lock[it]);

            if((double)rand() / (double)RAND_MAX < zone_prob[it]) { //Check if the vaccination was successful
                green();
                printf("\nStudent %d has tested positive for antibodies.\n\033[0m", id);
                stu_vac[id]=1; //If successful, student can enter college
                return 1;
            }
            else {
                red();
                printf("\nStudent %d has tested negative for antibodies.\n\033[0m", id);
                stu_vac[id]=0;  
                stu_trail[id]++; //If not successful, increase the trail number for the student
                return 1;
            }
        }
        pthread_mutex_unlock(&zone_lock[it]);
        it = it+1;
        it = it%m; //Circular loop
    }
    return 0;
}

void* student(void* inp) {
    sleep(rand()%3+1); //Sleep to ensure random arrival of students
    int id = ((struct st*)inp)->id;
    int ret;
    while(stu_vac[id]!=1 && stu_trail[id]<4) { //Add student to waiting queue unless he has already had 3 trails or is tested positive for antibodies
        printf("\nStudent %d has arrived for his round %d of Vaccination\n", id, stu_trail[id]);
        ret = waitzone(id); //Add student to waiting queue and try to assign
    }
    if(stu_vac[id]!=1) { //If tested negative for antibodies for all 3 times, student is sent back home
        printf("\nStudent %d is sent back home.\n", id);
    }
    
    pthread_mutex_lock(&common_lock);
    stu--; //Decrease the total number of students
    pthread_mutex_unlock(&common_lock);
    
    return NULL;
}

int main() {
    scanf("%d %d %d", &n, &m, &o); //Get the inputs
    if(n == 0 || m==0 || o==0) {
        printf("Wrong inputs\n");
        printf("\nSimulation Over.\n"); //Simulation ends
        return 0;
    }
    stu=o;
    waiting=0;
    int i;
    for(i=0;i<n;i++) {
        scanf("%lf", &com_prob[i]); //Get the probabilities for each company
    }
    pthread_mutex_init(&common_lock, NULL); //Initialize lock
    //Create structures for all students, zones and companies
    st stu_struct[o]; 
    st com_struct[n];
    st zone_struct[m];
    
    for(i=0;i<n;i++) { //Loop and initialize Company related info and create threads
        com_vac[i]=0;
        com_left[i]=0;
        com_struct[i].id=i;
        pthread_mutex_init(&com_lock[i], NULL);
    }
    
    for(i=0;i<m;i++) { //Loop and initialize zone related info and create threads
        zone_prob[i]=0;
        zone_prog[i]=0;
        zone_slots[i]=0;
        zone_over_slots[i]=0;
        zone_prev_slots[i]=0;
        zone_vacleft[i]=0;
        zone_struct[i].id=i;
        pthread_mutex_init(&zone_lock[i], NULL);
    }
    
    for(i=0;i<o;i++) { //Loop and initialize student related info and create threads
        stu_vac[i]=0;
        stu_trail[i]=1;
        stu_struct[i].id=i;
    }

    for(i=0;i<n;i++) { //Loop and initialize Company related info and create threads
        pthread_create(&com_thr[i], NULL, company, &com_struct[i]);
    }

    for(i=0;i<m;i++) { //Loop and initialize zone related info and create threads
        pthread_create(&zone_thr[i], NULL, zone, &zone_struct[i]);
    }

    for(i=0;i<o;i++) { //Loop and initialize student related info and create threads
        pthread_create(&stu_thr[i], NULL, student, &stu_struct[i]);
    }

    for(i=0;i<n;i++) { //Join all Company threads
        pthread_join(com_thr[i], NULL);
    }
    
    for(i=0;i<m;i++) { //Join all zone threads
        pthread_join(zone_thr[i], NULL);
    }
    
    for(i=0;i<o;i++) { //Join all student threads
        pthread_join(stu_thr[i], NULL);
    }
    
    printf("\nSimulation Over.\n"); //Simulation ends
    for(i=0;i<n;i++) { //Destroy all company locks
        pthread_mutex_destroy(&com_lock[i]);
    }
    for(i=0;i<m;i++) { //Destroy all zone locks
        pthread_mutex_destroy(&zone_lock[i]);
    }
    pthread_mutex_destroy(&common_lock); //Destroy common lock
    return 0;
}
