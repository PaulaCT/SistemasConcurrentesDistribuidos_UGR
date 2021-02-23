#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

const size_t num_fumadores = 3;
Semaphore atender[2] = {0,1};
Semaphore comprar[num_fumadores] = {0,0,0};
size_t estanquero_actual = 0;
mutex m;

//g++ -std=c++11 -pthread -I. -o fumadores_2e fumadores_2e.cpp  Semaphore.cpp

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

//-------------------------------------------------------------------------
// Función que simula la acción de producir un ingrediente, como un retardo
// aleatorio de la hebra (devuelve número de ingrediente producido)

int producir_ingrediente()
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_produ( aleatorio<10,100>() );

   // informa de que comienza a producir
   cout << "Estanquero " << estanquero_actual << " empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
   this_thread::sleep_for( duracion_produ );

   const int num_ingrediente = aleatorio<0,num_fumadores-1>() ;

   // informa de que ha terminado de producir
   cout << "Estanquero " << estanquero_actual << " termina de producir ingrediente " << num_ingrediente << endl;

   estanquero_actual = (estanquero_actual+1)%2;

   return num_ingrediente ;
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero( )
{
  int ingrediente;
  while (true){
    ingrediente=producir_ingrediente();
    sem_wait(atender[estanquero_actual]);
    //sem_signal(atender[estanquero_actual]);
    cout << "\tIngrediente " << ingrediente << " generado." << endl;
    sem_signal(comprar[ingrediente]);
  }
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar

    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar

    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;

}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador )
{
   while( true )
   {
     sem_wait(comprar[num_fumador]);
     cout << "\tIngrediente " << num_fumador << " retirado." << endl;
     fumar(num_fumador);
     //estanquero_actual = (estanquero_actual+1)%2;
     sem_signal(atender[estanquero_actual]);
   }
}

//----------------------------------------------------------------------

int main()
{
  thread estanquero[2];
  for(size_t i=0; i<2; i++) estanquero[i] = thread(funcion_hebra_estanquero);
  thread fumador[num_fumadores];
  for(size_t i=0; i<num_fumadores; i++) fumador[i] = thread(funcion_hebra_fumador, i);
  for(size_t i=0; i<2; i++) estanquero[i].join();
  for(size_t i=0; i<num_fumadores; i++) fumador[i].join();
}
