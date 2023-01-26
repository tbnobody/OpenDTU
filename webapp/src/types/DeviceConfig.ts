import type { Device } from "./PinMapping";

export interface Display {
    show_logo: boolean;
    power_safe: boolean;
    screensaver: boolean;
    contrast: number;
}

export interface DeviceConfig {
    curPin: Device;
    display: Display;
}