#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

mutex mtx;

//g++ -std=c++11 -pthread -I. -o prodcons_1p_2c prodcons_1p_2c.cpp  Semaphore.cpp

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

const int num_consumidores = 2;
Semaphore on(tam_vec);
Semaphore off(0);
Semaphore consumidor[num_consumidores] = {0,0};
size_t consumidor_actual = 0;
//Semaphore productor[num_productores] = {1,0};


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

int producir_dato( )
{
   static int contador = 0 ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

   cout << "producido: " << contador << endl << flush ;

   cont_prod[contador] ++ ;
   return contador++ ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato )
{
   assert( dato < num_items );
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

   cout << "                  consumido: " << dato << " por " << consumidor_actual << endl ;

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

void  funcion_hebra_productora( )
{
   for( unsigned i = 0 ; i < num_items ; i++ ){
			int dato = producir_dato() ;
			sem_wait(on);
			v_datos[elemento_actual] = dato;
			elemento_actual = (elemento_actual+1)%tam_vec;
			sem_signal(off);
			sem_signal(consumidor[consumidor_actual]);
   }
}


//----------------------------------------------------------------------

void funcion_hebra_consumidora( size_t n_cons )
{
   for( unsigned i = n_cons ; i < num_items ; i+=num_consumidores )
   {
      int dato ;
			sem_wait(off);
			sem_wait(consumidor[consumidor_actual]);
			consumidor_actual = (consumidor_actual+1)%num_consumidores;
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

   thread hebra_productora( funcion_hebra_productora ),
          hebra_consumidora[num_consumidores];

    for (int i = 0; i < num_consumidores; i++) {
      hebra_consumidora[i] = thread(funcion_hebra_consumidora, i);
    }

    hebra_productora.join() ;

    for (int i = 0; i < num_consumidores; i++) {
      hebra_consumidora[i].join();
    }

   test_contadores();
}
