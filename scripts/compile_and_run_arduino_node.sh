#!/usr/bin/env bash
set -euo pipefail

# Small helper to compile and run Arduino_Node.cpp
# Usage: ./compile_and_run_arduino_node.sh MASTER_IP PORT [SERIAL_PATH|stdin]

PROJ_DIR="/home/YiYhuan/Desktop/CI-0123-PIRS-incomprendidos"
SRC="$PROJ_DIR/SafeSpace/server/src/nodes/Arduino/Arduino_Node.cpp"
OUT="/tmp/Arduino_Node_test"
CXXFLAGS="-std=c++17 -Wall -Wextra -O2 -pthread"

echo "Compilando $SRC ..."
g++ $CXXFLAGS "$SRC" -o "$OUT"

echo "Compilado: $OUT"

if [ "$#" -lt 2 ]; then
  cat <<EOF
Uso:
  $0 MASTER_IP PORT [SERIAL_PATH|stdin]

Ejemplos:
  # Ejecutar con Arduino conectado en /dev/ttyACM0
  $0 192.168.1.10 9999 /dev/ttyACM0

  # Probar sin hardware (lee JSON desde stdin)
  echo '{"temp":23.45,"humidity":56.78}' | $0 127.0.0.1 9999 stdin
EOF
  exit 1
fi

MASTER_IP="$1"
PORT="$2"
SERIAL="${3:-/dev/ttyACM0}"

echo "Ejecutando: $OUT $MASTER_IP $PORT $SERIAL"
if [ "$SERIAL" = "stdin" ]; then
  # Ejecuta el binario leyendo desde stdin (útil para pruebas)
  exec "$OUT" "$MASTER_IP" "$PORT" stdin
else
  # Ejecuta usando el dispositivo serial indicado
  exec "$OUT" "$MASTER_IP" "$PORT" "$SERIAL"
fi
