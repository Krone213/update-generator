//---------------------------------------------------------------------------

#ifndef Unit2H
#define Unit2H
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ComCtrls.hpp>
#include <Vcl.Dialogs.hpp>
#include <Vcl.Samples.Spin.hpp>
#include <Vcl.ExtDlgs.hpp>
#include <vector>
#include <fstream>
#include <string>
#define NO_WIN32_LEAN_AND_MEAN
#include "shlobj.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
typedef signed char s8;
typedef signed short s16;
typedef signed long s32;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
//---------------------------------------------------------------------------

#pragma pack(push,1)
struct TUpdateTask
{
	u32 FileId;                         //маркер файла ( = "BLDR" , 0x52444C42)
	u32 BlockType;                      //тип блока ( = "0x00010000")
	u32 Version;                        //версия блока (= 1.00, = 0x00000100)
	u32 BlockSize;                      //общий размер блока, байт (=512)

	char ModelInfo[64];                 //строка модели(= "KR05.ru vA")
	char DescInfo[64];                  //строка доп.описания(= "")
	char DevSerial[64];                 //строка серийного номера(= "")
	char DevId[64];                     //строка ID устройства (= "")

	u32 EraseBeginAddr;                 //начальный адрес стираемой области
	u32 EraseEndAddr;                   //конечный адрес стираемой области

	u32 ProgramBeginAddr;               //начальный адрес прошивки
	u32 ProgramSize;                    //размер прошивки, байт
    u32 ProgramCrc;                     //CRC данных прошивки

	u32 LoaderBaseCmd;                  //маска основных команд загрузчику
	u32 LoaderExtCmd;                   //маска дополнительных команд загрузчику

	u32 LoaderBeforeCmd;                //маска команд загрузчику на операции до записи
	u32 LoaderAfterCmd;                 //маска команд загрузчику на операции после записи

	u8 Time[8];                         //время создания (= double/TDateTime)
	u8 Res[192];                        //резерв
	u32 Crc;                            //CRC блока кроме этого поля
};
#pragma pack(pop)
//----------------------------------------------------------------------------
//таблица описания программы
#pragma pack(push,1)
struct TProgramInfo
{
	u32 TableId;                        //маркер заголовка таблицы("BLDR")
	u32 ProgramSize;                    //размер основной программы
	u32 ProgramCrc;                     //CRC основной программы
	u32 TableCrc;                       //CRC таблицы, кроме этого поля
};
#pragma pack(pop)
//===========================================================================

//команды загрузчику
#define CMD_IS_BASE_PROGRAM              ((u32)0x00000001)        //прошивка содержит основную программу(можно переносить описатели, сравнивать по номеру, нужна таблица описателя программы)
#define CMD_UPDATE_SAME_SERIAL           ((u32)0x00000002)        //обновлять только если совпадает серийный номер
#define CMD_UPDATE_SAME_DEV_ID           ((u32)0x00000003)        //обновлять только если совпадает ID устройства
#define CMD_NO_MODEL_CHECK               ((u32)0x00000080)        //не проверять соответствие модели файла и устройства

#define CMD_SAVE_BTH_NAME                ((u32)0x00000001)        //не обновлять (сохранять прежний) блок "BTH имя прибора"
#define CMD_SAVE_BTH_ADDR                ((u32)0x00000002)        //не обновлять (сохранять прежний) блок "BTH адрес прибора"
#define CMD_SAVE_DEV_DESC                ((u32)0x00000004)        //не обновлять (сохранять прежний) блок "описание прибора"
#define CMD_SAVE_HW_VERSION              ((u32)0x00000008)        //не обновлять (сохранять прежний) блок "версия аппаратуры прибора"
#define CMD_SAVE_SW_VERSION              ((u32)0x00000010)        //не обновлять (сохранять прежний) блок "версия прошивки прибора"
#define CMD_SAVE_SERIAL                  ((u32)0x00000020)        //не обновлять (сохранять прежний) блок "серийный номер прибора"
#define CMD_SAVE_MODEL_INFO              ((u32)0x00000040)        //не обновлять (сохранять прежний) блок "модель прибора"
#define CMD_SET_RDP_LV1                  ((u32)0x00000001)        //установить защиту памяти level1  */
//===========================================================================

class TForm2 : public TForm
{
__published:	// IDE-managed Components
	TPageControl *PageControl1;
	TTabSheet *TabSheetFirstFile;
	TGroupBox *GroupBox5;
	TLabel *Label10;
	TLabel *Label11;
	TLabel *LabelFirstFileName;
	TLabel *LabelFirstFileSize;
	TButton *ButtonFirstSelectFile;
	TButton *ButtonFirstClearData;
	TGroupBox *GroupBox7;
	TLabel *Label15;
	TLabel *Label16;
	TLabel *LabelLoaderFileName;
	TLabel *LabelLoaderFileSize;
	TButton *ButtonLoaderSelectFile;
	TButton *ButtonFirstClearLoader;
	TGroupBox *GroupBox1;
	TLabel *Label17;
	TLabel *LabelSumSize;
	TButton *ButtonFirstClearParams;
	TGroupBox *GroupBox2;
	TLabel *Label12;
	TLabel *Label13;
	TEdit *EditFirstSerialBegin;
	TEdit *EditFirstSerialCount;
	TButton *ButtonFirstHelp;
	TButton *ButtonFirstMakeFile;
	TTabSheet *TabSheetUpdateFile;
	TGroupBox *GroupBox3;
	TLabel *Label1;
	TLabel *Label2;
	TLabel *LabelUpdateFileName;
	TLabel *LabelUpdateFileSize;
	TButton *ButtonUpdateSelectFile;
	TButton *ButtonUpdateClearData;
	TGroupBox *GroupBox4;
	TLabel *Label3;
	TLabel *Label4;
	TLabel *Label5;
	TLabel *Label6;
	TLabel *Label7;
	TLabel *Label8;
	TLabel *Label9;
	TButton *ButtonUpdateClearParams;
	TComboBox *EditDevModel;
	TEdit *EditDevDesc;
	TEdit *EditDevSerial;
	TEdit *EditDevId;
	TEdit *EditEraseBeginAddr;
	TEdit *EditEraseEndAddr;
	TEdit *EditProgBeginAddr;
	TGroupBox *GroupBox6;
	TCheckBox *CheckBoxIsBaseProg;
	TCheckBox *CheckBoxOnlySelSerial;
	TCheckBox *CheckBoxOnlySelDevId;
	TCheckBox *CheckBoxNoModelCheck;
	TGroupBox *GroupBox8;
	TCheckBox *CheckBoxSaveBthName;
	TCheckBox *CheckBoxSaveBthAddr;
	TCheckBox *CheckBoxSaveDesc;
	TCheckBox *CheckBoxSaveHwVersion;
	TCheckBox *CheckBoxSaveSwVersion;
	TCheckBox *CheckBoxSaveSerial;
	TCheckBox *CheckBoxSaveModel;
	TButton *ButtonUpdateHelp;
	TButton *ButtonUpdateMakeFile;
	TTabSheet *TabSheetUpdateFileCabel;
	TGroupBox *GroupBox9;
	TLabel *Label18;
	TLabel *Label19;
	TLabel *LabelCableFileName;
	TLabel *LabelCableFileSize;
	TButton *ButtonUpdateSelectFileCabel;
	TButton *ButtonUpdateClearDataCable;
	TGroupBox *GroupBox10;
	TLabel *Label22;
	TGroupBox *GroupBox11;
	TLabel *Label20;
	TLabel *Label21;
	TEdit *EditCableSerialBegin;
	TEdit *EditCableSerialCount;
	TButton *ButtonCableClearParams;
	TSpinEdit *CSpinEditChanCable;
	TCheckBox *CheckBoxProtectDefibrillator;
	TButton *ButtonCableUpdateMakeFile;
	TSaveDialog *SaveDialog1;
	TOpenDialog *OpenDialog1;
	TOpenDialog *OpenDialogCable;
	TOpenDialog *OpenDialogLoader;
	TLabel *Label23;
	TLabel *Label24;
	TEdit *EditNewCRC32;
	TCheckBox *CheckBoxUpdateCRC32;
	TLabel *Label25;
	TLabel *LabelCableFileType;
	TLabel *Label26;
	TSpinEdit *EditCableResistCorrection;
        TButton *ButtonAutoMakeFiles;
        TButton *ButtonUpdateAutoMakeFile;
        TCheckBox *CheckBoxEnableLvl1;
	TComboBox *ComboBox1;
	TLabel *Label27;
	TComboBox *ComboBox2;
	TLabel *Label14;
	TButton *Button1;
	TButton *Button2;

	void __fastcall ButtonUpdateClearDataClick(TObject *Sender);
	void __fastcall ButtonUpdateClearParamsClick(TObject *Sender);
	void __fastcall ButtonUpdateSelectFileClick(TObject *Sender);
	void __fastcall ButtonUpdateHelpClick(TObject *Sender);
	void __fastcall ButtonFirstClearDataClick(TObject *Sender);
	void __fastcall ButtonFirstClearLoaderClick(TObject *Sender);
	void __fastcall ButtonFirstSelectFileClick(TObject *Sender);
	void __fastcall ButtonLoaderSelectFileClick(TObject *Sender);
	void __fastcall ButtonFirstClearParamsClick(TObject *Sender);
	void __fastcall ButtonFirstHelpClick(TObject *Sender);
	void __fastcall ButtonFirstMakeFileClick(TObject *Sender);
	void __fastcall ButtonUpdateMakeFileClick(TObject *Sender);
	void __fastcall ButtonUpdateSelectFileCabelClick(TObject *Sender);
	void __fastcall ButtonUpdateClearDataCableClick(TObject *Sender);
	void __fastcall ButtonCableClearParamsClick(TObject *Sender);
	void __fastcall ButtonCableUpdateMakeFileClick(TObject *Sender);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall CheckBoxProtectDefibrillatorClick(TObject *Sender);
	void __fastcall ComboBox1Change(TObject *Sender);
	void __fastcall EditDevModelChange(TObject *Sender);
	void __fastcall ComboBox2Change(TObject *Sender);
	void __fastcall Button1Click(TObject *Sender);
	void __fastcall Button2Click(TObject *Sender);








private:	// User declarations
public:		// User declarations
	__fastcall TForm2(TComponent* Owner);
    TUpdateTask FirstTask;
	u8 FirstDataBuffer[1024 * 1024];
	int FirstDataSize;

	u8 LoaderDataBuffer[16 * 1024];
	int LoaderDataSize;

	TUpdateTask UpdateTask;
	u8 UpdateDataBuffer[1024 * 1024];
	int UpdateDataSize;

	u8 CableDataBuffer[1024 * 1024];
	int CableDataSize;
	AnsiString DirCableFileName;
	AnsiString CableFileType;

	TMemoryStream *StreamLoaderModel;
	u8 TmpBuffer[1024*1024];
	TProgramInfo ProgInfo;

	AnsiString ExpDirectoryName(AnsiString ADir);
    AnsiString GetModelName(int index);

	unsigned Table[256];
	unsigned RevTable[256];

    bool UserSelectDirectory(const UnicodeString ACaption, const WideString ARoot, UnicodeString &ADirectory);

};
//---------------------------------------------------------------------------
extern PACKAGE TForm2 *Form2;
//---------------------------------------------------------------------------
#endif
