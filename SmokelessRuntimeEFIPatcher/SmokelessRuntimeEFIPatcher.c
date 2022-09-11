#include <Uefi.h>
#include <Guid/FileInfo.h>
#include <Guid/FileSystemInfo.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/BlockIo.h>
#include <Library/PrintLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/FormBrowser2.h>
#include <Protocol/FormBrowserEx.h>
#include <Protocol/FormBrowserEx2.h>
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Protocol/DisplayProtocol.h>
#include <Protocol/HiiPopup.h>
#include <Library/MemoryAllocationLib.h>
#include "Utility.h"
#include "Opcode.h"
#define SREP_VERSION L"0.1.4"

EFI_BOOT_SERVICES *_gBS = NULL;
EFI_RUNTIME_SERVICES *_gRS = NULL;
EFI_FILE *LogFile;
char Log[512];
enum
{
    OFFSET = 1,
    PATTERN,
    REL_NEG_OFFSET,
    REL_POS_OFFSET
};

enum OPCODE
{
    NO_OP,
    LOADED,
    LOAD_FS,
    LOAD_FV,
    PATCH,
    EXEC
};

struct OP_DATA
{
    enum OPCODE ID;
    CHAR8 *Name;
    BOOLEAN Name_Dyn_Alloc;
    UINT64 PatterType;
    BOOLEAN PatterType_Dyn_Alloc;
    INT64 ARG3;
    BOOLEAN ARG3_Dyn_Alloc;
    UINT64 ARG4;
    BOOLEAN ARG4_Dyn_Alloc;
    UINT64 ARG5;
    BOOLEAN ARG5_Dyn_Alloc;
    UINT64 ARG6;
    BOOLEAN ARG6_Dyn_Alloc;
    UINT64 ARG7;
    BOOLEAN ARG7_Dyn_Alloc;
    struct OP_DATA *next;
    struct OP_DATA *prev;
};


void LogToFile( EFI_FILE *LogFile, char *String)
{
        UINTN Size = AsciiStrLen(String);
        LogFile->Write(LogFile,&Size,String);
}


VOID Add_OP_CODE(struct OP_DATA *Start, struct OP_DATA *opCode)
{
    struct OP_DATA *next = Start;
    while (next->next != NULL)
    {
        next = next->next;
    }
    next->next = opCode;
    opCode->prev = next;
}

VOID PrintOPChain(struct OP_DATA *Start)
{
    struct OP_DATA *next = Start;
    while (next != NULL)
    {
        AsciiSPrint(Log,512,"%s","OPCODE : ");
        LogToFile(LogFile,Log);
        switch (next->ID)
        {
        case NO_OP:
            AsciiSPrint(Log,512,"%s","NOP\n\r");
            LogToFile(LogFile,Log);
            break;
        case LOADED:
            AsciiSPrint(Log,512,"%s","LOADED\n\r");
            LogToFile(LogFile,Log);
            break;
        case LOAD_FS:
            AsciiSPrint(Log,512,"%s","LOAD_FS\n\r");
            LogToFile(LogFile,Log);
            AsciiSPrint(Log,512,"%s","\t FileName %a\n\r", next->Name);
            LogToFile(LogFile,Log);
            break;
        case LOAD_FV:
            AsciiSPrint(Log,512,"%s","LOAD_FV\n\r");
            LogToFile(LogFile,Log);
            AsciiSPrint(Log,512,"%s","\t FileName %a\n\r", next->Name);
            LogToFile(LogFile,Log);
            break;
        case PATCH:
            AsciiSPrint(Log,512,"%s","PATCH\n\r");
            LogToFile(LogFile,Log);
            break;
        case EXEC:
            AsciiSPrint(Log,512,"%s","EXEC\n\r");
            LogToFile(LogFile,Log);
            break;

        default:
            break;
        }
        next = next->next;
    }
}

VOID PrintDump(UINT16 Size, UINT8 *DUMP)
{
    for (UINT16 i = 0; i < Size; i++)
    {
        if (i % 0x10 == 0)
        {
            AsciiSPrint(Log,512,"%s","\n\t");
            LogToFile(LogFile,Log);
        }
        AsciiSPrint(Log,512,"%02x ", DUMP[i]);
        LogToFile(LogFile,Log);
    }
    AsciiSPrint(Log,512,"%s","\n\t");
    LogToFile(LogFile,Log);
}



EFI_STATUS EFIAPI SREPEntry(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS Status;
    EFI_HANDLE_PROTOCOL HandleProtocol;
    EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
    EFI_FILE *Root;
    EFI_FILE *ConfigFile;
    CHAR16 FileName[255];
    CHAR16 LogFileName[] = L"SREP.log";
    Print(L"Welcome to SREP (Smokeless Runtime EFI Patcher) %s\n\r", SREP_VERSION);    
    gBS->SetWatchdogTimer(0, 0, 0, 0);
    HandleProtocol = SystemTable->BootServices->HandleProtocol;
    HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (void **)&LoadedImage);
    HandleProtocol(LoadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (void **)&FileSystem);
    FileSystem->OpenVolume(FileSystem, &Root);

    Status = Root->Open(Root, &LogFile, LogFileName, EFI_FILE_MODE_WRITE, 0);
    if (Status != EFI_SUCCESS)
    {
        Print(L"Failed on Opening Log File : %r\n\r", Status);
        return Status;
    }
    AsciiSPrint(Log,512,"Welcome to SREP (Smokeless Runtime EFI Patcher) %s\n\r", SREP_VERSION);
    LogToFile(LogFile,Log);
    UnicodeSPrint(FileName, sizeof(FileName), L"%a", "SREP_Config.cfg");
    Status = Root->Open(Root, &ConfigFile, FileName, EFI_FILE_MODE_READ, 0);
    if (Status != EFI_SUCCESS)
    {
        AsciiSPrint(Log,512,"Failed on Opening SREP_Config : %r\n\r", Status);
        LogToFile(LogFile,Log);
        return Status;
    }

   AsciiSPrint(Log,512,"%s","Opened SREP_Config\n\r");
   LogToFile(LogFile,Log);
    EFI_GUID gFileInfo = EFI_FILE_INFO_ID;
    EFI_FILE_INFO *FileInfo = NULL;
    UINTN FileInfoSize = 0;
    Status = ConfigFile->GetInfo(ConfigFile, &gFileInfo, &FileInfoSize, &FileInfo);
    if (Status == EFI_BUFFER_TOO_SMALL)
    {
        FileInfo = AllocatePool(FileInfoSize);
        Status = ConfigFile->GetInfo(ConfigFile, &gFileInfo, &FileInfoSize, FileInfo);
        if (Status != EFI_SUCCESS)
        {
            AsciiSPrint(Log,512,"Failed Getting SREP_Config Info: %r\n\r", Status);
            LogToFile(LogFile,Log);
            return Status;
        }
    }
    UINTN ConfigDataSize = FileInfo->FileSize + 1; // Add Last null Terminalto
    AsciiSPrint(Log,512,"Config Size: 0x%x\n\r", ConfigDataSize);
    LogToFile(LogFile,Log);
    CHAR8 *ConfigData = AllocateZeroPool(ConfigDataSize);
    FreePool(FileInfo);

    Status = ConfigFile->Read(ConfigFile, &ConfigDataSize, ConfigData);
    if (Status != EFI_SUCCESS)
    {
        AsciiSPrint(Log,512,"Failed on Reading SREP_Config : %r\n\r", Status);
         LogToFile(LogFile,Log);
        return Status;
    }
    AsciiSPrint(Log,512,"%s","Parsing Config\n\r");
     LogToFile(LogFile,Log);
    ConfigFile->Close(ConfigFile);
    AsciiSPrint(Log,512,"%s","Stripping NewLine, Carriage and tab Return\n\r");
     LogToFile(LogFile,Log);
    for (UINTN i = 0; i < ConfigDataSize; i++)
    {
        if (ConfigData[i] == '\n' || ConfigData[i] == '\r' || ConfigData[i] == '\t')
        {
            ConfigData[i] = '\0';
        }
    }
    UINTN curr_pos = 0;

    struct OP_DATA *Start = AllocateZeroPool(sizeof(struct OP_DATA));
    struct OP_DATA *Prev_OP;
    Start->ID = NO_OP;
    Start->next = NULL;
    BOOLEAN NullByteSkipped = FALSE;
    while (curr_pos < ConfigDataSize)
    {

        if (curr_pos != 0 && !NullByteSkipped)
            curr_pos += AsciiStrLen(&ConfigData[curr_pos]);
        if (ConfigData[curr_pos] == '\0')
        {
            curr_pos += 1;
            NullByteSkipped = TRUE;
            continue;
        }
        NullByteSkipped = FALSE;
        AsciiSPrint(Log,512,"Current Parsing %a\n\r", &ConfigData[curr_pos]);
         LogToFile(LogFile,Log);
        if (AsciiStrStr(&ConfigData[curr_pos], "End"))
        {
            AsciiSPrint(Log,512,"%s","End OP Detected\n\r");
             LogToFile(LogFile,Log);
            continue;
        }
        if (AsciiStrStr(&ConfigData[curr_pos], "Op"))
        {
            AsciiSPrint(Log,512,"%s","OP Detected\n\r");
             LogToFile(LogFile,Log);
            curr_pos += 3;
            AsciiSPrint(Log,512,"Commnand %a \n\r", &ConfigData[curr_pos]);
             LogToFile(LogFile,Log);
            if (AsciiStrStr(&ConfigData[curr_pos], "LoadFromFS"))
            {
                Prev_OP = AllocateZeroPool(sizeof(struct OP_DATA));
                Prev_OP->ID = LOAD_FS;
                Add_OP_CODE(Start, Prev_OP);
                continue;
            }
            if (AsciiStrStr(&ConfigData[curr_pos], "LoadFromFV"))
            {
                Prev_OP = AllocateZeroPool(sizeof(struct OP_DATA));
                Prev_OP->ID = LOAD_FV;
                Add_OP_CODE(Start, Prev_OP);
                continue;
            }
            if (AsciiStrStr(&ConfigData[curr_pos], "Loaded"))
            {
                Prev_OP = AllocateZeroPool(sizeof(struct OP_DATA));
                Prev_OP->ID = LOADED;
                Add_OP_CODE(Start, Prev_OP);
                continue;
            }
            if (AsciiStrStr(&ConfigData[curr_pos], "Patch"))
            {
                Prev_OP = AllocateZeroPool(sizeof(struct OP_DATA));
                Prev_OP->ID = PATCH;
                Add_OP_CODE(Start, Prev_OP);
                continue;
            }

            if (AsciiStrStr(&ConfigData[curr_pos], "Exec"))
            {
                Prev_OP = AllocateZeroPool(sizeof(struct OP_DATA));
                Prev_OP->ID = EXEC;
                Add_OP_CODE(Start, Prev_OP);
                continue;
            }
            AsciiSPrint(Log,512,"Commnand %a Invalid \n\r", &ConfigData[curr_pos]);
             LogToFile(LogFile,Log);
            return EFI_INVALID_PARAMETER;
        }
        if ((Prev_OP->ID == LOAD_FS || Prev_OP->ID == LOAD_FV || Prev_OP->ID == LOADED) && Prev_OP->Name == 0)
        {
            AsciiSPrint(Log,512,"Found File %a \n\r", &ConfigData[curr_pos]);
             LogToFile(LogFile,Log);
            UINTN FileNameLength = AsciiStrLen(&ConfigData[curr_pos]) + 1;
            CHAR8 *FileName = AllocateZeroPool(FileNameLength);
            CopyMem(FileName, &ConfigData[curr_pos], FileNameLength);
            Prev_OP->Name = FileName;
            Prev_OP->Name_Dyn_Alloc = TRUE;
            continue;
        }
        if (Prev_OP->ID == PATCH && Prev_OP->PatterType == 0)
        {
            if (AsciiStrStr(&ConfigData[curr_pos], "Offset"))
            {
                AsciiSPrint(Log,512,"%s","Found Offset\n\r");
                 LogToFile(LogFile,Log);
                Prev_OP->PatterType = OFFSET;
            }
            if (AsciiStrStr(&ConfigData[curr_pos], "Pattern"))
            {
                AsciiSPrint(Log,512,"%s","Found Pattern\n\r");
                 LogToFile(LogFile,Log);
                Prev_OP->PatterType = PATTERN;
            }
            if (AsciiStrStr(&ConfigData[curr_pos], "RelNegOffset"))
            {
                AsciiSPrint(Log,512,"%s","Found Offset\n\r");
                 LogToFile(LogFile,Log);
                Prev_OP->PatterType = REL_NEG_OFFSET;
            }
            if (AsciiStrStr(&ConfigData[curr_pos], "RelPosOffset"))
            {
                AsciiSPrint(Log,512,"%s","Found Offset\n\r");
                 LogToFile(LogFile,Log);
                Prev_OP->PatterType = REL_POS_OFFSET;
            }
            continue;
        }

        //  this new itereration whe are just in from of the Pattern
        if (Prev_OP->ID == PATCH && Prev_OP->PatterType != 0 && Prev_OP->ARG3 == 0)
        {
            if (Prev_OP->PatterType == OFFSET || Prev_OP->PatterType == REL_NEG_OFFSET || Prev_OP->PatterType == REL_POS_OFFSET)
            {
                AsciiSPrint(Log,512,"%s","Decode Offset\n\r");
                 LogToFile(LogFile,Log);
                Prev_OP->ARG3 = AsciiStrHexToUint64(&ConfigData[curr_pos]);
            }
            if (Prev_OP->PatterType == PATTERN)
            {
                Prev_OP->ARG3 = 0xFFFFFFFF;
                Prev_OP->ARG6 = AsciiStrLen(&ConfigData[curr_pos]) / 2;
                AsciiSPrint(Log,512,"Found %d Bytes\n\r", Prev_OP->ARG6);
                 LogToFile(LogFile,Log);
                Prev_OP->ARG7 = (UINT64)AllocateZeroPool(Prev_OP->ARG6);
                AsciiStrHexToBytes(&ConfigData[curr_pos], Prev_OP->ARG6 * 2, (UINT8 *)Prev_OP->ARG7, Prev_OP->ARG6);
            }
            continue;
        }

        if (Prev_OP->ID == PATCH && Prev_OP->PatterType != 0 && Prev_OP->ARG3 != 0)
        {

            Prev_OP->ARG4 = AsciiStrLen(&ConfigData[curr_pos]) / 2;
            AsciiSPrint(Log,512,"Found %d Bytes\n\r", Prev_OP->ARG4);
             LogToFile(LogFile,Log);
            Prev_OP->ARG5_Dyn_Alloc = TRUE;
            Prev_OP->ARG5 = (UINT64)AllocateZeroPool(Prev_OP->ARG4);
            AsciiStrHexToBytes(&ConfigData[curr_pos], Prev_OP->ARG4 * 2, (UINT8 *)Prev_OP->ARG5, Prev_OP->ARG4);
            AsciiSPrint(Log,512,"%s","Patch Byte\n\r");
             LogToFile(LogFile,Log);
            PrintDump(Prev_OP->ARG4,  (UINT8 *)Prev_OP->ARG5);
            continue;
        }
    }
    FreePool(ConfigData);
   // PrintOPChain(Start);
    // dispatch
    struct OP_DATA *next;
    EFI_HANDLE AppImageHandle;
    EFI_LOADED_IMAGE_PROTOCOL *ImageInfo;
    INT64 BaseOffset;
    for (next = Start; next != NULL; next = next->next)
    {
        switch (next->ID)
        {
        case NO_OP:
            // AsciiSPrint(Log,512,"NOP\n\r");
            break;
        case LOADED:
            AsciiSPrint(Log,512,"%s","Executing Loaded OP\n\r");
             LogToFile(LogFile,Log);
            Status = FindLoadedImageFromName(ImageHandle, next->Name, &ImageInfo);
            AsciiSPrint(Log,512,"Loaded Image %r -> %x\n\r", Status, ImageInfo->ImageBase);
             LogToFile(LogFile,Log);
            break;
        case LOAD_FS:
            AsciiSPrint(Log,512,"%s","Executing Load from FS\n\r");    
            LogToFile(LogFile,Log);
            Status = LoadFS(ImageHandle, next->Name, &ImageInfo, &AppImageHandle);
            AsciiSPrint(Log,512,"Loaded Image %r -> %x\n\r", Status, ImageInfo->ImageBase);
             LogToFile(LogFile,Log);
            // AsciiSPrint(Log,512,"\t FileName %a\n\r", next->ARG2);
            break;
        case LOAD_FV:
            AsciiSPrint(Log,512,"%s","Executing Load from FV\n\r");    
            LogToFile(LogFile,Log);
            Status = LoadFV(ImageHandle, next->Name, &ImageInfo, &AppImageHandle, EFI_SECTION_PE32);
            AsciiSPrint(Log,512,"Loaded Image %r -> %x\n\r", Status, ImageInfo->ImageBase);
            LogToFile(LogFile,Log);
            break;
        case PATCH:
            AsciiSPrint(Log,512,"%s","Executing Patch\n\r");    
            LogToFile(LogFile,Log);
            AsciiSPrint(Log,512,"Patching Image Size %x: \n\r", ImageInfo->ImageSize);
            LogToFile(LogFile,Log);
            PrintDump(next->ARG6, (UINT8 *)next->ARG7);

            PrintDump(next->ARG6, ((UINT8 *)ImageInfo->ImageBase) + 0x1A383);
            // PrintDump(0x200, (UINT8 *)(LoadedImage->ImageBase));
            if (next->PatterType == PATTERN)
            {

                AsciiSPrint(Log,512,"%s","Finding Offset\n\r");
                LogToFile(LogFile,Log);
                for (UINTN i = 0; i < ImageInfo->ImageSize - next->ARG6; i += 1)
                {
                    if (CompareMem(((UINT8 *)ImageInfo->ImageBase) + i, (UINT8 *)next->ARG7, next->ARG6) == 0)
                    {
                        next->ARG3 = i;
                        AsciiSPrint(Log,512,"Found at %x\n\r", i);
                        LogToFile(LogFile,Log);
                        break;
                    }
                }
                if (next->ARG3 == 0xFFFFFFFF)
                {
                    AsciiSPrint(Log,512,"%s","No Patter Found\n\r");
                    LogToFile(LogFile,Log);
                    //goto cleanup;
                break;
                }
            }
            if (next->PatterType == REL_POS_OFFSET)
            {
                next->ARG3 = BaseOffset + next->ARG3;
            }
            if (next->PatterType == REL_NEG_OFFSET)
            {
                next->ARG3 = BaseOffset - next->ARG3;
            }
            BaseOffset = next->ARG3;
            AsciiSPrint(Log,512,"Offset %x\n\r", next->ARG3);
            LogToFile(LogFile,Log);
            // PrintDump(next->ARG4+10,ImageInfo->ImageBase + next->ARG3 -5 );
            CopyMem(ImageInfo->ImageBase + next->ARG3, (UINT8 *)next->ARG5, next->ARG4);
            AsciiSPrint(Log,512,"%s","Patched\n\r");
            LogToFile(LogFile,Log);
            // PrintDump(next->ARG4+10,ImageInfo->ImageBase + next->ARG3 -5 );
            break;
        case EXEC:
            Exec(&AppImageHandle);
            AsciiSPrint(Log,512,"%s","EXEC %r\n\r", Status);
            LogToFile(LogFile,Log);
            break;

        default:
            break;
        }
    }
//cleanup:
    for (next = Start; next != NULL; next = next->next)
    {
        if (next->Name_Dyn_Alloc)
            FreePool((VOID *)next->Name);
        if (next->PatterType_Dyn_Alloc)
            FreePool((VOID *)next->PatterType);
        if (next->ARG3_Dyn_Alloc)
            FreePool((VOID *)next->ARG3);
        if (next->ARG4_Dyn_Alloc)
            FreePool((VOID *)next->ARG4);
        if (next->ARG5_Dyn_Alloc)
            FreePool((VOID *)next->ARG5);
        if (next->ARG6_Dyn_Alloc)
            FreePool((VOID *)next->ARG6);
        if (next->ARG7_Dyn_Alloc)
            FreePool((VOID *)next->ARG7);
    }
    next = Start;
    while (next->next != NULL)
    {
        struct OP_DATA *tmp = next;
        next = next->next;
        FreePool(tmp);
    }
    return EFI_SUCCESS;
    // FindBaseAddressFromName(L"H2OFormBrowserDxe");
    // UINT8 *Buffer = NULL;
    // UINTN BufferSize = 0;
    // LocateAndLoadFvFromName(L"SetupUtilityApp", EFI_SECTION_PE32, &Buffer, &BufferSize);
}
