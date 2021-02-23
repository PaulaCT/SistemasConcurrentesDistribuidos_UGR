#include <iostream>
#include <cassert>
#include <iomanip>
#include <mutex>
#include <random>
#include <chrono>
#include <condition_variable>
#include "HoareMonitor.h"

using namespace std ;
using namespace HM ;

//g++ -std=c++11 -pthread -o activista_fumadores activista_fumadores.cpp HoareMonitor.cpp Semaphore.cpp

//**********************************************************************
//Variables globales

const int num_fumadores = 3;
mutex mtx;
size_t contador_activista = 0;
const size_t ciclos = 8;

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio(){
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

//**********************************************************************
// Monitor Estanco

class Estanco : public HoareMonitor{
  private:
    int mostrador;
    CondVar estanquero;
    CondVar fumador[num_fumadores];
    CondVar activista;
    bool activista_espera;

  public:
    Estanco();
    void obtenerIngrediente(int ingrediente);
    void ponerIngrediente(int ingrediente);
    void esperarRecogidaIngrediente();
    void tirarseAlSuelo();
};

// Funciones del monitor

Estanco::Estanco(){
  mostrador = -1;
  activista_espera = false;
  estanquero = newCondVar();
  activista = newCondVar();
  for (size_t i = 0; i < num_fumadores; i++) {
    fumador[i] = newCondVar();
  }
}

void Estanco::obtenerIngrediente(int ingrediente){
  if(mostrador != ingrediente){
    fumador[ingrediente].wait();
  }
  cout << "El fumador " << ingrediente << " retira su ingrediente"<< endl;
  mostrador = -1;
  estanquero.signal();
}

void Estanco::ponerIngrediente(int ingrediente){
  mostrador = ingrediente;
  //Si el activista espera
  if (activista_espera) {
    cout << "    -ACTIVISTA: ¡Ja ja ja! He destruido el ingrediente " << ingrediente << endl;
    activista_espera = false;
    activista.signal();
    mostrador = -1;
    estanquero.signal();
  //Si el activista no está esperando
  } else fumador[ingrediente].signal();
}

void Estanco::esperarRecogidaIngrediente(){
  if(mostrador != -1){
    estanquero.wait();
  }
}

void Estanco::tirarseAlSuelo(){
  //Se sienta a esperar al estanquero
  activista_espera = true;
  activista.wait();
}

//**********************************************************************
// Funciones para producir igrediente, fumar y lanzar proclamas antitabaco

int producir_ingrediente(){
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

void fumar(int num_fumador){
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

void proclama(){
  // Muestra su mensaje
  cout << "    -ACTIVISTA: ¡Fumar mata!  (" << contador_activista << ")" << endl;
  //Espera un tiempo aleatorio
  chrono::milliseconds duracion_retardo(aleatorio<10,100>());
  this_thread::sleep_for(duracion_retardo);
  contador_activista++;
}

//**********************************************************************
// Funciones de las hebras

void funcion_hebra_estanquero(MRef<Estanco> monitor){
  while(true){
    int ingrediente = producir_ingrediente();
    monitor->ponerIngrediente(ingrediente);
    monitor->esperarRecogidaIngrediente();
  }
}

void funcion_hebra_fumador(MRef<Estanco> monitor, int fumador){
  while(true){
    monitor->obtenerIngrediente(fumador);
    fumar(fumador);
  }
}

void funcion_hebra_activista(MRef<Estanco> monitor){
  while(true){
    proclama();
    if (contador_activista == ciclos){
      monitor->tirarseAlSuelo();
      contador_activista = 0;
    }
  }
}

//**********************************************************************
// Main

int main(){
  MRef<Estanco> monitor = Create<Estanco>();

  thread hebra_estanquero, hebra_fumadores[num_fumadores], hebra_activista;

  hebra_estanquero = thread(funcion_hebra_estanquero, monitor);
  hebra_activista = thread(funcion_hebra_activista, monitor);

  for (size_t i = 0; i < num_fumadores; i++) {
    hebra_fumadores[i] = thread(funcion_hebra_fumador, monitor, i);
  }

  hebra_estanquero.join();
  hebra_activista.join();
  for (size_t i = 0; i < num_fumadores; i++) {
    hebra_fumadores[i].join();
  }
}
