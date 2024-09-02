//SSOO-P3 23/24

#ifndef HEADER_FILE
#define HEADER_FILE


typedef struct element {
  int product_id;
  int op; //0: PURCHASE, 1: SALE      
  int units;
  int coste_de_compra;
  int precio_de_venta;
} element;

typedef struct queue {
  int head;
  int tail;
  int size;
  int capacity;
  element* data;
} queue;

queue* queue_init (int size);
int queue_destroy (queue* q);
int queue_put (queue* q, struct element* elem);
struct element * queue_get(queue* q);
int queue_empty (queue* q);
int queue_full(queue* q);

#endif
