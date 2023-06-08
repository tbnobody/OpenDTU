export interface NetworkStatus {
    // WifiStationInfo
    sta_status: boolean;
    sta_ssid: string;
    sta_bssid: string;
    sta_rssi: number;
    // WifiApInfo
    ap_status: boolean;
    ap_ssid: string;
    ap_stationnum: number;
    // InterfaceNetworkInfo
    network_hostname: string;
    network_ip: string;
    network_netmask: string;
    network_gateway: string;
    network_dns1: string;
    network_dns2: string;
    network_mac: string;
    network_mode: string;
    // InterfaceApInfo
    ap_ip: string;
    ap_mac: string;
}