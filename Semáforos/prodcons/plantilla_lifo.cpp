#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

//g++ -std=c++11 -pthread -I. -o prodcons_lifo prodcons_lifo.cpp  Semaphore.cpp

//**********************************************************************
//Variables globales

const int num_items = 40, tam_vec   = 10 ;
unsigned  cont_prod[num_items] = {0}, cont_cons[num_items] = {0};
size_t elemento_actual = 0;
size_t v_datos[tam_vec];
Semaphore on(tam_vec);
Semaphore off(0);

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
   cout << "producido: " << contador << endl << flush ;
   cont_prod[contador] ++ ;
   return contador++ ;
}

void consumir_dato( unsigned dato ){
   assert( dato < num_items );
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   cout << "                  consumido: " << dato << endl ;
}

void test_contadores(){
   bool ok = true ;
   cout << "comprobando contadores ...." ;
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
// Funciones de las hebras

void  funcion_hebra_productora( )
{
   for( unsigned i = 0 ; i < num_items ; i++ ){
      int dato = producir_dato() ;
      sem_wait(on);
			v_datos[elemento_actual] = dato;
			elemento_actual++;
			sem_signal(off);
   }
}

void funcion_hebra_consumidora(  ){
   for( unsigned i = 0 ; i < num_items ; i++ ){
	 		sem_wait(off);
      int dato = v_datos[elemento_actual-1];
			elemento_actual--;
      consumir_dato( dato ) ;
			sem_signal(on);
    }
}

// *****************************************************************************
// Main

int main(){
   cout << "--------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (solución LIFO)." << endl
        << "--------------------------------------------------------" << endl
        << flush ;

   thread hebra_productora ( funcion_hebra_productora ),
          hebra_consumidora( funcion_hebra_consumidora );

   hebra_productora.join() ;
   hebra_consumidora.join() ;

   test_contadores();
}
