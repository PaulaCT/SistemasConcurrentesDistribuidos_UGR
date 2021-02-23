#include <iostream>
#include <cassert>
#include <iomanip>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <condition_variable>
#include "HoareMonitor.h"

using namespace std;
using namespace HM;

//g++ -std=c++11 -pthread -o prodcons_lifo prodcons_lifo.cpp HoareMonitor.cpp Semaphore.cpp

constexpr int num_items  = 40 ;

mutex mtx ;
unsigned cont_prod[num_items], cont_cons[num_items];

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
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato()
{
   static int contador = 0 ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "producido: " << contador << endl << flush ;
   mtx.unlock();
   cont_prod[contador] ++ ;
   return contador++ ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato )
{
   if ( num_items <= dato )
   {
      cout << " dato === " << dato << ", num_items == " << num_items << endl ;
      assert( dato < num_items );
   }
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "                  consumido: " << dato << endl ;
   mtx.unlock();
}
//----------------------------------------------------------------------

void ini_contadores()
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  cont_prod[i] = 0 ;
      cont_cons[i] = 0 ;
   }
}

//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." << flush ;

   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      if ( cont_prod[i] != 1 )
      {
         cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {
         cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

// *****************************************************************************
// Monitor LIFO

class ProdConsLifo : public HoareMonitor
{
 private:
   static const int capacidad = 10;
   size_t v_datos[capacidad];
   size_t elemento_generado;
   size_t elemento_consumido;
   CondVar on;
   CondVar off;

 public:
   ProdConsLifo( );
   int consumir();
   void producir( int valor );
} ;
// -----------------------------------------------------------------------------

ProdConsLifo::ProdConsLifo(  )
{
   elemento_generado = 0;
   elemento_consumido = 0;
   on = newCondVar();
   off = newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int ProdConsLifo::consumir()
{
   //if (elemento_consumido == capacidad) off.wait();
   //assert(elemento_consumido < capacidad);
   const int valor = v_datos[elemento_consumido];
   elemento_consumido = (elemento_consumido+1)%capacidad;
   on.signal();
   return valor ;
}
// -----------------------------------------------------------------------------

void ProdConsLifo::producir( int valor )
{
   //if ( elemento_generado == capacidad ) on.wait();
   //assert( elemento_generado < capacidad );
   v_datos[elemento_generado] = valor;
   elemento_generado = (elemento_generado+1)%capacidad;
   off.signal();
}

// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora(MRef<ProdConsLifo> monitor )
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      int valor = producir_dato();
      monitor->producir(valor);
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora(MRef<ProdConsLifo> monitor )
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      int valor = monitor->consumir();
      consumir_dato( valor ) ;
   }
}


// *****************************************************************************

int main()
{
   cout << "-------------------------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (Monitor FIFO). " << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;

   MRef<ProdConsLifo> monitor = Create<ProdConsLifo>();

   thread hebra_productora ( funcion_hebra_productora, monitor ),
          hebra_consumidora( funcion_hebra_consumidora, monitor );

   hebra_productora.join() ;
   hebra_consumidora.join() ;

   test_contadores() ;
}
