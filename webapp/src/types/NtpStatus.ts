export interface NtpStatus {
    ntp_server: string;
    ntp_timezone: string;
    ntp_timezone_descr: string;
    ntp_status: boolean;
    ntp_localtime: string;
    sunset_enabled: boolean;
    timezone_offset: string;
    sunrise_time: string;
    sunset_time: string;
    sunset_isdaytime: boolean;
}