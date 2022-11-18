export interface NtpConfig {
    ntp_server: string;
    ntp_timezone: string;
    ntp_timezone_descr: string;
    sunset_enabled: boolean;
    longitude: string;
    latitude: string;
    sunrise_offset: number;
    sunset_offset: number;
}