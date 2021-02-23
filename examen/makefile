# Uso:	make ej -> ejemplo
# 
# 	sustituir ej por abreviatura
#	sustituir ejemplo por nombre del cpp
#

.SUFFIXES:
.PHONY: pc
.PRECIOUS: %_exe

compilador:=g++

#
# Semáforos (práctica 1)
#

ej: ejemplo_s_exe 
	./ejemplo_s_exe

%_s_exe: %.cpp Semaphore.cpp Semaphore.h
	$(compilador) -std=c++11 -pthread -o $@ $< Semaphore.cpp


#
# Monitores (práctica 2)
#

ej: ejemplo_m_exe 
	./ejemplo_m_exe

%_m_exe: %.cpp Semaphore.cpp Semaphore.h HoareMonitor.cpp HoareMonitor.h
	$(compilador) -std=c++11 -pthread -o $@ $< Semaphore.cpp HoareMonitor.cpp


#
# Paso de mensajes (práctica 3)
#

ej: ejemplo_pm_exe
	mpirun -np 11 ./ejemplo_pm_exe

%_pm_exe: %.cpp
	$(compilador) $(flagsc) -o $@ $<

#
# Tiempo real (práctica 4)
#

ej: ejemplo_t_exe
	./ejemplo_t_exe

%_t_exe: %.cpp
	$(compilador) $(flagsc) -o $@ $<

clean:
	rm -f *_exe
