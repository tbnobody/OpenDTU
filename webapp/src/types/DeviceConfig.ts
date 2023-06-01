import type { Device } from "./PinMapping";

export interface Display {
    rotation: number;
    power_safe: boolean;
    screensaver: boolean;
    contrast: number;
    language: number;
}

export interface DeviceConfig {
    curPin: Device;
    display: Display;
}