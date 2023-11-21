#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <Windows.h>
#include <list>
#include <assert.h>

using std::list;

#define ARMAG "!<arch>\n"
#define SARMAG 8

struct ar_hdr {
	char ar_name[16];
	char ar_date[12];
	char ar_uid[6];
	char ar_gid[6];
	char ar_mode[8];
	char ar_size[10];
	char ar_fmag[2];
};

enum MemberType{
	TYPE_HEAD_1 = 0,
	TYPE_HEAD_2,
	TYPE_DIR,
	TYPE_OBJ_FILE
};

struct Member{
	DWORD dwType;

	DWORD dwOffsetInFile;
	DWORD dwNewOffsetInFile;

	ar_hdr header;
	int  dwDataSize;
	char *data;
};


struct LibCtx{
	list<Member*> m_Members;
};

DWORD ToBigEndian(DWORD val){
	DWORD dwResult = 0;
	for (int i = 0; i < 4; i++){
		dwResult = (dwResult << 8) + (val & 0xff);
		val >>= 8;
	}
	return dwResult;
}

void PrintHdrName(ar_hdr*hdr){
	printf("Member Name: ");
	for (int i = 0; hdr->ar_name[i] != ' '; i++) 
		putchar(hdr->ar_name[i]);
}


/*
	��һ������:
	DWORD NumbersOfSymbols;							(���)
	DWORD dwOffset[], �������ڵ�Obj��lib�ļ��е�ƫ��.	(���)
	�����ַ���


	�ڶ�������:
	DWORD NumberOfObjs;      //
	DWORD dwOffsetOfObjs[];  //

	DWORD NumberOfSymbols;
	WORD  IndexOfSymbolObj[];
	�ַ�����.
*/

struct Header1{
	DWORD dwNumberOfSymbols;
	DWORD dwObjOffset[1];
};

struct Header2{
	DWORD NumberOfObjs;
	DWORD dwOffsetOfObjs[1];
};



typedef struct _FILEHEADER
{
	unsigned short machine;     // ƽ̨��
	unsigned short numberOfSections;// ������
	unsigned long  timeDateStamp;   // ʱ���
	unsigned long  pointerToSymbolTable;		// ���ű��ļ�ƫ��
	unsigned long  numberOfSymbols;				// �����ܸ���
	unsigned short sizeOfOptionalHeader;		// ��ѡͷ����
	unsigned short characteristics;				// �ļ����

} FILEHEADER, *PFILEHEADER;

/*
	���Ϊ8���ֽڵ�,�ԡ�\0��Ϊ��β��ASCII�ַ���.���ڼ�¼���ε�����
	.���ε�������Щ���ض����������. �������������������8���ֽ�,
	��name�ĵ�һ�ֽ���һ��б���ַ�:��/��,���ž���һ������,
	������־����ַ������һ������.����������һ�������������.


	֮�������:

	���name == '//',���������ļ���·��(���Ǹ��ַ�����)��
	֮������ŵ�n��member �����ƶ��� / + name��Ŀ¼�����е�ƫ��(���������ᵽ���ַ�����).

	����:
	name�����ļ���,���ݾ����ļ�������.

	ע��ÿ��name����/��ͷ.
*/

typedef struct  _SECTIONHEADER
{
	char name[8];							// ����
	unsigned long  virtualSize;			     // �����С  û����
	unsigned long  virtualAddress;		     // �����ַ û����
	unsigned long  sizeOfRawData;			 // �������ݵ��ֽ���
	unsigned long  pointerToRawData;		 // ��������ƫ��
	unsigned long  pointerToRelocations;     // �����ض�λ��ƫ��
	unsigned long  pointerToLinenumbers;     // �кű�ƫ��
	unsigned short  numberOfRelocations;     // �ض�λ�����
	unsigned short  numberOfLinenumbers;     // �кű����
	unsigned long  characteristics;          // �α�ʶ
}SECTIONHEADER, *PSECTIONHEADER;


#pragma pack(push, 2)
typedef struct _SYMBOL
{
	union {
		char name[8];               // ��������
		struct {
			unsigned long zero;     // �ַ������ʶ
			unsigned long offset;  // �ַ���ƫ��
		};
	};
	unsigned long value;            // ����ֵ
	short          section;          // �������ڶ�
	unsigned short type;            // ��������
	unsigned char Class;            // ���Ŵ洢����
	unsigned char numberOfAuxSymbols;// ���Ÿ��Ӽ�¼��
} SYMBOL, *PSYMBOL;
#pragma pack(pop)


typedef struct _STRIGTABLE{
	unsigned int Size;
	char         Data[1];
} STRIGTABLE, *PSTRIGTABLE;

/*
COFF x386 MAGIC
COFF Microsoft CIL Bytecode. ����ݲ�֧��.��̫�˽������ʽ.
*/


void AddSymbol(char * coff, size_t size, char**out, size_t * out_size){
	int i = 0;
	char * coff_end = coff + size;
	*out = NULL;
	*out_size = 0;

	FILEHEADER* fileheader = (FILEHEADER*)coff;
	SECTIONHEADER* sectionHdr = NULL;
	SYMBOL * Symbols = NULL,*pNextSym;
	char *StringTable = NULL;
	char * strTableCopy = NULL;
	int k = 0;

	if (fileheader->machine != IMAGE_FILE_MACHINE_I386)
		return;

	sectionHdr = (SECTIONHEADER*)(coff + sizeof(FILEHEADER));
	Symbols = (SYMBOL*)(coff + fileheader->pointerToSymbolTable);

	pNextSym = Symbols;
	//numbers of symbols ������ aux��symbols......
	printf("numberOfSymbols : %d\n", fileheader->numberOfSymbols);

	for (int i = 0; i < fileheader->numberOfSymbols; i++){
		if (!k){
			//printf("Sym Name : %s \n", pNextSym->name);
			k = pNextSym->numberOfAuxSymbols;
		}
		else{
			k--;
		}
		pNextSym++;
	}

	StringTable = (char*)pNextSym;
	//�����ַ�����.
	DWORD StrTableSize = coff_end - StringTable;
	strTableCopy = new char[StrTableSize];
	memcpy(strTableCopy, StringTable, StrTableSize);

	//�������.

	for (int i = 0; i < fileheader->numberOfSections; i++){
		
	}

	//�ڷ��ű������һ��.
	memset(pNextSym->name, 0, 8);
	strcpy(pNextSym->name, "_bkdr");

	pNextSym->type = 0x20;
	pNextSym->section = 0;
	pNextSym->Class = IMAGE_SYM_CLASS_EXTERNAL;
	pNextSym->value = 0x0;
	pNextSym->numberOfAuxSymbols = 0;

	//////
	char * newCoff = new char[size + sizeof(SYMBOL)];
	memcpy(newCoff, coff, size);
	SYMBOL * newSymbols = (SYMBOL*)(newCoff + ((FILEHEADER*)newCoff)->pointerToSymbolTable);
	SYMBOL * AddSym = newSymbols;
	for (i = 0; i < ((FILEHEADER*)newCoff)->numberOfSymbols; i++){
		AddSym++;
	}

	memcpy(AddSym, pNextSym, sizeof(SYMBOL));
	
	//�����ַ�����.
	memcpy(AddSym + 1, strTableCopy, StrTableSize);
	
	((FILEHEADER*)newCoff)->numberOfSymbols++;

	//���ڷ��ű������ַ�����ƫ����������ַ�����ģ����Բ���Ҫ�޸�
	delete[]strTableCopy;
	//�����µ�obj�ļ��ʹ�С.
	*out = newCoff;
	*out_size = size + sizeof(SYMBOL);
}



LibCtx * ParseLib(BYTE* buff,size_t len){
	//check headers.
	BYTE * it = buff,*end = buff + len;
	Member * member = NULL;
	DWORD dataSize = 0;
	ar_hdr temp = { 0 };

	if (memcmp(ARMAG, buff, SARMAG)){
		fprintf(stderr, "Invalid File!\n");
		return NULL;
	}
	it += SARMAG;
	
	////
	LibCtx * lib = new LibCtx;

	while (it < end){
		//read header 1.
		memcpy(&temp, it, sizeof(ar_hdr));
		PrintHdrName(&temp);
		//
		dataSize = atoll(temp.ar_size);

		printf("\t DataSize: %u\n", dataSize);

		member = new Member;
		member->dwOffsetInFile = it - buff;
		member->dwNewOffsetInFile = member->dwOffsetInFile;

		if (lib->m_Members.size() == 0)
			member->dwType = TYPE_HEAD_1;

		else if (lib->m_Members.size() == 1)
			member->dwType = TYPE_HEAD_2;
		else if (temp.ar_name[0] == '/' && temp.ar_name[1] == '/')
			member->dwType = TYPE_DIR;
		else
			member->dwType = TYPE_OBJ_FILE;
		
		member->header = temp;
		member->dwDataSize = dataSize;
		member->data = new char[dataSize];

		it += sizeof(ar_hdr);
		memcpy(member->data, it, dataSize);
		it += dataSize;

		//���һ���� \n ��������.
		if (dataSize & 1){
			printf("pad \n");
			it++;
		}
			
		lib->m_Members.push_back(member);
	}
	return lib;
}

void FixLib(LibCtx * lib){
	assert(lib);
	assert(lib->m_Members.size() > 2);


	//ע��header1 �����offset �Ǵ�˱�ʾ.
	Header1 * header1 = (Header1*)(lib->m_Members.front()->data);
	Header2 * header2 = NULL;

	DWORD dwNumbersOfSymbols = ToBigEndian(header1->dwNumberOfSymbols);
	for (int i = 0; i < dwNumbersOfSymbols; i++){
		//����header 1�����µ�ƫ��.

		for (auto it : lib->m_Members){
			if (it->dwType == TYPE_OBJ_FILE &&
				ToBigEndian(it->dwOffsetInFile) == header1->dwObjOffset[i]){
				header1->dwObjOffset[i] = ToBigEndian(it->dwNewOffsetInFile);
				break;
			}
		}
	}

	auto sec = lib->m_Members.begin();
	sec++;

	header2 = (Header2*)((*sec)->data);

	for (int i = 0; i < header2->NumberOfObjs; i++){
		//����header 1�����µ�ƫ��.
		for (auto it : lib->m_Members){
			if (it->dwType == TYPE_OBJ_FILE &&
				it->dwOffsetInFile == header2->dwOffsetOfObjs[i]){
				header2->dwOffsetOfObjs[i] = it->dwNewOffsetInFile;
				break;
			}
		}
	}
}

void DumpLib(LibCtx* lib,const char * FileName){
	FILE* fp = fopen(FileName, "wb");
	if (!fp){
		printf("Could not open %s \n", FileName);
		exit(-1);
	}

	fwrite(ARMAG, 1, SARMAG, fp);
	for (auto it : lib->m_Members){
		fwrite(&it->header, 1, sizeof(ar_hdr), fp);
		fwrite(it->data, 1, it->dwDataSize, fp);

		if (it->dwDataSize & 1)
			fwrite("\n", 1, 1, fp); 
	}

	fclose(fp);
}

void AddSymbol(LibCtx * lib){
	for (auto it = lib->m_Members.begin();
		it != lib->m_Members.end();it++){

		if ((*it)->dwType == TYPE_OBJ_FILE){
			Member * mem = (*it);
			size_t NewSize = 0;
			char * OutData = 0;
			
			//������ŷ���.
			AddSymbol(mem->data, mem->dwDataSize, &OutData, &NewSize);
			if (OutData && NewSize){
				//
				int OffsetDelta = 0;
				char szName[0x100] = { 0 };
				char szSize[0x100] = { 0 };

				for (int i = 0; i < sizeof(mem->header.ar_name); i++){
					if (mem->header.ar_name[i] == ' ')
						break;
					szName[i] = mem->header.ar_name[i];
				}

				printf("Write Symbol to %s Success\n", szName);

				delete[]mem->data;
				//����offset delta ֵ��
				OffsetDelta = NewSize - mem->dwDataSize;
				mem->dwDataSize = NewSize;
				mem->data = OutData;
				
				//����д���С
				memset(mem->header.ar_size, ' ', sizeof(mem->header.ar_size));
				sprintf(szSize, "%u", mem->dwDataSize);
				memcpy(mem->header.ar_size, szSize, strlen(szSize));

				//����֮������Members��ƫ��.
				auto k = it;
				k++;
				for (; k != lib->m_Members.end(); k++) (*k)->dwNewOffsetInFile += OffsetDelta;
			}
		}
	}
}

int main(int argc,char * argv[]){
	FILE * fp = NULL;
	size_t len = 0;
	char * LibBuff = NULL;

	if (argc != 3){
		printf("Usage : %s <input lib path> <output lib path>\n",argv[0]);
		exit(1);
	}

	fp = fopen(argv[1], "rb");
	//fp = fopen("c:/Users/lenovo/Desktop/CLibParse/Release/jsond.lib", "rb");
	
	if (!fp){
		fprintf(stderr, "Could not open input lib file �� %s\n", argv[1]);
		exit(-1);
	}

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	//////
	LibBuff = new char[len];
	fread(LibBuff, 1, len, fp);
	fclose(fp);

	LibCtx * lib = ParseLib((BYTE*)LibBuff,len);
	if (!lib){
		fprintf(stderr, "Parse lib file failed!\n");
		exit(-2);
	}

	//��Obj�ļ�����ⲿ����.
	AddSymbol(lib);
	//
	FixLib(lib);
	//������ļ�
	DumpLib(lib, argv[2]);
	delete[]LibBuff;
	return 0;
}