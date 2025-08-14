/*----------------------------------------------------------------------------
|
| File Name: capldll.cpp
|
|            Example of a capl DLL implementation module and using CAPLLbacks.
|-----------------------------------------------------------------------------
|               A U T H O R   I D E N T I T Y
|-----------------------------------------------------------------------------
|   Author             Initials
|   ------             --------
|   Thomas  Riegraf    Ri              Vector Informatik GmbH
|   Hans    Quecke     Qu              Vector Informatik GmbH
|-----------------------------------------------------------------------------
|               R E V I S I O N   H I S T O R Y
|-----------------------------------------------------------------------------
| Date         Ver  Author  Description
| ----------   ---  ------  --------------------------------------------------
| 2003-10-07   1.0  As      Created
| 2007-03-26   1.1  Ej      Export of the DLL function table as variable
|                           Use of CAPL_DLL_INFO3
|                           Support of long name CAPL function calls
| 2017-08-20   2.0  Jason   Adapt for CAPL DLL demo
|-----------------------------------------------------------------------------
|               C O P Y R I G H T
|-----------------------------------------------------------------------------
| Copyright (c) 1994 - 2003 by Vector Informatik GmbH.  All rights reserved.
----------------------------------------------------------------------------*/


#define USECDLL_FEATURE
#define _BUILDNODELAYERDLL

#pragma warning( disable : 4786 )

#include "..\Includes\cdll.h"
#include "..\Includes\via.h"
#include "..\Includes\via_CDLL.h"

#include <stdio.h>
#include <stdlib.h>
#include <map>

#include "aes.h"
#include "TI_aes_128.h"

class CaplInstanceData;
typedef std::map<uint32, CaplInstanceData*> VCaplMap;
typedef std::map<uint32, VIACapl*> VServiceMap;

#define MAX_PDU_LENGTH 64   // The maximum PDU length in [byte] observed in the project
#define DATA_ID_LENGTH 2    // Length of the data id in [byte] for SecOc in the project (source: Specification of Module Secure Onboard Communication Release 4.2.2 chapter 7.1.1.2)
#define CFV_LENGTH 8    // Number of bytes for the Complete Freshness Value based on the wish of Igor Gall
#define AES_BLOCKLEN 16 //Block length in bytes AES is 128b block only

// ============================================================================
// global variables
// ============================================================================

static unsigned long data = 0;
static char dlldata[100];

char        gModuleName[_MAX_FNAME];  // filename of this  dll 
HINSTANCE   gModuleHandle;            // windows instance handle of this DLL
VCaplMap    gCaplMap;
VServiceMap gServiceMap;

template <class T>
int getArraylen(T& array)
{
	return (sizeof(array)/sizeof(array[0]));
}

// ============================================================================
// CaplInstanceData
// 
// Data local for a single CAPL Block. 
//
// A CAPL-DLL can be used by more than one CAPL-Block, so every piece of 
// information thats like a globale variable in CAPL, must now be wraped into 
// an instance of an object.
// ============================================================================
class CaplInstanceData
{
public:
	CaplInstanceData(VIACapl* capl);

	void GetCallbackFunctions();
	void ReleaseCallbackFunctions();

	// Definition of the class function. 
	// This class function will call the 
	// CAPL callback functions 
	uint32 ShowValue(uint32 x);
	uint32 ShowDates(int16 x, uint32 y, int16 z);
	void   DllInfo(char* x);
	void   ArrayValues(uint32 flags, uint32 numberOfDatabytes, uint8 databytes[], uint8 controlcode);
	void DllVersion(char* y);

private:

	// Pointer of the CAPL callback functions 
	VIACaplFunction*  mShowValue;
	VIACaplFunction*  mShowDates;
	VIACaplFunction*  mDllInfo;
	VIACaplFunction*  mArrayValues;
	VIACaplFunction*  mDllVersion;

	VIACapl*          mCapl;
};


CaplInstanceData::CaplInstanceData(VIACapl* capl)
// This function will initialize the CAPL callback function 
// with the NLL Pointer 
	: mCapl(capl),
	mShowValue(NULL),
	mShowDates(NULL),
	mDllInfo(NULL),
	mArrayValues(NULL),
	mDllVersion(NULL)
{}

static bool sCheckParams(VIACaplFunction* f, char rtype, char* ptype)
{
	char      type;
	int32     pcount;
	VIAResult rc;

	// check return type
	rc = f->ResultType(&type);
	if (rc != kVIA_OK || type != rtype)
	{
		return false;
	}

	// check number of parameters
	rc = f->ParamCount(&pcount);
	if (rc != kVIA_OK || strlen(ptype) != pcount)
	{
		return false;
	}

	// check type of parameters
	for (int i = 0; i<pcount; ++i)
	{
		rc = f->ParamType(&type, i);
		if (rc != kVIA_OK || type != ptype[i])
		{
			return false;
		}
	}

	return true;
}

static VIACaplFunction* sGetCaplFunc(VIACapl* capl, const char * fname, char rtype, char* ptype)
{
	VIACaplFunction* f;

	// get capl function object
	VIAResult rc = capl->GetCaplFunction(&f, fname);
	if (rc != kVIA_OK || f == NULL)
	{
		return NULL;
	}

	// check signature of function
	if (sCheckParams(f, rtype, ptype))
	{
		return f;
	}
	else
	{
		capl->ReleaseCaplFunction(f);
		return NULL;
	}
}

void CaplInstanceData::GetCallbackFunctions()
{
	// Get a CAPL function handle. The handle stays valid until end of
	// measurement or a call of ReleaseCaplFunction. 
	mShowValue = sGetCaplFunc(mCapl, "CALLBACK_ShowValue", 'D', "D");
	mShowDates = sGetCaplFunc(mCapl, "CALLBACK_ShowDates", 'D', "IDI");
	mDllInfo = sGetCaplFunc(mCapl, "CALLBACK_DllInfo", 'V', "C");
	mArrayValues = sGetCaplFunc(mCapl, "CALLBACK_ArrayValues", 'V', "DBB");
	mDllVersion = sGetCaplFunc(mCapl, "CALLBACK_DllVersion", 'V', "C");
}

void CaplInstanceData::ReleaseCallbackFunctions()
{
	// Release all the requested Callback functions 
	mCapl->ReleaseCaplFunction(mShowValue);
	mShowValue = NULL;
	mCapl->ReleaseCaplFunction(mShowDates);
	mShowDates = NULL;
	mCapl->ReleaseCaplFunction(mDllInfo);
	mDllInfo = NULL;
	mCapl->ReleaseCaplFunction(mArrayValues);
	mArrayValues = NULL;
	mCapl->ReleaseCaplFunction(mDllVersion);
	mDllVersion = NULL;
}

void CaplInstanceData::DllVersion(char* y)
{
	// Prepare the parameters for the call stack of CAPL. 
	// Arrays uses a 8 byte on the stack, 4 Bytes for the number of element,
	// and 4 bytes for the pointer to the array
	int32 sizeX = strlen(y) + 1;

	uint8 params[8];               // parameters for call stack, 8 Bytes total    
	memcpy(params + 0, &sizeX, 4);   // array size    of first parameter, 4 Bytes
	memcpy(params + 4, &y, 4);   // array pointer of first parameter, 4 Bytes

	if (mDllVersion != NULL)
	{
		uint32 result; // dummy variable
		VIAResult rc = mDllVersion->Call(&result, params);
	}
}


uint32 CaplInstanceData::ShowValue(uint32 x)
{
	void* params = &x;   // parameters for call stack

	uint32 result;

	if (mShowValue != NULL)
	{
		VIAResult rc = mShowValue->Call(&result, params);
		if (rc == kVIA_OK)
		{
			return result;
		}
	}
	return -1;
}

uint32 CaplInstanceData::ShowDates(int16 x, uint32 y, int16 z)
{
	// Prepare the parameters for the call stack of CAPL. The stack grows
	// from top to down, so the first parameter in the parameter list is the last 
	// one in memory. CAPL uses also a 32 bit alignment for the parameters. 
	uint8 params[12];         // parameters for call stack, 12 Bytes total    
	memcpy(params + 0, &z, 2);  // third  parameter, offset 0, 2 Bytes
	memcpy(params + 4, &y, 4);  // second parameter, offset 4, 4 Bytes
	memcpy(params + 8, &x, 2);  // first  parameter, offset 8, 2 Bytes 

	uint32 result;

	if (mShowDates != NULL)
	{
		VIAResult rc = mShowDates->Call(&result, params);
		if (rc == kVIA_OK)
		{
			return rc;   // call successful
		}
	}

	return -1; // call failed
}

void CaplInstanceData::DllInfo(char* x)
{
	// Prepare the parameters for the call stack of CAPL. 
	// Arrays uses a 8 byte on the stack, 4 Bytes for the number of element,
	// and 4 bytes for the pointer to the array
	int32 sizeX = strlen(x) + 1;

	uint8 params[8];               // parameters for call stack, 8 Bytes total    
	memcpy(params + 0, &sizeX, 4);   // array size    of first parameter, 4 Bytes
	memcpy(params + 4, &x, 4);   // array pointer of first parameter, 4 Bytes

	if (mDllInfo != NULL)
	{
		uint32 result; // dummy variable
		VIAResult rc = mDllInfo->Call(&result, params);
	}
}

void CaplInstanceData::ArrayValues(uint32 flags, uint32 numberOfDatabytes, uint8 databytes[], uint8 controlcode)
{
	// Prepare the parameters for the call stack of CAPL. The stack grows
	// from top to down, so the first parameter in the parameter list is the last 
	// one in memory. CAPL uses also a 32 bit alignment for the parameters.
	// Arrays uses a 8 byte on the stack, 4 Bytes for the number of element,
	// and 4 bytes for the pointer to the array

	uint8 params[16];                           // parameters for call stack, 16 Bytes total    
	memcpy(params + 0, &controlcode, 1);   // third parameter,                  offset  0, 1 Bytes
	memcpy(params + 4, &numberOfDatabytes, 4);   // second parameter (array size),    offset  4, 4 Bytes
	memcpy(params + 8, &databytes, 4);   // second parameter (array pointer), offset  8, 4 Bytes
	memcpy(params + 12, &flags, 4);   // first  parameter,                 offset 12, 4 Bytes 

	if (mArrayValues != NULL)
	{
		uint32 result; // dummy variable
		VIAResult rc = mArrayValues->Call(&result, params);
	}

}

CaplInstanceData* GetCaplInstanceData(uint32 handle)
{
	VCaplMap::iterator lSearchResult(gCaplMap.find(handle));
	if (gCaplMap.end() == lSearchResult)
	{
		return NULL;
	}
	else {
		return lSearchResult->second;
	}
}

// ============================================================================
// CaplInstanceData
// 
// Data local for a single CAPL Block. 
//
// A CAPL-DLL can be used by more than one CAPL-Block, so every piece of 
// information thats like a global variable in CAPL, must now be wrapped into 
// an instance of an object.
// ============================================================================

void CAPLEXPORT far CAPLPASCAL appInit(uint32 handle)
{
	CaplInstanceData* instance = GetCaplInstanceData(handle);
	if (NULL == instance)
	{
		VServiceMap::iterator lSearchService(gServiceMap.find(handle));
		if (gServiceMap.end() != lSearchService)
		{
			VIACapl* service = lSearchService->second;
			try
			{
				instance = new CaplInstanceData(service);
			}
			catch (std::bad_alloc&)
			{
				return; // proceed without change
			}
			instance->GetCallbackFunctions();
			gCaplMap[handle] = instance;
		}
	}
}

void CAPLEXPORT far CAPLPASCAL appEnd(uint32 handle)
{
	CaplInstanceData* inst = GetCaplInstanceData(handle);
	if (inst == NULL)
	{
		return;
	}
	inst->ReleaseCallbackFunctions();

	delete inst;
	inst = NULL;
	gCaplMap.erase(handle);
}


// ============================================================================
// VIARegisterCDLL
// ============================================================================

VIACLIENT(void) VIARegisterCDLL(VIACapl* service)
{
	uint32    handle;
	VIAResult result;

	if (service == NULL)
	{
		return;
	}

	result = service->GetCaplHandle(&handle);
	if (result != kVIA_OK)
	{
		return;
	}

	// appInit (internal) resp. "DllInit" (CAPL code) has to follow
	gServiceMap[handle] = service;
}

void ClearAll()
{
	// destroy objects created by this DLL
	// may result from forgotten DllEnd calls
	VCaplMap::iterator lIter = gCaplMap.begin();
	const long cNumberOfEntries = gCaplMap.size();
	long i = 0;
	while (lIter != gCaplMap.end() && i<cNumberOfEntries)
	{
		appEnd((*lIter).first);
		lIter = gCaplMap.begin(); // first element should have vanished
		i++; // assure that no more erase trials take place than the original size of the map
	}

	// just for clarity (would be done automatically)
	gCaplMap.clear();
	gServiceMap.clear();
}

// SecOC
void print_hex(const unsigned char *str, uint8_t *buf, int len) {
	/* @brief Print input buffer in hex-format
	*/
	int i;

	for (i = 0; i < len; i++) {
		if ((i % BLOCK_SIZE) == 0 && i != 0)
			printf("%s", str);
		printf("%02x", buf[i]);
		if ((i % 4) == 3)
			printf(" ");
		if ((i % BLOCK_SIZE) == LAST_INDEX)
			printf("\n");
	}
	if ((i % BLOCK_SIZE) != 0)
		printf("\n");
}
void print128(const uint8_t *bytes) {
	/* @brief Print bytes in hex format.
	*/
	int j;
	for (j = 0; j < BLOCK_SIZE; j++) {
		printf("%02x", bytes[j]);
		if ((j % 4) == 3)
			printf(" ");
	}
}

void print96(const uint8_t *bytes) {
	int j;
	for (j = 0; j < 12; j++) {
		printf("%02x", bytes[j]);
		if ((j % 4) == 3)
			printf(" ");
	}
}

/* For CMAC Calculation */
static const uint8_t const_Rb[BLOCK_SIZE] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87 };
static const uint8_t const_Zero[BLOCK_SIZE] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

int AES_CMAC_CHECK(const uint8_t *key, const uint8_t *input, int length,
	const uint8_t *mac) {
	uint8_t T[BLOCK_SIZE];
	AES_CMAC(key, input, length, T);
	/*print128(T);
	printf("\n");
	print128(mac);*/
	return memcmp(mac, T, BLOCK_SIZE);
}

static void AES_128_ENC(const uint8_t *key, const uint8_t* msg, uint8_t *cipher) {
	uint8_t key_copy[BLOCK_SIZE];
	memcpy(cipher, msg, BLOCK_SIZE);
	memcpy(key_copy, key, BLOCK_SIZE);
	aes_enc_dec(cipher, key_copy, 0);
}

void AES_128_DEC(const uint8_t *key, const uint8_t* msg, uint8_t *cipher) {
	uint8_t key_copy[BLOCK_SIZE];
	memcpy(cipher, msg, BLOCK_SIZE);
	memcpy(key_copy, key, BLOCK_SIZE);
	aes_enc_dec(cipher, key_copy, 1);
}

void xor_128(const uint8_t *a, const uint8_t *b, uint8_t *out) {
	int i;
	for (i = 0; i < BLOCK_SIZE; i++) {
		out[i] = a[i] ^ b[i];
	}
}

static void padding_AES(const uint8_t *lastb, uint8_t *pad, int length) {
	int j;
	length = length % BLOCK_SIZE;

	if (length == 0) {
		memcpy(pad, lastb, BLOCK_SIZE);
		return;
	}

	/* original last block */
	for (j = 0; j < BLOCK_SIZE; j++) {
		if (j < length) {
			pad[j] = lastb[j];
		}
		else {
			pad[j] = 0x00;
		}
	}
}

int AES_CBC_ENC(const uint8_t *IV, const uint8_t *key, const uint8_t *input, int inputLength, uint8_t *output, int outputLength) {
	uint8_t X[BLOCK_SIZE], Y[BLOCK_SIZE], M_last[BLOCK_SIZE];

	if (inputLength <= 0)
		return 0; //nothing to encode

	int n = (inputLength + LAST_INDEX) / BLOCK_SIZE; //TODO: last

	memcpy(X, IV, BLOCK_SIZE);
	padding_AES(&input[BLOCK_SIZE * (n - 1)], M_last, inputLength);

	int i = 0;
	for (i = 0; (i < n) && outputLength > 0; i++) {
		const uint8_t * text = &input[BLOCK_SIZE * i];
		//      text = &input[BLOCK_SIZE * i];
		if (i == n - 1) {
			text = M_last;
		}
		int outLen = (BLOCK_SIZE < outputLength) ? BLOCK_SIZE : outputLength;
		xor_128(X, text, Y);
		AES_128_ENC(key, Y, X);
		memcpy(output, X, outLen);
		outputLength -= outLen;
		output += outLen;
	}

	return n * BLOCK_SIZE;
}

int AES_CBC_DEC(const uint8_t *IV, const uint8_t *key, const uint8_t *input, int inputLength, uint8_t *output, int outputLength) {
	uint8_t X[BLOCK_SIZE], text[BLOCK_SIZE], Z[BLOCK_SIZE];

	if (inputLength <= 0)
		return 0; //nothing to encode

	inputLength = (inputLength / BLOCK_SIZE) * BLOCK_SIZE;

	int n = (inputLength + LAST_INDEX) / BLOCK_SIZE;

	memcpy(Z, IV, BLOCK_SIZE);

	int i = 0;
	for (i = 0; (i < n) && outputLength > 0; i++) {
		const uint8_t * cipher = &input[BLOCK_SIZE * i];
		cipher = &input[BLOCK_SIZE * i];
		AES_128_DEC(key, cipher, X);
		xor_128(Z, X, text);
		memcpy(Z, cipher, BLOCK_SIZE);
		memcpy(output, text, BLOCK_SIZE);
		outputLength -= BLOCK_SIZE;
		output += BLOCK_SIZE;
	}

	return n * BLOCK_SIZE;
}

/* AES-CMAC Generation Function */

static void leftshift_onebit(const uint8_t *input, uint8_t *output) {
	int i;
	uint8_t overflow = 0;

	for (i = LAST_INDEX; i >= 0; i--) {
		output[i] = input[i] << 1;
		output[i] |= overflow;
		overflow = (input[i] & 0x80) ? 1 : 0;
	}
	return;
}

static void generate_subkey(const uint8_t *key, uint8_t *K1, uint8_t *K2) {
	uint8_t L[BLOCK_SIZE];
	uint8_t tmp[BLOCK_SIZE];

	AES_128_ENC(key, const_Zero, L);

	if ((L[0] & 0x80) == 0) { /* If MSB(L) = 0, then K1 = L << 1 */
		leftshift_onebit(L, K1);
	}
	else { /* Else K1 = ( L << 1 ) (+) Rb */

		leftshift_onebit(L, tmp);
		xor_128(tmp, const_Rb, K1);
	}

	if ((K1[0] & 0x80) == 0) {
		leftshift_onebit(K1, K2);
	}
	else {
		leftshift_onebit(K1, tmp);
		xor_128(tmp, const_Rb, K2);
	}
	return;
}

static void padding(const uint8_t *lastb, uint8_t *pad, int length) {
	int j;

	/* original last block */
	for (j = 0; j < BLOCK_SIZE; j++) {
		if (j < length) {
			pad[j] = lastb[j];
		}
		else if (j == length) {
			pad[j] = 0x80;
		}
		else {
			pad[j] = 0x00;
		}
	}
}

void AES_CMAC(const uint8_t *key, const uint8_t *input, int length, uint8_t *mac) {   //return CMAC is little endian ?
																					  /* @brief Perform encryption on the input data.
																					  *
																					  * @input key A 16 byte key as an array.
																					  * @input input The input data that has to be encrypted as a byte array, e.g {0x00, 0x01, ...}
																					  * @input length The number of input bytes that will be used for encryption. For a SECURED-PDU it is the sum of the length of DATA-ID, the payload length, and the length of the complete freshness value
																					  * @input mac The array that will contain the 16 bytes CMAC.
																					  * */

																					  //   printf("DEBUG - input: %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x\n", input[0], input[1], input[2], input[3],
																					  //              input[4], input[5], input[6], input[7],
																					  //              input[8], input[9], input[10], input[11],
																					  //              input[12], input[13], input[14], input[15],
																					  //              input[16], input[17]);

	uint8_t X[BLOCK_SIZE], Y[BLOCK_SIZE], M_last[BLOCK_SIZE], padded[BLOCK_SIZE];
	uint8_t K1[BLOCK_SIZE], K2[BLOCK_SIZE];
	int n, i, flag;
	generate_subkey(key, K1, K2);

	n = (length + LAST_INDEX) / BLOCK_SIZE; /* n is number of rounds */

	if (n == 0) {
		n = 1;
		flag = 0;
	}
	else {
		if ((length % BLOCK_SIZE) == 0) { /* last block is a complete block */
			flag = 1;
		}
		else { /* last block is not complete block -> Padding is needed. */
			flag = 0;
		}
	}

	if (flag) { /* last block is complete block */
		xor_128(&input[BLOCK_SIZE * (n - 1)], K1, M_last);
	}
	else { // padding is needed
		padding(&input[BLOCK_SIZE * (n - 1)], padded, length % BLOCK_SIZE);
		xor_128(padded, K2, M_last);
	}

	memset(X, 0, BLOCK_SIZE);
	for (i = 0; i < n - 1; i++) {
		xor_128(X, &input[BLOCK_SIZE * i], Y); /* Y := Mi (+) X  */
		AES_128_ENC(key, Y, X); /* X := AES-128(KEY, Y); */
	}

	xor_128(X, M_last, Y);
	AES_128_ENC(key, Y, X);

	memcpy(mac, X, BLOCK_SIZE);
}

uint8_t* aesCbcCompleteMac(uint8_t* buf, uint8_t key[], uint32_t length, uint16_t secOcDataId, uint64_t cfv, uint8_t nSecBytes)
{
	/*@brief This function is one step of the AES algorithm. It returns the complete CMAC (16 bytes).
	*
	* @input buf The data that will be encrypted
	* @input key A 16 byte array that contains a key in hex format
	* @input length The buffer length in [byte] including the security bytes. Usually, a multiple of 16.
	* @input secOcDataId The data ID that needs to be provided to the authenticator. Regular decimal number.
	*                    This is NOT the data ID for the CRC check!
	* @input cfv The Complete Freshness Value for calculating the authenticator. According to AUTOSAR
	*              specification Rel 4.2.2 on Secured Onboard Communication in chapter 7.1.1.2. According
	*              to Daimler SSA the complete freshness values consists of 5 bytes.
	* @input nSecBytes [DEFAULT = 4] The amount of bytes that are used for the security bytes in a SECURED-PDU.
	*                  Default value derived from BR223 project.
	*
	* @output cMac The encrypted buffer, specifically, the address to it. Starting with its LSB.
	* */
	uint8_t authLength = length - nSecBytes + DATA_ID_LENGTH + CFV_LENGTH; // authenticator contains the payload length ("signals"), data-ID, and complete freshness value. NO security bytes!
	uint8_t dataId[2];  // create a data id array in BIG ENDIAN format. E.g 0xff11 turns into [0xff, 0x11] -> Big Endian.
	uint16_t *ptr = &secOcDataId;
	dataId[0] = (ptr[0] & 0xff00) >> 8; // BIG ENDIAN. Switch to uint8_t
	dataId[1] = (ptr[0] & 0x00ff); // BIG ENDIAN

								   // construct the data of the complete freshness value in BIG ENDIAN format
	uint8_t cfvArr[8]; // The complete freshness value as an array of 8 bytes
	uint64_t *cvfPtr = &cfv;
	cfvArr[0] = (cvfPtr[0] & 0xff00000000000000) >> 56; // BIG ENDIAN. Switch to uint8_t
	cfvArr[1] = (cvfPtr[0] & 0xff000000000000) >> 48; // BIG ENDIAN.
	cfvArr[2] = (cvfPtr[0] & 0xff0000000000) >> 40; // BIG ENDIAN.
	cfvArr[3] = (cvfPtr[0] & 0xff00000000) >> 32; // BIG ENDIAN.
	cfvArr[4] = (cvfPtr[0] & 0xff000000) >> 24; // BIG ENDIAN
	cfvArr[5] = (cvfPtr[0] & 0xff0000) >> 16; // BIG ENDIAN.
	cfvArr[6] = (cvfPtr[0] & 0xff00) >> 8; // BIG ENDIAN.
	cfvArr[7] = (cvfPtr[0] & 0xff); // BIG ENDIAN



									// Construct the data to authenticator
	uint8_t inOutBuf[MAX_PDU_LENGTH + DATA_ID_LENGTH + CFV_LENGTH]; // This variable will contain the data to be authenticated. Expect the largest possible PDU.

	memcpy(inOutBuf, buf, authLength); // create a buffer to perform encryption on it. Copy the whole BlockLen
	uint8_t *inOutBuf_p = inOutBuf;
	::std::copy(dataId, dataId + DATA_ID_LENGTH, inOutBuf_p); // DATA-ID
	::std::copy(buf, buf + length - nSecBytes, inOutBuf_p + DATA_ID_LENGTH); // AUTHENTIC-I-PDU
	::std::copy(cfvArr, cfvArr + CFV_LENGTH, inOutBuf_p + length - nSecBytes + DATA_ID_LENGTH); // COMPLETE FRESHNESS VALUE
																								//   printf("DEBUG - inOutBuf_p: %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x\n", inOutBuf_p[0], inOutBuf_p[1], inOutBuf_p[2], inOutBuf_p[3],
																								//          inOutBuf_p[4], inOutBuf_p[5], inOutBuf_p[6], inOutBuf_p[7],
																								//          inOutBuf_p[8], inOutBuf_p[9], inOutBuf_p[10], inOutBuf_p[11],
																								//          inOutBuf_p[12], inOutBuf_p[13], inOutBuf_p[14], inOutBuf_p[15],
																								//          inOutBuf_p[16], inOutBuf_p[17]);

	static uint8_t cMac[16];
	AES_CMAC(key, inOutBuf_p, authLength, cMac);
	//    printf("DEBUG - cMac: %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x\n", cMac[0], cMac[1], cMac[2], cMac[3],
	//               cMac[4], cMac[5], cMac[6], cMac[7],
	//               cMac[8], cMac[9], cMac[10], cMac[11],
	//               cMac[12], cMac[13], cMac[14], cMac[15]);
	//    printf("DEBUG - At the end of aesCbcCompleteMac\n");
	return cMac; // Return the address to the CMAC
}

unsigned long long CAPLEXPORT far CAPLPASCAL calcAuth(unsigned char buf[], unsigned char key[], unsigned long length, unsigned char nMsb, unsigned short secOcDataId, unsigned long long cfv, unsigned short offset, unsigned short nSecBytes)
{
	/*@brief Return a TMAC value that is calculated based on a specific amount of MSBs.
	*
	* @input buf The data that will be encrypted
	* @input key A 16 byte array that contains a key in hex format
	* @input length The buffer length in [byte] including the security bytes. Usually, it is the SECURED-PDU length.
	* @input nMsb THe amount of MSBs that will be used to calculate the TMAC (Note: T!).
	* @input secOcDataId The data ID that needs to be provided to the authenticator. Regular decimal number.
	*                    This is NOT the data ID for the CRC check!
	* @input cfv The Complete Freshness Value for calculating the authenticator.
	* @input offset [DEFAULT = 0] The offset in [byte] in the beginning of the input data buf. The offset bytes will be
	*               ignored for CMAC calculation. Background: Messages of the Vehicle Security Master (VSS)
	*               use Multi-Frame-Broadcasting and therefore they contain frame counters that have to be
	*               neglected for CMAC calculation.
	*               Default value applies to SECURED-PDUs.
	* @input nSecBytes [DEFAULT = 4] The amount of bytes that are used for the security bytes in a SECURED-PDU.
	*              The Default value is four according to an arbitrary value by Igor Gall.
	*
	* @output tMac The truncated MAC, specifically, the value of its nMsb MSB. Starting with its LSB.
	*              E.g.: nMsb = 3; MSB are: [0xBD, 0x46, 0x6E]. Return value is: 12404334 (dec; hex = 0xBD466E)
	*              and NOT 7227069 (dec; hex: 0x6E46BD <- Little Endian)
	* */
	//    printf("DEBUG - In the beginning of calcAuth\n");
	uint64_t tMac = 0;
	//precondition: nMsb is less than 8 Byte and should be larger than 0 byte
	if (nMsb > 8 || nMsb == 0)
	{
		throw ::std::logic_error("ERROR: calcAuth() works only for less than 8 bytes or larger than 0 bytes.");
	}
	else
	{
		uint8_t *cMac = aesCbcCompleteMac(buf + offset, key, length, secOcDataId, cfv, nSecBytes);
		// Challenge: COnvert byte array to value where amount of relevant bytes in byte array is not fixed.
		uint64_t tMacLittleEndian[6]; // helping buffer to use only the amount of relevant bytes
		for (int i = 0; i < nMsb; i++) // First, create the TMAC as an array
		{
			tMacLittleEndian[i] = cMac[nMsb - 1 - i];
		}
		tMac = tMacLittleEndian[0];  // tMac has to consider the Big Endian format.

									 //		printf("DEBUG - tMacLittleEndian: %02llx%02llx%02llx%02llx %02llx%02llx%02llx%02llx \n", tMacLittleEndian[0], tMacLittleEndian[1], tMacLittleEndian[2], tMacLittleEndian[3],
									 //					tMacLittleEndian[4], tMacLittleEndian[5], tMacLittleEndian[6], tMacLittleEndian[7] ); //Debug

		tMac = tMacLittleEndian[0];
		for (int i = 1; i < nMsb; i++)
		{
			// create an integer number based on an array.
			tMac |= (tMacLittleEndian[i]) << (i * 8);
		}
	}
	//    printf("TMAC is:%llx \n", tMac);
	return tMac;
}

uint64_t calcAuthAdd(uint8_t *buf, uint8_t key[], uint32_t length, uint8_t nMsb, uint16_t secOcDataId, uint64_t cfv, uint8_t offset, uint8_t nSecBytes, uint8_t *addData, uint8_t addLength)
{
	/*@brief see calcAuth() for more info.
	*
	* @input addData Additional data that needs to be encrypted.
	* @input addLength The length of the additional data
	*
	* INFO:
	* Usually, only the data within one PDU is used to be encrypted. For the
	* secured broadcasting messages like VSS_TP_RealTmOffset_ST3 (SSA-IS-1206),
	* the MAC calculation is based on additional data. This yields that the
	* standard calcAuth()-function cannot be applied.
	*/
	// create a buffer that is able to hold the complete data
	uint8_t payloadToEncrypt[255];
	// append the additional data behind the buffer data
	memcpy(payloadToEncrypt, buf, offset + length + addLength);
	uint8_t *payloadToEncrypt_p = payloadToEncrypt;
	std::copy(buf, buf + offset + length, payloadToEncrypt_p); // original buffer
	std::copy(addData, addData + addLength, payloadToEncrypt_p + offset + length); // buffer data + additional data

																				   // extend the length
	length += addLength;
	return calcAuth(payloadToEncrypt, key, length, nMsb, secOcDataId, cfv, offset, nSecBytes);
}
////////////Zheng ling added: CFV in GWM is 8 bytes. CFV is highly project dependant, have to maintain by project tester ///////////////
//////////////////CFV:Complete Fresh value, data structure like below:////////////////////////////////////////////////////
//              MSB                                                                                                                   LSB
//              |----------------------|----------------------|----------------------|------------------------------------------------------
//              |TripCount: 3 bytes    | ResetCount: 2 bytes  |MsgCount: 22 bits     |ResetFlag: 2 bit( the lowest 2 bits of ResetCount)   |
//              |----------------------|----------------------|----------------------|------------------------------------------------------
///////////////////////logic of bytes values:////////////////////////////
//    TripCount: fixed value->1 (GWM requirement: value increases 1 when ECU reset/repower/...
//                                   Master init value: 1;
//                                   Slave init value: 0 )
//    ResetCount: fixed value ->65535 (GWM requirement: value increases 1 when Reset cycle timer to 30s;
//                                                      when ResetCount up to max value, keep max value;)
//    MsgCount:   increases 1 when message is sent;
//    ResetFlag:  fixed value->3 (GWM requirement: Keep up with ResetCount; eg, if ResetCount is 0xff ff, then ResetFlag is 0x11)
//    CFV should be ordered by Big Endian as an input for the AES-128-CMAC caculation
//    Big Endian: low address store the high bytes
//    Little Endian: low address store low bytes
//////////////////////////////////////////////////////////////////////

unsigned long long CAPLEXPORT far CAPLPASCAL GenerateCompleteFreshValue(unsigned long init_Msg_count, unsigned long Trip_value, unsigned short Reset_value, unsigned char Low_Reset_value, unsigned char CFV_LEN)
{   //for the first 5bytes, it contains 24 bits TripCount and 16bits Resetcount, they can be fixed value, so here i simulated 5 bytes with a fixed value
	uint8_t Trip_Count[3];
	uint8_t Reset_Count[2];
	uint8_t Meg_Reset_Count[3];
	uint32_t Meg_Reset_value;
	uint64_t cfv = 0;
	uint32_t *Meg_Reset_value_ptr = &Meg_Reset_value;
	unsigned long *Trip_Ptr = &Trip_value;
	unsigned short *Reset_Ptr = &Reset_value;
	Meg_Reset_value = init_Msg_count * 4 + Low_Reset_value;
	//	printf("The current time is: %lld \n",rt_timer_read()/1000000000);
	//	printf("the Msg Count is:%d", init_Msg_count);
	//	printf("the sum is:%X", Meg_Reset_value);

	Trip_Count[0] = (Trip_Ptr[0] & 0xff0000) >> 16; // BIG ENDIAN.
	Trip_Count[1] = (Trip_Ptr[0] & 0xff00) >> 8; // BIG ENDIAN.
	Trip_Count[2] = (Trip_Ptr[0] & 0xff); // BIG ENDIAN


	Reset_Count[0] = (Reset_Ptr[0] & 0xff00) >> 8; // BIG ENDIAN.
	Reset_Count[1] = (Reset_Ptr[0] & 0xff); // BIG ENDIAN

	Meg_Reset_Count[0] = (Meg_Reset_value_ptr[0] & 0xff0000) >> 16; // BIG ENDIAN.
	Meg_Reset_Count[1] = (Meg_Reset_value_ptr[0] & 0xff00) >> 8; // BIG ENDIAN.
	Meg_Reset_Count[2] = (Meg_Reset_value_ptr[0] & 0xff); // BIG ENDIAN


														  //    //for the last 3 bytes, it contains 22 bits Msgcount and 2 bits Low_RestCount, so here i simulated 3 bytes

	uint8_t CompleteFreshValue[8];
	memset(CompleteFreshValue, 0, sizeof(CompleteFreshValue));
	uint8_t *CompleteFreshValue_p = CompleteFreshValue;

	::std::copy(Trip_Count, Trip_Count + 3, CompleteFreshValue_p); // Trip_Count
	::std::copy(Reset_Count, Reset_Count + 2, CompleteFreshValue_p + 3); //
	::std::copy(Meg_Reset_Count, Meg_Reset_Count + 3, CompleteFreshValue_p + 3 + 2); // COMPLETE FRESHNESS VALUE

																					 //	printf("DEBUG - CompleteFreshValue_p: %02x%02x%02x%02x %02x%02x%02x%02x\n", CompleteFreshValue_p[0], CompleteFreshValue_p[1], CompleteFreshValue_p[2], CompleteFreshValue_p[3],
																					 //			CompleteFreshValue_p[4], CompleteFreshValue_p[5], CompleteFreshValue_p[6], CompleteFreshValue_p[7]);
																					 ///////////based on the array of CFV : CompleteFreshValue to create an integer number of CFV as output

	uint64_t tLittleEndian[8]; // helping buffer to use only the amount of relevant bytes
	for (int i = 0; i < CFV_LEN; i++) // First, create the TMAC as an array
	{
		tLittleEndian[i] = CompleteFreshValue_p[CFV_LEN - 1 - i];
	}
	cfv = tLittleEndian[0];

	for (int i = 1; i < CFV_LEN; i++)
	{
		// create an integer number based on an array.
		cfv |= (tLittleEndian[i]) << (i * 8);
	}
	//	printf("cfv is:%llx \n", cfv);

	return cfv;
}



long CAPLEXPORT far CAPLPASCAL SyncMessageGenerate(long SyncMessageDataId, unsigned long Trip_value, long Reset_value, unsigned char key[], long index, unsigned long length)
{

	uint8_t authLength = length; // authenticator contains the payload length ("signals"), data-ID, and complete freshness value. NO security bytes!
	uint8_t Trip_Count[3];
	uint8_t Reset_Count[2];
	uint8_t dataId[2];  // create a data id array in BIG ENDIAN format. E.g 0xff11 turns into [0xff, 0x11] -> Big Endian.
	long *ptr = &SyncMessageDataId;
	unsigned long *Trip_Ptr = &Trip_value;
	long *Reset_Ptr = &Reset_value;

	dataId[0] = (ptr[0] & 0xff00) >> 8; // BIG ENDIAN. Switch to uint8_t
	dataId[1] = (ptr[0] & 0x00ff); // BIG ENDIAN

	Trip_Count[0] = (Trip_Ptr[0] & 0xff0000) >> 16; // BIG ENDIAN.
	Trip_Count[1] = (Trip_Ptr[0] & 0xff00) >> 8; // BIG ENDIAN.
	Trip_Count[2] = (Trip_Ptr[0] & 0xff); // BIG ENDIAN


	Reset_Count[0] = (Reset_Ptr[0] & 0xff00) >> 8; // BIG ENDIAN.
	Reset_Count[1] = (Reset_Ptr[0] & 0xff); // BIG ENDIAN

	uint8_t DataInputForAES[7];
	memset(DataInputForAES, 0, sizeof(DataInputForAES));
	uint8_t *DataInputForAES_p = DataInputForAES;

	::std::copy(dataId, dataId + 2, DataInputForAES_p); // Trip_Count
	::std::copy(Trip_Count, Trip_Count + 3, DataInputForAES_p + 2); //
	::std::copy(Reset_Count, Reset_Count + 2, DataInputForAES_p + 3 + 2); // COMPLETE FRESHNESS VALUE

																		  //    printf("DEBUG - DataInputForAES_p: %02x%02x%02x%02x %02x%02x%02x\n", DataInputForAES_p[0], DataInputForAES_p[1], DataInputForAES_p[2], DataInputForAES_p[3],
																		  //    		DataInputForAES_p[4], DataInputForAES_p[5], DataInputForAES_p[6]);
																		  //
	static uint8_t cMac[16];
	AES_CMAC(key, DataInputForAES_p, authLength, cMac);
	//    printf("DEBUG - CompleteMac OF SyncMessage: %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x\n", cMac[0], cMac[1], cMac[2], cMac[3],
	//               cMac[4], cMac[5], cMac[6], cMac[7],
	//               cMac[8], cMac[9], cMac[10], cMac[11],
	//               cMac[12], cMac[13], cMac[14], cMac[15]);
	// create SyncMessage:TripCnt|ResetCnt|Truncated the cMac with highest 88 bits(11 bytes) = 16 bytes

	static uint8_t SynMesg[16];
	memset(SynMesg, 0, sizeof(SynMesg));
	uint8_t *SynMesg_p = SynMesg;

	::std::copy(Trip_Count, Trip_Count + 3, SynMesg_p); // Trip_Count
	::std::copy(Reset_Count, Reset_Count + 2, SynMesg_p + 3); //
	::std::copy(cMac, cMac + 11, SynMesg_p + 3 + 2); // COMPLETE FRESHNESS VALUE
											 //    printf("DEBUG - At the end of aesCbcCompleteMac\n");
	return SynMesg_p[index]; // Return the address to the CMAC
}

long CAPLEXPORT far CAPLPASCAL appSubtract(long x, long y)
{
	long z = x - y;

	return z;
}


// ============================================================================
// CAPL_DLL_INFO_LIST : list of exported functions
//   The first field is predefined and mustn't be changed!
//   The list has to end with a {0,0} entry!
// New struct supporting function names with up to 50 characters
// ============================================================================
CAPL_DLL_INFO4 table[] = {
	{ CDLL_VERSION_NAME, (CAPL_FARCALL)CDLL_VERSION, "", "", CAPL_DLL_CDECL, 0xabcd, CDLL_EXPORT },
	{ "dllInit",           (CAPL_FARCALL)appInit,          "CAPL_DLL","This function will initialize all callback functions in the CAPLDLL",'V', 1, "D", "",{ "handle" } },
	{ "dllEnd",            (CAPL_FARCALL)appEnd,           "CAPL_DLL","This function will release the CAPL function handle in the CAPLDLL",'V', 1, "D", "",{ "handle" } },
	{ "dllSubtract",		(CAPL_FARCALL)appSubtract,		"CAPL_DLL", "This function will substract two values. The return value is the result", 'L', 2, "LL", "",{ "x", "y" } },
	{ "dllcalcAuth",		(CAPL_FARCALL)calcAuth,		"CAPL_DLL", "This function will calulate MAC", 'U', 8, "BBDLLULL", "\001\001\000\000\000\000\000\000",{ "buf","key","length","nMsb","secOcDataId","cfv","offset","nSecBytes" } },
	{ "dllGenerateCompleteFreshValue",		(CAPL_FARCALL)GenerateCompleteFreshValue,		"CAPL_DLL", "This function will read cfv", 'U', 5, "DDLLL", "",{ "init_Msg_count","Trip_value","Reset_value","Low_Reset_value","CFV_LEN" } },
	{ "dllSyncMessageGenerate",		(CAPL_FARCALL)SyncMessageGenerate,		"CAPL_DLL", "This function will generate a message", 'L', 6, "LDLBLD", "\000\000\000\001\000\000",{ "SyncMessageDataId","Trip_value","Reset_value","key","index","length" } },
	//{ "dllaesCbcCompleteMac",		(CAPL_FARCALL)aesCbcCompleteMac,		"CAPL_DLL", "This function will generate a message", 'L', 7, "BBDLULL", "\001\001\000\000\000\000\000",{ "buf","key","length","secOcDataId","cfv","index","nSecBytes" } },
	{ 0, 0 }
};
CAPLEXPORT CAPL_DLL_INFO4 far * caplDllTable4 = table;

// ============================================================================
// DllMain, entry Point of DLL  
// ============================================================================

BOOL WINAPI DllMain(HINSTANCE handle, DWORD reason, void*)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
	{
		gModuleHandle = handle;

		// Get full filename of module
		wchar_t path_buffer[_MAX_PATH];
		DWORD result = GetModuleFileName(gModuleHandle, path_buffer, _MAX_PATH);

		// split filename into parts
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		char path_buffer_t;

		char * m_char;
		int len = WideCharToMultiByte(CP_ACP, 0, path_buffer, wcslen(path_buffer), NULL, 0, NULL, NULL);
		m_char = new char[len + 1];
		WideCharToMultiByte(CP_ACP, 0, path_buffer, wcslen(path_buffer), m_char, len, NULL, NULL);
		m_char[len] = '\0';

#if _MSC_VER>=1400 // >= Visual Studio 2005
		_splitpath_s(m_char, drive, dir, fname, ext);
		strcpy_s(gModuleName, fname);
#else
		_splitpath(path_buffer, drive, dir, fname, ext);
		strcpy(gModuleName, fname);
#endif

		return 1;   // Indicate that the DLL was initialized successfully.
	}

	case DLL_PROCESS_DETACH:
	{
		ClearAll();
		return 1;   // Indicate that the DLL was detached successfully.
	}
	}
	return 1;
}



