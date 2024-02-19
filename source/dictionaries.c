#include "dictionaries.h"

int sizeStarting = 0; // size for Starting
int sizeFinished = 0; // size for Finished

char keysStarting[MAX_SIZE][MAX_SIZE]; // Define the array
int valuesStarting[MAX_SIZE]; // Define the array

char keysFinished[MAX_SIZE][MAX_SIZE]; // Define the array
int valuesFinished[MAX_SIZE]; // Define the array

// Function to get the index of a key in the keys array for starting map
int getIndexStarting(const char key[]) 
{ 
    for (int i = 0; i < sizeStarting; i++) { 
        if (strcmp(keysStarting[i], key) == 0) { 
            return i; 
        } 
    } 
    return -1; // Key not found 
} 

// Function to get the index of a key in the keys array for finished map
int getIndexFinished(const char key[]) 
{ 
    for (int i = 0; i < sizeFinished; i++) { 
        if (strcmp(keysFinished[i], key) == 0) { 
            return i; 
        } 
    } 
    return -1; // Key not found 
}
  
// Function to insert a key-value pair into the starting map 
void insertStarting(const char key[], int value) 
{ 
    int index = getIndexStarting(key); 
    if (index == -1) { // Key not found 
        strcpy(keysStarting[sizeStarting], key); 
        valuesStarting[sizeStarting] = value; 
        sizeStarting++; 
    } 
    else { // Key found 
        valuesStarting[index] = value; 
    } 
} 

// Function to insert a key-value pair into the finished map 
void insertFinished(const char key[], int value) 
{ 
    int index = getIndexFinished(key); 
    if (index == -1) { // Key not found 
        strcpy(keysFinished[sizeFinished], key); 
        valuesFinished[sizeFinished] = value; 
        sizeFinished++; 
    } 
    else { // Key found 
        valuesFinished[index] = value; 
    } 
}  
  
// Function to get the value of a key in the starting map 
int getStarting(const char key[]) 
{ 
    int index = getIndexStarting(key); 
    if (index == -1) { // Key not found 
        return -1; 
    } 
    else { // Key found 
        return valuesStarting[index]; 
    } 
} 

// Function to get the value of a key in the finished map 
int getFinished(const char key[]) 
{ 
    int index = getIndexFinished(key); 
    if (index == -1) { // Key not found 
        return -1; 
    } 
    else { // Key found 
        return valuesFinished[index]; 
    } 
} 
  
// Function to print the starting map 
void printMapStarting() 
{ 
    for (int i = 0; i < sizeStarting; i++) { 
        printf("%s: %d\n", keysStarting[i], valuesStarting[i]); 
    } 
} 

// Function to print the finished map 
void printMapFinished() 
{ 
    for (int i = 0; i < sizeFinished; i++) { 
        printf("%s: %d\n", keysFinished[i], valuesFinished[i]); 
    } 
} 

// Function to get all keys of the finished map
void getAllKeysFinished(char *result[])
{
    for (int i = 0; i < sizeFinished; i++) {
        result[i] = keysFinished[i];
    }
}