export interface ModbusStatus {
    modbus_tcp_enabled: boolean;
    modbus_hostname: string;
    modbus_port: number;
    modbus_clients: number;
    modbus_ip: string;
    modbus_id_dtupro: number;
    modbus_id_total: number;
    modbus_id_meter: number;
}
