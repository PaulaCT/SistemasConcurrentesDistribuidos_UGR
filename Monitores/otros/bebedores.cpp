#include <iostream>
#include <cassert>
#include <iomanip>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <condition_variable>
#include "HoareMonitor.h"

using namespace std ;
using namespace HM ;

//g++ -std=c++11 -pthread -o bebedores bebedores.cpp HoareMonitor.cpp Semaphore.cpp

const int num_clientes = 4;
const int num_camareros = 2;


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
//Clase del monitor Bar
//**********************************************************************
class Bar : public HoareMonitor{
  private:
    int barra;
    int antiguo_cocktail;
    int camarero_actual;
    CondVar clientes[num_clientes];
    CondVar camareros[num_camareros];

  public:
    Bar();
    int elegir_cocktail(int camar);
    void servir_cocktail(int cocktail);
    void pedir_cocktail(int cocktail);
};


Bar::Bar(){
  barra = -1;
  antiguo_cocktail = -1;
  camarero_actual = (aleatorio<0,num_camareros-1>());
  for (size_t i = 0; i < num_clientes; i++) {
    clientes[i] = newCondVar();
  }
  for (size_t i = 0; i < num_camareros; i++) {
    camareros[i] = newCondVar();
  }
}



int Bar:: elegir_cocktail(int camar){
  int cocktail = (aleatorio<0,num_clientes-1>());

  while(cocktail == antiguo_cocktail){
    cocktail = (aleatorio<0,num_clientes-1>());
  }

  camarero_actual = camar;
  antiguo_cocktail = cocktail;

  cout << "El camarero " << camarero_actual << " va a preparar el cocktail " << cocktail << endl;

  return cocktail;
}




void Bar::pedir_cocktail(int cli){
  if(barra != cli){
    clientes[cli].wait();
  }
  cout << "El cliente " << cli << " recoge su cocktail" << endl;
  barra = -1;

  camareros[camarero_actual].signal();
}




void Bar::servir_cocktail(int cli){
  if(barra != -1){
    camareros[camarero_actual].wait();
  }
  barra = cli;
  cout << "El camarero  " << camarero_actual << " sirve el cocktail " << cli << endl;
  clientes[cli].signal();
}


void prepara_cocktail(){
  chrono::milliseconds duracion_espera( aleatorio<500,2000>() );
  this_thread::sleep_for(duracion_espera);
}
void beber(){
  chrono::milliseconds duracion_espera( aleatorio<500,1000>() );
  this_thread::sleep_for(duracion_espera);
}


void funcion_hebra_camarero(MRef<Bar> monitor, int camarero){
  int cocktail;
  while (true) {
    cocktail = monitor->elegir_cocktail(camarero);
    prepara_cocktail();
    monitor->servir_cocktail(cocktail);
  }
}

void funcion_hebra_bebedor(MRef<Bar> monitor, int cliente){
  while(true){
    monitor->pedir_cocktail(cliente);
    beber();
  }
}

int main(){
  MRef<Bar> monitor = Create<Bar>();

  thread hebra_camarero[num_camareros];
  thread hebra_bebedor[num_clientes];

  for (size_t i = 0; i < num_camareros; i++) {
    hebra_camarero[i] = thread(funcion_hebra_camarero, monitor, i);
  }
  for (size_t i = 0; i < num_clientes; i++) {
    hebra_bebedor[i] = thread(funcion_hebra_bebedor, monitor, i);
  }

  for (size_t i = 0; i < num_camareros; i++) {
    hebra_camarero[i].join();
  }
  for (size_t i = 0; i < num_clientes; i++) {
    hebra_bebedor[i].join();
  }
}



















//
