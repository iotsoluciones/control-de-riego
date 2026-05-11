#include "usuarios.h"
#include "variables.h"

bool esAdmin(String id){

  return id == CHAT_ID;
}

void guardarUsuarios(){

prefs.begin("users", false);

prefs.putInt("cant", cantidadUsuarios);

for(int i=0;i<MAX_USERS;i++){

    prefs.remove(("id"+String(i)).c_str());
    prefs.remove(("nom"+String(i)).c_str());
}

for(int i=0;i<cantidadUsuarios;i++){
  prefs.putString(("id"+String(i)).c_str(), usuariosID[i]);
  prefs.putString(("nom"+String(i)).c_str(), usuariosNombre[i]);
}

prefs.end();
}

bool usuarioAutorizado(String chat_id){

    if(chat_id == CHAT_ID){
        return true;
    }

    for(int i=0; i<cantidadUsuarios; i++){

        if(usuariosID[i] == chat_id){
            return true;
        }
    }

    return false;
}

String obtenerNombreUsuario(String id){

  if(id == CHAT_ID) return "ADMIN";

  for(int i=0;i<cantidadUsuarios;i++){
    if(usuariosID[i] == id){
      return usuariosNombre[i];
    }
  }

  return "Desconocido";
}

