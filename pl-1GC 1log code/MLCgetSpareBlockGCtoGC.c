#include "simu.h"
#include "MLC.h"

//每個 region 各自做 GC 來找到想要的 sparce block

DWORD MLCgetSpareBlockGCtoGC(MLC *MLCptr, DWORD rNo)
{
	DWORD victim, ori_regNo, vicPageIndex, gc_regNo;
	DWORD gcBlock, gcPageIndex, logBlock, LPA;

	// when spare blocks is not enough, start to GC
	DWORD count = 0;
	while (MLCptr->Sparelist.Count <= 3){
		count++;
		if (count == 10000) {
			printf("stack in while !!!\n");
			exit(0);
		}
		DWORD i;
		for (i=0; i<32; i++){

			//step1: 用 victim selection algorithm 選一個 victim
			// 我後來加的
			// victim = MLCgetGCvictimOurMethod(MLCptr);
			// if (victim == NULL){
			// 	victim		= MLCgetGCvictimGreedy(MLCptr);	
			// }
			victim		= MLCgetGCvictimGreedy(MLCptr);	

			ori_regNo = MLCptr->blocks[victim].regionNo;
			vicPageIndex = 0;

			if (MLCptr->blocks[victim].validPages > MLCptr->stat.maxCopy)
				MLCptr->stat.maxCopy = MLCptr->blocks[victim].validPages;


			//step2: 把 victim 從 GCList 移除
			// assert(MLCptr->predTable[victim] == 0xffffffff);
			MLCremoveList(MLCptr, &MLCptr->GClist[MLCptr->blocks[victim].validPages], victim, MLCptr->predTable[victim]);

			//step3: 把 victim block 裡面的 valid page data 逐頁移動到 GCblock，並更新 mapTable[LPA]
			DWORD while_count = 0, if_count = 0;
			while (MLCptr->blocks[victim].validPages != 0){

				while_count++;
				while (MLCptr->blocks[victim].pages[vicPageIndex].valid == 0 && vicPageIndex < MLCptr->config.blockSizePage)
					vicPageIndex++;
				LPA = MLCptr->blocks[victim].pages[vicPageIndex].sector / MLCptr->config.pageSizeSector;

				// if this page stay in GC block long enough, then demote it to the previous region 
				gc_regNo = 0;
				gcBlock = MLCptr->regionTable[gc_regNo].activeGCBlockNo;
				gcPageIndex = MLCptr->regionTable[gc_regNo].activeGCPageNo;

				// if the gc block of this region hasn't been allocated, then allocate one from spare list
				if (gcBlock == MLC_LOGBLOCK_NULL){
					if_count++;
					gcBlock = MLCptr->Sparelist.Head;
					MLCremoveList(MLCptr, &MLCptr->Sparelist, gcBlock, MLC_LOGBLOCK_NULL);
					assert(MLCptr->blocks[gcBlock].GCflag == 0);
					gcPageIndex = 0;

					MLCptr->regionTable[gc_regNo].activeGCBlockNo = gcBlock;
					MLCptr->regionTable[gc_regNo].activeGCPageNo = 0;
					MLCptr->blocks[gcBlock].regionNo = gc_regNo;
					MLCptr->regionBlockCounter[gc_regNo]++;
				}

				// invalidate the original data
				MLCptr->blocks[victim].pages[vicPageIndex].valid = 0;
				MLCptr->blocks[victim].validPages--;

				// emulate (a page) wrtie to the gc block
				MLCptr->blocks[gcBlock].pages[gcPageIndex].sector = MLCptr->blocks[victim].pages[vicPageIndex].sector;
				MLCptr->blocks[gcBlock].pages[gcPageIndex].valid = 1;
				MLCptr->blocks[gcBlock].blockTimeStamp = MLCptr->systemTime;
				MLCptr->blocks[gcBlock].validPages++;

				// update map table
				MLCptr->mapTable[LPA].blockNo = gcBlock;
				MLCptr->mapTable[LPA].pageNo = gcPageIndex;
				MLCptr->mapTable[LPA].regionNo = gc_regNo;

				// proceed the gcBlock pageIndex
				gcPageIndex++;
				MLCptr->regionTable[gc_regNo].activeGCPageNo = gcPageIndex;

				// step4: gc block is full, 依 valid page 數掛回 GClist，並清掉該 region 的 activeGCBlockNo。
				if (MLCptr->regionTable[gc_regNo].activeGCPageNo == MLCptr->config.blockSizePage)
				{
					assert(MLCptr->repTable[MLCptr->regionTable[gc_regNo].activeGCBlockNo] == MLC_LOGBLOCK_NULL && MLCptr->predTable[MLCptr->regionTable[gc_regNo].activeGCBlockNo] == MLC_LOGBLOCK_NULL);
					MLCinsertList(MLCptr, &MLCptr->GClist[MLCptr->blocks[gcBlock].validPages], MLCptr->regionTable[gc_regNo].activeGCBlockNo);
					MLCptr->blocks[MLCptr->regionTable[gc_regNo].activeGCBlockNo].GCflag = 1;
					MLCptr->regionTable[gc_regNo].activeGCBlockNo = MLC_LOGBLOCK_NULL;
					MLCptr->regionTable[gc_regNo].activeGCPageNo = MLC_PAGE_FREE;
				}

				// update the statistics
				MLCptr->stat.pageWrite++;
			}

			// step5: victim block 有效頁清到 0 後，擦除並丟回 Sparelist，同時更新 stat.blockErase / blocks[...].ECs
			assert(MLCptr->repTable[victim] == MLC_LOGBLOCK_NULL && MLCptr->predTable[victim] == MLC_LOGBLOCK_NULL);
			MLCinsertList(MLCptr, &MLCptr->Sparelist, victim);

			MLCptr->blocks[victim].blockTimeStamp = MLCptr->systemTime;
			MLCptr->blocks[victim].GCflag = 0;
			MLCptr->regionBlockCounter[MLCptr->blocks[victim].regionNo]--;
			MLCptr->stat.regGCBlockCount[MLCptr->blocks[victim].regionNo]++;

			MLCptr->stat.blockErase++;
			MLCptr->blocks[victim].ECs++;
		}
	}

	logBlock = MLCptr->Sparelist.Head;
	MLCremoveList(MLCptr, &MLCptr->Sparelist, MLCptr->Sparelist.Head, MLC_LOGBLOCK_NULL);
	assert(MLCptr->Sparelist.Head < MLCptr->config.PsizeBlock);
	assert(MLCptr->blocks[logBlock].GCflag == 0); 
	return logBlock;
}