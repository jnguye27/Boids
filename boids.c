/* 
Program: Boids
Creator: Jessica Nguyen
Date: 2023-11-13
Purpose: To create a swarm of pixelated birds that fly to the center of the terminal.

Boids using ASCII graphics
- Original NCurses code from "Game Programming in C with the Ncurses Library"
   https://www.viget.com/articles/game-programming-in-c-with-the-ncurses-library/
	and from "NCURSES Programming HOWTO"
	http://tldp.org/HOWTO/NCURSES-Programming-HOWTO/
- Boids algorithms from "Boids Pseudocode:
	http://www.kfish.org/boids/pseudocode.html
*/

#include<time.h>   // timespec()
#include<omp.h>    // openmp functions
#include<stdio.h>
#include<stdlib.h>
#include<math.h>   // fabs()
#ifndef NOGRAPHICS
#include<unistd.h>
#include<ncurses.h>
#endif

#define DELAY 50000

// population size, number of boids
#define POPSIZE 50

// maximum screen size, both height and width
#define SCREENSIZE 100

// boid location (x,y,z) and velocity (vx,vy,vz) in boidArray[][]
#define BX 0
#define BY 1
#define BZ 2
#define VX 3
#define VY 4
#define VZ 5

#ifndef NOGRAPHICS
	// maximum screen dimensions
   int max_y = 0, max_x = 0;
#endif

// location and velocity of boids
float boidArray[POPSIZE][6];

// change in velocity is stored for each boid (x,y,z)
// this is applied after all changes are calculated for each boid
float boidUpdate[POPSIZE][3];

// rule 1: boids try to fly towards the centre of mass of neighbouring boids
float rule1(int j, int position) {
   // declaring variables
   int i;
   float pc;

   // initializing variables
   pc = 0;

   // add up all boid positions (excluding boid j)
   for (i=0; i<POPSIZE; i++) {
      if (i!=j) {
         pc += boidArray[i][position];
      }
   }

   // calculate the perceived center
   pc /= (POPSIZE-1);

   // return the first vector offset of boid j (moving it 1% of the way towards the centre)
   return ((pc - boidArray[j][position]) / 100);
}

// rule 2: boids try to keep a small distance away from other objects (including other boids)
float rule2(int j, int position) {
   // declaring variables
   int i;
   float c;

   // initializing variables
   c = 0;

   // calculate the distance and move away from each other if needed
   for (i=0; i<POPSIZE; i++) {
      if (i!=j) {
         if (fabs(boidArray[i][position]-boidArray[j][position]) < 1) {
            c -= (boidArray[i][position]-boidArray[j][position]);
         }
      }
   }

   // return the second vector offset of boid j
   return c;
}

// rule 3. boids try to match velocity with near boids
float rule3(int j, int velocity) {
   // declaring variables
   int i;
   float pv;

   // initializing variables
   pv = 0;

   // add up all boid velocities (excluding boid j)
   for (i=0; i<POPSIZE; i++) {
      if (i!=j) {
         pv += boidArray[i][velocity];
      }
   }
   
   // calculate the perceived velocity
   pv /= (POPSIZE-1);

   // return the third vector offset of boid j
   return ((pv - boidArray[j][velocity]) / 8);
}

// converts the recorded time into milliseconds
long int millis() {
   // declaring variables
   struct timespec now;
   long int now_1;

   // save the time
   timespec_get(&now, TIME_UTC);

   // convert the time into milliseconds
   now_1 = ((long int) now.tv_sec) * 1000 + ((long int) now.tv_nsec) / 1000000;

   return now_1;
}

void initBoids() {
   int i;

	// calculate initial random locations for each boid, scaled based on the screen size
   for(i=0; i<POPSIZE; i++) {
      boidArray[i][BX] = (float) (random() % SCREENSIZE); 
      boidArray[i][BY] = (float) (random() % SCREENSIZE); 
      boidArray[i][BZ] = (float) (random() % SCREENSIZE); 
      boidArray[i][VX] = 0.0; 
      boidArray[i][VY] = 0.0; 
      boidArray[i][VZ] = 0.0; 
   }
}

#ifndef NOGRAPHICS
   int drawBoids() {
      int c, i;
      float multx, multy;

      // update screen maximum size
      getmaxyx(stdscr, max_y, max_x);

      // used to scale position of boids based on screen size
      multx = (float)max_x / SCREENSIZE;
      multy = (float)max_y / SCREENSIZE;

      clear();

      // display boids
      for (i=0; i<POPSIZE; i++) {
         mvprintw((int)(boidArray[i][BX]*multy), (int)(boidArray[i][BY]*multx), "o");
      }

      refresh();

      usleep(DELAY);

      // read keyboard and exit if 'q' pressed
      c = getch();
      if (c == 'q') return(1);
      else return(0);
   }
#endif

// move the flock between two points
// you can optimize this function if you wish, or you can replace it and move it's
// functionality into another function
void moveFlock() {
   int i;
   static int count = 0;
   static int sign = 1;
   float px, py, pz;	// (x,y,z) point which the flock moves towards

	// pull flock towards two points as the program runs
	// every 200 iterations change point that flock is pulled towards
   if (count % 200 == 0) {
      sign = sign * -1;
   } 

   if (sign == 1) {
	// move flock towards position (40,40,40)
      px = 40.0;
      py = 40.0;
      pz = 40.0;
   } else {
	// move flock towards position (60,60,60)
      px = 60.0;
      py = 60.0;
      pz = 60.0;
   }

	// add offset (px,py,pz) to each boid location (x,y,z) in order to pull it
	// towards the current target point
   for(i=0; i<POPSIZE; i++) {
      boidUpdate[i][BX] += (px - boidArray[i][BX])/200.0;
      boidUpdate[i][BY] += (py - boidArray[i][BY])/200.0;
      boidUpdate[i][BZ] += (pz - boidArray[i][BZ])/200.0;
   }
   count++;
}

void moveBoids() {
   int i;

   #pragma omp parallel for num_threads(5)
   for (i=0; i<POPSIZE; i++) {
      // boidUpdate[i][BX/BY/BZ] represents v1,v2,v3
      // rule 1: boids try to fly towards the centre of mass of neighbouring boids
      boidUpdate[i][BX] = rule1(i, BX);
      boidUpdate[i][BY] = rule1(i, BY);
      boidUpdate[i][BZ] = rule1(i, BZ);
      
      // rule 2: boids try to keep a small distance away from other objects (including other boids)
      boidUpdate[i][BX] += rule2(i, BX);
      boidUpdate[i][BY] += rule2(i, BY);
      boidUpdate[i][BZ] += rule2(i, BZ);

      // rule 3. boids try to match velocity with near boids
      boidUpdate[i][BX] += rule3(i, VX);
      boidUpdate[i][BY] += rule3(i, VY);
      boidUpdate[i][BZ] += rule3(i, VZ);
   }
   
   // rule 4. tendency towards a particular location, moves the boid 1% of the way towards the goal at each step
   moveFlock();

	// move boids by calculating updated velocity and new position
	// you can optimize this code if you wish and you can add omp directives here
   #pragma omp parallel for num_threads(5)
   for (i=0; i<POPSIZE; i++) {
	   // update velocity for each boid using value stored in boidUpdate[][]
      boidArray[i][VX] += boidUpdate[i][BX];
      boidArray[i][VY] += boidUpdate[i][BY];
      boidArray[i][VZ] += boidUpdate[i][BZ];

	   // update position for each boid using newly updated velocity
      boidArray[i][BX] += boidArray[i][VX];
      boidArray[i][BY] += boidArray[i][VY];
      boidArray[i][BZ] += boidArray[i][VZ];
   }
}

int main(int argc, char *argv[]) {
   int i, count;
   long int  t1, t2, time;

   #ifndef NOGRAPHICS
      // initialize curses
      initscr();
      noecho();
      cbreak();
      timeout(0);
      curs_set(FALSE);
      
      // global var `stdscr` is created by the call to `initscr()`
      getmaxyx(stdscr, max_y, max_x);
   #endif

   // place boids in initial position
   initBoids();

   // draw and move boids using ncurses 
   // do not calculate timing in this loop, ncurses will reduce performance
   #ifndef NOGRAPHICS
      while(1) {
         if (drawBoids() == 1) break;
         moveBoids();
      }
   #endif

   // calculate movement of boids but do not use ncurses to draw
   // this is used for timing the parallel implementation of the program
   #ifdef NOGRAPHICS
      count = 1000;

      // read number of iterations from the command line
      if (argc == 2)
         sscanf(argv[1], "%d", &count);
      printf("Number of iterations %d\n", count);

      // start timing here
      t1 = millis();

      #pragma omp parallel for num_threads(5)
      for(i=0; i<count; i++) {
         moveBoids();
      }
      
      // end timing here
      t2 = millis();

      // print timing results
      time = t2 - t1;
      printf("Execution Time: %ld ms\n", time);
   #endif

   #ifndef NOGRAPHICS
      // shut down curses
      endwin();
   #endif
}