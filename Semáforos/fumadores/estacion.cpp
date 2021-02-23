#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include <chrono>
#include "Semaphore.h"
#include <condition_variable>
#include "HoareMonitor.h"

using namespace std ;
using namespace HM ;
using namespace SEM ;

//g++ -std=c++11 -pthread -o estacion estacion.cpp HoareMonitor.cpp Semaphore.cpp

//**********************************************************************
//Variables globales

const size_t num_estudiantes = 60;
const size_t num_maquinas = 5; // 1 cola
const size_t num_tornos = 2;
Semaphore maquina[num_maquinas] = {1,1,1,1,1};
mutex mtx;
//Lo suyo sería usar un bool para cada hebra pero funciona así que da igual
bool primera_iteracion = true;
size_t recuento_maquinas = 0;
size_t recuento_tornos = 0;

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
// Monitor Tornos

class Tornos : public HoareMonitor{
  private:
    size_t esperando[num_tornos];
    CondVar cola[num_tornos];
  public:
    Tornos();
    size_t elegir_torno(size_t i);
    void pasar_torno(size_t torno, size_t i);
};

//-------------------------------------------------------------------------
// Funciones del monitor

Tornos::Tornos(){
  esperando[0] = 0;
  esperando[1] = 0;
  for (size_t i = 0; i < num_tornos; i++) {
    cola[i] = newCondVar();
  }
}

size_t Tornos::elegir_torno(size_t i){
  size_t torno;
  if (esperando[0] >= esperando[1]) torno = 1;
  else torno = 0;
  mtx.lock();
  esperando[torno]++;
  mtx.unlock();
  if (esperando[torno]>1) cola[torno].wait();
  cout << "        ->Estudiante " << i << " llega al torno " << torno << endl;
  return torno;
}

void Tornos::pasar_torno(size_t torno, size_t i){
  mtx.lock();
  esperando[torno]--;
  mtx.unlock();
  cout << "            ->Estudiante " << i << " deja el torno " << torno << endl;
  cola[torno].signal();
}

//**********************************************************************
// Funciones de los estudiantes

void llegar_al_hall(size_t i){
  chrono::milliseconds duracion_llegada( aleatorio<200,2000>() );
  this_thread::sleep_for(duracion_llegada);
  cout << "->Estudiante " << i << " ha llegado al hall" << endl;
}

void sacar_billete_maquina(size_t i){
  //Elige máquina aleatoriamente y la bloquea
  int maquina_elegida = aleatorio<0,num_maquinas-1>();
  sem_wait(maquina[maquina_elegida]);
  cout << "    ->Estudiante " << i << " sacandose el billete" << endl;
  //Espera y desbloquea la máquina
  chrono::milliseconds duracion_billete( aleatorio<200,2000>() );
  this_thread::sleep_for(duracion_billete);
  sem_signal(maquina[maquina_elegida]);
}

//**********************************************************************
// Funciones de las hebras

void funcion_hebra_estudiante(MRef<Tornos> monitor, size_t i){
  while (primera_iteracion){
    //Parte Semáforos
    llegar_al_hall(i);
    sacar_billete_maquina(i);
    mtx.lock();
    recuento_maquinas++;
    mtx.unlock();
    if (recuento_maquinas == num_estudiantes)
      cout << "++++++++++++++++++++ RECUENTO MAQUINAS: " << recuento_maquinas << " ++++++++++++++++++++" << endl;
    //Parte Monitor
    size_t torno_elegido = monitor->elegir_torno(i);
    chrono::milliseconds duracion_maletas( aleatorio<200,2000>() );
    this_thread::sleep_for(duracion_maletas);
    monitor->pasar_torno(torno_elegido,i);
    mtx.lock();
    recuento_tornos++;
    mtx.unlock();
    if (recuento_tornos == num_estudiantes)
      cout << "+++++++++++++++++++++ RECUENTO TORNOS: " << recuento_maquinas << " +++++++++++++++++++++" << endl;
    //Solo una iteración
    primera_iteracion = false;
  }
}

//**********************************************************************
// Main

int main(){
  MRef<Tornos> monitor = Create<Tornos>();
  thread estudiante[num_estudiantes];
  for(size_t i=0; i<num_estudiantes;i++) estudiante[i] = thread(funcion_hebra_estudiante, monitor, i);
  for(size_t i=0; i<num_estudiantes;i++) estudiante[i].join();
}
