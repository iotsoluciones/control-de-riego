#ifndef USUARIOS_H
#define USUARIOS_H
#include <Arduino.h>

bool esAdmin(String id);
void guardarUsuarios();
bool usuarioAutorizado(String id);
String obtenerNombreUsuario(String id);

#endif