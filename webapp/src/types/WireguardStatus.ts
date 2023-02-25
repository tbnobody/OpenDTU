export interface WireguardStatus {
    wg_enabled: boolean;

    wg_endpoint_address : string;
    wg_endpoint_port : string;
    wg_endpoint_public_key : string;

    wg_opendtu_local_ip: string;
    wg_opendtu_public_key: string;
    wg_opendtu_allowed_ip : string;
    wg_opendtu_allowed_mask : string;
}