import type { Device } from "./PinMapping";

export interface Display {
    rotation: number;
    power_safe: boolean;
    screensaver: boolean;
    contrast: number;
    language: number;
}

export interface Leds {
    led0bri: number;
    led1bri: number;
}

export interface DeviceConfig {
    curPin: Device;
    display: Display;
    leds: Leds
}