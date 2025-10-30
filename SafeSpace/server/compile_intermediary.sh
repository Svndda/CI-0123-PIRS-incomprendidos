#!/bin/bash
cd "$(dirname "$0")"

# Crear directorio bin si no existe
mkdir -p bin

echo "Compilando Intermediary_Node..."
g++ -std=c++17 -Wall -Wextra -O2 -pthread -Isrc/model/structures \
    src/nodes/Intermediary/Intermediary_Node.cpp \
    src/model/structures/sensordata.cpp \
    -o bin/Intermediary_Node

if [ $? -eq 0 ]; then
    echo " Compilación exitosa!"
    echo ""
    echo "Ejecutar con:"
    echo "  bin/Intermediary_Node <PUERTO_ESCUCHA> <MASTER_IP> <MASTER_PUERTO>"
    echo ""
    echo "Ejemplo:"
    echo "  bin/Intermediary_Node 9999 127.0.0.1 8888"
else
    echo " Error en la compilación"
    exit 1
fi