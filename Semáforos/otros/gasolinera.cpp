#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

const size_t num_sur_gasolina = 3;
const size_t num_sur_gasoil = 2;
const size_t num_gasolina = 6;
const size_t num_gasoil = 4;

size_t num_surtidores = 0;
Semaphore gasolina[num_sur_gasolina] = {1,1,1};
Semaphore gasoil[num_sur_gasoil] = {1,1};

pthread_mutex_t candado;


//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

// Función que simula la acción de llenar el depósito del coche

void llenar_deposito(size_t num_coche, size_t tipo, int surtidor){
  chrono::milliseconds duracion(aleatorio<200,600>());
  string tipo_surtidor;
  switch (tipo) {
    case 0: tipo_surtidor = "Gasolina: "; break;
    case 1: tipo_surtidor = "\t\t\t\t\t\t\t\t\t\t\tGasoil: "; break;
  }
  cout << "\t\t\t\t\t\t\t\tSurtidores ocupados: " << num_surtidores << endl;
  cout << tipo_surtidor << "coche " << num_coche << " empieza a llenar su deposito en surtidor " << surtidor << endl;
  this_thread::sleep_for(duracion);
  cout << tipo_surtidor << "coche " << num_coche << " ha llenado su deposito (" << duracion.count() << ") en surtidor " << surtidor << endl;
}

// Función que ejecuta la hebra "gasolina"

void funcion_hebra_gasolina(size_t num_coche){
  while (true){
    int surtidor = aleatorio<0,num_sur_gasolina-1>();
    sem_wait(gasolina[surtidor]);
    pthread_mutex_lock(&candado);
    cout << "Surtidor gasolina numero " << surtidor << " ocupado." << endl;
    num_surtidores++;
    pthread_mutex_unlock(&candado);
    llenar_deposito(num_coche, 0, surtidor);
    pthread_mutex_lock(&candado);
    num_surtidores--;
    chrono::milliseconds retardo(aleatorio<20,200>());
    cout << "Surtidor gasolina numero " << surtidor << " libre." << endl;
    pthread_mutex_unlock(&candado);
    sem_signal(gasolina[surtidor]);
    this_thread::sleep_for(retardo);
  }
}

//Función que ejecuta la hebra "gasoil"

void funcion_hebra_gasoil(size_t num_coche){
  while (true){
    int surtidor = aleatorio<0,num_sur_gasoil-1>();
    sem_wait(gasoil[surtidor]);
    pthread_mutex_lock(&candado);
    cout << "\t\t\t\t\t\t\t\t\t\t\tSurtidor gasoil numero " << surtidor << " ocupado." << endl;
    num_surtidores++;
    pthread_mutex_unlock(&candado);
    llenar_deposito(num_coche, 1, surtidor);
    pthread_mutex_lock(&candado);
    num_surtidores--;
    chrono::milliseconds retardo(aleatorio<200,2000>());
    cout << "\t\t\t\t\t\t\t\t\t\t\tSurtidor gasoil numero " << surtidor << " libre." << endl;
    pthread_mutex_unlock(&candado);
    sem_signal(gasoil[surtidor]);
    this_thread::sleep_for(retardo);
  }
}

//----------------------------------------------------------------------

int main()
{
  thread coche_gasolina[num_gasolina];
  for(size_t i=0; i<num_gasolina;i++) coche_gasolina[i] = thread(funcion_hebra_gasolina, i);
  thread coche_gasoil[num_gasoil];
  for(size_t i=0; i<num_gasoil;i++) coche_gasoil[i] = thread(funcion_hebra_gasoil, i);

  for(size_t i=0; i<num_gasolina;i++) coche_gasolina[i].join();
  for(size_t i=0; i<num_gasoil;i++) coche_gasoil[i].join();
}
