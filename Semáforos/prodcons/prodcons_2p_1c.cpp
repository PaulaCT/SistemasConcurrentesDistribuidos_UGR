#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

mutex mtx;

//g++ -std=c++11 -pthread -I. -o prodcons_2p_1c prodcons_2p_1c.cpp  Semaphore.cpp

/*
									CASO LIFO
*/

//**********************************************************************
// variables compartidas

const int num_items = 40 ,   // número de items
	       tam_vec   = 10 ;   // tamaño del buffer
unsigned  cont_prod[num_items] = {0}, // contadores de verificación: producidos
          cont_cons[num_items] = {0}; // contadores de verificación: consumidos

size_t v_datos[tam_vec];

int elemento_actual = 0;

const int num_productores = 2;
Semaphore on(tam_vec);
Semaphore off(0);
Semaphore productor[num_productores] = {1,0};


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

int producir_dato( int n_productor )
{
   static int contador = 0 ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

   cout << "producido: " << contador << " por " << n_productor << endl << flush ;

   cont_prod[contador] ++ ;
   return contador++ ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato )
{
   assert( dato < num_items );
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

   cout << "                  consumido: " << dato << endl ;

}


//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." ;
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  if ( cont_prod[i] != 1 )
      {  cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {  cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

//----------------------------------------------------------------------

void  funcion_hebra_productora( int n_productor )
{
   for( unsigned i = n_productor ; i < num_items ; i+=2 ){
		 	sem_wait(productor[n_productor]);

      int dato = producir_dato( n_productor ) ;
			v_datos[elemento_actual] = dato;
			elemento_actual++;

			sem_signal(off);
			sem_signal(productor[(n_productor+1)%2]);

   }
}


//----------------------------------------------------------------------

void funcion_hebra_consumidora(  )
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      int dato ;
			sem_wait(off);
			dato = v_datos[elemento_actual-1];
			elemento_actual--;
      sem_signal(on);
      consumir_dato( dato ) ;
    }
}
//----------------------------------------------------------------------

int main()
{
   cout << "--------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (solución LIFO)." << endl
        << "--------------------------------------------------------" << endl
        << flush ;

   thread hebra_productora[num_productores],
          hebra_consumidora( funcion_hebra_consumidora );

    for (int i = 0; i < num_productores; i++) {
      hebra_productora[i] = thread(funcion_hebra_productora, i);
    }

    for (int i = 0; i < num_productores; i++) {
      hebra_productora[i].join();
    }
    hebra_consumidora.join() ;

   test_contadores();
}
