//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include <Memory>
#include "Unit2.h"
#include "CrcUnit.h"
//#include "SelDirAdv.hpp"
#include "FileCtrl.hpp"
#include <vector>
#include <Xml.XMLDoc.hpp>
#include <Xml.xmldom.hpp>
#include <Xml.XMLIntf.hpp>
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm2 *Form2;
//---------------------------------------------------------------------------
class Model
{
public:
String name;
String mainfile;
String boot;
Model()
  {
  }
Model(String txt1, String txt2, String txt3)
  {
  name = txt1;
  mainfile = txt2;
  boot = txt3;
  }
};

__fastcall TForm2::TForm2(TComponent* Owner)
	: TForm(Owner)
{
std::vector<Model> models;

_di_IXMLDocument document = interface_cast<Xmlintf::IXMLDocument>(new TXMLDocument(NULL));
document->LoadFromFile(GetCurrentDir() + "\\config.xml");
_di_IXMLNodeList nodeList = document->DocumentElement->ChildNodes;
for (int i = 0; i < nodeList->Count; i++)
  {
  _di_IXMLNodeList nodeListRev = nodeList->Get(i)->ChildNodes;
  for (int j = 0; j < nodeListRev->Count; j++)
	{
	_di_IXMLNode modelNode = nodeListRev->Get(j);

	EditDevModel->AddItem(modelNode->Attributes["BldrDevModel"], NULL);
	ComboBox1->AddItem(modelNode->Attributes["category"],NULL);
	ComboBox2->AddItem(modelNode->Attributes["category"],NULL);
	}
  }

//ComboBox1->AddItem("KR02Ru",NULL);
//ComboBox1->AddItem("KR03Ru",NULL);
//ComboBox1->AddItem("KR04Ru",NULL);
//ComboBox1->AddItem("KR05Ru",NULL);
//// КР06 ревизии 2,3,4
//ComboBox1->AddItem("KR06Ru_R2",NULL);
//ComboBox1->AddItem("KR06Ru_R3",NULL);
//ComboBox1->AddItem("KR06Ru_R4",NULL);
//// КР07 - 3,4
//ComboBox1->AddItem("KR07Ru_R3",NULL);
//ComboBox1->AddItem("KR07Ru_R4",NULL);
//
//ComboBox1->AddItem("KR08Ru_R3",NULL);
//
//ComboBox1->AddItem("IN22_R1",NULL);
//
//ComboBox1->AddItem("IN33_R1",NULL);
//ComboBox1->AddItem("IN33_R2",NULL);
//
//ComboBox1->AddItem("KAMA_R1",NULL);
//ComboBox1->AddItem("KAMA_R2",NULL);
//
//// МД01 - vB(STM32L4 based), vC(GD32F2 based)
//ComboBox1->AddItem("MD01M_vB",NULL);
//ComboBox1->AddItem("MD01M_vC",NULL);
//ComboBox1->AddItem("MDZ_R1",NULL);
ComboBox1->AddItem("OthDev",NULL);
ComboBox2->AddItem("OthDev",NULL);
EditDevModel->AddItem("OthDev", NULL);
}
//---------------------------------------------------------------------------


//-----------------------------Вкладка "Обновление прошивки"------------------//
void __fastcall TForm2::ButtonUpdateClearDataClick(TObject *Sender)
{
UpdateDataSize = 0;
LabelUpdateFileName->Caption = "-";
LabelUpdateFileSize->Caption = "-";
}
//---------------------------------------------------------------------------

void __fastcall TForm2::ButtonUpdateClearParamsClick(TObject *Sender)
{
EditDevDesc->Text = "";
EditDevSerial->Text = "";
EditDevId->Text = "";
EditEraseBeginAddr->Text = "0";
EditEraseEndAddr->Text = "0";


CheckBoxIsBaseProg->Checked    = 1;
CheckBoxOnlySelSerial->Checked = 0;
CheckBoxOnlySelDevId->Checked  = 0;
CheckBoxNoModelCheck->Checked  = 0;

CheckBoxSaveBthName->Checked   = 0;
CheckBoxSaveBthAddr->Checked   = 0;
CheckBoxSaveDesc->Checked      = 0;
CheckBoxSaveHwVersion->Checked = 0;
CheckBoxSaveSwVersion->Checked = 0;
CheckBoxSaveSerial->Checked    = 1;
CheckBoxSaveModel->Checked     = 0;
}
//---------------------------------------------------------------------------

#define BTH_NAME_OFFSET         0x0800
#define BTH_ADDR_OFFSET         0x0830
#define DEV_DESC_OFFSET         0x0840
#define HW_DESC_OFFSET          0x0880
#define SW_DESC_OFFSET          0x08C0
#define DEV_SERIAL_OFFSET       0x0900
#define DEV_MODEL_OFFSET        0x0940


void AutoDefineDevModel(u8 *ProgramData)
{
AnsiString DevHwDesc,DevDesc;
DevDesc.printf((char*) &ProgramData[DEV_DESC_OFFSET]);
DevHwDesc.printf((char*) &ProgramData[HW_DESC_OFFSET]);



if(DevDesc == "KR02ru Monitor")
  {
  //Form2->ComboBox1->ItemIndex = 0;
  if(DevHwDesc == "102")
	{
	Form2->EditDevModel->Text = "KR05Ru.vB";
	}
  }
else if(DevDesc == "KR05ru Monitor")
  {
  //Form2->ComboBox1->ItemIndex = 3;
  if(DevHwDesc == "103")
	{
	Form2->EditDevModel->Text = "KR05Ru.vB";
	}
  }
else if(DevDesc == "KR06ru Monitor")
  {
  if(DevHwDesc == "002.R2B")
	{
	Form2->EditDevModel->Text = "KR06ru.r2";
	//Form2->ComboBox1->ItemIndex = 4;
	}
  if(DevHwDesc == "002.R3B")
	{
	Form2->EditDevModel->Text = "KR06ru.r3";
	//Form2->ComboBox1->ItemIndex = 5;
	}
  if(DevHwDesc == "002.R4B")
	{
	Form2->EditDevModel->Text = "KR06ru.r4";
	//Form2->ComboBox1->ItemIndex = 6;
	}
  }
else if(DevDesc == "KR07ru Monitor")
  {
  if(DevHwDesc == "002.R3B")
	{
	Form2->EditDevModel->Text = "KR07ru.r3";
	//Form2->ComboBox1->ItemIndex = 7;
	}
  if(DevHwDesc == "002.R4B")
	{
	Form2->EditDevModel->Text = "KR07ru.r4";
	//Form2->ComboBox1->ItemIndex = 8;
	}
  }
else if(DevDesc == "CardiolinkECG/BP/S")
  {
  if(DevHwDesc == "002.R3B")
	{
	Form2->EditDevModel->Text = "KR08ru.r3";
	//Form2->ComboBox1->ItemIndex = 9;
	}
  if(DevHwDesc == "002.R4B") Form2->EditDevModel->Text = "KR08ru.r4";
  }
else if(DevDesc == "CardiolinkBP")
  {
  if(DevHwDesc == "MDZ.Rev1.Bldr")
	{
	//Form2->ComboBox1->ItemIndex = 17;
	Form2->EditDevModel->Text = "MDZ.r1";
	}
  }
else if(DevDesc == "ICAR8 ECG Monitor")
  {
  //Form2->ComboBox1->ItemIndex = 11;
  if(DevHwDesc == "2.00.Seps") Form2->EditDevModel->Text = "IN33";
  if(DevHwDesc == "3.00.Ssd") Form2->EditDevModel->Text = "IN33";
  if(DevHwDesc == "4.00.Seps")
	{
	Form2->EditDevModel->Text = "IN33.R2";
	//Form2->ComboBox1->ItemIndex = 12;
	}
	if(DevHwDesc == "5.00.Ssd")
	{
    Form2->EditDevModel->Text = "IN33.R2";
	//Form2->ComboBox1->ItemIndex = 12;
	}
  }
else if(DevDesc == "IN22M ECG Monitor")
  {
  //Form2->ComboBox1->ItemIndex = 11;
  if(DevHwDesc == "2.00") Form2->EditDevModel->Text = "IN33";
  }
else if(DevDesc == "KAMA ECG Monitor")
  {
  //Form2->ComboBox1->ItemIndex = 13;
  if(DevHwDesc == "2.00.Seps") Form2->EditDevModel->Text = "IN33";
  if(DevHwDesc == "3.00.Ssd") Form2->EditDevModel->Text = "IN33";
  if(DevHwDesc == "4.00.Ssd")
    {
	Form2->EditDevModel->Text = "IN33.R2";
	//Form2->ComboBox1->ItemIndex = 14;
	}
  }
else if(DevDesc == "MD-01 Monitor")
  {
  if(DevHwDesc == "103")
	{
	//Form2->ComboBox1->ItemIndex = 15;
	Form2->EditDevModel->Text = "KR05Ru.vB";
	}
  if(DevHwDesc == "MD01.VerC.Bldr")
	{
	//Form2->ComboBox1->ItemIndex = 16;
	Form2->EditDevModel->Text = "MD01.VerC";
	}
  }
else
  {
  //Form2->ComboBox1->ItemIndex = 20;
  Form2->EditDevModel->Text = "UnknownDev";
  ShowMessage("Неизвестный прибор. Определите модель прибора вручную");
  }
}


void __fastcall TForm2::ButtonUpdateSelectFileClick(TObject *Sender)
{
String Str = GetCurrentDir();
Str += "\\MainProgram_File";
OpenDialog1->InitialDir = Str;
if(!OpenDialog1->Execute()) return;

try
  {
  ButtonUpdateClearDataClick(this);
  AnsiString FileName = OpenDialog1->FileName;

  std::auto_ptr<TFileStream> Stream(new TFileStream(FileName, fmOpenRead));
  if(Stream->Size > ((1024 - 16) * 1024))
	{
	throw Exception("Файл больше максимально допустимого размера (1024-16 Кб)");
	}
  int ReadSize = Stream->Read(&UpdateDataBuffer,Stream->Size);
  if(ReadSize != Stream->Size)
	{
	throw Exception("Ошибка при чтении данных файла");
	}
  UpdateDataSize = Stream->Size;
  LabelUpdateFileName->Caption = FileName;
  LabelUpdateFileSize->Caption = IntToStr(Stream->Size) + "байт(а)";
  //AutoDefineDevModel(UpdateDataBuffer);
  }
catch (Exception &Exc)
  {
  //AnsiString Hdr = "Ошибка";
  //AnsiString Msg = Exc.Message;
  UnicodeString Hdr = "Ошибка";
  UnicodeString Msg = Exc.Message;
  MessageBox(Handle, Msg.c_str(),Hdr.c_str(),MB_OK | MB_ICONERROR);
  }
}
//---------------------------------------------------------------------------

void __fastcall TForm2::ButtonUpdateHelpClick(TObject *Sender)
{
//AnsiString Hdr = "Инфо";
//AnsiString Msg = "Создание файлов первоначальной прошивки для устройств серий КР, МД, ИН. (Максимальный размер файла прошивки 1024 Кб)";
UnicodeString Hdr = "Инфо";
UnicodeString Msg = "Создание файлов первоначальной прошивки для устройств серий КР, МД, ИН. (Максимальный размер файла прошивки 1024 Кб)";
MessageBox(Handle, Msg.c_str(),Hdr.c_str(),MB_OK | MB_ICONINFORMATION);
}
//---------------------------------------------------------------------------

String GetDirSaveUp(String n){
_di_IXMLDocument document = interface_cast<Xmlintf::IXMLDocument>(new TXMLDocument(NULL));
document->LoadFromFile(GetCurrentDir() + "\\config.xml");
_di_IXMLNodeList nodeList = document->DocumentElement->ChildNodes;
if (n != "OthDev")
  {
  for (int i = 0; i < nodeList->Count; i++)
	{
	_di_IXMLNodeList nodeListRev = nodeList->Get(i)->ChildNodes;
	for (int j = 0; j < nodeListRev->Count; j++)
	  {
	  _di_IXMLNode modelNode = nodeListRev->Get(j);
	  WideString category = modelNode->Attributes["category"];
	  if (category == n) return nodeListRev->Get(j)->ChildNodes->Get(6)->Text;
	  }
	}
  }
else
  {
  UnicodeString Hdr = "Ошибка";
  UnicodeString Msg = "Выберите самостоятельно путь сохранения!";
  MessageBox(GetForegroundWindow(), Msg.c_str(), Hdr.c_str(), MB_OK | MB_ICONINFORMATION);
  }
}

void __fastcall TForm2::ButtonUpdateMakeFileClick(TObject *Sender)
{
try
  {
  AnsiString S;
  int EraseBeginAddr,EraseEndAddr;
  int ProgBeginAddr;

  //чистим строки
  S = EditDevModel->Text;  S = S.Trim(); EditDevModel->Text = S;
  S = EditDevDesc->Text;   S = S.Trim(); EditDevDesc->Text = S;
  S = EditDevSerial->Text; S = S.Trim(); EditDevSerial->Text = S;
  S = EditDevId->Text;     S = S.Trim(); EditDevId->Text = S;

  S = EditDevModel->Text;
  if(S == "") throw Exception("Строка описателя модели не указана!");

  //анализ адреса для стирания
  S = EditEraseBeginAddr->Text; S = S.Trim(); EditEraseBeginAddr->Text = S;
  if(!TryStrToInt(S,EraseBeginAddr)) throw Exception("Начальный адрес для стирания указан неверно (не число)!");

  S = EditEraseEndAddr->Text; S = S.Trim(); EditEraseEndAddr->Text = S;
  if(!TryStrToInt(S,EraseEndAddr)) throw Exception("Конечный адрес для стирания указан неверно (не число)!");

  if((EraseBeginAddr) || (EraseEndAddr))
    {
    if(EraseBeginAddr > EraseEndAddr) throw Exception("Адреса для стирания указаны неверно (Начальный больше конечного)!");
    if((EraseBeginAddr < 0x08004000) || (EraseBeginAddr > 0x0807FFFF)) throw Exception("Начальный адрес для стирания указан неверно (вне допустимого диапазона)!");
    if((EraseEndAddr < 0x08004000) || (EraseEndAddr > 0x0807FFFF)) throw Exception("Начальный адрес для стирания указан неверно (вне допустимого диапазона)!");
    }

  //анализ адреса программы
  S = EditProgBeginAddr->Text; S = S.Trim(); EditProgBeginAddr->Text = S;
  if (!TryStrToInt(S, ProgBeginAddr)) throw Exception("Начальный адрес программы указан неверно (не число)!");

  if(UpdateDataSize)
    {
    if((ProgBeginAddr < 0x08004000) || (ProgBeginAddr > 0x0807FFFF)) throw Exception("Начальный адрес программы указан неверно (вне допустимых)!");
    }

  // проверка настроек

  if((CheckBoxIsBaseProg->Checked) && (UpdateDataSize < (2 * 2048))) throw Exception("Файл программы не указан или меньше допустимого (<4 Кб)!");
  if((CheckBoxOnlySelSerial->Checked) && (EditDevSerial->Text == "")) throw Exception("Не указан серийный номер!");
  if((CheckBoxOnlySelDevId->Checked) && (EditDevId->Text == "")) throw Exception("Не указан идентификатор устройства!");

  if(((CheckBoxSaveBthName->Checked) || (CheckBoxSaveBthAddr->Checked) ||
      (CheckBoxSaveDesc->Checked) || (CheckBoxSaveHwVersion->Checked)  ||
      (CheckBoxSaveSwVersion->Checked) || (CheckBoxSaveSerial->Checked)||
      (CheckBoxSaveModel->Checked)) && (!CheckBoxIsBaseProg->Checked)) throw Exception("Сохранение полей допустимо только для основной программы!");

  if((!UpdateDataSize) && ((!EraseBeginAddr) || (!EraseEndAddr))) throw Exception("Задание пустое!");

  //создание описателя задания на обновление прошивки
  memset(&UpdateTask, 0, sizeof(UpdateTask));

  //поля заголовка задания
  UpdateTask.FileId    = 0x52444C42;
  UpdateTask.BlockType = 0x00010000;
  UpdateTask.Version   = 0x00000100;
  UpdateTask.BlockSize = 512;

  //строки, модели, номера и т.д.
  S = EditDevModel->Text;
  strncpy(UpdateTask.ModelInfo, S.c_str(), sizeof(UpdateTask.ModelInfo) - 1);

  S = EditDevDesc->Text;
  strncpy(UpdateTask.DescInfo, S.c_str(), sizeof(UpdateTask.DescInfo) - 1);

  S = EditDevSerial->Text;
  strncpy(UpdateTask.DevSerial, S.c_str(), sizeof(UpdateTask.DevSerial) - 1);

  S = EditDevId->Text;
  strncpy(UpdateTask.DevId, S.c_str(), sizeof(UpdateTask.DevId) - 1);

  // настраиваем адреса
  UpdateTask.EraseBeginAddr = EraseBeginAddr;
  UpdateTask.EraseEndAddr   = EraseEndAddr;

  u32 DataCrc = CalcCrc32(&UpdateDataBuffer, UpdateDataSize);
  UpdateTask.ProgramBeginAddr = ProgBeginAddr;
  UpdateTask.ProgramSize = UpdateDataSize;
  UpdateTask.ProgramCrc = DataCrc;

  //Команды для задания
  UpdateTask.LoaderBaseCmd   = 0;
  UpdateTask.LoaderExtCmd    = 0;
  UpdateTask.LoaderBeforeCmd = 0;
  UpdateTask.LoaderAfterCmd  = 0;

  if(CheckBoxIsBaseProg->Checked)     UpdateTask.LoaderBaseCmd |= CMD_IS_BASE_PROGRAM;
  if(CheckBoxOnlySelSerial->Checked)  UpdateTask.LoaderBaseCmd |= CMD_UPDATE_SAME_SERIAL;
  if(CheckBoxOnlySelDevId->Checked)   UpdateTask.LoaderBaseCmd |= CMD_UPDATE_SAME_DEV_ID;
  if(CheckBoxNoModelCheck->Checked)   UpdateTask.LoaderBaseCmd |= CMD_NO_MODEL_CHECK;

  if(CheckBoxSaveBthName->Checked)   UpdateTask.LoaderBeforeCmd |= CMD_SAVE_BTH_NAME;
  if(CheckBoxSaveBthAddr->Checked)   UpdateTask.LoaderBeforeCmd |= CMD_SAVE_BTH_ADDR;
  if(CheckBoxSaveDesc->Checked)      UpdateTask.LoaderBeforeCmd |= CMD_SAVE_DEV_DESC;
  if(CheckBoxSaveHwVersion->Checked) UpdateTask.LoaderBeforeCmd |= CMD_SAVE_HW_VERSION;
  if(CheckBoxSaveSwVersion->Checked) UpdateTask.LoaderBeforeCmd |= CMD_SAVE_SW_VERSION;
  if(CheckBoxSaveModel->Checked)     UpdateTask.LoaderBeforeCmd |= CMD_SAVE_MODEL_INFO;
  if(CheckBoxSaveSerial->Checked)    UpdateTask.LoaderBeforeCmd |= CMD_SAVE_SERIAL;
  if(CheckBoxEnableLvl1->Checked)    UpdateTask.LoaderBeforeCmd |= CMD_SET_RDP_LV1;

  //Время дял искажения CRC
  TDateTime Time = Now();
  memmove(&UpdateTask.Time, &Time, sizeof(UpdateTask.Time));
  UpdateTask.Crc = CalcCrc32(&UpdateTask, sizeof(UpdateTask) - sizeof(u32));

  //наложение ПСП на задание
  u8 *TBuf = (u8*) &UpdateTask;
  u32 RandSeed = UpdateTask.Crc;
  for(int i = 0; i < (sizeof(UpdateTask) - sizeof(u32)); i++)
    {
    RandSeed = (RandSeed * 0x08088405) + 1;
    TBuf[i] ^= (RandSeed >> 24);
    }

  //наложение ПСП на данные
  if(UpdateDataSize)
    {
    memmove(&TmpBuffer, &UpdateDataBuffer, sizeof(TmpBuffer));
    u8 *DBuf = (u8*) &TmpBuffer;
    RandSeed = UpdateTask.Crc ^ DataCrc;
    for(int i = 0; i < UpdateDataSize; i++)
      {
      RandSeed = (RandSeed * 0x08088405) + 1;
      DBuf[i] ^= (RandSeed >> 24);
      }
    }


  SaveDialog1->Options = TOpenOptions() << ofPathMustExist << ofEnableSizing << ofOverwritePrompt;
  SaveDialog1->FileName = "Firmware.bin";


  //запись выходного файла
  UnicodeString Dir_1 = GetCurrentDir();
  Dir_1 += "\\" + GetDirSaveUp(ComboBox1->Text) + "\\";
  if (Sender == ButtonUpdateAutoMakeFile)
	{
	Dir_1 += ComboBox1->Text;
	Dir_1 += "\\";
	if (!DirectoryExists(Dir_1)) ForceDirectories(Dir_1);
	SaveDialog1->InitialDir = Dir_1;
	SaveDialog1->FileName = Dir_1 + "Firmware.bin";
    }
  else if (Sender == ButtonUpdateMakeFile)
    {
	//if(!SelectDirectory("Выберите папку","",Dir)) return;
    //Dir += "\\Файл обновления CPU1 с карточки";
    SaveDialog1->InitialDir = Dir_1;
    if(!SaveDialog1->Execute()) return;

    }
  else return;

  AnsiString FileName = SaveDialog1->FileName;
  std::auto_ptr <TMemoryStream> Stream(new TMemoryStream());

  Stream->Write(&UpdateTask, sizeof(UpdateTask));
  if(UpdateDataSize) Stream->Write(&TmpBuffer, UpdateDataSize);
  Stream->Position = 0;
  Stream->SaveToFile(FileName);

  //сообщаем о завершении
  UnicodeString Hdr = "Инфо"; //в старой версии использовался тип AnsiString
  UnicodeString Msg = "Файл обновления успешно создан";
  MessageBox(Handle, Msg.c_str(), Hdr.c_str(), MB_OK | MB_ICONINFORMATION);
  }
catch(Exception &Exc)
  {
  UnicodeString Hdr = "Ошибка";
  UnicodeString Msg = Exc.Message;
  MessageBox(Handle, Msg.c_str(), Hdr.c_str(), MB_OK | MB_ICONERROR);
  }
}
//---------------------------------------------------------------------------


//-----------------------------Вкладка "Начальная прошивка"-------------------//

void __fastcall TForm2::ButtonFirstSelectFileClick(TObject *Sender)
{
String Str = GetCurrentDir();
Str += "\\MainProgram_File";
OpenDialog1->InitialDir = Str;

if(!OpenDialog1->Execute())return;
try
  {
  ButtonFirstClearDataClick(this);
  AnsiString FileName = OpenDialog1->FileName;
  std::auto_ptr<TFileStream> Stream(new TFileStream(FileName, fmOpenRead));
  if(Stream->Size > ((1024 - 16) * 1024))
	{
	throw Exception("Файл больше максимально допустимого размера (1024-16 Кб)");
	}
  int ReadSize = Stream->Read(&FirstDataBuffer, Stream->Size);
  if(ReadSize != Stream->Size)
	{
	throw Exception("Ошибка при чтении данных файла");
    }
  FirstDataSize = Stream->Size;
  LabelFirstFileName->Caption = FileName;
  LabelFirstFileSize->Caption = IntToStr(Stream->Size) + " байт(а)";
  if(LoaderDataSize)
    {
    LabelSumSize->Caption = IntToStr(FirstDataSize + LoaderDataSize) + " байт(а)";
	}
  AutoDefineDevModel(FirstDataBuffer);
  }
catch(Exception &Exc)
  {
  UnicodeString Hdr = "Ошибка";
  UnicodeString Msg = Exc.Message;
  MessageBox(Handle, Msg.c_str(),Hdr.c_str(),MB_OK | MB_ICONERROR);
  return;
  }
}
//---------------------------------------------------------------------------

void __fastcall TForm2::ButtonFirstClearDataClick(TObject *Sender)
{
FirstDataSize = 0;

LabelFirstFileName->Caption = "-";
LabelFirstFileSize->Caption = "-";
}
//---------------------------------------------------------------------------

void __fastcall TForm2::ButtonLoaderSelectFileClick(TObject *Sender)
{
String Str = GetCurrentDir();
Str += "\\Bootloader_File";
OpenDialogLoader->InitialDir = Str;

if(!OpenDialogLoader->Execute()) return;
try
  {
  ButtonFirstClearLoaderClick(this);
  AnsiString FileName = OpenDialogLoader->FileName;
  std::auto_ptr<TFileStream> Stream(new TFileStream(FileName, fmOpenRead));
  if(Stream->Size > (16 * 1024))
	{
	throw Exception("Файл больше максимально допустимого размера (16 Кб)");
	}
  int ReadSize = Stream->Read(&LoaderDataBuffer, Stream->Size);
  if(ReadSize != Stream->Size)
	{
	throw Exception("Ошибка при чтении данных файла");
	}
	LoaderDataSize = Stream->Size;
	LabelLoaderFileName->Caption = FileName;
	LabelLoaderFileSize->Caption = IntToStr(Stream->Size) + " байт(а)";
	if(FirstDataSize)
	{
	LabelSumSize->Caption = IntToStr(FirstDataSize + LoaderDataSize) + " байт(а)";
	}
	AutoDefineDevModel(FirstDataBuffer);
  }
catch(Exception &Exc)
  {
  UnicodeString Hdr = "Ошибка";
  UnicodeString Msg = Exc.Message;
  MessageBox(Handle, Msg.c_str(),Hdr.c_str(),MB_OK | MB_ICONERROR);
  return;
  }
}
//---------------------------------------------------------------------------


void __fastcall TForm2::ButtonFirstClearLoaderClick(TObject *Sender)
{
LoaderDataSize = 0;

LabelLoaderFileName->Caption = "-";
LabelLoaderFileSize->Caption = "-";

ComboBox1->ItemIndex = 3;
}
//---------------------------------------------------------------------------


void __fastcall TForm2::ButtonFirstClearParamsClick(TObject *Sender)
{
EditFirstSerialBegin->Text = 0;
EditFirstSerialCount->Text = 1;
CheckBoxUpdateCRC32->Checked = true;
EditNewCRC32->Text = "880F7E60";
}
//---------------------------------------------------------------------------

void __fastcall TForm2::ButtonFirstHelpClick(TObject *Sender)
{
UnicodeString Hdr = "Инфо";
UnicodeString Msg = "Создание файлов первоначальной прошивки для устройств КР, МД, ИН. (Максимальный размер файла прошивки - 1024 Кб)";
MessageBox(Handle, Msg.c_str(), Hdr.c_str(), MB_OK | MB_ICONINFORMATION);
}
//---------------------------------------------------------------------------

String GetDirSaveFw(String n){
_di_IXMLDocument document = interface_cast<Xmlintf::IXMLDocument>(new TXMLDocument(NULL));
document->LoadFromFile(GetCurrentDir() + "\\config.xml");
_di_IXMLNodeList nodeList = document->DocumentElement->ChildNodes;
if (n != "OthDev")
  {
  for (int i = 0; i < nodeList->Count; i++)
	{
	_di_IXMLNodeList nodeListRev = nodeList->Get(i)->ChildNodes;
	for (int j = 0; j < nodeListRev->Count; j++)
	  {
	  _di_IXMLNode modelNode = nodeListRev->Get(j);
	  WideString category = modelNode->Attributes["category"];
	  if (category == n) return nodeListRev->Get(j)->ChildNodes->Get(5)->Text;
	  }
	}
  }
else
  {
  UnicodeString Hdr = "Ошибка";
  UnicodeString Msg = "Выберите самостоятельно путь сохранения!";
  MessageBox(GetForegroundWindow(), Msg.c_str(), Hdr.c_str(), MB_OK | MB_ICONINFORMATION);
  }
}

void __fastcall TForm2::ButtonFirstMakeFileClick(TObject *Sender)
{
try
  {
  AnsiString S;
  int SerialBegin,SerialCount;

  //Чистим строки
  S = EditFirstSerialBegin->Text; S = S.Trim(); EditFirstSerialBegin->Text = S;
  S = EditFirstSerialCount->Text; S = S.Trim(); EditFirstSerialCount->Text = S;

  S = EditFirstSerialBegin->Text;
  if(S == "") throw Exception("Начальный серийный номер не указан");

  S = EditFirstSerialCount->Text;
  if(S == "") throw Exception("Число серийных номеров не указано");

  //анализ параметров номеров
  S = EditFirstSerialBegin->Text;
  if(!TryStrToInt(S,SerialBegin)) throw Exception("Начальный серийный номер указан неверно(не число)!");

  if(SerialBegin <= 0) throw Exception("Начальный серийный номер указан неверно (<1)!");

  S = EditFirstSerialCount->Text;
  if(!TryStrToInt(S,SerialCount)) throw Exception("Число серийных номеров указано неверно(не число)!");

  if (SerialCount <= 0) throw Exception("Число серийных номеров указано неверно (<1)!");

  //анализ параметров основной прошивки
  if(FirstDataSize < (2 * 2048)) throw Exception("Файл программы не указан или меньше допустимого (<4 Кб)!");

  //анализ параметров загрузчика
  if(LoaderDataSize < (2 * 2048)) throw Exception("Файл загрузчика не указан или меньше допустимого (<4 Кб)!");

  int ValueCRC32;
  AnsiString Str = EditNewCRC32->Text;
  Str = Str.Trim();
  if (!TryStrToInt("0x" + Str, ValueCRC32)) throw Exception("Контрольная сумма введена неверно!");

  //Спрашиваем куда записывать файлы
  //if(!SelectDirectory(Dir,TSelectDirOpts() << sdAllowCreate << sdPerformCreate << sdPrompt,1000)) return;
  //Dir += "\\";
  String Dir_2 = GetCurrentDir();
  if (Sender == ButtonAutoMakeFiles)
    {
	//Dir += "\\Файл прошивки CPU1\\";
	//Dir += "\\";
	//Dir += ComboBox1->Text;
	//Dir += "\\";
		Dir_2 += "\\" + GetDirSaveFw(ComboBox1->Text) + "\\" + ComboBox1->Text + "\\";
		if (!DirectoryExists(Dir_2)) ForceDirectories(Dir_2);
	}
  else if (Sender == ButtonFirstMakeFile)
	{
	if(!SelectDirectory("Выберите папку","",Dir_2)) return;
	Dir_2 += "\\";
	}

  //Копируем первый блок данных(загрузчик)
  int DataSize = 0;
  memmove(&TmpBuffer, LoaderDataBuffer, LoaderDataSize);
  DataSize = LoaderDataSize;

  //расширяем первый блок данных до 16 Кб
  while(DataSize < (16*1024)) TmpBuffer[DataSize++] = 0xFF;

  //добавляем второй блок данных (программа)
  memmove(&TmpBuffer[DataSize],&FirstDataBuffer, FirstDataSize);
  DataSize += FirstDataSize;

  //Формируем файлы прошивок
  for(int i = 0; i < SerialCount; i++)
    {
    //формируем новый серийный номер в образе прошивки
    AnsiString SSerial = IntToStr(SerialBegin++);
    memset(&TmpBuffer[0x4000 + 0x0900], 0, 64 - 1);
    memmove(&TmpBuffer[0x4000 + 0x0900], SSerial.c_str(), SSerial.Length());

    //формируем таблицу описателя программы
    memset(&TmpBuffer[0x4000 + (2 * 2048) - sizeof(ProgInfo)],0xFF,sizeof(ProgInfo));
    ProgInfo.TableId     = 0x52444C42;
    ProgInfo.ProgramSize = FirstDataSize;
    ProgInfo.ProgramCrc  = CalcCrc32(&TmpBuffer[0x4000],FirstDataSize);
    ProgInfo.TableCrc    = CalcCrc32(&ProgInfo, sizeof(ProgInfo) - sizeof(u32));
    memcpy(&TmpBuffer[0x4000 + (2*2048) - sizeof(ProgInfo)], &ProgInfo, sizeof(ProgInfo));

    //формируем имя файла прошивки
    AnsiString FileName;
	FileName = ComboBox1->Text + "-Ldr+Prog-sn-" + SSerial + ".bin";

    //сохраняем файл прошивки
    TMemoryStream *Stream = new TMemoryStream();
    Stream->Write(&TmpBuffer, DataSize);

    //код реверса crc
    // подготовка таблиц
    //проверка на выбор CRC
    if(CheckBoxUpdateCRC32->Checked)
      {
      unsigned poly = 0xedb88320;
      unsigned startxor = 0xffffffff;
      unsigned fwd, rev;

      for (int i = 0; i < 256; i++)
        {
        fwd = (unsigned) i;
        rev = ((unsigned) i) << (3 * 8);

        for (int j = 8; j > 0; j--)
          {
          if ((fwd & 1) == 1) { fwd = (unsigned)((fwd >> 1) ^ poly); }
          else { fwd >>= 1; }

          if ((rev & 0x80000000) != 0) { rev = ((rev ^ poly) << 1) | 1; }
          else { rev <<= 1; }
          }

        Table[i] = fwd;
        RevTable[i] = rev;
        }


      int SrcSize = Stream->Size;
      Stream->Position = Stream->Size;
      Stream->Write(&SrcSize, 4);
      int BufSize = Stream->Size;
      byte *Src = (byte*) Stream->Memory;
      unsigned wantcrc = (unsigned)ValueCRC32;

      unsigned crc = startxor;
	  for (int i = 0; i < SrcSize; i++) crc = (crc >> 8) ^ Table[(crc ^ Src[i]) & 0xff];
      memmove(&Src[SrcSize], &crc, 4);

      crc = wantcrc ^ startxor;
	  for (int i = BufSize - 1; i >= SrcSize; i--) crc = (crc << 8) ^ RevTable[crc >> (3 * 8)] ^ Src[i];
      memmove(&Src[SrcSize], &crc, 4);
      }


    Stream->Position = 0;
	Stream->SaveToFile(Dir_2 + FileName);
    delete Stream;
    }

  //сообщаем о завершении
  UnicodeString Hdr = "Инфо";
  UnicodeString Msg = "Файлы прошивки успешно созданы";
  MessageBox(Handle,Msg.c_str(),Hdr.c_str(), MB_OK | MB_ICONINFORMATION);
  }
catch(Exception &Exc)
  {
  UnicodeString Hdr = "Ошибка";
  UnicodeString Msg = Exc.Message;
  MessageBox(Handle,Msg.c_str(), Hdr.c_str(), MB_OK | MB_ICONERROR);
  }
}
//---------------------------------------------------------------------------


//-----------------------------Вкладка "Начальная прошивка кабеля"------------//

void __fastcall TForm2::ButtonUpdateSelectFileCabelClick(TObject *Sender)
{
if(!OpenDialogCable->Execute()) return;

try
  {
  ButtonUpdateClearDataCableClick(this);
  DirCableFileName = OpenDialogCable->FileName;

  CableFileType = DirCableFileName.SubString(DirCableFileName.Length()-3,DirCableFileName.Length());

  std::auto_ptr <TFileStream> Stream(new TFileStream(DirCableFileName, fmOpenRead));
  if(CableFileType == ".bin" || CableFileType == ".hex")
  {
  if (Stream->Size > (32 * 1024)) throw Exception("Файл больше максимально допустимого размера (32 Кб)!");
  if (Stream->Size < (1 * 1024)) throw Exception("Файл меньше допустимого размера (1 Кб)!");
  }
  int ReadSize = Stream->Read(&CableDataBuffer,Stream->Size);
  if (ReadSize != Stream->Size) throw Exception("Ошибка при чтении данных файла!");

  CableDataSize = Stream->Size;
  LabelCableFileName->Caption = DirCableFileName;
  LabelCableFileType->Caption = CableFileType;
  LabelCableFileSize->Caption = IntToStr(Stream->Size) + " байт(а)";
  }
catch(Exception &Exc)
  {
  UnicodeString Hdr = "Ошибка";
  UnicodeString Msg = Exc.Message;
  MessageBox(Handle, Msg.c_str(), Hdr.c_str(), MB_OK | MB_ICONERROR);
  }
}
//---------------------------------------------------------------------------

void __fastcall TForm2::ButtonUpdateClearDataCableClick(TObject *Sender)
{
CableDataSize = 0;
LabelCableFileName->Caption = "-";
LabelCableFileSize->Caption = "-";
LabelCableFileType->Caption = "-";
}
//---------------------------------------------------------------------------

void __fastcall TForm2::ButtonCableClearParamsClick(TObject *Sender)
{
CSpinEditChanCable->Value  = 0;
EditCableSerialBegin->Text = 1;
EditCableSerialCount->Text = 0;

CheckBoxProtectDefibrillator->Checked = false;
Label26->Visible = false;
EditCableResistCorrection->Visible = false;
}
//---------------------------------------------------------------------------


void __fastcall TForm2::ButtonCableUpdateMakeFileClick(TObject *Sender)
{
try
  {
  AnsiString S;
  int SerialBegin,SerialCount;

  //чистим строки
  S = EditCableSerialBegin->Text; S = S.Trim(); EditCableSerialBegin->Text = S;
  S = EditCableSerialCount->Text; S = S.Trim(); EditCableSerialCount->Text = S;

  S = EditCableSerialBegin->Text;
  if(S == "") throw Exception("Начальный серийный номер не указан!");

  S = EditCableSerialCount->Text;
  if(S == "") throw Exception("Число серийных номеров не указано!");

  if(CSpinEditChanCable->Value <= 0) throw Exception("Не указан тип кабеля!");

  //анализ параметров номеров
  S = EditCableSerialBegin->Text;
  if(!TryStrToInt(S, SerialBegin)) throw Exception("Начальный серийный номер указан неверно (не число)!");

  if(SerialBegin <= 0) throw Exception("Начальный серийный номер указан неверно (<1)!");

  S = EditCableSerialCount->Text;
  if(!TryStrToInt(S,SerialCount)) throw Exception("Число серийных номеров указано неверно (не число)!");

  if(SerialCount <= 0) throw Exception("Число серийных номеров указано неверно (<1)!");

  //анализ параметров основной прошивки
  if(CableDataSize < (1 * 1024)) throw Exception ("Файл программы не указан или меньше допустимого (<1 Кб)!");
  if(CableDataSize > (32 * 1024)) throw Exception ("Файл программы не указан или больше допустимого (>32 Кб)!");

  // спрашиваем куда записывать файлы
  /*UnicodeString Dir;
  if(!SelectDirectory(Dir,TSelectDirOpts() << sdAllowCreate << sdPerformCreate << sdPrompt,1000)) return;
  Dir += "\\"; */
  UnicodeString Dir;
  if(!SelectDirectory("Выберите папку","",Dir)) return;
  Dir += "\\";

  int CountChan, TypeProtect,ResistanceCorrection;
  ResistanceCorrection =  EditCableResistCorrection->Value;
  CountChan = CSpinEditChanCable->Value;
  TypeProtect = CheckBoxProtectDefibrillator->Checked ? 1 : 0;

  if(CableFileType == ".bin")
    {
    TMemoryStream *Stream = new TMemoryStream();
    Stream->LoadFromFile(DirCableFileName);

    for(int i = 0; i < SerialCount; i++)
      {
      int SerialCable = SerialBegin++;

      //смещаем поток
      Stream->Position = 0x80;
      AnsiString ASerial = IntToStr(SerialCable);
      if(ASerial.Length() > 15) throw Exception("Слишком большой серийный номер!");

      u8 OutputBuffer[16];

      //Записываем серийный номер кабеля
      memset((u8*) &OutputBuffer,0x00,sizeof(OutputBuffer));
      memcpy(&OutputBuffer, ASerial.c_str(),ASerial.Length());
      OutputBuffer[15] = 0xFF;
      Stream->Write(OutputBuffer,sizeof(OutputBuffer));

      //записываем серийный тип кабеля
      memset((u8*) &OutputBuffer,0x00, sizeof(OutputBuffer));
      OutputBuffer[15] = 0xFF;
      OutputBuffer[0] = TypeProtect & 0xFF;
      OutputBuffer[1] = CountChan & 0xFF;
      Stream->Position = 0x90;
      Stream->Write(OutputBuffer, sizeof(OutputBuffer));

      //формируем имя файла прошивки
      AnsiString FileName;
      FileName = "CableFw-type" + IntToStr(TypeProtect) + "-chan" + IntToStr(CountChan) + "-sn" + IntToStr(SerialCable) + ".bin";

      //сохраняем файл прошивки
      Stream->Position = 0;
      Stream->SaveToFile(Dir + FileName);

      Stream->Position = 0;
      }
    delete Stream;
    }
  if(CableFileType == ".hex")
    {
    if(CSpinEditChanCable->Value > 12) throw Exception("Неверное значение типа кабеля!");
    if(EditCableSerialBegin->Text.Length() < 8) throw Exception("Недостаточная длина серийного номера! (Требуется 8 цифр)");
    TStringList *List = new TStringList;
    List->LoadFromFile(DirCableFileName);
    int SerialCount = EditCableSerialCount->Text.ToInt();
    int SerialBegin = EditCableSerialBegin->Text.ToInt();

    for(int i = 0; i < SerialCount; i++)
      {
      int SerialCable = SerialBegin++;
      AnsiString AddrOfSerial = "8080";
      u16 IndexOfSerial = 0;
      AnsiString str;
      for(u16 i = 0; i < List->Count; i++)//находим в файле строку с адресом 8080
        {
        str = List->Strings[i];
        if(str.SubString(4,4) == AddrOfSerial)
          {
          IndexOfSerial = i; //индекс строки сохраняем
          break;
          }
        }
      //-------------------Формирование строки серийного номера---------//
      AnsiString StrSerial = List->Strings[IndexOfSerial];// обращаемся к нужной строке через найденный индекс
      //AnsiString NewSerialNumber = EditCableSerialBegin->Text;
      AnsiString NewSerialNumber = IntToStr(SerialCable);
      const char* strc = NewSerialNumber.c_str();// получаем данные о серийном номере из формы
      AnsiString ResultSerialNumber;
      for(short i = 0; i < NewSerialNumber.Length(); i++)
        {
        ResultSerialNumber += IntToHex((int)strc[i],2);//записываем серийный номер
        }
      StrSerial = StrSerial.SubString(1,9);
      StrSerial = StrSerial + ResultSerialNumber;
      StrSerial = StrSerial + "00000000000000FF";

      //--------Считаем контрольную сумму для поля "серийный номер"-----//
      AnsiString BuffString = StrSerial;
      s32 DataCheckSum = 0;
      AnsiString CheckSum;
      AnsiString buff;
      u16 val = 0;
      while(BuffString.Length() > 1)
        {
	buff = BuffString.SubString(2,2);
	buff = "0x" + buff;
	val = StrToInt(buff);
	DataCheckSum += val;
	BuffString = BuffString.Delete(2,2);
        }
      DataCheckSum = 0 - DataCheckSum;
      DataCheckSum = (DataCheckSum >> (8*4)) & 0xff; // получаем байт контрольной суммы
      CheckSum = IntToHex((int)DataCheckSum,2);// записываем число в формат hex

      StrSerial = StrSerial + CheckSum;//Строка серийного номера сформирована

      //------------------------Формирование поля "тип"-----------------//

      AnsiString StrChanCount = List->Strings[IndexOfSerial + 1];
      int NewChanCount = CSpinEditChanCable->Value;
      AnsiString ChanCountFiller = "00000000000000000000000000FF";//заполнитель для поля с количеством каналов
      AnsiString ResultChanCount = ResultChanCount.IntToHex(NewChanCount,4);//записываем число каналов
      StrChanCount = StrChanCount.SubString(1,9) + ResultChanCount + ChanCountFiller;

      //--------Считаем контрольную сумму для поля "тип кабеля"-----//
      AnsiString BuffStringChan = StrChanCount;
      s32 ChanDataCheckSum = 0;
      AnsiString ChanCheckSum;
      AnsiString ChanBuff;
      u16 ResVal = 0;
      while(BuffStringChan.Length() > 1)
        {
		ChanBuff = BuffStringChan.SubString(2,2);
		ChanBuff = "0x" + ChanBuff;
		ResVal = StrToInt(ChanBuff);
		ChanDataCheckSum += ResVal;
		BuffStringChan = BuffStringChan.Delete(2,2);
        }
      ChanDataCheckSum = 0 - ChanDataCheckSum;
      ChanDataCheckSum = (ChanDataCheckSum >> (8*4)) & 0xff; // получаем байт контрольной суммы
      ChanCheckSum = IntToHex((int)ChanDataCheckSum,2);// записываем число в формат hex

      StrChanCount = StrChanCount + ChanCheckSum;//Строка "тип кабеля" сформирована

      //--Формирование поля "Результирующая коррекция сопротивления"----//

      AnsiString ResultCableResist;//Результирующая коррекция сопротивления
      AnsiString ResultCableResistString;
      int NewCableResist;
      if(CheckBoxProtectDefibrillator->Checked && (EditCableResistCorrection->Value == 0)) throw Exception("Коррекция сопротивления должна быть > 0!");
      if(CheckBoxProtectDefibrillator->Checked && (EditCableResistCorrection > 0))
      	{
	NewCableResist = EditCableResistCorrection->Value;
	}
      if(!CheckBoxProtectDefibrillator->Checked)
        {
        NewCableResist = 0;
        }
      AnsiString BuffStringRes = List->Strings[IndexOfSerial + 2];
      AnsiString ResistFiller = "00000000000000000000000000FF";//заполнитель для поля с коррекцией сопротивления
      ResultCableResist = ResultCableResist.IntToHex(NewCableResist,4);
      ResultCableResistString = BuffStringRes.SubString(1,9) + ResultCableResist + ResistFiller;

      //----расчёт контольной суммы для строки "коррекция сопротивления"--//
      AnsiString BuffResist = ResultCableResistString;
      s32 ResDataCheckSum = 0;
      AnsiString ResCheckSum;
      AnsiString ResBuff;
      u16 ResistVal = 0;
      while(BuffResist.Length() > 1)
        {
		ResBuff = BuffResist.SubString(2,2);
		ResBuff = "0x" + ResBuff;
		ResistVal = StrToInt(ResBuff);
		ResDataCheckSum += ResistVal;
		BuffResist = BuffResist.Delete(2,2);
        }
      ResDataCheckSum = 0 - ResDataCheckSum;
      ResDataCheckSum = (ResDataCheckSum >> (8*4)) & 0xff; // получаем байт контрольной суммы
      ResCheckSum = IntToHex((int)ResDataCheckSum,2);// записываем число в формат hex
      ResultCableResistString += ResCheckSum;

      //Обновление данных в строках 8080,8090,80А0
      List->Strings[IndexOfSerial] = StrSerial;
      List->Strings[IndexOfSerial + 1] = StrChanCount;
      List->Strings[IndexOfSerial + 2] = ResultCableResistString;
      //Формирование имени файла
      AnsiString FileName;
      FileName = "CableFw-type" + IntToStr(TypeProtect) + "-chan" + IntToStr(CountChan) + "-sn" + NewSerialNumber + ".hex";

      List->SaveToFile(Dir + FileName);
      }
    delete List;
    }
  //сообщаем о завершении
  UnicodeString Hdr = "Инфо";
  UnicodeString Msg = "Файлы прошивки успешно созданы.";
  MessageBox(Handle, Msg.c_str(),Hdr.c_str(),MB_OK | MB_ICONINFORMATION);
  }
catch(Exception &Exc)
  {
  UnicodeString Hdr = "Ошибка";
  UnicodeString Msg = Exc.Message;
  MessageBox(Handle, Msg.c_str(), Hdr.c_str(), MB_OK | MB_ICONERROR);
  }
}
//---------------------------------------------------------------------------

AnsiString TForm2::GetModelName(int index)
{
AnsiString ModelName;
switch(index)
  {
  case 0: { ModelName = "KR02Ru"; break;}
  case 1: { ModelName = "KR03Ru"; break;}
  case 2: { ModelName = "KR04Ru"; break;}
  case 3: { ModelName = "KR05Ru"; break;}

  case 4: { ModelName = "KR06Ru_R2"; break;}
  case 5: { ModelName = "KR06Ru_R3"; break;}
  case 6: { ModelName = "KR06Ru_R4"; break;}

  case 7: { ModelName = "KR07Ru_R3"; break;}
  case 8: { ModelName = "KR07Ru_R4"; break;}

  case 9: { ModelName = "KR08Ru_R3"; break;}

  case 10: { ModelName = "IN22_R1";   break;}
  case 11: { ModelName = "IN33_R1";   break;}
  case 12: { ModelName = "IN33_R2";   break;}
  case 13: { ModelName = "KAMA_R1";   break;}
  case 14: { ModelName = "KAMA_R2";   break;}

  case 15: { ModelName = "MD01M_vB";  break;}
  case 16: { ModelName = "MD01M_vC";  break;}
  case 17: { ModelName = "MDZ_R1";  break;}

  default:{ ModelName = "OthDev"; break;}
  }
return ModelName;
}
//---------------------------------------------------------------------------

void __fastcall TForm2::FormCreate(TObject *Sender)
{
PageControl1->ActivePage = TabSheetFirstFile;
// ***********

StreamLoaderModel = new TMemoryStream();

LoaderDataSize = 0;
FirstDataSize = 0;

// *******

ButtonFirstClearDataClick(this);
ButtonFirstClearLoaderClick(this);
ButtonFirstClearParamsClick(this);

ButtonUpdateClearDataClick(this);
ButtonUpdateClearParamsClick(this);
}
//---------------------------------------------------------------------------

void __fastcall TForm2::CheckBoxProtectDefibrillatorClick(TObject *Sender)
{
if(CheckBoxProtectDefibrillator->Checked == true)
  {
  Label26->Visible = true;
  EditCableResistCorrection->Visible = true;
  EditCableResistCorrection->Text = "0";
  }
  else
  {
  Label26->Visible = false;
  EditCableResistCorrection->Visible = false;
  }
}
//---------------------------------------------------------------------------

bool TForm2::UserSelectDirectory(const UnicodeString ACaption, const WideString ARoot, UnicodeString &ADirectory)
{
wchar_t *Buffer = new wchar_t[MAX_PATH];

ITEMIDLIST *RootItemIDList = NULL;
IShellFolder *IDesktopFolder;
if (ARoot != WideString(""))
  {
  SHGetDesktopFolder(&IDesktopFolder);
  IDesktopFolder->ParseDisplayName(0, NULL,
  POleStr(ARoot), NULL, &RootItemIDList, NULL);
  }

BROWSEINFO bi;
bi.hwndOwner = 0;
bi.pidlRoot = RootItemIDList;
bi.lpszTitle = ACaption.c_str();
bi.ulFlags = BIF_NEWDIALOGSTYLE|BIF_EDITBOX;
bi.lpfn = NULL;
bi.lParam = 0;
bi.pszDisplayName = Buffer;

LPITEMIDLIST pidlBrowse = SHBrowseForFolder(&bi);
if (pidlBrowse != NULL)
  {
  if (SHGetPathFromIDList(pidlBrowse, Buffer))
  ADirectory = Buffer;
  }
delete [] Buffer;
return DirectoryExists(ADirectory);
}
//---------------------------------------------------------------------------

Model GetModel(String name, std::vector<Model> m)
{
Model h;
for (int i = 0; i < m.size(); i++)
  {
  if(m[i].name == name)h = m[i];
  }
return h;
}
//---------------------------------------------------------------------------

String GetBldrDevModel(String n){
_di_IXMLDocument document = interface_cast<Xmlintf::IXMLDocument>(new TXMLDocument(NULL));
document->LoadFromFile(GetCurrentDir() + "\\config.xml");
_di_IXMLNodeList nodeList = document->DocumentElement->ChildNodes;
for (int i = 0; i < nodeList->Count; i++)
  {
  _di_IXMLNodeList nodeListRev = nodeList->Get(i)->ChildNodes;
  for (int j = 0; j < nodeListRev->Count; j++)
	{
	_di_IXMLNode modelNode = nodeListRev->Get(j);
	WideString category = modelNode->Attributes["category"];
	if (category == n) return modelNode->Attributes["BldrDevModel"];
	}
  }
}

void __fastcall TForm2::ComboBox1Change(TObject *Sender)
{
try
  {
  std::vector<Model> models;

  ComboBox2->Text = ComboBox1->Text;

  _di_IXMLDocument document = interface_cast<Xmlintf::IXMLDocument>(new TXMLDocument(NULL));
  document->LoadFromFile(GetCurrentDir() + "\\config.xml");
  _di_IXMLNodeList nodeList = document->DocumentElement->ChildNodes;
  for (int i = 0; i < nodeList->Count; i++)
	{
	_di_IXMLNodeList nodeListRev = nodeList->Get(i)->ChildNodes;
	for (int j = 0; j < nodeListRev->Count; j++)
	  {
	  _di_IXMLNode modelNode = nodeListRev->Get(j);
	  WideString category = modelNode->Attributes["category"];
	  _di_IXMLNode childNode_boot = modelNode->ChildNodes->Get(0);
	  _di_IXMLNode childNode = modelNode->ChildNodes->Get(1);

	  String c = category;
	  String d = childNode->Text;
	  String e = childNode_boot->Text;

	  Model a(c,d, e);
	  models.push_back(a);
	  }
	}
  Model m = GetModel(ComboBox1->Text, models);

  if (ComboBox1->Text == "OthDev") EditDevModel->Text = ComboBox1->Text;
  else EditDevModel->Text = GetBldrDevModel(ComboBox1->Text);

  EditDevModelChange(0);

  if (m.name.IsEmpty())
	{
	LabelFirstFileName->Caption = "-";
	LabelLoaderFileName->Caption = "-";
	LabelSumSize->Caption = "-";
	LabelLoaderFileSize->Caption = "-";
	LabelFirstFileSize->Caption = "-";
	}
  else
	{
	String FileName =  GetCurrentDir() + "\\" + m.mainfile;
	String FileName_boot =  GetCurrentDir() + "\\" + m.boot;
	LabelFirstFileName->Caption = FileName;
	LabelLoaderFileName->Caption = FileName_boot;

	std::auto_ptr<TFileStream> Stream(new TFileStream(FileName, fmOpenRead));
	if (Stream->Size > ((1024 - 16) * 1024))throw Exception("Файл больше максимально допустимого размера (1024-16 Кб)");
	int ReadSize = Stream->Read(&FirstDataBuffer, Stream->Size);
	if(ReadSize != Stream->Size)throw Exception("Ошибка при чтении данных файла");
	FirstDataSize = Stream->Size;
	LabelFirstFileName->Caption = FileName;
	LabelFirstFileSize->Caption = IntToStr(Stream->Size) + " байт(а)";
	if(LoaderDataSize)LabelSumSize->Caption = IntToStr(FirstDataSize + LoaderDataSize) + " байт(а)";
	//AutoDefineDevModel(FirstDataBuffer);


	std::auto_ptr<TFileStream> Stream1(new TFileStream(FileName_boot, fmOpenRead));
	if(Stream1->Size > (16 * 1024))throw Exception("Файл больше максимально допустимого размера (16 Кб)");
	int ReadSize1 = Stream1->Read(&LoaderDataBuffer, Stream1->Size);
	if(ReadSize1 != Stream1->Size)throw Exception("Ошибка при чтении данных файла");
	LoaderDataSize = Stream1->Size;
	LabelLoaderFileName->Caption = FileName_boot;
	LabelLoaderFileSize->Caption = IntToStr(Stream1->Size) + " байт(а)";
	if(FirstDataSize)LabelSumSize->Caption = IntToStr(FirstDataSize + LoaderDataSize) + " байт(а)";
	//AutoDefineDevModel(FirstDataBuffer);
	}
  }
catch(Exception &Exc)
  {
  UnicodeString Hdr = "Ошибка";
  UnicodeString Msg = Exc.Message;
  MessageBox(Handle, Msg.c_str(), Hdr.c_str(), MB_OK | MB_ICONERROR);
  }
}
//---------------------------------------------------------------------------
String GetDev(String n){
_di_IXMLDocument document = interface_cast<Xmlintf::IXMLDocument>(new TXMLDocument(NULL));
document->LoadFromFile(GetCurrentDir() + "\\config.xml");
_di_IXMLNodeList nodeList = document->DocumentElement->ChildNodes;
for (int i = 0; i < nodeList->Count; i++)
  {
  _di_IXMLNodeList nodeListRev = nodeList->Get(i)->ChildNodes;
  for (int j = 0; j < nodeListRev->Count; j++)
	{
	_di_IXMLNode modelNode = nodeListRev->Get(j);
	WideString category = modelNode->Attributes["BldrDevModel"];
	if (category == n) return modelNode->Attributes["category"];
	}
  }
}

void __fastcall TForm2::EditDevModelChange(TObject *Sender)
{
try
  {
  if (EditDevModel->Text != GetBldrDevModel(ComboBox1->Text))
	{
	ComboBox1->Text = GetDev(EditDevModel->Text);
	ComboBox2->Text = GetDev(EditDevModel->Text);
	}

  String FileName;

  _di_IXMLDocument document = interface_cast<Xmlintf::IXMLDocument>(new TXMLDocument(NULL));
  document->LoadFromFile(GetCurrentDir() + "\\config.xml");
  _di_IXMLNodeList nodeList = document->DocumentElement->ChildNodes;

  if (EditDevModel->Text != "OthDev")
	{
	for (int i = 0; i < nodeList->Count; i++)
	  {
	  _di_IXMLNodeList nodeListRev = nodeList->Get(i)->ChildNodes;
	  for (int j = 0; j < nodeListRev->Count; j++)
		{
		_di_IXMLNode modelNode = nodeListRev->Get(j);
		WideString category = modelNode->Attributes["BldrDevModel"];
		if (category == EditDevModel->Text)
		  {
		  FileName = GetCurrentDir() + "\\" + modelNode->ChildNodes->Get(1)->Text;
		  LabelUpdateFileName->Caption = FileName;

		  CheckBoxIsBaseProg->Checked = (modelNode->ChildNodes->Get(2)->Attributes["main_prog"] == "0") ? false : true;
		  CheckBoxOnlySelSerial->Checked = (modelNode->ChildNodes->Get(2)->Attributes["OnlyForEnteredSN"] == "0") ? false : true;
		  CheckBoxOnlySelDevId->Checked = (modelNode->ChildNodes->Get(2)->Attributes["OnlyForEnteredDevID"] == "0") ? false : true;
		  CheckBoxNoModelCheck->Checked = (modelNode->ChildNodes->Get(2)->Attributes["NoCheckModel"] == "0") ? false : true;
		  CheckBoxSaveBthName->Checked = (modelNode->ChildNodes->Get(3)->Attributes["SaveBthName"] == "0") ? false : true;
		  CheckBoxSaveBthAddr->Checked = (modelNode->ChildNodes->Get(3)->Attributes["SaveBthAddr"] == "0") ? false : true;
		  CheckBoxSaveDesc->Checked = (modelNode->ChildNodes->Get(3)->Attributes["SaveDevDesc"] == "0") ? false : true;
		  CheckBoxSaveHwVersion->Checked = (modelNode->ChildNodes->Get(3)->Attributes["SaveHwVerr"] == "0") ? false : true;
		  CheckBoxSaveSwVersion->Checked = (modelNode->ChildNodes->Get(3)->Attributes["SaveSwVerr"] == "0") ? false : true;
		  CheckBoxSaveSerial->Checked = (modelNode->ChildNodes->Get(3)->Attributes["SaveDevSerial"] == "0") ? false : true;
		  CheckBoxSaveModel->Checked = (modelNode->ChildNodes->Get(3)->Attributes["SaveDevModel"] == "0") ? false : true;
		  CheckBoxEnableLvl1->Checked = (modelNode->ChildNodes->Get(3)->Attributes["Lvl1MemProtection"] == "0") ? false : true;
		  EditProgBeginAddr->Text = modelNode->ChildNodes->Get(4)->Text;
		  break;
		  }

		}
	  }

	std::auto_ptr<TFileStream> Stream(new TFileStream(FileName, fmOpenRead));
	if(Stream->Size > ((1024 - 16) * 1024))throw Exception("Файл больше максимально допустимого размера (1024-16 Кб)");
	int ReadSize = Stream->Read(&UpdateDataBuffer,Stream->Size);
	if(ReadSize != Stream->Size)throw Exception("Ошибка при чтении данных файла");
	UpdateDataSize = Stream->Size;
	LabelUpdateFileName->Caption = FileName;
	LabelUpdateFileSize->Caption = IntToStr(Stream->Size) + "байт(а)";
	//AutoDefineDevModel(UpdateDataBuffer);
	}
  else
	{
	LabelUpdateFileName->Caption = "-";
	LabelUpdateFileSize->Caption = "-";
	CheckBoxIsBaseProg->Checked = false;
	CheckBoxOnlySelSerial->Checked = false;
	CheckBoxOnlySelDevId->Checked = false;
	CheckBoxNoModelCheck->Checked = false;
	CheckBoxSaveBthName->Checked = false;
	CheckBoxSaveBthAddr->Checked = false;
	CheckBoxSaveDesc->Checked = false;
	CheckBoxSaveHwVersion->Checked = false;
	CheckBoxSaveSwVersion->Checked = false;
	CheckBoxSaveSerial->Checked = false;
	CheckBoxSaveModel->Checked = false;
	CheckBoxEnableLvl1->Checked = false;
	EditProgBeginAddr->Text = "";
	}
  }
catch(Exception &Exc)
  {
  UnicodeString Hdr = "Ошибка";
  UnicodeString Msg = Exc.Message;
  MessageBox(Handle, Msg.c_str(), Hdr.c_str(), MB_OK | MB_ICONERROR);
  }
}
//---------------------------------------------------------------------------


void __fastcall TForm2::ComboBox2Change(TObject *Sender)
{
ComboBox1->Text = ComboBox2->Text;

ComboBox1Change(0);
}
//---------------------------------------------------------------------------

void DeleteDir(AnsiString DirName)
{
TSearchRec sr;
if (DirName.Length())
  {
   if (!FindFirst(DirName+"\\*.*",faAnyFile,sr))
   do
	{
	if (!(sr.Name=="." || sr.Name==".."))
	if (((sr.Attr & faDirectory) == faDirectory ) ||
	(sr.Attr == faDirectory))// найдена папка
	  {
	  FileSetAttr(DirName+"\\"+sr.Name, faDirectory );// сброс всяких read-only
	  DeleteDir(DirName+"\\"+sr.Name);//рекурсивно удаляем содержимое
	  RemoveDir(DirName + "\\"+sr.Name);// удаляем теперь уже пустую папку
	  }
	 else// иначе найден файл
	   {
	   FileSetAttr(DirName+"\\"+sr.Name, 0);// сброс всяких read-only
	   DeleteFile(DirName+"\\"+sr.Name);// удаляем файл
	   }
	}
  while (!FindNext(sr));// ищем опять, пока не найдем все
  FindClose(sr);
  }
RemoveDir(DirName);
}

void __fastcall TForm2::Button1Click(TObject *Sender)
{
_di_IXMLDocument document = interface_cast<Xmlintf::IXMLDocument>(new TXMLDocument(NULL));
document->LoadFromFile(GetCurrentDir() + "\\config.xml");
_di_IXMLNodeList nodeList = document->DocumentElement->ChildNodes;
for (int i = 0; i < nodeList->Count; i++)
  {
  _di_IXMLNodeList nodeListRev = nodeList->Get(i)->ChildNodes;
  for (int j = 0; j < nodeListRev->Count; j++)
	{
	_di_IXMLNode modelNode = nodeListRev->Get(j);
	_di_IXMLNode childNode_save = modelNode->ChildNodes->Get(5);

	DeleteDir(GetCurrentDir() + "\\" + childNode_save->Text);
	}
  }

UnicodeString Hdr = "Очищено";
UnicodeString Msg = "Все сохраненные файлы прошивки удалены!";
MessageBox(Handle, Msg.c_str(), Hdr.c_str(), MB_OK | MB_ICONINFORMATION);
}
//---------------------------------------------------------------------------

void __fastcall TForm2::Button2Click(TObject *Sender)
{
_di_IXMLDocument document = interface_cast<Xmlintf::IXMLDocument>(new TXMLDocument(NULL));
document->LoadFromFile(GetCurrentDir() + "\\config.xml");
_di_IXMLNodeList nodeList = document->DocumentElement->ChildNodes;
for (int i = 0; i < nodeList->Count; i++)
  {
  _di_IXMLNodeList nodeListRev = nodeList->Get(i)->ChildNodes;
  for (int j = 0; j < nodeListRev->Count; j++)
	{
	_di_IXMLNode modelNode = nodeListRev->Get(j);
	_di_IXMLNode childNode_save = modelNode->ChildNodes->Get(6);

	DeleteDir(GetCurrentDir() + "\\" + childNode_save->Text);
	}
  }

UnicodeString Hdr = "Очищено";
UnicodeString Msg = "Все сохраненные файлы обновления прошивки удалены!";
MessageBox(Handle, Msg.c_str(), Hdr.c_str(), MB_OK | MB_ICONINFORMATION);
}
//---------------------------------------------------------------------------

