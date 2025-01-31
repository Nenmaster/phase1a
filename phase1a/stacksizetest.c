#include <stdio.h>

static char *p1;
static char *p2;

int add(int* a, int* b){
    char size;
    p1 = &size;

    int sum = *a + *b;

    return sum;
}


int main(){
    int a,b;
    int stacksize;

    stacksize = malloc(sizeof(USLOSS_MIN_STACK));
    char size2;
    p2 = &size2;
    
    unsigned char total = *p2 - *p1; 
    printf("sum of total is %d",total);
    add(&a,&b);

    return 0;

}
