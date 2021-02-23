#include <iostream>
#include <cassert>
#include <iomanip>
#include <mutex>
#include <random>
#include <chrono>
#include <condition_variable>
#include "HoareMonitor.h"

using namespace std;
using namespace HM;

//g++ -std=c++11 -pthread -o nombre nombre.cpp HoareMonitor.cpp Semaphore.cpp

//**********************************************************************
//Variables globales

constexpr int num_items  = 40 ;

mutex mtx ;
unsigned cont_prod[num_items], cont_cons[num_items];

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
// Funciones comunes a las dos soluciones (fifo y lifo)

int producir_dato(){
   static int contador = 0 ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "producido: " << contador << endl << flush ;
   mtx.unlock();
   cont_prod[contador] ++ ;
   return contador++ ;
}

void consumir_dato( unsigned dato ){
   if ( num_items <= dato ){
      cout << " dato === " << dato << ", num_items == " << num_items << endl ;
      assert( dato < num_items );
   }
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "                  consumido: " << dato << endl ;
   mtx.unlock();
}

void ini_contadores(){
   for( unsigned i = 0 ; i < num_items ; i++ ){
     cont_prod[i] = 0 ;
     cont_cons[i] = 0 ;
   }
}

void test_contadores(){
   bool ok = true ;
   cout << "comprobando contadores ...." << flush ;

   for( unsigned i = 0 ; i < num_items ; i++ ){
      if ( cont_prod[i] != 1 ){
         cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 ){
         cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok) cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

// *****************************************************************************
// Monitor LIFO

class ProdConsLifo : public HoareMonitor {
 private:
   static const int capacidad = 10;
   size_t v_datos[capacidad];
   size_t elemento_actual;
   CondVar on;
   CondVar off;

 public:
   ProdConsLifo();
   int consumir();
   void producir(int valor);
} ;

// Funciones del monitor

ProdConsLifo::ProdConsLifo(){
   elemento_actual = 0 ;
   on = newCondVar();
   off = newCondVar();
}

int ProdConsLifo::consumir(){
   if (elemento_actual == 0) off.wait();
   assert(0 < elemento_actual);
   elemento_actual-- ;
   const int valor = v_datos[elemento_actual] ;
   on.signal();
   return valor ;
}

void ProdConsLifo::producir(int valor){
   if (elemento_actual == capacidad) on.wait();
   assert( elemento_actual < capacidad );
   v_datos[elemento_actual] = valor ;
   elemento_actual++ ;
   off.signal();
}

// *****************************************************************************
// Funciones de las hebras

void funcion_hebra_productora(MRef<ProdConsLifo> monitor){
   for( unsigned i = 0 ; i < num_items ; i++ )   {
      int valor = producir_dato() ;
      monitor->producir(valor);
   }
}

void funcion_hebra_consumidora(MRef<ProdConsLifo> monitor){
   for( unsigned i = 0 ; i < num_items ; i++ ){
      int valor = monitor->consumir();
      consumir_dato( valor ) ;
   }
}

// *****************************************************************************
// Main

int main(){
   cout << "-------------------------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (Monitor LIFO). " << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;

   MRef<ProdConsLifo> monitor = Create<ProdConsLifo>();

   thread hebra_productora ( funcion_hebra_productora, monitor ),
          hebra_consumidora( funcion_hebra_consumidora, monitor );

   hebra_productora.join() ;
   hebra_consumidora.join() ;

   test_contadores() ;
}
