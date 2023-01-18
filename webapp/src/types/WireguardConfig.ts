export interface WireguardConfig {
    wg_enabled: boolean;

    wg_endpoint_address : string;
    wg_endpoint_local_ip : string;
    wg_endpoint_port : string;
    wg_endpoint_public_key : string;

    wg_opendtu_local_ip: string;
    wg_opendtu_public_key: string;
    wg_opendtu_private_key: string;

}