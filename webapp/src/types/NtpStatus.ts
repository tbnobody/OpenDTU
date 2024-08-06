export interface NtpStatus {
    ntp_server: string;
    ntp_timezone: string;
    ntp_timezone_descr: string;
    ntp_status: boolean;
    ntp_localtime: string;
    sun_risetime: string;
    sun_settime: string;
    sun_isDayPeriod: boolean;
    sun_isSunsetAvailable: boolean;
}
