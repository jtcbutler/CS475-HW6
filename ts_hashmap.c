//#include <bits/pthreadtypes.h>
#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ts_hashmap.h"



/**
 * Creates a new thread-safe hashmap. 
 *
 * @param capacity initial capacity of the hashmap.
 * @return a pointer to a new thread-safe hashmap.
 */
ts_hashmap_t *initmap(int capacity) {
	ts_hashmap_t *map = malloc(sizeof(*map));
	map->table = malloc(sizeof(ts_entry_t*) * capacity);
	map->capacity = capacity;
	map->numOps = 0;
	map->size = 0;

	map->tableLocks = malloc(sizeof(pthread_mutex_t) * capacity);
	pthread_mutex_init(&map->numOpsLock, NULL);
	pthread_mutex_init(&map->sizeLock, NULL);

	for(int i = 0; i < capacity; i++) {
		map->table[i] = NULL;
		pthread_mutex_init(&map->tableLocks[i], NULL);
	}

	return map;
}

/**
 * Obtains the value associated with the given key.
 * @param map a pointer to the map
 * @param key a key to search
 * @return the value associated with the given key, or INT_MAX if key not found
 */
int get(ts_hashmap_t *map, int key) {
	int index = key % map->capacity;
	int value = INT_MAX;

	pthread_mutex_lock(&map->tableLocks[index]);
	ts_entry_t *entry = map->table[index];
	while(entry != NULL){
		if(entry->key == key){
			value = entry->value;
			break;
		}
		else{
			entry = entry->next;
		}
	}
	pthread_mutex_unlock(&map->tableLocks[index]);

	pthread_mutex_lock(&map->numOpsLock);
	map->numOps++;
	pthread_mutex_unlock(&map->numOpsLock);

	return value;
}

/**
 * Associates a value associated with a given key.
 * @param map a pointer to the map
 * @param key a key
 * @param value a value
 * @return old associated value, or INT_MAX if the key was new
 */
int put(ts_hashmap_t *map, int key, int value) {
	int index = key % map->capacity;
	int old_value = INT_MAX;

	pthread_mutex_lock(&map->tableLocks[index]);
	ts_entry_t *entry = map->table[index];
	while(entry != NULL){
		if(entry->key == key){
			old_value = entry->value;
			entry->value = value;
			break;
		}
		else{
			entry = entry->next;
		}
	}
	if(old_value == INT_MAX){
		ts_entry_t *entry = malloc(sizeof(*entry));	
		entry->key = key;
		entry->value = value;
		entry->next = map->table[index];
		map->table[index] = entry;
	}
	pthread_mutex_unlock(&map->tableLocks[index]);

	if(old_value != INT_MAX){
		pthread_mutex_lock(&map->sizeLock);
		map->size++;
		pthread_mutex_unlock(&map->sizeLock);
	}

	pthread_mutex_lock(&map->numOpsLock);
	map->numOps++;
	pthread_mutex_unlock(&map->numOpsLock);

	return old_value;
}

/**
 * Removes an entry in the map
 * @param map a pointer to the map
 * @param key a key to search
 * @return the value associated with the given key, or INT_MAX if key not found
 */
int del(ts_hashmap_t *map, int key) {
	int index = key % map->capacity;
	int value = INT_MAX;

	pthread_mutex_lock(&map->tableLocks[index]);

	ts_entry_t *previous_entry = NULL;
	ts_entry_t *entry = map->table[index];
	while(entry != NULL){
		if(entry->key == key){
			if(previous_entry != NULL){
				previous_entry->next = entry->next;
			}
			else{
				map->table[index] = entry->next;
			}
			value = entry->value;
			free(entry);
			break;
		}
		else{
			previous_entry = entry;
			entry = entry->next;
		}
	}
	pthread_mutex_unlock(&map->tableLocks[index]);

	if(value != INT_MAX){
		pthread_mutex_lock(&map->sizeLock);
		map->size--;
		pthread_mutex_unlock(&map->sizeLock);
	}

	pthread_mutex_lock(&map->numOpsLock);
	map->numOps++;
	pthread_mutex_unlock(&map->numOpsLock);

	return value;
}


/**
 * Prints the contents of the map (given)
 */
void printmap(ts_hashmap_t *map) {
	for (int i = 0; i < map->capacity; i++) {
		printf("[%d] -> ", i);
		ts_entry_t *entry = map->table[i];
		while (entry != NULL) {
			printf("(%d,%d)", entry->key, entry->value);
			if (entry->next != NULL)
				printf(" -> ");
			entry = entry->next;
		}
		printf("\n");
	}
}

/**
 * Free up the space allocated for hashmap
 * @param map a pointer to the map
 */
void freeMap(ts_hashmap_t *map) {
	for(int i = 0; i < map->capacity; i++)
	{
		ts_entry_t *entry = map->table[i];				
		while(entry != NULL){
			ts_entry_t *temp = entry;
			entry = entry->next;
			free(temp);
		}
		pthread_mutex_destroy(&map->tableLocks[i]);
	}

	free(map->table);
	free(map->tableLocks);
	pthread_mutex_destroy(&map->sizeLock);
	pthread_mutex_destroy(&map->numOpsLock);

	free(map);
}
