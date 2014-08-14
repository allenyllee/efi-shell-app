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
//EFI_GUID gEfiLoadFileProtocolGuid = LOAD_FILE_PROTOCOL;
EFI_GUID gEfiDevicePathProtocolGuid = DEVICE_PATH_PROTOCOL;

UINTN DPLength(EFI_DEVICE_PATH *DPath){
	UINTN Length = 0 ;
	EFI_DEVICE_PATH *tempDPath = DPath;
	
	while(!IsDevicePathEnd(tempDPath)){
		Length+=DevicePathNodeLength(tempDPath);
		tempDPath = NextDevicePathNode(tempDPath);
	}
	
	return Length + sizeof(EFI_DEVICE_PATH);
}


EFI_DEVICE_PATH* AddDevicePath(EFI_DEVICE_PATH *dpath1, EFI_DEVICE_PATH *dpath2){
	
	VOID *newdpath;
	UINTN Length1,Length2;
	
	Length1 = DPLength(dpath1)-sizeof(EFI_DEVICE_PATH);
	Length2 = DPLength(dpath2);
	
	newdpath = AllocatePool(Length1+Length2);
	
	CopyMem(newdpath,dpath1,Length1);
	CopyMem((((UINT8*)newdpath)+Length1+1),dpath2,Length2);
	
	return (EFI_DEVICE_PATH*)newdpath;
}
/*
EFI_DEVICE_PATH *AddFilePath(EFI_DEVICE_PATH *dpath, CHAR16 *fpath){
	
	EFI_DEVICE_PATH FilePath;
	
	FilePath.
	
	

}
*/

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
	UINTN								LableIndex;
	
	//CHAR16								LableId[] = {L'T',L'F',L'S',L'E',L'R',L'V',L'_',L'P',L'A',L'R',L'T'};
	CHAR16								*LableId = L"TFSERV_PART";
	
	EFI_LOAD_FILE_INTERFACE				*LoadFileInterface;
	EFI_DEVICE_PATH						*DevicePath, *DevicePathNode,*TempDevicePathNode, *EfiFileDevicePath;
	CHAR16								*FilePath = L"\\TFOKR\\tfloader.efi";
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
		
		Status = mBS->OpenProtocol (
				HandleBuffer[index], 
				&gEfiDevicePathProtocolGuid, 
				(VOID **)&DevicePath,
				ImageHandle,
				NULL,
				EFI_OPEN_PROTOCOL_GET_PROTOCOL
				);
		
		if(EFI_ERROR(Status)) continue;
		
		EfiFileDevicePath = FileDevicePath(HandleBuffer[index],FilePath);
		
		DevicePathNode = EfiFileDevicePath;
		while(!IsDevicePathEnd(DevicePathNode)){
			Print(L"Type %d Sub-Type %d  ",DevicePathType(DevicePathNode),DevicePathSubType(DevicePathNode));
			TempDevicePathNode = DevicePathNode;
			DevicePathNode = NextDevicePathNode(DevicePathNode);
		}
		Print(L"%s\n",(CHAR16*)(((UINT8*)TempDevicePathNode)+4));
		//Print(L"\n");
		
		

					
		/*
		Status = mBS->OpenProtocol (
					HandleBuffer[index], 
					&gEfiLoadFileProtocolGuid, 
					(VOID **)&LoadFileInterface,
					ImageHandle,
					NULL,
					EFI_OPEN_PROTOCOL_GET_PROTOCOL
					);
					
		if(EFI_ERROR(Status)) continue;
		*/
		
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
				
				LableIndex = 0;
				while(((CHAR16*)LableIdBuffer)[LableIndex] != '\0' && LableId[LableIndex] != '\0'){
					if(((CHAR16*)LableIdBuffer)[LableIndex] == LableId[LableIndex]){
						LableIndex++;
					}else{
						break;
					}
				}
				if(((CHAR16*)LableIdBuffer)[LableIndex] == LableId[LableIndex]){
					Print(L"Volume Lable is correct.\n");
					//VolumeRoot->GetPosition(VolumeRoot,)
					/*
					LoadFileInterface->LoadFile(LoadFileInterface,
									)
					*/
					//break;
				}else{
					Print(L"Volume Lable is not correct.\n");
				}
				
			}else{
				Print(L"\n");
				continue;
			}
			
		}else{
			Print(L"\n");
			continue;
		}
		
	}
	
	

	
	
	FreePool(BlockBuffer);
	FreePool(LableIdBuffer);
	FreePool(HandleBuffer);
	
	return EFI_SUCCESS;
	
}