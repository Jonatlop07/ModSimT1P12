#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include "lcgrand.h"

#define Q_LIMIT 100
#define BUSY 1
#define IDLE 0

// Model settings variables
int numDelaysRequired;
float meanInterarrival, meanService;

// Simulation clock
float simulationTime;

struct customer {
   float timeOfArrival;
   bool  hasPriority;
};

// State variables
int nextEventType, numCopsInQueue, numOrdCustsInQueue, numCustsInQueue, gasolinePumpStatus;
float timeOfNextEvent[ 4 ], timeOfLastEvent;
struct customer customersInQueue[ Q_LIMIT + 1 ];

// Statistical counters
int numCopsDelayed, numOrdCustsDelayed, numCustsDelayed;
float areaNumCustsInQueue, areaGasolinePumpStatus, totalOfCopsDelays, totalOfOrdCustsDelays;

int numEvents;

FILE *inFile, *outFile;

void initialize ( void );
void timing ( void );
void arriveCop ( void );
void arriveOrdinaryCustomer ( void );
void queueOverflow( void );
void depart ( void );
void report ( void );
void updateTimeAvgStats ( void );
float exponentialDistribution ( float mean );

int main ( void ) {

   inFile  = fopen( "p12.in",  "r" );
   outFile = fopen( "p12.out", "w" );

   numEvents = 3;

   fscanf( inFile, "%f %f %d", &meanInterarrival, &meanService, &numDelaysRequired );

   fprintf( outFile, "Single-server queueing system (Gasoline Station)\n\n" );
   fprintf( outFile, "Mean interarrival time%11.3f minutes\n\n", meanInterarrival );
   fprintf( outFile, "Mean service time%16.3f minutes\n\n", meanService );
   fprintf( outFile, "Number of customers%14d\n\n", numDelaysRequired );

   initialize();

   while ( numCustsDelayed < numDelaysRequired ) {
      timing();
      updateTimeAvgStats();

      switch ( nextEventType ) {

         case 1:
            arriveCop();
            break;
         case 2:
            arriveOrdinaryCustomer();
            break;
         case 3:
            depart();
            break;
      }
   }

   report();

   fclose( inFile );
   fclose( outFile );

   return 0;
}

void initialize ( void ) {

   simulationTime  = 0.0;

   gasolinePumpStatus = IDLE;
   numCustsInQueue    = 0.0;
   numCopsInQueue     = 0.0;
   numOrdCustsInQueue = 0.0;
   timeOfLastEvent    = 0.0;

   numCustsDelayed       = 0;
   numCopsDelayed        = 0;
   numOrdCustsDelayed    = 0;
   totalOfCopsDelays     = 0;
   totalOfOrdCustsDelays = 0;

   areaNumCustsInQueue    = 0.0;
   areaGasolinePumpStatus = 0.0;

   timeOfNextEvent[ 1 ] = 15.0;
   timeOfNextEvent[ 2 ] = 0.0;
   timeOfNextEvent[ 3 ] = simulationTime + exponentialDistribution( meanInterarrival );
}

void timing ( void ) {

   float minTimeOfNextEvent = 1.0e+29;

   nextEventType = 0;

   for ( int i = 1; i <= numEvents; ++i ) {
      if ( timeOfNextEvent[ i ] < minTimeOfNextEvent ) {

         minTimeOfNextEvent = timeOfNextEvent[ i ];
         nextEventType = i;
      }
   }

   if ( nextEventType == 0 ) {

      fprintf( outFile, "\nEvent list empty at time %f", simulationTime );
      exit( 1 );
   }

   simulationTime = minTimeOfNextEvent;
}

void queueOverflow ( void ) {

   fprintf( outFile, "\nOverflow of the array customersInQueue at");
   fprintf( outFile, "time %f", simulationTime );
   exit( 2 );
}

void arriveCop ( void ) {

   timeOfNextEvent[ 1 ] = simulationTime + 30.0;

   if ( gasolinePumpStatus == BUSY ) {

      ++numCustsInQueue, ++numCopsInQueue;

      struct customer cop = { simulationTime, true };

      if ( numCustsInQueue  > Q_LIMIT )
         queueOverflow();

      for ( int i = numCustsInQueue + 1; i > 1; --i )
         customersInQueue[ i ] = customersInQueue[ i - 1 ];

      customersInQueue[ 1 ] = cop;

   } else {

      float delay        = 0.0;
      totalOfCopsDelays += delay;
      ++numCustsDelayed, ++numCopsDelayed;

      gasolinePumpStatus   = BUSY;
      timeOfNextEvent[ 3 ] = simulationTime + exponentialDistribution( meanService );
   }
}

void arriveOrdinaryCustomer ( void ) {

   timeOfNextEvent[ 2 ] = simulationTime + exponentialDistribution( meanInterarrival );

   if ( gasolinePumpStatus == BUSY ) {

      ++numCustsInQueue, ++numOrdCustsInQueue;

      if ( numCustsInQueue  > Q_LIMIT )
         queueOverflow();

      struct customer ordinaryCustomer = { simulationTime, false };
      customersInQueue[ numCustsInQueue ] = ordinaryCustomer;

   } else {

      float delay            = 0.0;
      totalOfOrdCustsDelays += delay;
      ++numCustsDelayed, ++numOrdCustsDelayed;

      gasolinePumpStatus   = BUSY;
      timeOfNextEvent[ 3 ] = simulationTime + exponentialDistribution( meanService );
   }
}

void depart ( void ) {

   if ( numCustsInQueue <= 0 ) {

      gasolinePumpStatus = IDLE;
      timeOfNextEvent[ 3 ] = 1.0e+30;

   } else {

      struct customer custToDepart = customersInQueue[ 1 ];

      --numCustsInQueue;

      float delay    = simulationTime - custToDepart.timeOfArrival;

      ++numCustsDelayed;

      if ( custToDepart.hasPriority ) {

         --numCopsInQueue;

         totalOfCopsDelays += delay;

         ++numCopsDelayed;

      } else {

         --numOrdCustsInQueue;

         totalOfOrdCustsDelays += delay;

         ++numOrdCustsDelayed;
      }

      timeOfNextEvent[ 3 ] = simulationTime + exponentialDistribution( meanService );

      for ( int i = 1; i <= ( numOrdCustsInQueue + numCopsInQueue ); ++i )
         customersInQueue[ i ] = customersInQueue[ i + 1];
   }
}

void report ( void ) {

   fprintf( outFile, "\n\nAverage delay of cops in queue%25.3f minutes\n\n",
            totalOfCopsDelays / numCopsDelayed );
   fprintf( outFile, "Average delay of ordinary customers in queue%11.3f minutes\n\n",
            totalOfOrdCustsDelays / numOrdCustsDelayed );
   fprintf( outFile, "Average number of customers in queue%19.3f\n\n",
            areaNumCustsInQueue / simulationTime );
   fprintf( outFile, "Gasoline pump utilization%30.3f\n\n",
            areaGasolinePumpStatus / simulationTime );
}

void updateTimeAvgStats ( void ) {

   float timeSinceLastEvent = simulationTime - timeOfLastEvent;
   timeOfLastEvent = simulationTime;

   areaNumCustsInQueue    += numCustsInQueue * timeSinceLastEvent;
   areaGasolinePumpStatus += gasolinePumpStatus * timeSinceLastEvent;
}

float exponentialDistribution ( float mean ) {
   return -mean * log( lcgrand( 1 ) );
}
