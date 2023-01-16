export interface Nrf24 {
    miso: number;
    mosi: number;
    clk: number;
    irq: number;
    en: number;
    cs: number;
}

export interface Device  {
    name: string;
    nrf24: Nrf24;
}

export interface PinMapping extends Array<Device>{}