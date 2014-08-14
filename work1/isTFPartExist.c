#include <efi.h>
//#include "Dxe/Graphics/Print.h"
//#include "Dxe\Include\EfiPrintLib.h"
//#include "EfiPrintLib.h"
//#include "lib\libefishell\shelllib.h"
#include <efilib.h>
//#include <BlockIo.h>
//#include <shelllib.h>

EFI_GUID gEfiBlockIoProtocolGuid = BLOCK_IO_PROTOCOL;
EFI_GUID gEfiSimpleFileSystemProtocolGuid = SIMPLE_FILE_SYSTEM_PROTOCOL;
EFI_GUID gEfiFileSystemVolumeLableIdGuid = EFI_FILE_SYSTEM_VOLUME_LABEL_INFO_ID;

EFI_STATUS
EFIAPI
isTFPartExistEntry(
	IN EFI_HANDLE			ImageHandle,
	IN EFI_SYSTEM_TABLE		*SystemTable
)
{
	EFI_BOOT_SERVICES 					*mBS;
	EFI_STATUS 							Status;
	
	EFI_HANDLE 							*HandleBuffer;
	UINTN 								HandleCount;
	UINTN								index;					
	
	VOID 								*BlockBuffer;
	EFI_BLOCK_IO						*BlockIo;
	
	EFI_FILE_IO_INTERFACE				*SimpleFileSystem;
	EFI_FILE							*VolumeRoot;
	UINTN								LableIdBufferSize;
	VOID								*LableIdBuffer;
	
	//SIMPLE_TEXT_OUTPUT_INTERFACE *con_out;

	//con_out = SystemTable->ConOut;
	//con_out -> OutputString(con_out, L"HelloWorld!\n");
	
	InitializeLib(ImageHandle, SystemTable);
	mBS = SystemTable->BootServices;
	
	Status = mBS->LocateHandleBuffer(
				ByProtocol,
				&gEfiBlockIoProtocolGuid,
				NULL,
				&HandleCount,
				&HandleBuffer
				);
				
	if(EFI_ERROR(Status)){
		Print(L"Locate Handle Buffer error.\n");
		return EFI_UNSUPPORTED;
	}
	

	for(index = 0 ; index < HandleCount ; index++ ){
		Print(L"Open Protocol...\n");
		
		Status = mBS->OpenProtocol (
					HandleBuffer[index], 
					&gEfiBlockIoProtocolGuid, 
					(VOID **)&BlockIo,
					ImageHandle,
					NULL,
					EFI_OPEN_PROTOCOL_GET_PROTOCOL
					);
					
		if(EFI_ERROR(Status)) continue;
		
		Status = mBS->OpenProtocol (
					HandleBuffer[index], 
					&gEfiSimpleFileSystemProtocolGuid, 
					(VOID **)&SimpleFileSystem,
					ImageHandle,
					NULL,
					EFI_OPEN_PROTOCOL_GET_PROTOCOL
					);
					
		if(EFI_ERROR(Status)) continue;
		
		Print(L"Media Id: %02d, Removable Media: %02d, Last Block: %.10d ", BlockIo->Media->MediaId, BlockIo->Media->RemovableMedia, BlockIo->Media->LastBlock);
		
		BlockBuffer = AllocatePool(BlockIo->Media->BlockSize);
		if(BlockBuffer != NULL){
			Status = BlockIo->ReadBlocks(
							BlockIo,
							BlockIo->Media->MediaId,
							0,
							BlockIo->Media->BlockSize,
							BlockBuffer
							);

			Print(L"Signature: 0x%X ", (*(UINT16*)&(((UINT8*)BlockBuffer)[510])));
			
			FreePool(BlockBuffer);
		}else{
			Print(L"\n");
			continue;
		}	
			
		SimpleFileSystem->OpenVolume(SimpleFileSystem,&VolumeRoot);
		
		LableIdBufferSize = 0;
		Status = VolumeRoot->GetInfo(VolumeRoot, 
				&gEfiFileSystemVolumeLableIdGuid,
				&LableIdBufferSize,
				LableIdBuffer
				);
		
		if(Status == EFI_BUFFER_TOO_SMALL){
			LableIdBuffer = AllocatePool(LableIdBufferSize);
			if(LableIdBuffer != NULL){
				Status = VolumeRoot->GetInfo(VolumeRoot, 
						&gEfiFileSystemVolumeLableIdGuid,
						&LableIdBufferSize,
						LableIdBuffer
						);
				
				Print(L"Volume Lable:%s\n",LableIdBuffer);
				FreePool(LableIdBuffer);
			}else{
				Print(L"\n");
				continue;
			}
			
		}else{
			Print(L"\n");
			continue;
		}
		
	}
	
	FreePool(HandleBuffer);
	
	return EFI_SUCCESS;
	
}