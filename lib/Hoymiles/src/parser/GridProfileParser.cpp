// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "GridProfileParser.h"
#include "../Hoymiles.h"
#include <cstring>
#include <iostream>

GridProfileParser::GridProfileParser()
    : Parser()
{
    clearBuffer();
}

void GridProfileParser::clearBuffer()
{
    memset(_payloadGridProfile, 0, GRID_PROFILE_SIZE);
    _gridProfileLength = 0;
}

void GridProfileParser::appendFragment(uint8_t offset, uint8_t* payload, uint8_t len)
{
    if (offset + len > GRID_PROFILE_SIZE) {
        Hoymiles.getMessageOutput()->printf("FATAL: (%s, %d) grid profile packet too large for buffer\r\n", __FILE__, __LINE__);
        return;
    }
    memcpy(&_payloadGridProfile[offset], payload, len);
    _gridProfileLength += len;
}

std::vector<uint8_t> GridProfileParser::getRawData()
{
    std::vector<uint8_t> ret;
    HOY_SEMAPHORE_TAKE();
    for (uint8_t i = 0; i < GRID_PROFILE_SIZE; i++) {
        ret.push_back(_payloadGridProfile[i]);
    }
    HOY_SEMAPHORE_GIVE();
    return ret;
}


int GridProfileParser::modbusCrc(std::string msg) {
    int crc = 0xFFFF;
    for (int n = 0; n < msg.length(); n++) {
        crc ^= msg[n];
        for (int i = 0; i < 8; i++) {
            if (crc & 1) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

std::string GridProfileParser::parseGridProfile(std::string hex_string) {

    std::vector<unsigned char> binary_string;
    for (int i = 0; i < hex_string.length(); i += 2) {
        std::string byte_string = hex_string.substr(i, 2);
        unsigned char byte = std::stoi(byte_string, nullptr, 16);
        binary_string.push_back(byte);
    }
    int binary_length = binary_string.size();

    int str_header1 = binary_string[0];
    int str_header2 = binary_string[1];
    int str_version1 = binary_string[2];
    int str_version2 = binary_string[3];

    try {
        std::cout << "Grid Profile: " << profile_types[str_header1][str_header2] << std::endl;
    } catch (...) {
        std::cout << "Grid Profile: unknown" << std::endl;
    }

    std::cout << "Version: " << ((str_version1 >> 4) & 0x0F) << "." << (str_version1 & 0x0F) << "." << str_version2 << std::endl;

    int position = 4;
    while (position < binary_length) {
        int str_table_n = binary_string[position];
        int str_table_v = binary_string[position + 1];

        try {
            std::cout << "Table Type: " << profile_structs[str_table_n] << std::endl;
        } catch (...) {
            // pass
        }

        try {
            std::vector<std::vector<std::string>> tables_diz = profile_details[str_table_n][str_table_v];
            int table_length = tables_diz.size();

            position += 2;
            for (int x = 0; x < table_length; x++) {
                std::vector<std::string> table_diz = tables_diz[x];
                int str_int = (binary_string[position] << 8) | binary_string[position + 1];
                float str_val = str_int / std::stof(table_diz[2]);

                std::cout << "position: " << position << "\t: " << std::hex << str_int << "\t" << std::dec << str_val << "\t[" << table_diz[1] << "]\t\t[" << table_diz[0] << "]" << std::endl;
                position += 2;
            }
        } catch (...) {
            std::string crc = std::to_string(binary_string[position]) + std::to_string(binary_string[position + 1]);
            int crc2 = modbusCrc(hex_string.substr(0, position));
            std::string crcc = std::to_string(crc2);

            if (crc == crcc) {
                std::cout << "CRC (ok): " << crcc << std::endl;
            } else {
                std::cout << "CRC (?): " << crc << std::endl;
                std::cout << "CRC calced: " << crcc << std::endl;
                std::cout << " - possible unknown table (module)" << std::endl;
            }

            std::cout << "end" << std::endl;
            break;
        }
    }

    return 0;
}