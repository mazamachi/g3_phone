#ifndef G123_PHONE
#define G123_PHONE 
#include <assert.h>
#include <complex.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define SAMPLING_FREQEUENCY 44100
typedef short sample_t;

void die(char * s) ;
ssize_t fread_n(FILE fd, ssize_t n, void * buf);
ssize_t write_n(int fd, ssize_t n, void * buf);
void sample_to_complex(sample_t * s, 
					complex double * X, 
					long n);
void complex_to_sample(complex double * X, 
					 sample_t * s, 
					 long n);
void fft_r(complex double * x, 
		 complex double * y, 
		 long n, 
		 complex double w);
void fft(complex double * x, 
	 complex double * y, 
	 long n);
void ifft(complex double * y, 
		complex double * x, 
		long n);
int pow2check(long N);
#endif