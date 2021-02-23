#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;


//g++ -std=c++11 -pthread -I. -o fumadores_almacen fumadores_almacen.cpp  Semaphore.cpp

//Defino variables globales necesarias
const int num_fumadores = 3;
Semaphore elemento_disponible[3] = {Semaphore(0), Semaphore(0), Semaphore(0)};
Semaphore mostrador_libre(1);

const int tam_vec = 15;
int almacen[tam_vec];
int primera_libre = 0;
//Añado los semaforos necesarios
Semaphore libre(tam_vec);
Semaphore ocupadas(0);

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
   cout << "Proveedor : empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
   this_thread::sleep_for( duracion_produ );

   const int num_ingrediente = aleatorio<0,num_fumadores-1>() ;

   // informa de que ha terminado de producir
   cout << "Proveedor : termina de producir ingrediente " << num_ingrediente << endl;

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

    sem_wait(ocupadas);									//Las celdas estan ocupadas por un producto para ser consumido
    sem_wait(mostrador_libre);
    //EL dato para consumir esta en la celda anterior, ya que hemos aumentado el valor de primera_libre
    ingrediente_producido = almacen[primera_libre-1];
    primera_libre--;										//Al consumirse el dato, la celda queda libre para nuevo uso
    sem_signal(libre);

    std::cout << "El estanquero coloca ingrediente " << ingrediente_producido << '\n';

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

void funcion_proveedor(){
  while(true){
    //Produce un número y si puedo escribir lo guarda en el buffer.
     int dato = producir_ingrediente();
     sem_wait(libre);										//Debe haber espacio en el buffer intermedio

     almacen[primera_libre] = dato;
     primera_libre++;

     std::cout << "El proveedor coloca en el almacen el ingrediente " << dato << '\n';

     sem_signal(ocupadas);
  }
}

//----------------------------------------------------------------------

int main()
{
   // declarar hebras y ponerlas en marcha
   thread hebra_estanquero(funcion_hebra_estanquero);
   thread hebra_fumador[num_fumadores];
   thread hebra_proveedor(funcion_proveedor);

   for (int i = 0; i < num_fumadores; i++) {
     hebra_fumador[i] = thread(funcion_hebra_fumador, i);
   }

   hebra_estanquero.join();
   hebra_proveedor.join();
   for (int i = 0; i < num_fumadores; i++) {
     hebra_fumador[i].join();
   }
}
