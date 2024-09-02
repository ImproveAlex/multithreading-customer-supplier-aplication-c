//SSOO-P3 23/24

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "queue.h"

//Función que crea la cola y reserva el tamaño especificado como parámetro
queue* queue_init(int num_elements){
  queue* q = (queue*)malloc(sizeof(queue));
  q->head = 0;
  q->tail = 0;
  q->size = 0;
  q->capacity = num_elements;
  q->data = (element*)malloc(num_elements * sizeof(element));
  return q;
}

//Función que elimina la cola y libera todos los recursos asignados
int queue_destroy(queue* q){
  free(q->data);
  free(q);
  return 0;
}

//Función que inserta elementos en la cola si hay espacio disponible. Si no hay espacio disponible, debe esperar hasta que pueda ser realizada la inserción.
int queue_put(queue* q, struct element* ele){
  //if (q->size == q->capacity){
    //Esperar
  //}
  q->data[q->tail] = *ele;
  q->tail = (q->tail + 1) % q->capacity;
  q->size++;
  return 0;
}

//Función que extrae elementos de la cola si no está vacía. Si está vacía, debe esperar a que haya un elemento disponible.
struct element* queue_get(queue* q){
  //if (q->size == 0){
    //Esperar
  //}
  struct element* element = malloc(sizeof(element));
  *element = q->data[q->head];
  q->head = (q->head + 1) % q->capacity;
  q->size--;
  return element;
}

//Función que consulta el estado de la cola y determina si está vacía "1" o no "0"
int queue_empty(queue* q){
  if (q->size == 0){
    return 1;
  }
  return 0;
}

//Función que consulta el estado de la cola y determina si está llena "1" o no "0"
int queue_full(queue* q){
  if (q->size == q->capacity){
    return 1;
  }
  return 0;
}