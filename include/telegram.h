#ifndef TELEGRAM_H
#define TELEGRAM_H

bool enviarTelegram(String chatId, String texto);
void manejarTelegram();
void procesarTelegram();
void enviarATodos(const String mensaje);

#endif