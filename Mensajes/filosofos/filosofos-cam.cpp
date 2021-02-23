// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: filosofos-plantilla.cpp
// Implementación del problema de los filósofos (sin camarero).
// Plantilla para completar.
//
// Historial:
// Actualizado a C++11 en Septiembre de 2017
// -----------------------------------------------------------------------------


#include <mpi.h>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <iostream>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int num_filosofos = 5, num_procesos = 2*num_filosofos + 1, id_camarero = 0, num_sillas = 4;


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
// Funciones

void funcion_filosofos(int id){
  int id_ten_izq = (id+1)              % num_procesos, //id. tenedor izq.
      id_ten_der = (id+num_procesos-1) % num_procesos; //id. tenedor der.
  int peticion, valor;
  MPI_Status estado;

  while ( true ){
    cout <<"Filosofo " << id << " solicita sentarse." << endl;
    //Solicita sentarse
    MPI_Ssend(&id, 1, MPI_INT, id_camarero, 2, MPI_COMM_WORLD);
    //Se ha sentado
    MPI_Recv(&peticion, 1, MPI_INT, id_camarero, 2, MPI_COMM_WORLD, &estado);

    if (id == 2) {
      cout <<"Filósofo " <<id <<" solicita ten. der." <<id_ten_der <<endl;
      //Solicita tenedor der
      MPI_Ssend(&id, 1, MPI_INT, id_ten_der, 0, MPI_COMM_WORLD);

      cout <<"Filósofo " <<id << " solicita ten. izq." <<id_ten_izq <<endl;
      //Solicita tenedor izq
      MPI_Ssend(&id, 1, MPI_INT, id_ten_izq, 0, MPI_COMM_WORLD);

    } else {
      cout <<"Filósofo " <<id << " solicita ten. izq." <<id_ten_izq <<endl;
      //Solicita tenedor izq
      MPI_Ssend(&id, 1, MPI_INT, id_ten_izq, 0, MPI_COMM_WORLD);

      cout <<"Filósofo " <<id <<" solicita ten. der." <<id_ten_der <<endl;
      //Solicita tenedor der
      MPI_Ssend(&id, 1, MPI_INT, id_ten_der, 0, MPI_COMM_WORLD);
    }

    cout <<"Filósofo " <<id <<" comienza a comer" <<endl ;
    sleep_for( milliseconds( aleatorio<10,100>() ) );

    cout <<"Filósofo " <<id <<" suelta ten. izq. " <<id_ten_izq <<endl;
    //Solicita soltar ten izq
    MPI_Ssend(&id, 1, MPI_INT, id_ten_izq, 1, MPI_COMM_WORLD);

    cout<< "Filósofo " <<id <<" suelta ten. der. " <<id_ten_der <<endl;
    //Solicita soltar ten der
    MPI_Ssend(&id, 1, MPI_INT, id_ten_der, 1, MPI_COMM_WORLD);


    cout <<"Filosofo " << id << " solicita levantarse." << endl;
    //Solicita levantarse
    MPI_Ssend(&id, 1, MPI_INT, id_camarero, 3, MPI_COMM_WORLD);

    cout << "Filosofo " << id << " comienza a pensar" << endl;
    sleep_for( milliseconds( aleatorio<10,100>() ) );
 }
}

// ---------------------------------------------------------------------

void funcion_tenedores(int id){
  int valor, id_filosofo ;  // valor recibido, identificador del filósofo
  MPI_Status estado ;       // metadatos de las dos recepciones

  while ( true ){
     // ...... recibir petición de cualquier filósofo (completar)
     // ...... guardar en 'id_filosofo' el id. del emisor (completar)
     MPI_Recv (&valor, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &estado);
     id_filosofo = valor;
     cout << ">>>>>>>>>>>>>> Ten. " <<id <<" ha sido cogido por filosofo " <<id_filosofo <<endl;

     // ...... recibir liberación de filósofo 'id_filosofo' (completar)
     MPI_Recv (&valor, 1, MPI_INT, id_filosofo, 1, MPI_COMM_WORLD, &estado);
     cout << "<<<<<<<<<<<<<< Ten. "<< id<< " ha sido liberado por filosofo " <<id_filosofo <<endl ;
  }
}

// ---------------------------------------------------------------------

void funcion_camarero(){
  int valor, filosofos_mesa, tag_aceptable;
  MPI_Status estado;

  while(true){
    if (filosofos_mesa < num_sillas) tag_aceptable = MPI_ANY_TAG;
    else tag_aceptable = 3;

    MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, tag_aceptable, MPI_COMM_WORLD, &estado);

    switch(estado.MPI_TAG){
       case 2: filosofos_mesa++;
          cout << "-------------- Filosofo " << valor << " se ha sentado.   --------------" << endl
            << "               HAY " << filosofos_mesa << " FILOSOFOS EN LA MESA" << endl;
          MPI_Ssend (&valor, 1, MPI_INT, valor, 2, MPI_COMM_WORLD);
          break;

       case 3: filosofos_mesa--;
          cout << "-------------- Filosofo " << valor << " se ha levantado. --------------" << endl
            << "               HAY " << filosofos_mesa << " FILOSOFOS EN LA MESA" << endl;
          MPI_Ssend (&valor, 1, MPI_INT, valor, 3, MPI_COMM_WORLD);
          break;
    }
  }
}

//**********************************************************************
// Main

int main(int argc, char** argv){
   int id_propio, num_procesos_actual ;

   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );


   if ( num_procesos == num_procesos_actual )
   {
      // ejecutar la función correspondiente a 'id_propio'
      if (id_propio == 0) funcion_camarero();
      else if (id_propio % 2 == 0)          // si es par
         funcion_filosofos( id_propio ); //   es un filósofo
      else                               // si es impar
         funcion_tenedores( id_propio ); //   es un tenedor
   }
   else
   {
      if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
      { cout << "el número de procesos esperados es:    " << num_procesos << endl
             << "el número de procesos en ejecución es: " << num_procesos_actual << endl
             << "(programa abortado)" << endl ;
      }
   }

   MPI_Finalize( );
   return 0;
}
