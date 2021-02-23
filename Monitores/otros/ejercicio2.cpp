#include <iostream>
#include <cassert>
#include <iomanip>
#include <mutex>
#include <random>
#include <chrono>
#include <condition_variable>
#include "HoareMonitor.h"
#include <vector>

using namespace std ;
using namespace HM ;

//g++ -std=c++11 -pthread -o ejercicio2 ejercicio2.cpp HoareMonitor.cpp Semaphore.cpp

const size_t num_coches = 10;
const size_t num_cabinas = 2;
mutex mut;

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
//Clase del monitor Peaje
//**********************************************************************

class Peaje : public HoareMonitor{
  private:
    size_t coches_cabina[num_cabinas];
    CondVar cola[num_cabinas];
  public:
    Peaje();
    size_t llegadaPeaje(size_t num_coche);
    void pagado(size_t cabina, size_t num_coche);
};

//-------------------------------------------------------------------------
// Funciones del monitor

Peaje::Peaje(){
  coches_cabina[0] = 0;
  coches_cabina[1] = 0;
  for (size_t i = 0; i < num_cabinas; i++) {
    cola[i] = newCondVar();
  }
}

size_t Peaje::llegadaPeaje(size_t num_coche){
  size_t cabina;
  if (coches_cabina[0] >= coches_cabina[1]) cabina = 1;
  else cabina = 0;
  mut.lock();
  coches_cabina[cabina]++;
  mut.unlock();
  if (coches_cabina[cabina]>1) cola[cabina].wait();
  return cabina;
}

void Peaje::pagado(size_t cabina, size_t num_coche){
  mut.lock();
  coches_cabina[cabina]--;
  mut.unlock();
  cola[cabina].signal();
}




//-------------------------------------------------------------------------
// Funciones que simulan las acciones de pagar e irse de la cabina

void pagar(size_t coche){
   chrono::milliseconds duracion_produ( aleatorio<200,2000>() );
   //cout << "Coche " << coche << " tarda "<< duracion_produ.count() << " en pagar." << endl;
   this_thread::sleep_for( duracion_produ );
}

void dejar_cabina(size_t coche){
  chrono::milliseconds duracion_produ( aleatorio<200,2000>() );
  //cout << "Coche " << coche << " tarda "<< duracion_produ.count() << " en irse." << endl;
  this_thread::sleep_for( duracion_produ );
}


//-------------------------------------------------------------------------
// Función hebra coche

void funcion_hebra_coche(MRef<Peaje> monitor, size_t i){
  size_t cabina;
  while (true) {
    cabina = monitor->llegadaPeaje(i);
    cout << "Coche " << i << " llega a cola de cabina " << cabina << ", ¡hola coche " << i << "!" << endl;
    pagar(i);
    cout << "Coche " << i << " paga en cabina " << cabina << endl;
    monitor->pagado(cabina,i);
    dejar_cabina(i);
    cout << "Coche " << i << " deja la cabina " << cabina << " [ondea, ondea, adios, adios]" << endl;

  }
}

//-------------------------------------------------------------------------
// Función main

int main(){
  MRef<Peaje> monitor = Create<Peaje>();

  thread hebra_coche[num_coches];

  for (size_t i = 0; i < num_coches; i++) {
    hebra_coche[i] = thread(funcion_hebra_coche, monitor, i);
  }

  for (size_t i = 0; i < num_coches; i++) {
    hebra_coche[i].join();
  }
}
