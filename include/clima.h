#ifndef CLIMA_H
#define CLIMA_H


void consultarClima();
bool buscarCoordenadasOSM(String ciudadBusqueda);
bool buscarCoordenadas(String ciudadBusqueda);
bool obtenerCiudadPorCoordenadas(float lat, float lon);

#endif