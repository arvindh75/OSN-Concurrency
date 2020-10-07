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

void swap(int* a, int* b) {
    int t = *a;
    *a = *b;
    *b = t;
}

int * shareMem(size_t size) {
    key_t mem_key = IPC_PRIVATE;
    int shm_id = shmget(mem_key, size, IPC_CREAT | 0666);
    return (int*)shmat(shm_id, NULL, 0);
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

void normal_mergeSort(int *arr, int low, int high, int n) {
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

void mergeSort(int *arr, int low, int high, int n) {
    int mid;
    if(low<high){
        mid = (low+high)/2;
        int pid1 = fork();
        int pid2;
        if(pid1 == 0) {
            if(mid - low + 1 < 5)
                selection_sort(arr, n);
            else
                normal_mergeSort(arr, low, mid, n);
            _exit(1);
        }
        else {
            pid2=fork();
            if(pid2 == 0) {
                if(high - mid < 5)
                    selection_sort(arr, n);
                else
                    normal_mergeSort(arr, mid + 1, high, n);
                _exit(1);
            }
            else {
                int status;
                waitpid(pid1, &status, 0);
                waitpid(pid2, &status, 0);
            }
        }
        merge(arr, low, mid, high);
        return;
    }
}

struct arg {
    int low;
    int high;
    int n;
    int* arr;
};

void *threaded_mergeSort(void* a) {
    struct arg *args = (struct arg*) a;
    int high = args->high;
    int low = args->low;
    int *arr = args->arr;
    int n = args->n;
    
    if(low >= high)
        return NULL;
    int mid = (low+high)/2;

    struct arg a1;
    a1.low = low;
    a1.high = mid;
    a1.arr = arr;
    pthread_t tid1;
    pthread_create(&tid1, NULL, threaded_mergeSort, &a1);

    struct arg a2;
    a2.low = mid+1;
    a2.high = high;
    a2.arr = arr;
    pthread_t tid2;
    pthread_create(&tid2, NULL, threaded_mergeSort, &a2);

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    merge(arr, low, mid, high);
}

void runSorts(long long int n) {
    struct timespec ts;
    int *arr = shareMem(sizeof(int)*(n+1));
    for(int i=0;i<n;i++) scanf("%d", arr+i);

    int brr[n+1];
    for(int i=0;i<n;i++) brr[i] = arr[i];
    printf("\nRunning Concurrent Mergesort for n = %lld\n", n);
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    long double st = ts.tv_nsec/(1e9)+ts.tv_sec;
    mergeSort(arr, 0, n-1, n);
    for(int i=0; i<n; i++){
        printf("%d ",arr[i]);
    }
    printf("\n");
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    long double en = ts.tv_nsec/(1e9)+ts.tv_sec;
    printf("Time = %Lf\n", en - st);
    long double t1 = en-st;

    pthread_t tid;
    struct arg a;
    a.low = 0;
    a.high = n-1;
    a.arr = brr;
    printf("\nRunning Threaded Concurrent Mergesort for n = %lld\n", n);
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    st = ts.tv_nsec/(1e9)+ts.tv_sec;
    pthread_create(&tid, NULL, threaded_mergeSort, &a);
    pthread_join(tid, NULL);
    for(int i=0; i<n; i++){
        printf("%d ",a.arr[i]);
    }
    printf("\n");
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    en = ts.tv_nsec/(1e9)+ts.tv_sec;
    printf("Time = %Lf\n", en - st);
    long double t2 = en-st;

    printf("\nRunning Normal Mergesort for n = %lld\n", n);
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    st = ts.tv_nsec/(1e9)+ts.tv_sec;
    normal_mergeSort(brr, 0, n-1, n);
    for(int i=0; i<n; i++){
        printf("%d ",brr[i]);
    }
    printf("\n");
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    en = ts.tv_nsec/(1e9)+ts.tv_sec;
    printf("Time = %Lf\n", en - st);
    long double t3 = en - st;

    printf("\nNormal Mergesort ran:\n\t[ %Lf ] times faster than Concurrent Mergesort\n\t[ %Lf ] times faster than Threaded Concurrent Mergesort\n\n\n", t1/t3, t2/t3);
    shmdt(arr);
    return;
}

int main() {
    long long int n;
    scanf("%lld", &n);
    runSorts(n);
    return 0;
}
