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
#define SREP_VERSION L"0.1"

EFI_BOOT_SERVICES *_gBS = NULL;
EFI_RUNTIME_SERVICES *_gRS = NULL;

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
        Print(L"OPCODE : ");
        switch (next->ID)
        {
        case NO_OP:
            Print(L"NOP\n\r");
            break;
        case LOADED:
            Print(L"LOADED\n\r");
            break;
        case LOAD_FS:
            Print(L"LOAD_FS\n\r");
            Print(L"\t FileName %a\n\r", next->Name);
            break;
        case LOAD_FV:
            Print(L"LOAD_FV\n\r");
            Print(L"\t FileName %a\n\r", next->Name);
            break;
        case PATCH:
            Print(L"PATCH\n\r");
            break;
        case EXEC:
            Print(L"EXEC\n\r");
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
            Print(L"\n\t");
        }
        Print(L"%02x ", DUMP[i]);
    }
    Print(L"\n\t");
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

    Print(L"Welcome to SREP (Smokeless Runtime EFI Patcher) %s\n\r", SREP_VERSION);

    HandleProtocol = SystemTable->BootServices->HandleProtocol;
    HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (void **)&LoadedImage);
    HandleProtocol(LoadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (void **)&FileSystem);
    FileSystem->OpenVolume(FileSystem, &Root);

    UnicodeSPrint(FileName, sizeof(FileName), L"%a", "SREP_Config.cfg");
    Status = Root->Open(Root, &ConfigFile, FileName, EFI_FILE_MODE_READ, 0);
    if (Status != EFI_SUCCESS)
    {
        Print(L"Failed on Opening SREP_Config : %r\n\r", Status);
        return Status;
    }
    Print(L"Opened SREP_Config\n\r");
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
            Print(L"Failed Getting SREP_Config Info: %r\n\r", Status);
            return Status;
        }
    }
    UINTN ConfigDataSize = FileInfo->FileSize + 1; // Add Last null Terminalto
    Print(L"Config Size: 0x%x\n\r", ConfigDataSize);
    CHAR8 *ConfigData = AllocateZeroPool(ConfigDataSize);
    FreePool(FileInfo);

    Status = ConfigFile->Read(ConfigFile, &ConfigDataSize, ConfigData);
    if (Status != EFI_SUCCESS)
    {
        Print(L"Failed on Reading SREP_Config : %r\n\r", Status);
        return Status;
    }
    Print(L"Parsing Config\n\r");
    ConfigFile->Close(ConfigFile);
    Print(L"Stripping NewLine, Carriage and tab Return\n\r");
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
        Print(L"Current Parsing %a\n\r", &ConfigData[curr_pos]);
        if (AsciiStrStr(&ConfigData[curr_pos], "End"))
        {
            Print(L"End OP Detected\n\r");
            continue;
        }
        if (AsciiStrStr(&ConfigData[curr_pos], "Op"))
        {
            Print(L"OP Detected\n\r");
            curr_pos += 3;
            Print(L"Commnand %a \n\r", &ConfigData[curr_pos]);
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
            Print(L"Commnand %a Invalid \n\r", &ConfigData[curr_pos]);
            return EFI_INVALID_PARAMETER;
        }
        if ((Prev_OP->ID == LOAD_FS || Prev_OP->ID == LOAD_FV || Prev_OP->ID == LOADED) && Prev_OP->Name == 0)
        {
            Print(L"Found File %a \n\r", &ConfigData[curr_pos]);
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
                Print(L"Found Offset\n\r");
                Prev_OP->PatterType = OFFSET;
            }
            if (AsciiStrStr(&ConfigData[curr_pos], "Pattern"))
            {
                Print(L"Found Pattern\n\r");
                Prev_OP->PatterType = PATTERN;
            }
            if (AsciiStrStr(&ConfigData[curr_pos], "RelNegOffset"))
            {
                Print(L"Found Offset\n\r");
                Prev_OP->PatterType = REL_NEG_OFFSET;
            }
            if (AsciiStrStr(&ConfigData[curr_pos], "RelPosOffset"))
            {
                Print(L"Found Offset\n\r");
                Prev_OP->PatterType = REL_POS_OFFSET;
            }
            continue;
        }

        //  this new itereration whe are just in from of the Pattern
        if (Prev_OP->ID == PATCH && Prev_OP->PatterType != 0 && Prev_OP->ARG3 == 0)
        {
            if (Prev_OP->PatterType == OFFSET || Prev_OP->PatterType == REL_NEG_OFFSET || Prev_OP->PatterType == REL_POS_OFFSET)
            {
                Print(L"Decode Offset\n\r");
                Prev_OP->ARG3 = AsciiStrHexToUint64(&ConfigData[curr_pos]);
            }
            if (Prev_OP->PatterType == PATTERN)
            {
                Prev_OP->ARG3 = 0xFFFFFFFF;
                Prev_OP->ARG6 = AsciiStrLen(&ConfigData[curr_pos]) / 2;
                Print(L"Found %d Bytes\n\r", Prev_OP->ARG6);
                Prev_OP->ARG7 = (UINT64)AllocateZeroPool(Prev_OP->ARG6);
                AsciiStrHexToBytes(&ConfigData[curr_pos], Prev_OP->ARG6 * 2, (UINT8 *)Prev_OP->ARG7, Prev_OP->ARG6);
            }
            continue;
        }

        if (Prev_OP->ID == PATCH && Prev_OP->PatterType != 0 && Prev_OP->ARG3 != 0)
        {

            Prev_OP->ARG4 = AsciiStrLen(&ConfigData[curr_pos]) / 2;
            Print(L"Found %d Bytes\n\r", Prev_OP->ARG4);
            Prev_OP->ARG5_Dyn_Alloc = TRUE;
            Prev_OP->ARG5 = (UINT64)AllocateZeroPool(Prev_OP->ARG4);
            AsciiStrHexToBytes(&ConfigData[curr_pos], Prev_OP->ARG4 * 2, (UINT8 *)Prev_OP->ARG5, Prev_OP->ARG4);
            Print(L"Patch Byte\n\r");
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
            // Print(L"NOP\n\r");
            break;
        case LOADED:
            Print(L"LOADED\n\r");
            Status = FindLoadedImageFromName(ImageHandle, next->Name, &ImageInfo);
            Print(L"Loaded Image %r -> %x\n\r", Status, ImageInfo->ImageBase);
            break;
        case LOAD_FS:
            Status = LoadFS(ImageHandle, next->Name, &ImageInfo, &AppImageHandle);
            Print(L"Loaded Image %r -> %x\n\r", Status, ImageInfo->ImageBase);
            // Print(L"\t FileName %a\n\r", next->ARG2);
            break;
        case LOAD_FV:
            Status = LoadFV(ImageHandle, next->Name, &ImageInfo, &AppImageHandle, EFI_SECTION_PE32);
            Print(L"Loaded Image %r -> %x\n\r", Status, ImageInfo->ImageBase);
            break;
        case PATCH:
            Print(L"Patching Image Size %x: \n\r", ImageInfo->ImageSize);
            PrintDump(next->ARG6, (UINT8 *)next->ARG7);

            PrintDump(next->ARG6, ((UINT8 *)ImageInfo->ImageBase) + 0x1A383);
            // PrintDump(0x200, (UINT8 *)(LoadedImage->ImageBase));
            if (next->PatterType == PATTERN)
            {

                Print(L"Finding Offset\n\r");
                for (UINTN i = 0; i < ImageInfo->ImageSize - next->ARG6; i += 1)
                {
                    if (CompareMem(((UINT8 *)ImageInfo->ImageBase) + i, (UINT8 *)next->ARG7, next->ARG6) == 0)
                    {
                        next->ARG3 = i;
                        Print(L"Found at %x\n\r", i);
                        break;
                    }
                }
                if (next->ARG3 == 0xFFFFFFFF)
                {
                    Print(L"No Patter Found\n\r");
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
            Print(L"Offset %x\n\r", next->ARG3);
            // PrintDump(next->ARG4+10,ImageInfo->ImageBase + next->ARG3 -5 );
            CopyMem(ImageInfo->ImageBase + next->ARG3, (UINT8 *)next->ARG5, next->ARG4);
            Print(L"Patched\n\r");
            // PrintDump(next->ARG4+10,ImageInfo->ImageBase + next->ARG3 -5 );
            break;
        case EXEC:
            Exec(&AppImageHandle);
            Print(L"EXEC %r\n\r", Status);
            break;

        default:
            break;
        }
    }
cleanup:
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
