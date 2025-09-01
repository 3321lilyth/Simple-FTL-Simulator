#include "simu.h"
#include "MLC.h"
#include <stdint.h>
#include <inttypes.h>

void MLCinit(MLC *MLCptr, FILE *fp)
{
	BYTE	buf[101], junk[101];
	DWORD	i, j;
	MLCconfig	*C = &(MLCptr->config);
	C->HotBlockCounter = TOTAL_HOT_BLOCK;		//我後來加的


	assert(fp != NULL);

	// initialize the configurations

	// blockEndurance
	fgets(buf, 100, fp);		


	// PsizeByte
	fgets(buf, 100, fp);
	// sscanf(buf, "%s %I64i", junk, &(C->PsizeByte));//用這個讀 int64 好像會出錯，所以我改用 lld
	sscanf(buf, "%s %lld", junk, &(C->PsizeByte));
	assert(strcmp(junk, "PsizeByte") == 0);
	printf("PsizeByte(lld) = %lld\n", C->PsizeByte);

	// LsizeByte
	fgets(buf, 100, fp);
	// sscanf(buf, "%s %I64i" ,junk, &(C->LsizeByte));
	sscanf(buf, "%s %lld" ,junk, &(C->LsizeByte));
	assert(strcmp(junk, "LsizeByte") == 0);
	printf("LsizeByte(lld) = %lld\n", C->LsizeByte);

	// blockSizeByte
	fgets(buf, 100, fp);
	sscanf(buf, "%s %lu", junk, &(C->blockSizeByte));
	assert(strcmp(junk, "blockSizeByte") == 0);
	printf("blockSizeByte = %lu\n", C->blockSizeByte);

	// pageSizeByte
	fgets(buf, 100, fp);
	sscanf(buf, "%s %lu", junk, &(C->pageSizeByte));
	assert(strcmp(junk, "pageSizeByte") == 0);
	printf("pageSizeByte = %lu\n", C->pageSizeByte);

	// sectorSizeByte
	fgets(buf, 100, fp);
	sscanf(buf, "%s %lu", junk, &(C->sectorSizeByte));
	assert(strcmp(junk, "sectorSizeByte") == 0);
	printf("sectorSizeByte = %lu\n", C->sectorSizeByte);

	// maxLogBlocks
	fgets(buf, 100, fp);

	// foldingLookahead
	fgets(buf, 100, fp);

	fclose(fp);

	assert(C->LsizeByte % C->blockSizeByte == 0);
	assert(C->PsizeByte % (long long)C->blockSizeByte == 0);
	assert(C->blockSizeByte % C->pageSizeByte == 0);
	assert(C->pageSizeByte % C->sectorSizeByte == 0);
	assert(C->LsizeByte < C->PsizeByte);

	C->pageSizeSector	= C->pageSizeByte  / C->sectorSizeByte;
	C->blockSizePage	= C->blockSizeByte / C->pageSizeByte;
	C->blockSizeSector	= C->blockSizeByte / C->sectorSizeByte;
	C->PsizeBlock		= (DWORD)(C->PsizeByte / (I64)C->blockSizeByte);
	C->PsizePage		= (DWORD)(C->PsizeByte / (I64)C->pageSizeByte);
	C->LsizeSector		= (DWORD)(C->LsizeByte / (I64)C->sectorSizeByte);	
	C->LsizePage		= (DWORD)(C->LsizeByte / (I64)C->pageSizeByte);
	C->LsizeBlock		= (DWORD)(C->LsizeByte / (I64)C->blockSizeByte);

//=====================================   MLC initialize   ================================================

	MLCptr->blocks = (MLCblock *)calloc(C->PsizeBlock, sizeof(MLCblock));
	// the 1st page pointes to the beginning of the array of all pages
	MLCptr->blocks[0].pages = (MLCpage *)calloc(C->PsizePage, sizeof(MLCpage));	
	// let the pages of subsequent points to proper offsets of the array of all pages...


	for(i = 0; i < C->PsizeBlock; i++){
		MLCptr->blocks[i].pages = MLCptr->blocks[0].pages + i * C->blockSizePage;	
		MLCptr->blocks[i].validPages = 0;
		MLCptr->blocks[i].is_hot = NON_HOT_BLOCK;	//我後來加的

		for(j = 0; j < C->blockSizePage; j++){
			MLCptr->blocks[i].pages[j].sector = MLC_PAGE_FREE;	
			MLCptr->blocks[i].pages[j].valid = 0;
		}
	}

//====================================   table initialize   ===============================================

	MLCptr->repTable	= (DWORD *)calloc(C->PsizeBlock, sizeof(DWORD));
	MLCptr->predTable	= (DWORD *)calloc(C->PsizeBlock, sizeof(DWORD));
	MLCptr->mapTable	= (MLCpageTableElement *)calloc(C->LsizePage, sizeof(MLCpageTableElement));
	MLCptr->regionTable = (MLCregTableElement  *)calloc(MLCptr->REGION_NUMBER, sizeof(MLCregTableElement));    
	MLCptr->regionBlockCounter   = (DWORD *)calloc(MLCptr->REGION_NUMBER, sizeof(DWORD));
	MLCptr->stat.regGCBlockCount = (DWORD *)calloc(MLCptr->REGION_NUMBER, sizeof(DWORD));
	MLCptr->GClist = (MLClist *)calloc(MLCptr->config.blockSizePage + 1, sizeof(MLClist));
	
//====================================     table filling    ===============================================
//因為 LBA 看的到的大小只有 LsizeBlock，所以就只需要把 LsizeBlock 個 block 的所有 page 視為初始的 logical page 即可
	
	for(i = 0; i <= MLCptr->config.blockSizePage; i++){
		MLCptr->GClist[i].Count = 0;
		MLCptr->GClist[i].Head = MLCptr->GClist[i].Tail = MLC_LOGBLOCK_NULL;
	}

	MLCptr->unRefList.Head = MLCptr->unRefList.Tail = MLC_LOGBLOCK_NULL;
	MLCptr->unRefList.Count = 0;

	MLCptr->Sparelist.Head	= MLCptr->Sparelist.Tail = MLC_LOGBLOCK_NULL;
	MLCptr->Sparelist.Count = 0;

	// init the region table
	for(i = 0; i < MLCptr->REGION_NUMBER; i++)                                                                      {
		MLCptr->regionTable[i].activeBlockNo = MLC_LOGBLOCK_NULL;
		MLCptr->regionTable[i].activePageNo = MLC_PAGE_FREE;
		MLCptr->regionTable[i].activeGCBlockNo = MLC_LOGBLOCK_NULL;
		MLCptr->regionTable[i].activeGCPageNo = MLC_PAGE_FREE;
		MLCptr->regionBlockCounter[i] = 0;
		MLCptr->stat.regGCBlockCount[i] = 0;
	}

	// map all data onto flash
	for(i = 0; i < C->LsizeBlock; i++)	{
		MLCptr->blocks[i].ECs = 0;
		MLCptr->blocks[i].regionNo = 0;			// block region #
		MLCptr->blocks[i].blockTimeStamp = 0;
		
		//我後來加的

		if (i > C->LsizeBlock-1-TOTAL_HOT_BLOCK/2){		// i = C->LsizeBlock-50 ~ C->LsizeBlock-1，最新的 50 個
			MLCptr->blocks[i].is_hot = HOT_BLOCK;
		}else if(i > C->LsizeBlock-1- TOTAL_HOT_BLOCK){	//i = C->LsizeBlock-100 ~ C->LsizeBlock-51，倒數第二新的 50 個
			MLCptr->blocks[i].is_hot = NOT_THAT_HOT_BLOCK;	
		}else{
			MLCptr->blocks[i].is_hot = NON_HOT_BLOCK;
		}
		MLCptr->config.HotBlockCounter = 0;

		MLCptr->predTable[i] = MLCptr->repTable[i] = MLC_LOGBLOCK_NULL;
		MLCinsertList(MLCptr, &MLCptr->unRefList, i);

		for(j = 0; j < C->blockSizePage; j++){
			MLCptr->blocks[i].pages[j].sector = i * C->blockSizeSector + j *  C->pageSizeSector;	// map the sector # to page
			MLCptr->blocks[i].pages[j].valid = 1;
			MLCptr->blocks[i].pages[j].regionNo = 0;												// page region #
			MLCptr->blocks[i].pages[j].lifeTime = 0xffffffff;
			MLCptr->blocks[i].validPages++;
			MLCptr->mapTable[i * C->blockSizePage + j].blockNo = i;
			MLCptr->mapTable[i * C->blockSizePage + j].pageNo = j;
			MLCptr->mapTable[i * C->blockSizePage + j].regionNo = 0;								// page table region #
			MLCptr->mapTable[i * C->blockSizePage + j].pageTimeStamp = 0xffffffff;
		}
		MLCptr->blocks[i].GCflag = 3;
		MLCptr->regionBlockCounter[0]++;
		assert(MLCptr->blocks[i].validPages == MLCptr->config.blockSizePage);		
	}

	MLCptr->Sparelist.Count = 0;
	
	// chain the unused blocks together as spare blocks

	for(; i < C->PsizeBlock; i++){
		MLCptr->blocks[i].ECs = 0;
		MLCptr->blocks[i].GCflag = 0;

		if(i == C->LsizeBlock){
			MLCptr->repTable[i] = MLC_LOGBLOCK_NULL;
			MLCptr->predTable[i] = i + 1;
			MLCptr->Sparelist.Tail = i;
		}else if(i == C->PsizeBlock - 1){
			MLCptr->repTable[i] = i - 1;
			MLCptr->predTable[i] = MLC_LOGBLOCK_NULL;
			MLCptr->Sparelist.Head = i;
		}else{
			MLCptr->repTable[i] = i - 1;
			MLCptr->predTable[i] = i + 1;
		}
		MLCptr->Sparelist.Count++;
		// MLCptr->blocks[i].GCflag = 0;
	}
	printf("LsizeBlock:%d\tPsizeBlock:%d\tSpareBlockCount:%d\tLsizeBlock + SpareBlockCount:%d\n", C->LsizeBlock, C->PsizeBlock, MLCptr->Sparelist.Count, C->LsizeBlock + MLCptr->Sparelist.Count);

	MLCptr->stat.maxCopy = 0;
	MLCptr->stat.blockErase = 0;
	MLCptr->stat.pageRead = 0;
	MLCptr->stat.pageWrite = 0;
	MLCptr->stat.userPageRead = 0;
	MLCptr->stat.userPageWrite = 0;	
	MLCptr->systemTime = 1;
}

void MLCfree(MLC *MLCptr){
	free(MLCptr->blocks[0].pages);
	free(MLCptr->blocks);
	free(MLCptr->repTable);	
	free(MLCptr->predTable);
	free(MLCptr->mapTable);
	free(MLCptr->regionTable);	
	free(MLCptr->regionBlockCounter);
	free(MLCptr->stat.regGCBlockCount);
	free(MLCptr->GClist);
}