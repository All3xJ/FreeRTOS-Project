#ifndef DICTIONARIES_H
#define DICTIONARIES_H

#include <stdio.h> 
#include <string.h>

#define MAX_SIZE 100 // Maximum number of elements in the map 

extern int size; // size of the maps

extern char keysStarting[MAX_SIZE][MAX_SIZE]; // keys for starting time of tasks
extern int valuesStarting[MAX_SIZE]; // value for starting time of tasks

extern char keysFinished[MAX_SIZE][MAX_SIZE]; // keys for elapsing time of tasks
extern int valuesFinished[MAX_SIZE]; // value for elapsing time of tasks

// Function to get the index of a key in the keys array 
int getIndexStarting(const char key[]);

// Function to insert a key-value pair into the map 
void insertStarting(const char key[], int value);

// Function to get the value of a key in the map 
int getStarting(const char key[]);

// Function to print the map 
void printMapStarting();

// Function to get the index of a key in the keys array 
int getIndexFinished(const char key[]);

// Function to insert a key-value pair into the map 
void insertFinished(const char key[], int value);

// Function to get the value of a key in the map 
int getFinished(const char key[]);

// Function to print the map 
void printMapFinished();

#endif // DICTIONARIES_H
