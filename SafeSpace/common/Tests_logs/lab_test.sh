#!/bin/bash

# Script para probar LogManager en laboratorio

echo "=== CONFIGURACIÓN DE LOGMANAGER PARA LABORATORIO ==="

# 1. Configurar variables de entorno para logging distribuido
# ⚠️ CAMBIAR ESTAS IPs POR LAS REALES DEL LABORATORIO ⚠️

# AuthNode envía logs al ProxyNode
export PROXY_LOG_IP="192.168.1.200"   # IP real del PC que ejecuta ProxyNode
export PROXY_LOG_PORT="8081"          # Puerto para logs en proxy

# ProxyNode reenvía logs al Master  
export MASTER_LOG_IP="192.168.1.100"  # IP real del PC que ejecuta Master
export MASTER_LOG_PORT="9090"         # Puerto del master

# Mostrar la IP actual de esta máquina
echo "IP actual de esta máquina:"
hostname -I

echo "Variables configuradas:"
echo "PROXY_LOG_IP=$PROXY_LOG_IP"
echo "PROXY_LOG_PORT=$PROXY_LOG_PORT"
echo "MASTER_LOG_IP=$MASTER_LOG_IP"
echo "MASTER_LOG_PORT=$MASTER_LOG_PORT"

# 2. Compilar el servidor
echo "Compilando servidor..."
cd /home/rolbin/Escritorio/CI-0123-PIRS-incomprendidos/SafeSpace/server/build
make clean && make -j4

if [ $? -eq 0 ]; then
    echo "✅ Compilación exitosa"
else
    echo "❌ Error en compilación"
    exit 1
fi

# 3. Iniciar servidor AuthUDP
echo "Iniciando servidor AuthUDP en puerto 8080..."
./server server 8080 &
SERVER_PID=$!

echo "Servidor iniciado con PID: $SERVER_PID"
echo "Flujo de logs: AuthNode → ProxyNode → Master → CriticalEvents"
echo "Auth logs van a: $PROXY_LOG_IP:$PROXY_LOG_PORT"
echo "Proxy reenvía a: $MASTER_LOG_IP:$MASTER_LOG_PORT"

# 4. Para detener el servidor más tarde:
echo "Para detener: kill $SERVER_PID"
echo ""
echo "🚀 SERVIDOR LISTO PARA LABORATORIO"
echo "   - AuthUDPServer escuchando en puerto 8080"
echo "   - Logs enviándose automáticamente al master"
echo "   - Usa clientes para probar autenticación"