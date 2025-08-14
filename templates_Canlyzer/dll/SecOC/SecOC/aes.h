/**
* File: cmac.h
* Created on: 21 авг. 2015 г.
* Description:
*
*
* Author: Roman Savrulin <romeo.deepmind@gmail.com>
* Copyright: 2015 Roman Savrulin
* Copying permission statement:
*
*  This file is part of AES-CMAC.
*
*  AES-CMAC is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*
*/

#include <stdint.h>
#ifndef AES_CBC_CMAC_H_
#define AES_CBC_CMAC_H_

#ifdef __cplusplus
extern "C" {
#endif

#define BLOCK_SIZE 16
#define LAST_INDEX (BLOCK_SIZE - 1)

	int AES_CBC_ENC(const uint8_t *IV, const uint8_t *key,
		const uint8_t *input, int inputLength, uint8_t *output,
		int outputLength);

	int AES_CBC_DEC(const uint8_t *IV, const uint8_t *key,
		const uint8_t *input, int inputLength, uint8_t *output,
		int outputLength);

	void AES_CMAC(const uint8_t *key, const uint8_t *input, int length,
		uint8_t *mac);

	int AES_CMAC_CHECK(const uint8_t *key, const uint8_t *input,
		int length, const uint8_t *mac);

	void xor_128(const uint8_t *a, const uint8_t *b, uint8_t *out);
	void AES_128_DEC(const uint8_t *key, const uint8_t* msg, uint8_t *cipher);

	unsigned long long CAPLEXPORT far CAPLPASCAL calcAuth(unsigned char buf[], unsigned char key[], unsigned long length, unsigned char nMsb, unsigned short secOcDataId, unsigned long long cfv, unsigned short offset, unsigned short nSecBytes);
	uint64_t calcAuthAdd(uint8_t* buf, uint8_t key[], uint32_t length, uint8_t nMsb, uint16_t secOcDataId, uint64_t cfv, uint8_t offset, uint8_t nSecBytes, uint8_t *addData, uint8_t addLength);
	uint8_t getPduBlockLen(uint32_t length);
	uint8_t* aesCbcCompleteMac(uint8_t* buf, uint8_t key[], uint32_t length, uint16_t secOcDataId, uint64_t cfv, uint8_t nSecBytes=8);
	long CAPLEXPORT far CAPLPASCAL SyncMessageGenerate(long SyncMessageDataId, unsigned long Trip_value, long Reset_value, unsigned char key[], long index, unsigned long length = 7);
	unsigned long long CAPLEXPORT far CAPLPASCAL GenerateCompleteFreshValue(unsigned long init_Msg_count, unsigned long Trip_value, unsigned short Reset_value, unsigned char Low_Reset_value, unsigned char CFV_LEN);
	
	void print_hex(const unsigned char *str, uint8_t *buf, int len);
	void print128(const uint8_t *bytes);
	void print96(const uint8_t *bytes);


#ifdef __cplusplus
}
#endif

#endif /* AES_CBC_CMAC_H_ */
