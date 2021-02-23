#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;


//g++ -std=c++11 -pthread -I. -o fumadores_activista fumadores_activista.cpp  Semaphore.cpp

//Defino variables globales necesarias
const int num_fumadores = 3;
Semaphore elemento_disponible[3] = {Semaphore(0), Semaphore(0), Semaphore(0)};
Semaphore mostrador_libre(1);

Semaphore activista(0);
int contador = 0;
bool duerme = false;

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
   cout << "Estanquero : empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
   this_thread::sleep_for( duracion_produ );

   const int num_ingrediente = aleatorio<0,num_fumadores-1>() ;

   // informa de que ha terminado de producir
   cout << "Estanquero : termina de producir ingrediente " << num_ingrediente << endl;

   return num_ingrediente ;
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
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(  )
{
  int ingrediente_producido;
  while(true){
    sem_wait(mostrador_libre);
    ingrediente_producido = producir_ingrediente();

    if(duerme){
      sem_signal(activista);
      std::cout << "\nEl activista tira el ingrediente "<< ingrediente_producido << '\n';
      duerme = false;
    }
    else
      sem_signal(elemento_disponible[ingrediente_producido]);
  }
}



//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador )
{
   while( true )
   {
     sem_wait(elemento_disponible[num_fumador]);
     std::cout << "El fumador " << num_fumador << " retira su ingrediente" << '\n';
     sem_signal(mostrador_libre);
     fumar(num_fumador);
   }
}

void espera(){
  chrono::milliseconds duracion_espera( aleatorio<10,200>() );
  this_thread::sleep_for(duracion_espera);
}

void funcion_hebra_activista(){
  while (true) {
    std::cout << "NO FUMES" << '\n';
    contador++;
    espera();

    if(contador % 8 == 0){
      duerme = true;
      sem_wait(activista);
      sem_signal(mostrador_libre);
    }
  }
}

//----------------------------------------------------------------------

int main()
{
   // declarar hebras y ponerlas en marcha
   thread hebra_estanquero(funcion_hebra_estanquero);
   thread hebra_fumador[num_fumadores];
   thread hebra_activista(funcion_hebra_activista);

   for (int i = 0; i < num_fumadores; i++) {
     hebra_fumador[i] = thread(funcion_hebra_fumador, i);
   }

   hebra_estanquero.join();
   hebra_activista.join();
   for (int i = 0; i < num_fumadores; i++) {
     hebra_fumador[i].join();
   }
}
