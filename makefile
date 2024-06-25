all: boids

boids: boids.c
	gcc boids.c -o boids -fopenmp -lncurses -lm 