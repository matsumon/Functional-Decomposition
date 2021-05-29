#include <omp.h>
#include <stdlib.h>
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#ifndef NUMT                                                                     
#define NUMT    4                                                                
#endif 

int	NowYear;		// 2021 - 2026
int	NowMonth;		// 0 - 11

float	NowPrecip;		// inches of rain per month
float	NowTemp;		// temperature this month
float	NowHeight;		// grain height in inches
int	NowNumDeer;		// number of deer in the current population
float	NextPrecip;		// inches of rain per month
float	NextTemp;		// temperature this month
float	NextHeight;		// grain height in inches
int	NextNumDeer;		// number of deer in the current population
int MeteorStrike;

const float GRAIN_GROWS_PER_MONTH =		9.0;
const float ONE_DEER_EATS_PER_MONTH =		1.0;

const float AVG_PRECIP_PER_MONTH =		7.0;	// average
const float AMP_PRECIP_PER_MONTH =		6.0;	// plus or minus
const float RANDOM_PRECIP =			2.0;	// plus or minus noise

const float AVG_TEMP =				60.0;	// average
const float AMP_TEMP =				20.0;	// plus or minus
const float RANDOM_TEMP =			10.0;	// plus or minus noise

const float MIDTEMP =				40.0;
const float MIDPRECIP =				10.0;

unsigned int seed = 0;

omp_lock_t	Lock;
int		NumInThreadTeam;
int		NumAtBarrier;
int		NumGone;

float SQR( float x ){
    return x*x;
}

float Ranf( unsigned int *seedp,  float low, float high ){
    float r = (float) rand_r( seedp );              // 0 - RAND_MAX

    return(   low  +  r * ( high - low ) / (float)RAND_MAX   );
}


int Ranf( unsigned int *seedp, int ilow, int ihigh ){
    float low = (float)ilow;
    float high = (float)ihigh + 0.9999f;

    return (int)(  Ranf(seedp, low,high) );
}

void Grain(){
    float tempFactor = exp(   -SQR(  ( NowTemp - MIDTEMP ) / 10.  )   );
    float precipFactor = exp(   -SQR(  ( NowPrecip - MIDPRECIP ) / 10.  )   );
    NextHeight = NowHeight;
    NextHeight += tempFactor * precipFactor * GRAIN_GROWS_PER_MONTH;
    NextHeight -= (float)NowNumDeer * ONE_DEER_EATS_PER_MONTH;
    if( NextHeight < 0. ) NextHeight = 0.; 
}



void InitBarrier( int n ) {
        NumInThreadTeam = n;
        NumAtBarrier = 0;
	omp_init_lock( &Lock );
}


// have the calling thread wait here until all the other threads catch up:

void WaitBarrier( ) {
        omp_set_lock( &Lock );
        {
                NumAtBarrier++;
                if( NumAtBarrier == NumInThreadTeam )
                {
                        NumGone = 0;
                        NumAtBarrier = 0;
                        // let all other threads get back to what they were doing
			// before this one unlocks, knowing that they might immediately
			// call WaitBarrier( ) again:
                        while( NumGone != NumInThreadTeam-1 );
                        omp_unset_lock( &Lock );
                        return;
                }
        }
        omp_unset_lock( &Lock );

        while( NumAtBarrier != 0 );	// this waits for the nth thread to arrive

        #pragma omp atomic
        NumGone++;			// this flags how many threads have returned
        // printf("%d\n",omp_get_thread_num());
}

void Deer(){
    NextNumDeer = NowNumDeer;
    int carryingCapacity = (int)( NowHeight );
    if( NextNumDeer < carryingCapacity )
            NextNumDeer++;
    else
            if( NextNumDeer > carryingCapacity )
                    NextNumDeer--;

    if( NextNumDeer < 0 )
            NextNumDeer = 0;
}
void Watcher(){
       float ang = (  30.*(float)NowMonth + 15.  ) * ( M_PI / 180. );

        float temp = AVG_TEMP - AMP_TEMP * cos( ang );
        NowTemp = temp + Ranf( &seed, -RANDOM_TEMP, RANDOM_TEMP );

        float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin( ang );
        NowPrecip = precip + Ranf( &seed,  -RANDOM_PRECIP, RANDOM_PRECIP );
        if( NowPrecip < 0. )
            NowPrecip = 0.;
        float convertedTemp = (5./9.)*(NowTemp-32);
        if(MeteorStrike == 1){
            NowNumDeer = 0;
            NowHeight = 0;
        }
        float convertedHeight = NowHeight * 2.54;
        printf("%d/%d, %d, %5.5f, %5.5f, %5.5f,  %d\n",NowMonth,NowYear,NowNumDeer, convertedHeight, convertedTemp,NowPrecip,  MeteorStrike);
        MeteorStrike = 0;
}
void Meteor(){
    int temp = Ranf(&seed,0,100);
    // int temp = 53;
    if(temp == 53){
       MeteorStrike = 1; 
    }
}
int main() {
    // starting date and time:
    NowMonth =    0;
    NowYear  = 2021;

    // starting state (feel free to change this if you want):
    NowNumDeer = 1;
    NowHeight =  1.;
     MeteorStrike = 0;
    InitBarrier(NUMT);
    while (NowYear < 2027) {
        // printf("\nYear %d\n",NowYear);
        float nextHeight;
        omp_set_num_threads( NUMT );	// same as # of sections
        #pragma omp parallel sections
        {
            #pragma omp section
            {
                Deer( );
                WaitBarrier();
                NowNumDeer = NextNumDeer;
                // printf("ONE1\n");

                WaitBarrier();
                //  printf("ONE2\n");
                WaitBarrier();
                //  printf("ONE3\n");
            }
            #pragma omp section
            {
                Grain( );
                WaitBarrier();
                NowHeight = NextHeight;
                // printf("TWO1\n");
                WaitBarrier();
                // printf("TWO2\n");
                WaitBarrier();
                // printf("TWO3\n");
            }

            #pragma omp section
            {
                WaitBarrier();
                WaitBarrier();
                Watcher( );
                NowMonth++;
                if(NowMonth > 12){
                    NowMonth = 1;
                    NowYear++;
                }
                WaitBarrier();
                // printf("THREE1\n");
                // WaitBarrier();
                // printf("THREE2\n");
                // WaitBarrier();
                // printf("THREE3\n");
            }

            #pragma omp section
            {
                Meteor( );	// your own
                  WaitBarrier();
                // printf("ONE1\n");

                WaitBarrier();
                //  printf("ONE2\n");
                WaitBarrier();
            }
        }       // implied barrier -- all functions must return in order
            // to allow any of them to get past here
    }
}
