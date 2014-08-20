#include <efi.h>
#include <efilib.h>

#define EfiBootServiceData 4
#pragma pack(1)
typedef struct{
	UINT8	BootIndicator;
	UINT8	StartingCHS[3];
	UINT8	OSType;
	UINT8	EndingCHS[3];
	UINT32	StartingLBA;
	UINT32	SizeInLBA;
}LEGACY_MBR_PARTITION_RECORD;

typedef struct{
	UINT8							BootCode[440];
	UINT32							UniqueMBRDiskSignature;
	UINT16							Unknown;
	LEGACY_MBR_PARTITION_RECORD		PartitionRecord[4];
	UINT16							Signature;
}LEGACY_MBR;
#pragma pack()

EFI_GUID 							gEfiBlockIoProtocolGuid = BLOCK_IO_PROTOCOL;
EFI_GUID 							gEfiSimpleFileSystemProtocolGuid = SIMPLE_FILE_SYSTEM_PROTOCOL;
EFI_GUID 							gEfiFileSystemVolumeLableIdGuid = EFI_FILE_SYSTEM_VOLUME_LABEL_INFO_ID;
EFI_GUID 							gEfiDevicePathProtocolGuid = DEVICE_PATH_PROTOCOL;

EFI_BOOT_SERVICES 					*mBS;


VOID *
GetLastDevicePathNode(
	IN EFI_DEVICE_PATH *DevicePath
)
{
	EFI_DEVICE_PATH *CurrentDPNode, *PreviousDPNode;
	
	CurrentDPNode = DevicePath;
	while(!IsDevicePathEnd(CurrentDPNode)){
		PreviousDPNode = CurrentDPNode;
		CurrentDPNode = NextDevicePathNode(CurrentDPNode);
	}
	
	return PreviousDPNode;
}

VOID *
GetUpperDevicePath(
	IN EFI_DEVICE_PATH *DevicePath
)
{
	EFI_STATUS			Status;
	EFI_DEVICE_PATH 	*UpperDevicePath, *LastDPNode, *EndDPNode;
	UINTN				UpperDPLength;
	
	LastDPNode = GetLastDevicePathNode(DevicePath);
	UpperDPLength = (UINTN)LastDPNode - (UINTN)DevicePath;
	
	Status = mBS->AllocatePool(
				EfiBootServiceData, 
				UpperDPLength+sizeof(EFI_DEVICE_PATH), 
				(VOID**)&UpperDevicePath
				);
	
	if(EFI_ERROR(Status)){
		return NULL;
	}
	
	mBS->CopyMem(
			(VOID*)UpperDevicePath,
			(VOID*)DevicePath,
			UpperDPLength
			);
	
	EndDPNode = (EFI_DEVICE_PATH*)((UINT8*)UpperDevicePath+UpperDPLength);
	EndDPNode->Type = END_DEVICE_PATH_TYPE;
	EndDPNode->SubType = END_ENTIRE_DEVICE_PATH_SUBTYPE;
	SetDevicePathNodeLength(EndDPNode,END_DEVICE_PATH_LENGTH);
	
	return UpperDevicePath;
}


EFI_HANDLE
GetPhysicalBlockIoHandle(
	IN EFI_HANDLE ImageHandle,
	IN EFI_HANDLE PartitionHandle
)
{
	EFI_STATUS 				Status;
	EFI_DEVICE_PATH			*PartitionDevicePath, *UpperDevicePath;
	EFI_HANDLE				PhysicalBlockIoHandle;
	
	Status = mBS->OpenProtocol (
				PartitionHandle, 
				&gEfiDevicePathProtocolGuid, 
				(VOID **)&PartitionDevicePath,
				ImageHandle,
				NULL,
				EFI_OPEN_PROTOCOL_GET_PROTOCOL
				);
	
	if(EFI_ERROR(Status)){
		return PartitionHandle;
	}
	
	UpperDevicePath = (EFI_DEVICE_PATH*) GetUpperDevicePath(PartitionDevicePath);
	
	Status = mBS->LocateDevicePath(
				&gEfiBlockIoProtocolGuid,
				&UpperDevicePath,
				&PhysicalBlockIoHandle
				);
				
	if(EFI_ERROR(Status)){
		Print(L"LocateDevicePath error\n.");
		return PartitionHandle;
	}
	
	FreePool(UpperDevicePath);
	
	return PhysicalBlockIoHandle;
}

VOID*
GetFileSystemVolumeLableID(
	IN EFI_FILE_IO_INTERFACE *SimpleFileSystem
)
{
	EFI_STATUS 			Status;
	EFI_FILE			*VolumeRoot;
	UINTN				LableIdBufferSize = 0;
	VOID				*LableIdBuffer=NULL;
	
	Status = SimpleFileSystem->OpenVolume(SimpleFileSystem,&VolumeRoot);
	
	if(EFI_ERROR(Status)){
		return NULL;
	}
	
	Status = VolumeRoot->GetInfo(
					VolumeRoot, 
					&gEfiFileSystemVolumeLableIdGuid,
					&LableIdBufferSize,
					LableIdBuffer
					);
	
	if(Status == EFI_BUFFER_TOO_SMALL){
		Status = mBS->AllocatePool(
					EfiBootServiceData,
					LableIdBufferSize,
					(VOID**)&LableIdBuffer
					);
					
		if(EFI_ERROR(Status)){
			return NULL;
		}
		
		Status = VolumeRoot->GetInfo(
						VolumeRoot, 
						&gEfiFileSystemVolumeLableIdGuid,
						&LableIdBufferSize,
						LableIdBuffer
						);
		
		
		return LableIdBuffer;
		
	}else{
	
		return NULL;
	}
		
}
/*
INTN
StrCmp(
	IN CHAR16 *s1,
	IN CHAR16 *s2
)
{
	UINTN		Index = 0;
	INTN		result;
	
	while(s1[Index] != '\0' && s2[Index] != '\0'){
		if(s1[Index] == s2[Index]){
			Index++;
		}else{
			break;
		}
	}
	
	if(s1[Index] == s2[Index]){
		result = 0; 
	}else if(s1[Index] != '\0'){
		result = 1;
	}else if(s2[Index] != '\0'){
		result = 2;
	}else{
		result = -1;
	}
	
	return result;
}
*/

EFI_STATUS
EFIAPI
isTFPartExistEntry(
	IN EFI_HANDLE			ImageHandle,
	IN EFI_SYSTEM_TABLE		*SystemTable
)
{
	
	EFI_STATUS 							Status;
	
	EFI_HANDLE 							*HandleBuffer;
	UINTN 								HandleCount;
	UINTN								index;					
	
	VOID 								*PhysicalBlockBuffer;
	EFI_BLOCK_IO						*PhysicalBlockIo;
	EFI_HANDLE							PhysicalBlockIoHandle;
	
	EFI_FILE_IO_INTERFACE				*SimpleFileSystem;
	VOID								*LableIdBuffer;
	CHAR16								*LableId = L"TFSERV_PART";
	//CHAR16								*LableId = L"ALLEN4G1";

	EFI_DEVICE_PATH						*DevicePath;
	UINTN								PartitionNumber;
	EFI_DEVICE_PATH						*TargetFileDevicePath=NULL;
	CHAR16								*FilePath = L"\\TFOKR\\tfloader.efi";
	//CHAR16								*FilePath = L"\\EFI\\X64\\RU.efi";
	EFI_HANDLE							TargetFileImageHandle;
	
	//initial Print function
	InitializeLib(ImageHandle, SystemTable);
	//initial BootServices
	mBS = SystemTable->BootServices;
	
	//locate all handles that supports simple file system protocol
	//must call Free(HandleBuffer)
	Status = mBS->LocateHandleBuffer(
				ByProtocol,
				&gEfiSimpleFileSystemProtocolGuid,
				NULL,
				&HandleCount,
				&HandleBuffer
				);
				
	if(EFI_ERROR(Status)){
		Print(L"Locate SimpleFileSystem Handle Buffer error.\n");
		return EFI_UNSUPPORTED;
	}

	//scan all simple file system volume
	for(index = 0 ; index < HandleCount ; index++ ){
		Print(L"Open SimpleFileSystem Protocol...\n");
		
		//open simple file system protocol
		Status = mBS->OpenProtocol (
					HandleBuffer[index], 
					&gEfiSimpleFileSystemProtocolGuid, 
					(VOID **)&SimpleFileSystem,
					ImageHandle,
					NULL,
					EFI_OPEN_PROTOCOL_GET_PROTOCOL
					);
		
		if(EFI_ERROR(Status)) 
		{
			Print(L"Open SimpleFileSystem protocol error.\n");
			continue;
		}
		
		//get lable id of this volume
		//must call Free(LableIdBuffer)
		LableIdBuffer = GetFileSystemVolumeLableID(SimpleFileSystem);
		
		//compare lable id of this volume
		if( StrCmp((CHAR16*)LableIdBuffer,LableId)!=0 ){
			Print(L"LableId not match.\n");
			continue;
		}
		
		//get device path of this volume
		Status = mBS->OpenProtocol (
				HandleBuffer[index], 
				&gEfiDevicePathProtocolGuid, 
				(VOID **)&DevicePath,
				ImageHandle,
				NULL,
				EFI_OPEN_PROTOCOL_GET_PROTOCOL
				);
		
		if(EFI_ERROR(Status)) 
		{
			Print(L"Open DevicePath error.\n");
			continue;
		}
		
		//get partition number of this volume
		PartitionNumber = ((HARDDRIVE_DEVICE_PATH*)GetLastDevicePathNode(DevicePath))->PartitionNumber;
		
		//get BlockIo Handle of Physical Drive that contains this volume 
		PhysicalBlockIoHandle = GetPhysicalBlockIoHandle(ImageHandle,HandleBuffer[index]);
		
		//open BlockIo protocol of Physical Drive
		Status = mBS->OpenProtocol (
			PhysicalBlockIoHandle, 
			&gEfiBlockIoProtocolGuid, 
			(VOID **)&PhysicalBlockIo,
			ImageHandle,
			NULL,
			EFI_OPEN_PROTOCOL_GET_PROTOCOL
			);
					
		if(EFI_ERROR(Status)) 
		{
			Print(L"Open BlockIo protocol error.\n");
			continue;
		}
		
		//allocate memory for reading block data
		//must call Free(PhysicalBlockBuffer)
		Status = mBS->AllocatePool(
					EfiBootServicesData,
					PhysicalBlockIo->Media->BlockSize,
					&PhysicalBlockBuffer
					);
		
		if(EFI_ERROR(Status)) 
		{
			Print(L"AllocatePool error.\n");
			continue;
		}
		
		//read LBA0 Block ioto memory
		Status = PhysicalBlockIo->ReadBlocks(
							PhysicalBlockIo,
							PhysicalBlockIo->Media->MediaId,
							0,
							PhysicalBlockIo->Media->BlockSize,
							PhysicalBlockBuffer
							);
		
		if(EFI_ERROR(Status)) 
		{
			Print(L"ReadBlocks error.\n");
			continue;
		}
		
		//compare MBR signature
		if( ((LEGACY_MBR*)PhysicalBlockBuffer)->Signature != 0xAA55 ){
			Print(L"Not MBR Signature.\n");
			continue;
		}
		
		//compare OSType of the PartitionRecord of this volume
		if( ((LEGACY_MBR*)PhysicalBlockBuffer)->PartitionRecord[PartitionNumber-1].OSType != 0x12 ){
			Print(L"OS Type != 0x12.\n");
			continue;
		}
		
		//append file path after the device path of this volume
		TargetFileDevicePath = FileDevicePath(HandleBuffer[index],FilePath);
		break;
	}
	
	if(!TargetFileDevicePath){
		Print(L"There are no TF Partition Exist.\n");
		return EFI_UNSUPPORTED;
	}
	
	//load .efi file as image
	Status = mBS->LoadImage(
				FALSE,
				ImageHandle,
				TargetFileDevicePath,
				NULL,
				0,
				&TargetFileImageHandle
				);
	
	if(EFI_ERROR(Status)){
		Print(L"Load file image error.\n");
		return Status;
	}
	
	//start this image 
	Status = mBS->StartImage(
				TargetFileImageHandle,
				NULL,
				NULL
				);
				
	if(EFI_ERROR(Status)){
		Print(L"Start image error.\n");
		return Status;	
	}
	
	
	FreePool(PhysicalBlockBuffer);
	FreePool(LableIdBuffer);
	FreePool(HandleBuffer);
	
	return EFI_SUCCESS;
	
}