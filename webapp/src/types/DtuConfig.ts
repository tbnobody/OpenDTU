export interface DtuConfig {
    serial: number;
    pollinterval: number;
    verbose_logging: boolean;
    nrf_enabled: boolean;
    nrf_palevel: number;
    cmt_enabled: boolean;
    cmt_palevel: number;
    cmt_frequency: number;
}