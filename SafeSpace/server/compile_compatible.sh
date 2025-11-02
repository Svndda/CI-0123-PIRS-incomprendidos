#!/bin/bash
cd "$(dirname "$0")"

# Crear directorios si no existen
mkdir -p bin
mkdir -p build

echo "🔨 Compilando IntermediaryNode compatible con ArduinoNode extendido..."
g++ -std=c++17 -Wall -Wextra -O2 -pthread -Isrc/model/structures -Isrc/nodes/Intermediary \
    src/nodes/Intermediary/IntermediaryNode.cpp \
    src/model/structures/sensordata.cpp \
    src/main.cpp \
    -o bin/IntermediaryNodeApp

if [ $? -eq 0 ]; then
    echo "✅ Compilación exitosa!"
    echo ""
    echo "🎯 Ejecutar con:"
    echo "   bin/IntermediaryNodeApp"
    echo "   bin/IntermediaryNodeApp <PUERTO> <MASTER_IP> <MASTER_PUERTO>"
    echo ""
    echo "📋 Compatible con ArduinoNode en formato binary"
else
    echo "❌ Error en la compilación"
    exit 1
fi