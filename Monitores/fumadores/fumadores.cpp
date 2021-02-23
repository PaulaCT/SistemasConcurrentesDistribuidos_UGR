#include <iostream>
#include <cassert>
#include <iomanip>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <condition_variable>
#include "HoareMonitor.h"

using namespace std ;
using namespace HM ;

//g++ -std=c++11 -pthread -o fumadores fumadores.cpp HoareMonitor.cpp Semaphore.cpp

const int num_fumadores = 3;
mutex mtx;

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

//**********************************************************************
//Clase del monitor Estanco
class Estanco : public HoareMonitor{
  private:
    int mostrador;      //si tiene valor -1 el mostrador está vacio
    CondVar estanquero;
    CondVar fumador[num_fumadores];

  public:
    Estanco();
    void obtenerIngrediente(int ingrediente);
    void ponerIngrediente(int ingrediente);
    void esperarRecogidaIngrediente();
};




Estanco::Estanco(){
  mostrador = -1;
  estanquero = newCondVar();
  for (size_t i = 0; i < num_fumadores; i++) {
    fumador[i] = newCondVar();
  }
}


void Estanco::obtenerIngrediente(int ingrediente){
  if(mostrador != ingrediente){
    fumador[ingrediente].wait();
  }
  //si está el ingrediente del fumador, lo retira y el mostrador queda vacio
  std::cout << "El fumador " << ingrediente << " retira el ingrediente " << ingrediente << '\n';
  mostrador = -1;
  estanquero.signal();
}


void Estanco::ponerIngrediente(int ingrediente){
  mostrador = ingrediente;
  fumador[ingrediente].signal();
}

void Estanco::esperarRecogidaIngrediente(){
  if(mostrador != -1){
    estanquero.wait();
  }
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

//**********************************************************************


void funcion_hebra_estanquero(MRef<Estanco> monitor){
  while(true){
    int ingrediente = producir_ingrediente();
    monitor->ponerIngrediente(ingrediente);
    monitor->esperarRecogidaIngrediente();
  }
}


void funcion_hebra_fumador(MRef<Estanco> monitor, int fumador){
  while (true) {
    monitor->obtenerIngrediente(fumador);
    fumar(fumador);
  }
}

//**********************************************************************

int main(){
  MRef<Estanco> monitor = Create<Estanco>();

  thread hebra_estanquero, hebra_fumadores[num_fumadores];

  hebra_estanquero = thread(funcion_hebra_estanquero, monitor);

  for (size_t i = 0; i < num_fumadores; i++) {
    hebra_fumadores[i] = thread(funcion_hebra_fumador, monitor, i);
  }


  hebra_estanquero.join();
  for (size_t i = 0; i < num_fumadores; i++) {
    hebra_fumadores[i].join();
  }
}
