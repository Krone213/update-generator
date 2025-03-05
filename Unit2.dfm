object Form2: TForm2
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu, biMinimize]
  BorderStyle = bsSingle
  Caption = 'UpdateGenerator v2.4'
  ClientHeight = 545
  ClientWidth = 860
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poDesigned
  PixelsPerInch = 96
  TextHeight = 13
  object PageControl1: TPageControl
    Left = 0
    Top = 0
    Width = 860
    Height = 545
    ActivePage = TabSheetUpdateFile
    Align = alClient
    TabOrder = 0
    object TabSheetFirstFile: TTabSheet
      Caption = #1053#1072#1095#1072#1083#1100#1085#1072#1103' '#1087#1088#1086#1096#1080#1074#1082#1072
      ExplicitLeft = 8
      ExplicitTop = 28
      object Label27: TLabel
        Left = 317
        Top = 21
        Width = 41
        Height = 13
        Caption = #1056#1077#1074#1080#1079#1080#1103
      end
      object GroupBox5: TGroupBox
        Left = 19
        Top = 56
        Width = 817
        Height = 105
        Caption = #1060#1072#1081#1083' '#1087#1088#1086#1075#1088#1072#1084#1084#1099'/'#1076#1072#1085#1085#1099#1093
        TabOrder = 0
        object Label10: TLabel
          Left = 16
          Top = 64
          Width = 30
          Height = 13
          Caption = #1060#1072#1081#1083':'
        end
        object Label11: TLabel
          Left = 16
          Top = 83
          Width = 39
          Height = 13
          Caption = #1056#1072#1079#1084#1077#1088':'
        end
        object LabelFirstFileName: TLabel
          Left = 61
          Top = 64
          Width = 4
          Height = 13
          Caption = '-'
        end
        object LabelFirstFileSize: TLabel
          Left = 61
          Top = 83
          Width = 4
          Height = 13
          Caption = '-'
        end
        object ButtonFirstSelectFile: TButton
          Left = 16
          Top = 24
          Width = 105
          Height = 25
          Caption = #1042#1099#1073#1088#1072#1090#1100' '#1092#1072#1081#1083
          TabOrder = 0
          OnClick = ButtonFirstSelectFileClick
        end
        object ButtonFirstClearData: TButton
          Left = 672
          Top = 24
          Width = 129
          Height = 25
          Caption = #1054#1095#1080#1089#1090#1080#1090#1100
          TabOrder = 1
          OnClick = ButtonFirstClearDataClick
        end
      end
      object GroupBox7: TGroupBox
        Left = 19
        Top = 172
        Width = 817
        Height = 105
        Caption = #1047#1072#1075#1088#1091#1079#1095#1080#1082
        TabOrder = 1
        object Label15: TLabel
          Left = 16
          Top = 64
          Width = 30
          Height = 13
          Caption = #1060#1072#1081#1083':'
        end
        object Label16: TLabel
          Left = 16
          Top = 83
          Width = 39
          Height = 13
          Caption = #1056#1072#1079#1084#1077#1088':'
        end
        object LabelLoaderFileName: TLabel
          Left = 61
          Top = 64
          Width = 4
          Height = 13
          Caption = '-'
        end
        object LabelLoaderFileSize: TLabel
          Left = 61
          Top = 83
          Width = 4
          Height = 13
          Caption = '-'
        end
        object ButtonLoaderSelectFile: TButton
          Left = 16
          Top = 24
          Width = 105
          Height = 25
          Caption = #1042#1099#1073#1088#1072#1090#1100' '#1092#1072#1081#1083
          TabOrder = 0
          OnClick = ButtonLoaderSelectFileClick
        end
        object ButtonFirstClearLoader: TButton
          Left = 672
          Top = 24
          Width = 129
          Height = 25
          Caption = #1054#1095#1080#1089#1090#1080#1090#1100
          TabOrder = 1
          OnClick = ButtonFirstClearLoaderClick
        end
      end
      object GroupBox1: TGroupBox
        Left = 19
        Top = 283
        Width = 817
        Height = 181
        Caption = #1055#1072#1088#1072#1084#1077#1090#1088#1099
        TabOrder = 2
        object Label17: TLabel
          Left = 17
          Top = 136
          Width = 187
          Height = 13
          Caption = #1057#1091#1084#1084#1072#1088#1085#1099#1081' '#1088#1072#1079#1084#1077#1088' '#1092#1072#1081#1083#1072' '#1087#1088#1086#1096#1080#1074#1082#1080':'
        end
        object LabelSumSize: TLabel
          Left = 210
          Top = 136
          Width = 4
          Height = 13
          Caption = '-'
        end
        object Label23: TLabel
          Left = 16
          Top = 139
          Width = 3
          Height = 13
        end
        object ButtonFirstClearParams: TButton
          Left = 672
          Top = 16
          Width = 129
          Height = 25
          Caption = #1054#1095#1080#1089#1090#1080#1090#1100
          TabOrder = 0
          OnClick = ButtonFirstClearParamsClick
        end
        object GroupBox2: TGroupBox
          Left = 16
          Top = 24
          Width = 449
          Height = 106
          Caption = #1057#1077#1088#1080#1081#1085#1099#1077' '#1085#1086#1084#1077#1088#1072
          TabOrder = 1
          object Label12: TLabel
            Left = 8
            Top = 24
            Width = 94
            Height = 13
            Caption = #1053#1072#1095#1072#1083#1100#1085#1099#1081' '#1085#1086#1084#1077#1088':'
          end
          object Label13: TLabel
            Left = 8
            Top = 50
            Width = 79
            Height = 13
            Caption = #1063#1080#1089#1083#1086' '#1085#1086#1084#1077#1088#1086#1074':'
          end
          object Label24: TLabel
            Left = 8
            Top = 75
            Width = 114
            Height = 13
            Caption = #1053#1086#1074#1072#1103' CRC32 ('#1073#1077#1079' 0'#1093'):'
          end
          object EditFirstSerialBegin: TEdit
            Left = 144
            Top = 20
            Width = 161
            Height = 21
            TabOrder = 0
            Text = '2300511'
          end
          object EditFirstSerialCount: TEdit
            Left = 144
            Top = 47
            Width = 161
            Height = 21
            TabOrder = 1
            Text = '1'
          end
          object EditNewCRC32: TEdit
            Left = 144
            Top = 74
            Width = 161
            Height = 21
            TabOrder = 2
            Text = '880F7E60'
          end
          object CheckBoxUpdateCRC32: TCheckBox
            Left = 327
            Top = 75
            Width = 122
            Height = 17
            Caption = #1054#1073#1085#1086#1074#1080#1090#1100' CRC32'
            Checked = True
            State = cbChecked
            TabOrder = 3
          end
        end
      end
      object ButtonFirstHelp: TButton
        Left = 19
        Top = 470
        Width = 118
        Height = 25
        Caption = #1048#1085#1092#1086
        TabOrder = 3
        OnClick = ButtonFirstHelpClick
      end
      object ButtonFirstMakeFile: TButton
        Left = 688
        Top = 470
        Width = 145
        Height = 25
        Caption = #1057#1086#1079#1076#1072#1090#1100' '#1092#1072#1081#1083#1099
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = []
        ParentFont = False
        ParentShowHint = False
        ShowHint = False
        TabOrder = 4
        OnClick = ButtonFirstMakeFileClick
      end
      object ButtonAutoMakeFiles: TButton
        Left = 522
        Top = 470
        Width = 160
        Height = 25
        Caption = #1057#1086#1079#1076#1072#1090#1100' '#1092#1072#1081#1083#1099' ('#1040#1074#1090#1086')'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = []
        ParentFont = False
        TabOrder = 5
        OnClick = ButtonFirstMakeFileClick
      end
      object ComboBox1: TComboBox
        Left = 362
        Top = 18
        Width = 145
        Height = 21
        TabOrder = 6
        OnChange = ComboBox1Change
      end
      object Button1: TButton
        Left = 691
        Top = 16
        Width = 129
        Height = 25
        Caption = #1054#1095#1080#1089#1090#1082#1072
        TabOrder = 7
        OnClick = Button1Click
      end
    end
    object TabSheetUpdateFile: TTabSheet
      Caption = #1054#1073#1085#1086#1074#1083#1077#1085#1080#1077' '#1087#1088#1086#1096#1080#1074#1082#1080
      ImageIndex = 1
      ExplicitLeft = 0
      ExplicitTop = 28
      object Label14: TLabel
        Left = 297
        Top = 16
        Width = 41
        Height = 13
        Caption = #1056#1077#1074#1080#1079#1080#1103
      end
      object GroupBox3: TGroupBox
        Left = 16
        Top = 40
        Width = 817
        Height = 105
        Caption = #1060#1072#1081#1083' '#1087#1088#1086#1075#1088#1072#1084#1084#1099'/'#1076#1072#1085#1085#1099#1093
        TabOrder = 0
        object Label1: TLabel
          Left = 16
          Top = 64
          Width = 30
          Height = 13
          Caption = #1060#1072#1081#1083':'
        end
        object Label2: TLabel
          Left = 16
          Top = 83
          Width = 39
          Height = 13
          Caption = #1056#1072#1079#1084#1077#1088':'
        end
        object LabelUpdateFileName: TLabel
          Left = 61
          Top = 64
          Width = 4
          Height = 13
          Caption = '-'
        end
        object LabelUpdateFileSize: TLabel
          Left = 61
          Top = 83
          Width = 4
          Height = 13
          Caption = '-'
        end
        object ButtonUpdateSelectFile: TButton
          Left = 16
          Top = 24
          Width = 105
          Height = 25
          Caption = #1042#1099#1073#1088#1072#1090#1100' '#1092#1072#1081#1083
          TabOrder = 0
          OnClick = ButtonUpdateSelectFileClick
        end
        object ButtonUpdateClearData: TButton
          Left = 672
          Top = 24
          Width = 129
          Height = 25
          Caption = #1054#1095#1080#1089#1090#1080#1090#1100
          TabOrder = 1
          OnClick = ButtonUpdateClearDataClick
        end
      end
      object GroupBox4: TGroupBox
        Left = 16
        Top = 151
        Width = 817
        Height = 321
        Caption = #1055#1072#1088#1072#1084#1077#1090#1088#1099
        TabOrder = 1
        object Label3: TLabel
          Left = 15
          Top = 24
          Width = 104
          Height = 13
          Caption = #1052#1086#1076#1077#1083#1100' '#1091#1089#1090#1088#1086#1081#1089#1090#1074#1072':'
        end
        object Label4: TLabel
          Left = 15
          Top = 55
          Width = 120
          Height = 13
          Caption = #1054#1087#1080#1089#1072#1090#1077#1083#1100' '#1091#1089#1090#1088#1086#1081#1089#1090#1074#1072':'
        end
        object Label5: TLabel
          Left = 15
          Top = 78
          Width = 116
          Height = 13
          Caption = #1057#1077#1088#1080#1081#1085#1099#1081' '#1085#1086#1084#1077#1088' (S/N):'
        end
        object Label6: TLabel
          Left = 16
          Top = 111
          Width = 195
          Height = 13
          Caption = #1040#1087#1087#1072#1088#1072#1090#1085#1099#1081' '#1080#1076#1077#1085#1090#1080#1092#1080#1082#1072#1090#1086#1088'  (DevID):'
        end
        object Label7: TLabel
          Left = 16
          Top = 139
          Width = 166
          Height = 13
          Caption = #1053#1072#1095#1072#1083#1100#1085#1099#1081' '#1072#1076#1088#1077#1089' '#1076#1083#1103' '#1089#1090#1080#1088#1072#1085#1080#1103':'
        end
        object Label8: TLabel
          Left = 16
          Top = 168
          Width = 160
          Height = 13
          Caption = #1050#1086#1085#1077#1095#1085#1099#1081' '#1072#1076#1088#1077#1089' '#1076#1083#1103' '#1089#1090#1080#1088#1072#1085#1080#1103':'
        end
        object Label9: TLabel
          Left = 440
          Top = 139
          Width = 152
          Height = 13
          Caption = #1053#1072#1095#1072#1083#1100#1085#1099#1081' '#1072#1076#1088#1077#1089' '#1087#1088#1086#1075#1088#1072#1084#1084#1099':'
        end
        object ButtonUpdateClearParams: TButton
          Left = 672
          Top = 24
          Width = 129
          Height = 25
          Caption = #1054#1095#1080#1089#1090#1080#1090#1100
          TabOrder = 0
          OnClick = ButtonUpdateClearParamsClick
        end
        object EditDevModel: TComboBox
          Left = 217
          Top = 21
          Width = 200
          Height = 21
          TabOrder = 1
          OnChange = EditDevModelChange
        end
        object EditDevDesc: TEdit
          Left = 217
          Top = 48
          Width = 200
          Height = 21
          TabOrder = 2
        end
        object EditDevSerial: TEdit
          Left = 217
          Top = 75
          Width = 200
          Height = 21
          TabOrder = 3
        end
        object EditDevId: TEdit
          Left = 217
          Top = 108
          Width = 200
          Height = 21
          TabOrder = 4
        end
        object EditEraseBeginAddr: TEdit
          Left = 217
          Top = 139
          Width = 200
          Height = 21
          TabOrder = 5
          Text = '0'
        end
        object EditEraseEndAddr: TEdit
          Left = 217
          Top = 166
          Width = 200
          Height = 21
          TabOrder = 6
          Text = '0'
        end
        object EditProgBeginAddr: TEdit
          Left = 598
          Top = 136
          Width = 200
          Height = 21
          TabOrder = 7
        end
        object GroupBox6: TGroupBox
          Left = 15
          Top = 200
          Width = 250
          Height = 112
          Caption = #1050#1086#1084#1072#1085#1076#1099
          TabOrder = 8
          object CheckBoxIsBaseProg: TCheckBox
            Left = 15
            Top = 16
            Width = 152
            Height = 17
            Caption = #1054#1089#1085#1086#1074#1085#1072#1103' '#1087#1088#1086#1075#1088#1072#1084#1084#1072
            Checked = True
            Ctl3D = True
            ParentCtl3D = False
            State = cbChecked
            TabOrder = 0
          end
          object CheckBoxOnlySelSerial: TCheckBox
            Left = 15
            Top = 39
            Width = 158
            Height = 17
            Caption = #1058#1086#1083#1100#1082#1086' '#1076#1083#1103' '#1091#1082#1072#1079#1072#1085#1085#1086#1075#1086' S/N'
            TabOrder = 1
          end
          object CheckBoxOnlySelDevId: TCheckBox
            Left = 15
            Top = 62
            Width = 170
            Height = 17
            Caption = #1058#1086#1083#1100#1082#1086' '#1076#1083#1103' '#1091#1082#1072#1079#1072#1085#1085#1086#1075#1086' DevID'
            TabOrder = 2
          end
          object CheckBoxNoModelCheck: TCheckBox
            Left = 15
            Top = 85
            Width = 170
            Height = 17
            Caption = #1041#1077#1079' '#1087#1088#1086#1074#1077#1088#1082#1080' '#1084#1086#1076#1077#1083#1080
            TabOrder = 3
          end
        end
        object GroupBox8: TGroupBox
          Left = 288
          Top = 200
          Width = 440
          Height = 112
          Caption = #1044#1086#1087#1086#1083#1085#1080#1090#1077#1083#1100#1085#1086
          TabOrder = 9
          object CheckBoxSaveBthName: TCheckBox
            Left = 15
            Top = 16
            Width = 170
            Height = 17
            Caption = #1057#1086#1093#1088#1072#1085#1103#1090#1100' "BTH '#1080#1084#1103' '#1087#1088#1080#1073#1086#1088#1072'"'
            Ctl3D = True
            ParentCtl3D = False
            TabOrder = 0
          end
          object CheckBoxSaveBthAddr: TCheckBox
            Left = 15
            Top = 39
            Width = 178
            Height = 17
            Caption = #1057#1086#1093#1088#1072#1085#1103#1090#1100' "BTH '#1072#1076#1088#1077#1089' '#1087#1088#1080#1073#1086#1088#1072'"'
            TabOrder = 1
          end
          object CheckBoxSaveDesc: TCheckBox
            Left = 15
            Top = 62
            Width = 178
            Height = 17
            Caption = #1057#1086#1093#1088#1072#1085#1103#1090#1100' "'#1086#1087#1080#1089#1072#1085#1080#1077' '#1087#1088#1080#1073#1086#1088#1072'"'
            TabOrder = 2
          end
          object CheckBoxSaveHwVersion: TCheckBox
            Left = 15
            Top = 85
            Width = 194
            Height = 17
            Caption = #1057#1086#1093#1088#1072#1085#1103#1090#1100' "'#1074#1077#1088#1089#1080#1103' '#1072#1087#1087'. '#1087#1088#1080#1073#1086#1088#1072'"'
            TabOrder = 3
          end
          object CheckBoxSaveSwVersion: TCheckBox
            Left = 215
            Top = 16
            Width = 210
            Height = 17
            Caption = #1057#1086#1093#1088#1072#1085#1103#1090#1100' "'#1074#1077#1088#1089#1080#1103' '#1087#1088#1086#1096'. '#1087#1088#1080#1073#1086#1088#1072'"'
            TabOrder = 4
          end
          object CheckBoxSaveSerial: TCheckBox
            Left = 215
            Top = 39
            Width = 210
            Height = 17
            Caption = #1057#1086#1093#1088#1072#1085#1103#1090#1100' "'#1089#1077#1088'. '#1085#1086#1084#1077#1088' '#1087#1088#1080#1073#1086#1088#1072'"'
            Checked = True
            State = cbChecked
            TabOrder = 5
          end
          object CheckBoxSaveModel: TCheckBox
            Left = 215
            Top = 62
            Width = 210
            Height = 17
            Caption = #1057#1086#1093#1088#1072#1085#1103#1090#1100' "'#1084#1086#1076#1077#1083#1100' '#1087#1088#1080#1073#1086#1088#1072'"'
            TabOrder = 6
          end
          object CheckBoxEnableLvl1: TCheckBox
            Left = 215
            Top = 85
            Width = 210
            Height = 17
            Caption = #1042#1082#1083#1102#1095#1080#1090#1100' '#1079#1072#1097#1080#1090#1091' '#1087#1072#1084#1103#1090#1080' "Level1"'
            TabOrder = 7
          end
        end
      end
      object ButtonUpdateHelp: TButton
        Left = 16
        Top = 478
        Width = 118
        Height = 25
        Caption = #1048#1085#1092#1086
        TabOrder = 2
        OnClick = ButtonUpdateHelpClick
      end
      object ButtonUpdateMakeFile: TButton
        Left = 688
        Top = 478
        Width = 145
        Height = 25
        Caption = #1057#1086#1079#1076#1072#1090#1100' '#1092#1072#1081#1083
        TabOrder = 3
        OnClick = ButtonUpdateMakeFileClick
      end
      object ButtonUpdateAutoMakeFile: TButton
        Left = 522
        Top = 478
        Width = 160
        Height = 25
        Caption = #1057#1086#1079#1076#1072#1090#1100' '#1092#1072#1081#1083' ('#1040#1074#1090#1086')'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = []
        ParentFont = False
        TabOrder = 4
        OnClick = ButtonUpdateMakeFileClick
      end
      object ComboBox2: TComboBox
        Left = 344
        Top = 13
        Width = 145
        Height = 21
        TabOrder = 5
        OnChange = ComboBox2Change
      end
      object Button2: TButton
        Left = 688
        Top = 9
        Width = 129
        Height = 25
        Caption = #1054#1095#1080#1089#1090#1082#1072
        TabOrder = 6
        OnClick = Button2Click
      end
    end
    object TabSheetUpdateFileCabel: TTabSheet
      Caption = #1053#1072#1095#1072#1083#1100#1085#1072#1103' '#1087#1088#1086#1096#1080#1074#1082#1072' '#1082#1072#1073#1077#1083#1103
      ImageIndex = 2
      object GroupBox9: TGroupBox
        Left = 16
        Top = 16
        Width = 816
        Height = 142
        Caption = #1060#1072#1081#1083' '#1087#1088#1086#1075#1088#1072#1084#1084#1099'/'#1076#1072#1085#1085#1099#1093
        TabOrder = 0
        object Label18: TLabel
          Left = 16
          Top = 64
          Width = 30
          Height = 13
          Caption = #1060#1072#1081#1083':'
        end
        object Label19: TLabel
          Left = 16
          Top = 110
          Width = 39
          Height = 13
          Caption = #1056#1072#1079#1084#1077#1088':'
        end
        object LabelCableFileName: TLabel
          Left = 61
          Top = 64
          Width = 4
          Height = 13
          Caption = '-'
        end
        object LabelCableFileSize: TLabel
          Left = 61
          Top = 110
          Width = 4
          Height = 13
          Caption = '-'
        end
        object Label25: TLabel
          Left = 16
          Top = 87
          Width = 57
          Height = 13
          Caption = #1058#1080#1087' '#1092#1072#1081#1083#1072':'
        end
        object LabelCableFileType: TLabel
          Left = 84
          Top = 87
          Width = 4
          Height = 13
          Caption = '-'
        end
        object ButtonUpdateSelectFileCabel: TButton
          Left = 16
          Top = 24
          Width = 105
          Height = 25
          Caption = #1042#1099#1073#1088#1072#1090#1100' '#1092#1072#1081#1083
          TabOrder = 0
          OnClick = ButtonUpdateSelectFileCabelClick
        end
        object ButtonUpdateClearDataCable: TButton
          Left = 672
          Top = 24
          Width = 129
          Height = 25
          Caption = #1054#1095#1080#1089#1090#1080#1090#1100
          TabOrder = 1
          OnClick = ButtonUpdateClearDataCableClick
        end
      end
      object GroupBox10: TGroupBox
        Left = 16
        Top = 164
        Width = 816
        Height = 149
        Caption = #1055#1072#1088#1072#1084#1077#1090#1088#1099
        TabOrder = 1
        object Label22: TLabel
          Left = 424
          Top = 52
          Width = 79
          Height = 13
          Caption = #1063#1080#1089#1083#1086' '#1082#1072#1085#1072#1083#1086#1074':'
        end
        object Label26: TLabel
          Left = 424
          Top = 110
          Width = 139
          Height = 13
          Caption = #1050#1086#1088#1088#1077#1082#1094#1080#1103' '#1089#1086#1087#1088#1086#1090#1080#1074#1083#1077#1085#1080#1103':'
          Visible = False
        end
        object GroupBox11: TGroupBox
          Left = 16
          Top = 28
          Width = 300
          Height = 78
          Caption = #1057#1077#1088#1080#1081#1085#1099#1077' '#1085#1086#1084#1077#1088#1072
          TabOrder = 0
          object Label20: TLabel
            Left = 12
            Top = 24
            Width = 90
            Height = 13
            Caption = #1053#1072#1095#1072#1083#1100#1085#1099#1081' '#1085#1086#1084#1077#1088
          end
          object Label21: TLabel
            Left = 12
            Top = 48
            Width = 75
            Height = 13
            Caption = #1063#1080#1089#1083#1086' '#1085#1086#1084#1077#1088#1086#1074
          end
          object EditCableSerialBegin: TEdit
            Left = 132
            Top = 20
            Width = 150
            Height = 21
            MaxLength = 8
            TabOrder = 0
            Text = '1'
          end
          object EditCableSerialCount: TEdit
            Left = 132
            Top = 47
            Width = 150
            Height = 21
            TabOrder = 1
            Text = '0'
          end
        end
        object ButtonCableClearParams: TButton
          Left = 672
          Top = 24
          Width = 129
          Height = 25
          Caption = #1054#1095#1080#1089#1090#1080#1090#1100
          TabOrder = 1
          OnClick = ButtonCableClearParamsClick
        end
        object CSpinEditChanCable: TSpinEdit
          Left = 509
          Top = 48
          Width = 121
          Height = 22
          MaxValue = 12
          MinValue = 0
          TabOrder = 2
          Value = 0
        end
        object CheckBoxProtectDefibrillator: TCheckBox
          Left = 424
          Top = 76
          Width = 169
          Height = 17
          Caption = #1047#1072#1097#1080#1090#1072' '#1086#1090' '#1076#1077#1092#1080#1073#1088#1080#1083#1083#1103#1090#1086#1088#1072
          TabOrder = 3
          OnClick = CheckBoxProtectDefibrillatorClick
        end
        object EditCableResistCorrection: TSpinEdit
          Left = 575
          Top = 107
          Width = 121
          Height = 22
          MaxValue = 65000
          MinValue = 0
          TabOrder = 4
          Value = 0
          Visible = False
        end
      end
      object ButtonCableUpdateMakeFile: TButton
        Left = 673
        Top = 454
        Width = 160
        Height = 25
        Caption = #1057#1086#1079#1076#1072#1090#1100' '#1092#1072#1081#1083
        TabOrder = 2
        OnClick = ButtonCableUpdateMakeFileClick
      end
    end
  end
  object SaveDialog1: TSaveDialog
    Left = 332
    Top = 40
  end
  object OpenDialog1: TOpenDialog
    Filter = #1041#1080#1085#1072#1088#1085#1099#1081'(.bin) |*.bin'
    Left = 380
    Top = 32
  end
  object OpenDialogCable: TOpenDialog
    Filter = #1041#1080#1085#1072#1088#1085#1099#1081'(.bin)|*.bin|Intel-HEX|*.hex'
    Left = 452
    Top = 40
  end
  object OpenDialogLoader: TOpenDialog
    Filter = #1041#1080#1085#1072#1088#1085#1099#1081'(.bin)|*.bin'
    Left = 540
    Top = 64
  end
end
