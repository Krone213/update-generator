

/*
Имя файла: CrcUnit.h
автор: Гапонов Р.В.
Версия: 1.0.0
дата: 2012-10-24
описание: содержит заголовки функций для расчёта CRC
*/

#ifndef CrcUnitH
#define CrcUnitH
//================================================================================

#include <vector>
//================================================================================

extern const unsigned char Crc8Table[256];
extern const unsigned short Crc16Table[256];
extern const unsigned int Crc32Table[256];

unsigned char CalcCrc8 (void *ABuffer, int ASize);
unsigned short CalcCrc16 (void *ABuffer, int ASize);
unsigned int CalcCrc32 (void *ABuffer, int ASize);

unsigned char CalcCrc8 (std::vector <unsigned char> &ABuffer, int ASize);
unsigned short CalcCrc16 (std::vector <unsigned char> &ABuffer, int ASize);
unsigned int CalcCrc32 (std::vector <unsigned char> &ABuffer, int ASize);
//================================================================================

#endif


