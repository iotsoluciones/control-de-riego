#include <Arduino.h>
#ifndef RELES_H
#define RELES_H
#include "historial.h"

void controlarBomba();
void controlarTanque();
void apagarTodosReles();
void ejecutarRele(int r, String origen, String usuario, String chat_id);
void iniciarReles();

#endif // RELES_H