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
  num_procesos = num_clientes + 1; // Le sumamos al proceso intermedio

const int ID_PROCESO_INTERMEDIO = num_procesos - 1;
const int MAX_CAJAS = 3;

// Definimos las etiquetas para mayor legibilidad
const int etiq_PASAR_POR_CAJA = 0, etiq_SALIR_DE_CAJA = 1;

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
      confirmacion; // Confirmación a recibir.

    MPI_Status estado;

    while (true) {
        // Entramos en la tienda y compramos
        cout << "Cliente " << id << " ha entrado y está comprando." << endl;
        sleep_for(milliseconds(aleatorio<10, 100>() ) );

        // Pedimos pagar
        cout << "Cliente " << id << " ha terminado de ver cosas y quiere pasar por caja." << endl;
        MPI_Ssend(&peticion, 1, MPI_INT, ID_PROCESO_INTERMEDIO, etiq_PASAR_POR_CAJA, MPI_COMM_WORLD);

        // Esperamos a recibir la confirmación
        MPI_Recv(&confirmacion, 1, MPI_INT, ID_PROCESO_INTERMEDIO, etiq_PASAR_POR_CAJA, MPI_COMM_WORLD, &estado);
        cout << "Cliente " << id << " ha sido aceptado para pasar por caja." << endl;

        // Pasamos por caja
        cout << "Cliente " << id << " está pasando por caja." << endl;
        sleep_for(milliseconds(aleatorio<10, 100>() ) );

        // Nos vamos de la caja
        MPI_Ssend(&peticion, 1, MPI_INT, ID_PROCESO_INTERMEDIO, etiq_SALIR_DE_CAJA, MPI_COMM_WORLD);
        cout << "Cliente " << id << " ha terminado de pagar." << endl;

        cout << "Cliente " << id << " ha terminado su compra y sale del comercio." << endl;
        sleep_for(milliseconds(aleatorio<10, 100>() ) );
    }
} // funcion_cliente

// ---------------------------------------------------------------------

void funcion_intermedio(){
    int cajas_ocupadas = 0, // Cajas ocupadas
      peticion= 1;             // Petición a recibir o realizar
    int cliente, etiqueta;           // Cliente, etiqueta
    MPI_Status estado;

    while (true) {

	if (cajas_ocupadas < MAX_CAJAS) { // Hay alguna caja libre
	    etiqueta= MPI_ANY_TAG;
        } else { // solo se puede recibir comunicaciones de salida de la caja
	    etiqueta= etiq_SALIR_DE_CAJA;
        }

	MPI_Recv(&peticion, 1, MPI_INT, MPI_ANY_SOURCE, etiqueta, MPI_COMM_WORLD, &estado);
        cliente = estado.MPI_SOURCE;

        if (estado.MPI_TAG == etiq_PASAR_POR_CAJA) {
            cajas_ocupadas++;
            MPI_Ssend(&peticion, 1, MPI_INT, cliente, etiq_PASAR_POR_CAJA, MPI_COMM_WORLD);
            cout << "Intermedio comunica que Cliente " << cliente << " ha sido aceptado para pasar por caja. Hay " << cajas_ocupadas << " cajas ocupadas." << endl;
        } else {
            cout << "Intermedio comunica que Cliente " << cliente << " ha informado de que ha terminado de pagar y se va de la caja. ";
            cajas_ocupadas--;
            cout << "Quedan " << cajas_ocupadas << " cajas libres" << endl;
        }
    }
} // funcion_intermedio

// ---------------------------------------------------------------------

int main(int argc, char ** argv){
    int id_propio, num_procesos_actual;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id_propio);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procesos_actual);


    if (num_procesos == num_procesos_actual) {
        // ejecutar la función correspondiente a 'id_propio'
        if (id_propio < ID_PROCESO_INTERMEDIO) {
            funcion_cliente(id_propio);
        } else {                  // si es impar
            funcion_intermedio(); //   es un tenedor
        }
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
