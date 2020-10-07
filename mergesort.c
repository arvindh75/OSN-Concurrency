#define _POSIX_C_SOURCE 199309L //required for clock
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <inttypes.h>
#include <math.h>

void swap(int* a, int* b)
{
    int t = *a;
    *a = *b;
    *b = t;
}

int * shareMem(size_t size){
    key_t mem_key = IPC_PRIVATE;
    int shm_id = shmget(mem_key, size, IPC_CREAT | 0666);
    return (int*)shmat(shm_id, NULL, 0);
}

int partition (int *arr, int low, int high){
    int pivot = arr[high]; // pivot
    int i=low-1;
    for (int j=low; j<=high-1; j++){
        if (arr[j] < pivot){
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return (i + 1);
}

void normal_quickSort(int *arr, int low, int high){
    if(low<high){
        /* pi is partitioning index, arr[p] is now
           at right place */
        int pi = partition(arr, low, high);

        // Separately sort elements before
        // partition and after partition
        normal_quickSort(arr, low, pi - 1);
        normal_quickSort(arr, pi + 1, high);
    }
}

void merge(int *arr, int low, int mid, int high) {
    int s1 = mid - low + 1;
    int s2 = high - mid;
    int l[s1];
    int r[s2];
    int i=0,j,k;

    for(i=0;i<s1;i++) {
        l[i] = arr[low+i]; 
    }

    for(i=0;i<s2;i++) {
        r[i] = arr[mid+1+i];
    }

    i=0;
    j=0;
    k=low;

    while((i<s1) && (j<s2)) {
        if(l[i] <= r[j]) {
            arr[k] = l[i];
            i++;
        }
        else {
            arr[k] = r[j];
            j++;
        }
        k++;
    }

    while(i<s1) {
        arr[k] = l[i];
        i++;
        k++;
    }

    while(j<s2) {
        arr[k] = r[j];
        j++;
        k++;
    }
}

void selection_sort(int *arr, int n) {
    int i,j,m;
    for(i=0;i<n-1;i++) {
        m=i;
        for(j=i+1;j<n;j++) {
            if(arr[j] < arr[m])
                m=j;
        }
        swap(&arr[m], &arr[i]);
    }
}

void normal_mergeSort(int *arr, int low, int high, int n){
    int mid;
    if(low<high){
        mid = (low+high)/2;
        if(mid - low + 1 < 5)
            selection_sort(arr, n);
        else
            normal_mergeSort(arr, low, mid, n);
        if(high - mid < 5)
            selection_sort(arr, n);
        else
            normal_mergeSort(arr, mid + 1, high, n);
        merge(arr, low, mid, high);
    }
}

void quickSort(int *arr, int low, int high){
    if(low<high){
        /* pi is partitioning index, arr[p] is now
           at right place */
        int pi = partition(arr, low, high);
        int pid1=fork();
        int pid2;
        if(pid1==0){
            quickSort(arr, low, pi - 1);
            _exit(1);
        }
        else{
            pid2=fork();
            if(pid2==0){
                quickSort(arr, pi + 1, high);
                _exit(1);
            }
            else{
                int status;
                waitpid(pid1, &status, 0);
                waitpid(pid2, &status, 0);
            }
        }
        return;
        // Separately sort elements before
        // partition and after partition
    }
}

struct arg{
    int l;
    int r;
    int* arr;
};

void *threaded_quickSort(void* a){
    //note that we are passing a struct to the threads for simplicity.
    struct arg *args = (struct arg*) a;

    int l = args->l;
    int r = args->r;
    int *arr = args->arr;
    if(l>r) return NULL;


    int ind=partition(arr,l,r);
    //sort left half array
    struct arg a1;
    a1.l = l;
    a1.r = ind-1;
    a1.arr = arr;
    pthread_t tid1;
    pthread_create(&tid1, NULL, threaded_quickSort, &a1);

    //sort right half array
    struct arg a2;
    a2.l = ind+1;
    a2.r = r;
    a2.arr = arr;
    pthread_t tid2;
    pthread_create(&tid2, NULL, threaded_quickSort, &a2);

    //wait for the two halves to get sorted
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    //merge(arr,l,r);
}

void runSorts(long long int n){

    struct timespec ts;

    //getting shared memory
    int *arr = shareMem(sizeof(int)*(n+1));
    for(int i=0;i<n;i++) scanf("%d", arr+i);

    int brr[n+1];
    for(int i=0;i<n;i++) brr[i] = arr[i];
    if(0) {
        printf("Running concurrent_quicksort for n = %lld\n", n);
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
        long double st = ts.tv_nsec/(1e9)+ts.tv_sec;

        //multiprocess mergesort
        quickSort(arr, 0, n-1);
        for(int i=0; i<n; i++){
            printf("%d ",arr[i]);
        }
        printf("\n");
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
        long double en = ts.tv_nsec/(1e9)+ts.tv_sec;
        printf("time = %Lf\n", en - st);
        long double t1 = en-st;

        pthread_t tid;
        struct arg a;
        a.l = 0;
        a.r = n-1;
        a.arr = brr;
        printf("Running threaded_concurrent_quicksort for n = %lld\n", n);
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
        st = ts.tv_nsec/(1e9)+ts.tv_sec;

        //multithreaded mergesort
        pthread_create(&tid, NULL, threaded_quickSort, &a);
        pthread_join(tid, NULL);
        for(int i=0; i<n; i++){
            printf("%d ",a.arr[i]);
        }
        printf("\n");
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
        en = ts.tv_nsec/(1e9)+ts.tv_sec;
        printf("time = %Lf\n", en - st);
        long double t2 = en-st;

        printf("Running normal_quicksort for n = %lld\n", n);
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
        st = ts.tv_nsec/(1e9)+ts.tv_sec;
    }

    // normal mergesort
    normal_mergeSort(brr, 0, n-1, n);
    for(int i=0; i<n; i++){
        printf("%d ",brr[i]);
    }
    printf("\n");
    //clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    //en = ts.tv_nsec/(1e9)+ts.tv_sec;
    //printf("time = %Lf\n", en - st);
    //long double t3 = en - st;

    //printf("normal_quicksort ran:\n\t[ %Lf ] times faster than concurrent_quicksort\n\t[ %Lf ] times faster than threaded_concurrent_quicksort\n\n\n", t1/t3, t2/t3);
    shmdt(arr);
    return;
}

int main(){

    long long int n;
    scanf("%lld", &n);
    runSorts(n);
    return 0;
}
