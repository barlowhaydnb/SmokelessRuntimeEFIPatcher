#pragma once
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
#include <PiDxe.h>
#include <Protocol/FirmwareVolume2.h>
#include <Library/BaseMemoryLib.h>

CHAR16 *
FindLoadedImageFileName(
    IN EFI_LOADED_IMAGE_PROTOCOL *LoadedImage);

EFI_STATUS LoadandRunImage(EFI_HANDLE ImageHandle,
                           EFI_SYSTEM_TABLE *SystemTable,
                           CHAR16 *FileName,
                           EFI_HANDLE *AppImageHandle);

UINT8 *FindBaseAddressFromName(const CHAR16 *Name);

EFI_STATUS LocateAndLoadFvFromName(CHAR16 *Name, EFI_SECTION_TYPE Section_Type,UINT8 **Buffer,UINTN *BufferSize);
