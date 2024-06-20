# OpenDTU-DUO-Fusion

This is a fork from the Hoymiles project [OpenDTU](https://github.com/tbnobody/OpenDTU).

[![OpenDTU Build](https://github.com/funghi999/OpenDTU-DUO-Fusion/actions/workflows/build.yml/badge.svg)](https://github.com/funghi999/OpenDTU-DUO-Fusion/actions/workflows/build.yml)
[![cpplint](https://github.com/funghi999/OpenDTU-DUO-Fusion/actions/workflows/cpplint.yml/badge.svg)](https://github.com/funghi999/OpenDTU-DUO-Fusion/actions/workflows/cpplint.yml)
[![Yarn Linting](https://github.com/funghi999/OpenDTU-DUO-Fusion/actions/workflows/yarnlint.yml/badge.svg)](https://github.com/funghi999/OpenDTU-DUO-Fusion/actions/workflows/yarnlint.yml)

## Documentation

The origin documentation of openDTU can be found [here](https://tbnobody.github.io/OpenDTU-docs/).

### Ducumentation for openDTU-DUO-Fusion

* Will comming soon!

## Highlights of OpenDTU-DUO-Fusion

This project is still under development and adds following features:

* Supports the specially designed circuit board and pre-configures the connected components (CMT2300a, NRF24l01+, display, etc.).
* Disable the power-save mode for displays

## Currently supported Inverters

| Model                | Required RF Module | DC Inputs | MPP-Tracker | AC Phases |
| ---------------------| ------------------ | --------- | ----------- | --------- |
| Hoymiles HM-300-1T   | NRF24L01+          | 1         | 1           | 1         |
| Hoymiles HM-350-1T   | NRF24L01+          | 1         | 1           | 1         |
| Hoymiles HM-400-1T   | NRF24L01+          | 1         | 1           | 1         |
| Hoymiles HM-600-2T   | NRF24L01+          | 2         | 2           | 1         |
| Hoymiles HM-700-2T   | NRF24L01+          | 2         | 2           | 1         |
| Hoymiles HM-800-2T   | NRF24L01+          | 2         | 2           | 1         |
| Hoymiles HM-1000-4T  | NRF24L01+          | 4         | 2           | 1         |
| Hoymiles HM-1200-4T  | NRF24L01+          | 4         | 2           | 1         |
| Hoymiles HM-1500-4T  | NRF24L01+          | 4         | 2           | 1         |
| Hoymiles HMS-300-1T  | CMT2300A           | 1         | 1           | 1         |
| Hoymiles HMS-350-1T  | CMT2300A           | 1         | 1           | 1         |
| Hoymiles HMS-400-1T  | CMT2300A           | 1         | 1           | 1         |
| Hoymiles HMS-450-1T  | CMT2300A           | 1         | 1           | 1         |
| Hoymiles HMS-500-1T  | CMT2300A           | 1         | 1           | 1         |
| Hoymiles HMS-600-2T  | CMT2300A           | 2         | 2           | 1         |
| Hoymiles HMS-700-2T  | CMT2300A           | 2         | 2           | 1         |
| Hoymiles HMS-800-2T  | CMT2300A           | 2         | 2           | 1         |
| Hoymiles HMS-900-2T  | CMT2300A           | 2         | 2           | 1         |
| Hoymiles HMS-1000-2T | CMT2300A           | 2         | 2           | 1         |
| Hoymiles HMS-1600-4T | CMT2300A           | 4         | 4           | 1         |
| Hoymiles HMS-1800-4T | CMT2300A           | 4         | 4           | 1         |
| Hoymiles HMS-2000-4T | CMT2300A           | 4         | 4           | 1         |
| Hoymiles HMT-1600-4T | CMT2300A           | 4         | 2           | 3         |
| Hoymiles HMT-1800-4T | CMT2300A           | 4         | 2           | 3         |
| Hoymiles HMT-2000-4T | CMT2300A           | 4         | 2           | 3         |
| Hoymiles HMT-1800-6T | CMT2300A           | 6         | 3           | 3         |
| Hoymiles HMT-2250-6T | CMT2300A           | 6         | 3           | 3         |
| Solenso SOL-H350     | NRF24L01+          | 1         | 1           | 1         |
| Solenso SOL-H400     | NRF24L01+          | 1         | 1           | 1         |
| Solenso SOL-H800     | NRF24L01+          | 2         | 2           | 1         |
| TSUN TSOL-M350       | NRF24L01+          | 1         | 1           | 1         |
| TSUN TSOL-M800       | NRF24L01+          | 2         | 2           | 1         |
| TSUN TSOL-M1600      | NRF24L01+          | 4         | 2           | 1         |
| E-Star HERF-800      | NRF24L01+          | 2         | 2           | 1         |
| E-Star HERF-1600     | NRF24L01+          | 4         | 2           | 1         |
| E-Star HERF-1800     | NRF24L01+          | 4         | 2           | 1         |


## Outline

A very special thank you to Thomas Basler (tbnobody), the author of the original [OpenDTU](https://github.com/tbnobody/OpenDTU)  project. You are doing an excellent job!
