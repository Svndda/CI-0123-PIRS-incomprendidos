# Arquitectura de Logging - SafeSpace

## Flujo de Logs
```
AuthNode → ProxyNode → Master → CriticalEventsNode
```

## Configuración por Nodo

### AuthNode (PC1)
```bash
export PROXY_LOG_IP="192.168.1.200"   # IP del ProxyNode
export PROXY_LOG_PORT="8081"          # Puerto para logs
./server server 8080                  # AuthUDPServer
```

### ProxyNode (PC2) 
```bash
export MASTER_LOG_IP="192.168.1.100"  # IP del Master
export MASTER_LOG_PORT="9090"         # Puerto del master
./server proxy 8081 192.168.1.X 8080  # ProxyNode
```

### Master (PC3)
```bash
python3 master_receiver.py 9090       # Recibe y procesa logs
```

### CriticalEventsNode (PC4)
```bash
# Recibe logs del Master según implementación del compañero
```

## Script de Laboratorio
Usar `./lab_test.sh` después de configurar las IPs reales.

## Protocolo UDP
- Header: "LOG" + nivel + tamaño_nodo
- Payload: timestamp + mensaje
- Identificación automática por nodo