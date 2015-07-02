#include <stdio.h>

void print_array(sample_t * ar, int n){
	int i;
	for(i=0;i<n;i++){
		printf("%3d\t",ar[i]);
	}
	printf("\n");
}
void print_array_double(double * ar, int n){
	int i;
	for(i=0;i<n;i++){
		printf("%3.5f ",ar[i]);
	}
	printf("\n");
}