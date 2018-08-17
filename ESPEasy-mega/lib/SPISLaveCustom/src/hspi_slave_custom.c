/*
  SPISlave library for esp8266

  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the esp8266 core for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <hspi_slave_custom.h>
#include <esp8266_peri.h>
#include <ets_sys.h>


void hspi_slave_read_data(uint8_t *result)
{
            uint8_t i;
            uint32_t data;
            uint8_t buffer[1];

            result[32] = 0;

            for(i=0; i<8; i++) {
                data=SPI1W(i);
                result[i<<2] = data & 0xff;
                result[(i<<2)+1] = (data >> 8) & 0xff;
                result[(i<<2)+2] = (data >> 16) & 0xff;
                result[(i<<2)+3] = (data >> 24) & 0xff;
            }

}

void hspi_slave_begin(uint8_t status_len)
{
    status_len &= 7;
    if(status_len > 4) {
        status_len = 4;    //max 32 bits
    }
    if(status_len == 0) {
        status_len = 1;    //min 8 bits
    }

    pinMode(SS, SPECIAL);
    pinMode(SCK, SPECIAL);
    pinMode(MISO, SPECIAL);
    pinMode(MOSI, SPECIAL);

    SPI1S = SPISE | SPISBE | 0x3E0;
    SPI1U = SPIUMISOH | SPIUCOMMAND | SPIUSSE;
    SPI1CLK = 0;
    SPI1U2 = (7 << SPILCOMMAND);
    SPI1S1 = (((status_len * 8) - 1) << SPIS1LSTA) | (0xff << SPIS1LBUF) | (7 << SPIS1LWBA) | (7 << SPIS1LRBA) | SPIS1RSTA;
    SPI1P = (1 << 19);
    SPI1CMD = SPIBUSY;
}
