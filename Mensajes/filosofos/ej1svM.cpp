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
using namespace std::this_thread;
using namespace std::chrono;

const int
  num_filosofos = 5,
  num_procesos  = 2 * num_filosofos + 1; // Le sumamos al camarero

const int ID_CAMARERO  = num_procesos - 1;
const int MAX_SENTADOS = num_filosofos - 1;

int sOcupadas, sentados = 0; // Filosofos sentados

// Definimos las etiquetas para mayor legibilidad
const int etiq_COGER = 0, etiq_SOLTAR = 1, etiq_SENTARSE = 2, etiq_LEVANTARSE = 3;

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

// ---------------------------------------------------------------------

void funcion_filosofos(int id){
    int id_ten_izq = (id + 1) % (num_procesos - 1),                // id. tenedor izq. Tenemos que restar al camarero del número de procesos.
      id_ten_der   = (id + num_procesos - 2) % (num_procesos - 1), // id. tenedor der. Tenemos que restar al camarero del número de procesos.
      peticion,                                                    // Mensaje a enviar.
      confirmacion;                                                // Confirmación a recibir.

    MPI_Status estado;

    while (true) {
        // Pedimos sentarnos
        cout << "Filosofo " << id << " solicita sentarse." << endl;
        MPI_Ssend(&peticion, 1, MPI_INT, ID_CAMARERO, etiq_SENTARSE, MPI_COMM_WORLD);

        // Esperamos a recibir la confirmación
        MPI_Recv(&confirmacion, 1, MPI_INT, ID_CAMARERO, etiq_SENTARSE, MPI_COMM_WORLD, &estado);
        cout << "Filosofo " << id << " recibe confirmación para sentarse." << endl;

        cout << "Filósofo " << id << " solicita tenedor izquierdo " << id_ten_izq << endl;
        MPI_Ssend(&peticion, 1, MPI_INT, id_ten_izq, etiq_COGER, MPI_COMM_WORLD); // Solicitar tenedor izquierdo

        cout << "Filósofo " << id << " solicita tenedor derecho " << id_ten_der << endl;
        MPI_Ssend(&peticion, 1, MPI_INT, id_ten_der, etiq_COGER, MPI_COMM_WORLD); // Solicitar tenedor derecho

        cout << "Filósofo " << id << " comienza a comer" << endl;
        sleep_for(milliseconds(aleatorio<10, 100>() ) );

        cout << "Filósofo " << id << " suelta tenedor izquierdo " << id_ten_izq << endl;
        MPI_Ssend(&peticion, 1, MPI_INT, id_ten_izq, etiq_SOLTAR, MPI_COMM_WORLD); // Soltar tenedor izquierdo

        cout << "Filósofo " << id << " suelta tenedor derecho " << id_ten_der << endl;
        MPI_Ssend(&peticion, 1, MPI_INT, id_ten_der, etiq_SOLTAR, MPI_COMM_WORLD); // Soltar tenedor derecho

        // Nos levantamos
        MPI_Ssend(&peticion, 1, MPI_INT, ID_CAMARERO, etiq_LEVANTARSE, MPI_COMM_WORLD);
        cout << "Filosofo " << id << " Se ha levantado." << endl;

        cout << "Filosofo " << id << " comienza a pensar" << endl;
        sleep_for(milliseconds(aleatorio<10, 100>() ) );
    }
} // funcion_filosofos

// ---------------------------------------------------------------------

void funcion_tenedores(int id){
    int valor, id_filosofo; // valor recibido, identificador del filósofo
    MPI_Status estado;      // metadatos de las dos recepciones

    while (true) {
        MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_COGER, MPI_COMM_WORLD, &estado); // Recibir petición de cualquier filósofo
        id_filosofo = estado.MPI_SOURCE;                                                   // Guardar en 'id_filosofo' el id. del emisor
        cout << "Tenedor " << id << " ha sido cogido por filo " << id_filosofo << endl;

        MPI_Recv(&valor, 1, MPI_INT, id_filosofo, etiq_SOLTAR, MPI_COMM_WORLD, &estado); // Recibir liberación de filósofo 'id_filosofo'
        cout << "Tenedor " << id << " ha sido liberado por filo " << id_filosofo << endl;
    }
}

// ---------------------------------------------------------------------

void funcion_camarero(){
    int peticion = 1;   // Confirmación de la petición
    // Filosofo que hace peticiones
    int cliente, etiqueta;
    MPI_Status estado;

    while (true) {				    // Los filósofos se sientan o se levantan si no llegan (ellos) al número que podrían producir bloqueo; 
        if (sentados < MAX_SENTADOS ) {             // En este caso las sillas no importan. Siempre habrá una libre que el gordito podría usar.
   	    etiqueta= MPI_ANY_TAG;
        } else { // Los filósofos unicamente pueden levantarse
	    etiqueta= etiq_LEVANTARSE;
         }

	MPI_Recv(&peticion, 1, MPI_INT, MPI_ANY_SOURCE, etiqueta, MPI_COMM_WORLD, &estado);
        cliente = estado.MPI_SOURCE;
        
            if (estado.MPI_TAG == etiq_LEVANTARSE) {
  		if (sentados == 4) {
			cout << "Camarero: El filosofo " << cliente << " se levanta. Estaban 4 filósofos sentados en la mesa." << endl;
		}
                sentados--;
		sOcupadas--;
		if (cliente == 6) {
			sOcupadas--;
		}
                
            } else if (estado.MPI_TAG == etiq_SENTARSE) {
                 sentados++;
		sOcupadas++;
		if (cliente == 6) {
			sOcupadas++;
		}
                MPI_Ssend(&peticion, 1, MPI_INT, cliente, etiq_SENTARSE, MPI_COMM_WORLD);
                cout << "Camarero: El filosofo " << cliente << " ha sido autorizado a sentarse. Hay " << sOcupadas << " sillas ocupadas en la mesa." << endl;
            }
        
    }
} // funcion_camarero

// ---------------------------------------------------------------------

int main(int argc, char ** argv){
    int id_propio, num_procesos_actual;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id_propio);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procesos_actual);


    if (num_procesos == num_procesos_actual) {
        // ejecutar la función correspondiente a 'id_propio'
        if (id_propio == ID_CAMARERO) {
            funcion_camarero();
        } else if (id_propio % 2 == 0) {  // si es par
            funcion_filosofos(id_propio); //   es un filósofo
        } else {                          // si es impar
            funcion_tenedores(id_propio); //   es un tenedor
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

// ---------------------------------------------------------------------
