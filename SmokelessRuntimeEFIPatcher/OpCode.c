#include "Opcode.h"
#include "Utility.h"
EFI_STATUS LoadFS(EFI_HANDLE ImageHandle, CHAR8 *FileName, EFI_LOADED_IMAGE_PROTOCOL **ImageInfo, EFI_HANDLE *AppImageHandle)
{
    //UINTN ExitDataSize;
    UINTN NumHandles;
    UINTN Index;
    EFI_HANDLE *SFS_Handles;
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_BLOCK_IO_PROTOCOL *BlkIo;
    EFI_DEVICE_PATH_PROTOCOL *FilePath;
    Status =
        gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid,
                                NULL, &NumHandles, &SFS_Handles);

    if (Status != EFI_SUCCESS)
    {
        Print(L"Could not find handles - %r\n", Status);
        return Status;
    }
    Print(L"No of Handle - %d\n", NumHandles);

    for (Index = 0; Index < NumHandles; Index++)
    {
        Status = gBS->OpenProtocol(
            SFS_Handles[Index], &gEfiSimpleFileSystemProtocolGuid, (VOID **)&BlkIo,
            ImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);

        if (Status != EFI_SUCCESS)
        {
            Print(L"Protocol is not supported - %r\n", Status);
            return Status;
        }
        CHAR16 FileName16[255] = {0};
        UnicodeSPrint(FileName16, sizeof(FileName16), L"%a", FileName);
        FilePath = FileDevicePath(SFS_Handles[Index], FileName16);
        Status = gBS->LoadImage(FALSE, ImageHandle, FilePath, (VOID *)NULL, 0,
                                AppImageHandle);

        if (Status != EFI_SUCCESS)
        {
            Print(L"Could not load the image  on Handle %d - %r\n", Index, Status);
            continue;
        }
        else
        {
            Print(L"Loaded the image with success on Handle %d\n", Index);
            // Print (L"Loaded the image with success\n");
            Status = gBS->OpenProtocol(*AppImageHandle, &gEfiLoadedImageProtocolGuid,
                                       (VOID **)ImageInfo, ImageHandle, (VOID *)NULL,
                                       EFI_OPEN_PROTOCOL_GET_PROTOCOL);
            return Status;
        }
    }
    return Status;
}
EFI_STATUS LoadFV(EFI_HANDLE ImageHandle, CHAR8 *FileName, EFI_LOADED_IMAGE_PROTOCOL **ImageInfo, EFI_HANDLE *AppImageHandle, EFI_SECTION_TYPE Section_Type)
{
    EFI_STATUS Status = EFI_SUCCESS;

    CHAR16 FileName16[255] = {0};
    UnicodeSPrint(FileName16, sizeof(FileName16), L"%a", FileName);
    UINT8 *Buffer = NULL;
    UINTN BufferSize = 0;
    Status = LocateAndLoadFvFromName(FileName16, Section_Type, &Buffer, &BufferSize);

    Status = gBS->LoadImage(FALSE, ImageHandle, (VOID *)NULL, Buffer, BufferSize,
                            AppImageHandle);
    if (Buffer != NULL)
        FreePool(Buffer);
    if (Status != EFI_SUCCESS)
    {
        Print(L"Could not Locate the image  from FV %r \n", Status);
    }
    else
    {
        Print(L"Loaded the image with success from FV\n");
        // Print (L"Loaded the image with success\n");
        Status = gBS->OpenProtocol(*AppImageHandle, &gEfiLoadedImageProtocolGuid,
                                   (VOID **)ImageInfo, ImageHandle, (VOID *)NULL,
                                   EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    }

    return Status;
};
EFI_STATUS Exec(EFI_HANDLE *AppImageHandle)
{
    UINTN ExitDataSize;
    EFI_STATUS Status = gBS->StartImage(*AppImageHandle, &ExitDataSize, (CHAR16 **)NULL);
    Print(L"\tImage Retuned - %r %x\n", Status);
    return Status;
}

EFI_STATUS FindLoadedImageFromName(EFI_HANDLE ImageHandle, CHAR8 *FileName, EFI_LOADED_IMAGE_PROTOCOL **ImageInfo)
{
    EFI_STATUS Status;
    UINTN HandleSize = 0;
    EFI_HANDLE *Handles;
    CHAR16 FileName16[255] = {0};
    UnicodeSPrint(FileName16, sizeof(FileName16), L"%a", FileName);
    Status = gBS->LocateHandle(ByProtocol, &gEfiLoadedImageProtocolGuid, NULL, &HandleSize, NULL);
    if (Status == EFI_BUFFER_TOO_SMALL)
    {
        Handles = AllocateZeroPool(HandleSize * sizeof(EFI_HANDLE));
        Status = gBS->LocateHandle(ByProtocol, &gEfiLoadedImageProtocolGuid, NULL, &HandleSize, Handles);
        Print(L"Retrived %d Handle, with %r\n\r", HandleSize, Status);
    }

    for (UINTN i = 0; i < HandleSize; i++)
    {
        Status = gBS->HandleProtocol(Handles[i], &gEfiLoadedImageProtocolGuid, (VOID **)ImageInfo);
        if (Status == EFI_SUCCESS)
        {
            CHAR16 *String = FindLoadedImageFileName(*ImageInfo);
            if (String != NULL)
            {
                if (StrCmp(FileName16, String) == 0)
                {
                    Print(L"Found %s at Address 0x%X\n\r", String, (*ImageInfo)->ImageBase);
                    return EFI_SUCCESS;
                }
            }
        }
    }
    return EFI_NOT_FOUND;
}