export interface NetworkConfig {
    ssid: string;
    password: string;
    hostname: string;
    dhcp: boolean;
    ipaddress: string;
    netmask: string;
    gateway: string;
    dns1: string;
    dns2: string;
    aptimeout: number;
    rescanenabled: boolean;
    rescaninterval: number;
    rescanthreshold: number;
    mdnsenabled: boolean;
    syslogenabled: boolean;
    sysloghostname: string;
    syslogport: number;
}
