//SSOO-P3 23/24

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>
#include <pthread.h>
#include "queue.h"
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

int profit = 0;
int product_stock [5] = {0};

int num_max_operaciones;

//int num_purchases = 0;
//int num_sales = 0;

typedef struct {
  element* a_operaciones;
  int start_index;
  int end_index;
} producer_arg;

pthread_mutex_t mutex;
pthread_mutex_t mutex_calculos;
pthread_cond_t no_lleno;
pthread_cond_t no_vacio;

queue* circular_buffer;

//Función auxiliar que almacena las operaciones en el array de operaciones
void almacenar_operaciones(int product_id, int coste_de_compra, int precio_de_venta, element* array_operaciones, char* buff, int i, int index){
  char tipo_operacion[16];
  int unidades;
  array_operaciones[i].product_id = product_id;
  array_operaciones[i].coste_de_compra = coste_de_compra;
  array_operaciones[i].precio_de_venta = precio_de_venta;
  //Tipo de operación
  index++;
  sscanf(&buff[index], "%s", tipo_operacion);
  if (strcmp(tipo_operacion, "PURCHASE") == 0){
    array_operaciones[i].op = 0;
    //num_purchases++;
  } else if (strcmp(tipo_operacion, "SALE") == 0){
    array_operaciones[i].op = 1;
    //num_sales++;
  } else {
    printf("ERROR. Tipo de operación inválida\n");
    exit(-1);
  }
  //Unidades
  index += strlen(tipo_operacion) + 1;
  sscanf(&buff[index], "%d", &unidades);
  array_operaciones[i].units = atoi(&buff[index]);
}

void calculos(element *ele){
  pthread_mutex_lock(&mutex_calculos);
  //PURCHASE
  if (ele->op == 0){
    if (ele->product_id == 1){
      profit = profit - (ele->coste_de_compra * ele->units);
      product_stock[0] += ele->units;
    } else if (ele->product_id == 2){
      profit = profit - (ele->coste_de_compra * ele->units);
      product_stock[1] += ele->units;
    } else if (ele->product_id == 3){
      profit = profit - (ele->coste_de_compra * ele->units);
      product_stock[2] += ele->units;
    } else if (ele->product_id == 4){
      profit = profit - (ele->coste_de_compra * ele->units);
      product_stock[3] += ele->units;
    } else if (ele->product_id == 5){
      profit = profit - (ele->coste_de_compra * ele->units);
      product_stock[4] += ele->units;
    }
  //SALE
  } else {
    if (ele->product_id == 1){
      profit = profit + (ele->precio_de_venta * ele->units);
      product_stock[0] -= ele->units;
    } else if (ele->product_id == 2){
      profit = profit + (ele->precio_de_venta * ele->units);
      product_stock[1] -= ele->units;
    } else if (ele->product_id == 3){
      profit = profit + (ele->precio_de_venta * ele->units);
      product_stock[2] -= ele->units;
    } else if (ele->product_id == 4){
      profit = profit + (ele->precio_de_venta * ele->units);
      product_stock[3] -= ele->units;
    } else if (ele->product_id == 5){
      profit = profit + (ele->precio_de_venta * ele->units);
      product_stock[4] -= ele->units;
    }
  }
  pthread_mutex_unlock(&mutex_calculos);
}

void* productor(void* arg){
  producer_arg* p_arg = (producer_arg*) arg;
  element* array_operaciones = p_arg->a_operaciones;
  int start_index = p_arg->start_index;
  int end_index = p_arg->end_index;
  int i;

  for (i = start_index; i < end_index; i++){
    pthread_mutex_lock(&mutex);
    while(queue_full(circular_buffer) == 1){
      pthread_cond_wait(&no_lleno, &mutex);
    }
    element* ele = (element*)malloc(sizeof(element));
    ele->product_id = array_operaciones[i].product_id;
    ele->coste_de_compra = array_operaciones[i].coste_de_compra;
    ele->precio_de_venta = array_operaciones[i].precio_de_venta;
    ele->op = array_operaciones[i].op;
    ele->units = array_operaciones[i].units;
    queue_put(circular_buffer, ele);
    pthread_cond_signal(&no_vacio);
    pthread_mutex_unlock(&mutex);
  }
  pthread_exit(0);
  return NULL;
}

void* consumidor(void* arg){
  int num_op_x_consumidor = *((int*)arg);
  int i;
  for (i = 0; i < num_op_x_consumidor; i++){
    pthread_mutex_lock(&mutex);
    while(queue_empty(circular_buffer) == 1){
      pthread_cond_wait(&no_vacio, &mutex);
    }
    element* ele = queue_get(circular_buffer);
    calculos(ele);
    free(ele);
    pthread_cond_signal(&no_lleno);
    pthread_mutex_unlock(&mutex);
  }
  pthread_exit(0);
  return NULL;
}

int main (int argc, const char * argv[]){

  //argv[0] = ./store_manager
  //argv[1] = file_name
  //argv[2] = num_producers
  //argv[3] = num_consumers
  //argv[4] = buff_size

  //Nº de argumentos introducido
  if (argc != 5){
    printf("ERROR. Nº de argumentos inválido\n");
    exit(-1);
  }

  //Argumentos de entrada
  const char* file_name = argv[1];
  int num_producers = atoi(argv[2]);
  int num_consumers = atoi(argv[3]);
  int buff_size = atoi(argv[4]);

  //Control de errores
  if (num_producers <= 0 || num_consumers <= 0 || buff_size <= 0){
    printf("ERROR. Argumentos inválidos\n");
    exit(-1);
  }

  //Comprobación para ver si el nº de operaciones del fichero es >= que la 1ª línea. Para ello: 
  int fd, i = 0, num_operaciones;

  //Creamos un array dinámico
  char* buffer = (char*)malloc(2048 * sizeof(char));

  if (buffer == NULL) {
    printf("ERROR al asignar memoria.\n");
    exit(-1);
  }

  //Abrimos el fichero
  if ((fd = open(file_name, O_RDONLY)) == -1){
    printf("ERROR al abrir el archivo.\n");
    free(buffer);
    exit(-1);
  }

  //Leemos la 1ª línea y guardamos su valor
  while (read(fd, buffer + i, 1) > 0){
    if (buffer[i] == '\n'){
      break;
    }
    i++;
  }

  num_max_operaciones = atoi(buffer);

  //##########################################################
  //printf("Numero max de operaciones: %d\n", num_max_operaciones);
  //##########################################################
  
  //Guardamos en el buffer las operaciones del fichero y contamos su cantidad 
  i = 0;

  while (read(fd, buffer + i, 1) > 0){
    if (buffer[i] == '\n'){
      num_operaciones++;
    }
    i++;
  }

  //##########################################################
  //printf("%s\n", buffer);
  //printf("Numero de operaciones: %d\n", num_operaciones);
  //##########################################################

  //Realizamos la comparación 
  if (num_operaciones < num_max_operaciones){
    printf("ERROR. num_operaciones < num_max_operaciones\n");
    free(buffer);
    close(fd);
    exit(-1);
  }

  //Liberamos la memoria del buffer y cerramos el descriptor de fichero
  free(buffer);
  close(fd);

  //Abrimos nuevamente el fichero, esta vez mediante fopen
  FILE* file = fopen(file_name, "r");

  if (file == NULL){
    printf("ERROR al abrir el archivo.\n");
    exit(-1);
  }

  //Creamos un array dinámico de tipo element de tamaño num_max_operaciones 
  element* array_operaciones = (element*)malloc(num_max_operaciones * sizeof(element));

  char buff[32];
  int index = 0;

  //Lee la 1ª línea del fichero "no nos interesa"
  fgets(buff, 32, file);

  //Bucle que recorre el fichero con el nº de operaciones de la 1ª línea
  for (i = 0; i < num_max_operaciones; i++){
    //Lee una línea de máximo 32 caracteres y la almacena en el buffer
    fgets(buff, 32, file);
    //Se inicializa cada campo de la estructura a 0
    array_operaciones[i].product_id = 0;
    array_operaciones[i].op = 0;
    array_operaciones[i].units = 0;
    array_operaciones[i].coste_de_compra = 0;
    array_operaciones[i].precio_de_venta = 0;
    //Id_producto
    if(buff[index] == '1'){
        almacenar_operaciones(1, 2, 3, array_operaciones, buff, i, index);
    } 
    else if (buff[index] == '2'){
        almacenar_operaciones(2, 5, 10, array_operaciones, buff, i, index);
    }
    else if (buff[index] == '3'){
        almacenar_operaciones(3, 15, 20, array_operaciones, buff, i, index);
    }
    else if (buff[index] == '4'){
        almacenar_operaciones(4, 25, 40, array_operaciones, buff, i, index);
    }
    else if (buff[index] == '5'){
        almacenar_operaciones(5, 100, 125, array_operaciones, buff, i, index);
    }
  }

  //##########################################################
  //for (i = 0; i < num_max_operaciones; i++) {
  //    printf("Operacion %d:\n", i+1);
  //    printf("  product_id: %d\n", array_operaciones[i].product_id);
  //    printf("  coste_de_compra: %d\n", array_operaciones[i].coste_de_compra);
  //    printf("  precio_de_venta: %d\n", array_operaciones[i].precio_de_venta);
  //    printf("  op: %d\n", array_operaciones[i].op);
  //    printf("  units: %d\n", array_operaciones[i].units);
  //}
  //##########################################################

  //##########################################################
  //printf("PURCHASES: %d\n", num_purchases);
  //printf("SALES: %d\n", num_sales);
  //##########################################################

  //Cerramos el fichero
  fclose(file);

  circular_buffer = queue_init(buff_size);

  pthread_t ths[num_producers + num_consumers];
  pthread_mutex_init(&mutex, NULL);
  pthread_mutex_init(&mutex_calculos, NULL);
  pthread_cond_init(&no_lleno, NULL);
  pthread_cond_init(&no_vacio, NULL);

  //Creación de hilos productores. Se hace un reparto de las operaciones entre ellos 
  producer_arg p_arg[num_producers];
  int num_op_x_productor = num_max_operaciones / num_producers;
  int num_op_restantes_productor = num_max_operaciones % num_producers;

  for (i = 0; i < num_producers; i++){
    p_arg[i].a_operaciones = array_operaciones;
    p_arg[i].start_index = i * num_op_x_productor;
    if (i == num_producers - 1){
      p_arg[i].end_index = (i + 1) * num_op_x_productor + num_op_restantes_productor;
    } else {
      p_arg[i].end_index = (i + 1) * num_op_x_productor;
    }
    if (pthread_create(&ths[i], NULL, productor, (void*)&p_arg[i]) != 0){
      printf("ERROR en pthread_create productor\n");
      exit(-1);
    }
  }

  //Creación de hilos consumidores
  int num_op_x_consumidor = num_max_operaciones / num_consumers;
  int num_op_restantes_consumidor = num_max_operaciones % num_consumers;
  for(i = 0; i < num_consumers; i++){
    int* num_op = (int*)malloc(sizeof(int));
    *num_op = num_op_x_consumidor;
    if (i == num_consumers - 1){
      *num_op += num_op_restantes_consumidor;
    }
    if (pthread_create(&ths[num_producers + i], NULL, consumidor, (void*)num_op) != 0){
      printf("ERROR en pthread_create consumidor\n");
      exit(-1);
    }
  }

  //Espera a que los hilos terminen
  for (i = 0; i < num_producers + num_consumers; i++){
    if (pthread_join(ths[i], NULL) != 0){
      printf("ERROR en pthread_join\n");
      exit(-1);
    }
  }

  //Destrucción de los recursos
  pthread_mutex_destroy(&mutex);
  pthread_mutex_destroy(&mutex_calculos);
  pthread_cond_destroy(&no_lleno);
  pthread_cond_destroy(&no_vacio);
  queue_destroy(circular_buffer);

  free(array_operaciones);

  // Output
  printf("Total: %d euros\n", profit);
  printf("Stock:\n");
  printf("  Product 1: %d\n", product_stock[0]);
  printf("  Product 2: %d\n", product_stock[1]);
  printf("  Product 3: %d\n", product_stock[2]);
  printf("  Product 4: %d\n", product_stock[3]);
  printf("  Product 5: %d\n", product_stock[4]);

  return 0;
}
