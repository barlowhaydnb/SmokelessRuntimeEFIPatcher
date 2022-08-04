#include "Utility.h"
CHAR16 *
FindLoadedImageFileName(
    IN EFI_LOADED_IMAGE_PROTOCOL *LoadedImage)
{
    EFI_GUID *NameGuid;
    EFI_STATUS Status;
    EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv;
    VOID *Buffer;
    UINTN BufferSize;
    UINT32 AuthenticationStatus;

    if ((LoadedImage == NULL) || (LoadedImage->FilePath == NULL))
    {
        return NULL;
    }

    NameGuid = EfiGetNameGuidFromFwVolDevicePathNode((MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *)LoadedImage->FilePath);

    if (NameGuid == NULL)
    {
        return NULL;
    }

    //
    // Get the FirmwareVolume2Protocol of the device handle that this image was loaded from.
    //
    Status = gBS->HandleProtocol(LoadedImage->DeviceHandle, &gEfiFirmwareVolume2ProtocolGuid, (VOID **)&Fv);

    //
    // FirmwareVolume2Protocol is PI, and is not required to be available.
    //
    if (EFI_ERROR(Status))
    {
        return NULL;
    }

    //
    // Read the user interface section of the image.
    //
    Buffer = NULL;
    Status = Fv->ReadSection(Fv, NameGuid, EFI_SECTION_USER_INTERFACE, 0, &Buffer, &BufferSize, &AuthenticationStatus);

    if (EFI_ERROR(Status))
    {
        return NULL;
    }

    //
    // ReadSection returns just the section data, without any section header. For
    // a user interface section, the only data is the file name.
    //
    return Buffer;
}

UINT8 *FindBaseAddressFromName(const CHAR16 *Name)
{
    EFI_STATUS Status;
    UINTN HandleSize = 0;
    EFI_HANDLE *Handles;

    Status = gBS->LocateHandle(ByProtocol, &gEfiLoadedImageProtocolGuid, NULL, &HandleSize, NULL);
    if (Status == EFI_BUFFER_TOO_SMALL)
    {
        Handles = AllocateZeroPool(HandleSize * sizeof(EFI_HANDLE));
        Status = gBS->LocateHandle(ByProtocol, &gEfiLoadedImageProtocolGuid, NULL, &HandleSize, Handles);
        Print(L"Retrived %d Handle, with %r\n\r", HandleSize, Status);
    }

    EFI_LOADED_IMAGE_PROTOCOL *LoadedImageProtocol;
    for (UINTN i = 0; i < HandleSize; i++)
    {
        Status = gBS->HandleProtocol(Handles[i], &gEfiLoadedImageProtocolGuid, (VOID **)&LoadedImageProtocol);
        if (Status == EFI_SUCCESS)
        {
            CHAR16 *String = FindLoadedImageFileName(LoadedImageProtocol);
            if (String != NULL)
            {
                if (StrCmp(Name, String) == 0)
                {
                    Print(L"Found %s at Address 0x%X\n\r", String, LoadedImageProtocol->ImageBase);
                    return LoadedImageProtocol->ImageBase;
                }
            }
        }
    }
    return NULL;
}

EFI_STATUS LoadandRunImage(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable, CHAR16 *FileName, EFI_HANDLE *AppImageHandle)
{
    UINTN ExitDataSize;
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

        FilePath = FileDevicePath(SFS_Handles[Index], FileName);
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
        }

        Status = gBS->StartImage(*AppImageHandle, &ExitDataSize, (CHAR16 **)NULL);
        Print(L"\tImage Retuned - %r %x\n", Status, Status);
        if (Status != EFI_SUCCESS)
        {
            return EFI_NOT_FOUND;
        }
        return EFI_SUCCESS;
    }
    return EFI_SUCCESS;
}

// TODO add in the dumping SectionInstance
EFI_STATUS
LocateAndLoadFvFromName(CHAR16 *Name, EFI_SECTION_TYPE Section_Type, UINT8 **Buffer, UINTN *BufferSize)
{
    EFI_STATUS Status;
    EFI_HANDLE *HandleBuffer;
    UINTN NumberOfHandles;
    //UINT32 FvStatus;
    //EFI_FV_FILE_ATTRIBUTES Attributes;
   // UINTN Size;
    UINTN Index;
    EFI_FIRMWARE_VOLUME2_PROTOCOL *FvInstance;

   // FvStatus = 0;

    //
    // Locate protocol.
    //
    Status = gBS->LocateHandleBuffer(
        ByProtocol,
        &gEfiFirmwareVolume2ProtocolGuid,
        NULL,
        &NumberOfHandles,
        &HandleBuffer);
    if (EFI_ERROR(Status))
    {
        //
        // Defined errors at this time are not found and out of resources.
        //
        return Status;
    }

    //
    // Looking for FV with ACPI storage file
    //
    Print(L"Found %d Instances\n\r", NumberOfHandles);
    for (Index = 0; Index < NumberOfHandles; Index++)
    {

        //
        // Get the protocol on this handle
        // This should not fail because of LocateHandleBuffer

        Status = gBS->HandleProtocol(
            HandleBuffer[Index],
            &gEfiFirmwareVolume2ProtocolGuid,
            (VOID **)&FvInstance);
        ASSERT_EFI_ERROR(Status);

        EFI_FV_FILETYPE FileType = EFI_FV_FILETYPE_ALL;
        EFI_FV_FILE_ATTRIBUTES FileAttributes;
        UINTN FileSize;
        EFI_GUID NameGuid = {0};
        VOID *Keys = AllocateZeroPool(FvInstance->KeySize);
        while (TRUE)
        {
            FileType = EFI_FV_FILETYPE_ALL;
            ZeroMem(&NameGuid, sizeof(EFI_GUID));
            Status = FvInstance->GetNextFile(FvInstance, Keys, &FileType, &NameGuid, &FileAttributes, &FileSize);
            if (Status != EFI_SUCCESS)
            {
                // Print(L"Breaking Cause %r\n\r", Status);
                break;
            }
            VOID *String;
            UINTN StringSize = 0;
            UINT32 AuthenticationStatus;
            String = NULL;
            Status = FvInstance->ReadSection(FvInstance, &NameGuid, EFI_SECTION_USER_INTERFACE, 0, &String, &StringSize, &AuthenticationStatus);
            if (StrCmp(Name, String) == 0)
            {

                Print(L"Guid :%g, FileSize %d, Name : %s, Type %d \n\r", NameGuid, FileSize, String, FileType);

                Status = FvInstance->ReadSection(FvInstance, &NameGuid, Section_Type, 0,(VOID **) Buffer, BufferSize, &AuthenticationStatus);
                Print(L"Result Cause %r\n\r", Status);
                FreePool(String);
                return EFI_SUCCESS;
            }
            FreePool(String);
        }
    }
    return EFI_NOT_FOUND;
}