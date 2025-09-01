#define MLC_PAGE_FREE		0xffffffff	
#define MLC_LOGBLOCK_NULL	0xffffffff
#define MLC_BLOCK_FREE		0xffffffff
#define MLC_LRU_EMPTY		0xffffffff
#define MLC_NOT_FOUNT		0xfffffffe
#define VAILD_PAGE_THRESHOLD 	1000		
#define TOTAL_HOT_BLOCK 		400		//the nearest update 100 blocks is defined as hot block
#define	NON_HOT_BLOCK 			0
#define	NOT_THAT_HOT_BLOCK 		1			
#define	HOT_BLOCK 				2				//0: init/non hot, 1: 51~100, 2: 1~50

// TABLEs
//=========================
typedef struct
{
	DWORD	activeBlockNo; 
	DWORD	activePageNo;
	DWORD	activeGCBlockNo;
	DWORD	activeGCPageNo;
}MLCregTableElement;


//這就是 MLC.mapTable ，也就是 LBA->PPA mapping 的每一個欄位，所以其實就是 PPA
//PPA（Physical Page Address） 物理位址由 (blockNo, pageNo[, regionNo]) 組成
typedef struct
{
	DWORD	pageNo;			//[0, config.blockSizePage-1]
	DWORD	blockNo;  		//the address of large granularity or sub mapping table, [0, config.PsizeBlock-1]
	DWORD	regionNo;		//多個 block 組成一個 region，分好幾區做 GC，就是 stable greedy 說的 “global 下的 victim 跟 local 下的 victim 不同” 
	DWORD	pageTimeStamp;	//the most recent modification time of page
}MLCpageTableElement;


//LISTs
//=========================
typedef struct
{
	DWORD	Head;
	DWORD	Tail;
	DWORD	Count;
}MLClist;       


// SRTUSTUREs
//=========================
typedef struct
{
	DWORD	sector;
	BYTE	valid;		// indicate which page is valid ,1:valid	0:invalid
	DWORD	regionNo;
	DWORD	lifeTime;
}MLCpage;

typedef struct
{
	MLCpage	*pages;
	DWORD	ECs;				//race count
	DWORD	validPages;
	DWORD	regionNo;
	DWORD	blockTimeStamp;		// the most recent modification time
	DWORD	GCflag;				//0: in spareList; 1: in GCList; 3: in unRefList
	DWORD	stable;
	DWORD 	is_hot;				//0: init/non hot, 1: 51~100, 2: 1~50
	DWORD	system_time;

}MLCblock;



// CONFIG
// 程式啟動時透過 MLCinit() 從 config 讀 simpleFTL/FTL_code/Config File/my_config/... 進來
// 裡面定義 blockEndurance, PsizeByte, LsizeByte, blockSizeByte, pageSizeByte, sectorSizeByte
//=========================
typedef struct
{
	DWORD	pageSizeByte;		//一個 page 是多少 byte 
	DWORD	pageSizeSector;		//= pageSizeByte  / sectorSizeByte;  -> 每個 page 有幾個 sector
	DWORD	blockSizeByte; 		//一個 block 是多少 byte 
	DWORD	blockSizePage;		//= C->blockSizeByte / C->pageSizeByte;  -> 每個 block 有幾個 page
	DWORD	blockSizeSector;	//= C->blockSizeByte / C->sectorSizeByte; -> 每個 block 有幾個 sector

	I64		PsizeByte;			//物理容量（實體 NAND）有幾 byte
	DWORD	PsizeBlock;			
	DWORD	PsizePage;

	I64		LsizeByte;			//邏輯容量（對主機可見）
	DWORD	LsizeSector;
	DWORD	LsizePage;			//LBA 範圍是 [0, config.LsizeSector-1]
	DWORD	LsizeBlock;

	DWORD	sectorSizeByte;
	DWORD	spareBlock;	
	DWORD	copyPages;
	int	HotBlockCounter;		//how many block left can be hot block

}MLCconfig;


//Statiscics
//=========================
typedef struct
{
	DWORD	pageWrite;
	DWORD	pageRead;

	DWORD	userPageWrite;
	DWORD	userPageRead;

	DWORD	blockErase;
	DWORD	maxCopy;
	DWORD	reqCount;
	DWORD	*regGCBlockCount;	//GC : garbage collection

}MLCstat;


// MLC
//=========================
typedef struct
{

	//===STRUCTUREs===================
	MLCblock	*blocks;				// all blocks
	MLClist		Sparelist;				// chain the unused blocks together as spare blocks
	MLClist		*GClist;				// multilevel block list, diff list have diff #valid page
	MLClist		unRefList;				// 已經寫完的、不是 active block 的 block，因為沒有被 write pointer 指到故名。一個 block 會同時出現再 GCList 跟 unRefList 裡面
	DWORD		systemTime;

	//===TABLEs=======================	
	MLCpageTableElement		*mapTable;		// map all data onto flash，LBA(Logical Block Address) to PPA(Physical Page Address) mapping
	MLCregTableElement	*regionTable;		// check
	DWORD					*repTable;		// to chain the log blocks. repTable 指向他所在 list 的 next block
	DWORD					*predTable;		// predTable 指向他所在 list 的 prev block

	//===SETTING & DATA===============
	MLCconfig	config;					// check
	DWORD		*regionBlockCounter;	// check
	MLCstat		stat;					// check


	//===PARAMETERS===================
	// LRU GC
	DWORD		REGION_NUMBER;
	DWORD		PROMOTE_THRESHOLD;
	DWORD		DEMOTE_THRESHOLD;

	// self-tunning stable time GC
	DWORD		STABLE_TIME_ORI;		
	DWORD		STABLE_TIME;
	DWORD		STABLE_TIME_TUNNING_SCALE;
	double		OLD_COUNT_TH;


}MLC;

void MLCinit(MLC *MLCptr,FILE *fp);
void MLCfree(MLC *MLCptr);
void MLCconvertReq(MLC *MLCptr,DWORD *offsetSector,DWORD *lenSector);
void MLCwriteSector(MLC *MLCptr,DWORD offsetSector,DWORD lenSector);
void MLCinsertList(MLC *MLCptr, MLClist *list, DWORD block);
void MLCremoveList(MLC *MLCptr,MLClist *list, DWORD block, DWORD pred);
// DWORD MLCfindPred(MLC *MLCptr, MLClist *list, DWORD block);
DWORD MLCgetSpareBlockGCtoLOG(MLC *MLCptr,DWORD rNo);
DWORD MLCgetSpareBlockGCtoGC(MLC *MLCptr,DWORD rNo);
DWORD MLCgetGCvictim2G(MLC *MLCptr);
DWORD MLCgetGCvictimGreedy(MLC *MLCptr);
DWORD MLCgetGCvictimOurMethod(MLC *MLCptr);
DWORD MLCgetGCvictimCAT(MLC *MLCptr);
DWORD MLCporTHtop(MLC *MLCptr);
DWORD MLCporTHtail(MLC *MLCptr);