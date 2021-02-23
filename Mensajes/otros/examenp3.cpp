#include <mpi.h>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <iostream>

using namespace std;
using namespace std::this_thread;
using namespace std::chrono;

const int
  num_clientes = 10,
  num_mostrador = 4,
  num_procesos = num_clientes + num_mostrador + 1,
  id_controlador = 14;

const int tag_llegar = 0, tag_irse = 1;

int mostradores[num_mostrador] = {0,0,0,0};
int mostradores_ocupados = 0;


// **********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
// ----------------------------------------------------------------------

template <int min, int max> int aleatorio(){
    static default_random_engine generador( (random_device())() );
    static uniform_int_distribution<int> distribucion_uniforme(min, max);

    return distribucion_uniforme(generador);
}

void funcion_cliente(int id){
    int peticion,   // Mensaje a enviar.
      confirmacion, // Confirmación a recibir.
      mostrador;

    MPI_Status estado;

    while (true) {
        cout << "Cliente " << id << " quiere pagar" << endl;
        MPI_Ssend(&peticion, 1, MPI_INT, id_controlador, tag_llegar, MPI_COMM_WORLD);
        MPI_Recv(&mostrador, 1, MPI_INT, id_controlador, tag_llegar, MPI_COMM_WORLD, &estado);

        cout << "Cliente " << id << " va al mostrador " << mostrador << endl;
        MPI_Ssend(&peticion, 1, MPI_INT, mostrador, 0, MPI_COMM_WORLD);
        MPI_Recv(&confirmacion, 1, MPI_INT, mostrador, 0, MPI_COMM_WORLD, &estado);

        cout << "Cliente " << id << " deja el mostrador "<< mostrador << endl;
        MPI_Ssend(&mostrador, 1, MPI_INT, id_controlador, tag_irse, MPI_COMM_WORLD);
        MPI_Recv(&confirmacion, 1, MPI_INT, id_controlador, tag_irse, MPI_COMM_WORLD, &estado);

        cout << "Cliente " << id << " se pone a trabajar" << endl;
        sleep_for(milliseconds(aleatorio<100, 1000>() ) );
    }
}

void funcion_controlador(){
    int cliente, peticion, tag, n_m = -1;
    MPI_Status estado;

    while (true) {
    	if (mostradores_ocupados < num_mostrador) {
	       tag = MPI_ANY_TAG;
      } else if (mostradores_ocupados == 0) {
	       tag = tag_llegar;
      } else tag = tag_irse;

	    MPI_Recv(&peticion, 1, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &estado);
      cliente = estado.MPI_SOURCE;

      if (tag == tag_llegar){

        mostradores_ocupados++;
        for (size_t i = 0; i < num_mostrador && n_m != -1; i++){
          if (mostradores[i] == 0){
            n_m = i + num_clientes;
            mostradores[i] = 1;
          }
        }
        cout << "              Controlador: Cliente " << cliente << " pase por mostrador " << n_m << endl;
        MPI_Ssend(&n_m, 1, MPI_INT, cliente, tag_llegar, MPI_COMM_WORLD);
      } else {
        mostradores_ocupados--;
        mostradores[peticion - num_clientes] = 0;
        cout << "              Controlador: Mostrador " << peticion << " libre " << endl;
      }
    }
}


void funcion_mostrador(int id){
    int peticion,   // Mensaje a enviar.
      confirmacion;

    MPI_Status estado;

    while (true) {

      MPI_Recv(&peticion, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &estado);
      cout << "    >>>>>>>>>> Operando, por favor espere <<<<<<<<<<<< " << endl;
      sleep_for(milliseconds(aleatorio<10, 100>() ) );
      MPI_Ssend(&confirmacion, 1, MPI_INT, estado.MPI_SOURCE, 0, MPI_COMM_WORLD);

  }
}

// ---------------------------------------------------------------------

int main(int argc, char ** argv){
    int id_propio, num_procesos_actual;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id_propio);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procesos_actual);


    if (num_procesos == num_procesos_actual) {
        // ejecutar la función correspondiente a 'id_propio'
        if (id_propio < num_clientes) {
            funcion_cliente(id_propio);
        } else if (id_propio < num_mostrador) {
            funcion_mostrador(id_propio);
        } else if (id_propio == id_controlador)
          funcion_controlador();
    } else {
        if (id_propio == 0) { // solo el primero escribe error, indep. del rol
            cout << "el número de procesos esperados es:    " << num_procesos << endl
                 << "el número de procesos en ejecución es: " << num_procesos_actual << endl
                 << "(programa abortado)" << endl;
        }
    }

    MPI_Finalize();
    return 0;
}
