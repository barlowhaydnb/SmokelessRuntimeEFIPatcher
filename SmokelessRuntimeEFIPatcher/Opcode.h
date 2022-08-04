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

EFI_STATUS LoadFS(EFI_HANDLE ImageHandle, CHAR8 *FileName,EFI_LOADED_IMAGE_PROTOCOL **ImageInfo, EFI_HANDLE *AppImageHandle);
EFI_STATUS LoadFV(EFI_HANDLE ImageHandle, CHAR8 *FileName, EFI_LOADED_IMAGE_PROTOCOL **ImageInfo, EFI_HANDLE *AppImageHandle, EFI_SECTION_TYPE Section_Type);
EFI_STATUS FindLoadedImageFromName(EFI_HANDLE ImageHandle, CHAR8 *FileName, EFI_LOADED_IMAGE_PROTOCOL **ImageInfo);
EFI_STATUS Exec(EFI_HANDLE *AppImageHandle);