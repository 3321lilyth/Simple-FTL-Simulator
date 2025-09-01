#include "simu.h"
#include "MLC.h"

// handle a write to a single page 
// lenSector 傳進來幹嘛又沒有用到
void MLCwriteSector(MLC *MLCptr,DWORD offsetSector,DWORD lenSector)
{
	DWORD	LPA;														//logical page addr
	DWORD	ori_regNo, wrt_regNo;
	DWORD	logBlock, pageIndex;										//這兩個是 out-of-place update 下實際上要寫入的 block 跟 page
	DWORD	invBlock, invPage, oriVp, invBlockPred;						//這兩個時 out-of-place update 下原本要寫入的 block 跟 page

	//1. 取得要寫入的邏輯位址和 region number
	LPA			= offsetSector / MLCptr->config.pageSizeSector;			//計算對應的 page number
	wrt_regNo	= 0; 													//noSeparation，因為這邊只有一個 region 所以直接設定為 0


	//2. 取得該 region 的 active block (logblock) 和 active page，如果沒有空 block 就要做 GC
	logBlock	= MLCptr->regionTable[wrt_regNo].activeBlockNo;			//當前 region 的 activ block number (活動塊號)
	if(logBlock == MLC_LOGBLOCK_NULL){									//activeBlockNo 初始就是 MLC_LOGBLOCK_NULL
		// 如果這個 region 的 active block 還不存在
		logBlock = MLCgetSpareBlockGCtoGC(MLCptr, wrt_regNo); 			//從給定的 region 中取得 spare block 當作 log block
		MLCptr->regionTable[wrt_regNo].activeBlockNo = logBlock;		//然後更新 activeBlockNo 跟 activePageNo
		MLCptr->regionTable[wrt_regNo].activePageNo	 = pageIndex = 0;	
		MLCptr->blocks[logBlock].regionNo = wrt_regNo;
		MLCptr->regionBlockCounter[wrt_regNo]++;

		assert(MLCptr->repTable[logBlock] == MLC_LOGBLOCK_NULL && MLCptr->predTable[logBlock] == MLC_LOGBLOCK_NULL);
		assert(MLCptr->blocks[logBlock].GCflag == 0);					//沒有對應在 flash 裡的 block 一開始初始化為 0
	}else{
		//如果已經存在 active block，則將 page index 設置為該 active block 的 active page number
		pageIndex = MLCptr->regionTable[wrt_regNo].activePageNo;
	}

	


	//3. ========================== invalidate the ori data (processing the old page) ============================
	// 由 mapTable[LPA] 找到舊的 LPA 對應到的 PPA (invBlock, invPage)，把舊頁標為 invalid
	invBlock = MLCptr->mapTable[LPA].blockNo;
	invPage  = MLCptr->mapTable[LPA].pageNo;
	oriVp	 = MLCptr->blocks[invBlock].validPages;						// original valid page? 反正後面沒用到 ==
	invBlockPred = MLCptr->predTable[invBlock];							//前驅塊，predecessor block，是指某個操作或狀態變化之前的物理塊


	// invalidate 的時候才計算 page lifetime
	MLCptr->blocks[invBlock].pages[invPage].valid = 0;
	MLCptr->blocks[invBlock].blockTimeStamp = MLCptr->systemTime;		//blockTimeStamp 是 the most recent modification time 
	MLCptr->blocks[invBlock].pages[invPage].lifeTime = MLCptr->systemTime - MLCptr->mapTable[LPA].pageTimeStamp;
	
	//我後來加的， for our own victim select method
	//到 threshold 了，就要把最新的 50 個 block 變成倒數第二新的，然後釋出原本倒數第二新的 bloc
	if (MLCptr->config.HotBlockCounter <= 0){
		// if (MLCptr->config.HotBlockCounter < 0) printf("++++ HotBlockCounter = %d\n", MLCptr->config.HotBlockCounter);
		DWORD i=0, release=0, counter=0;
		for (i = 0; i<MLCptr->config.PsizeBlock; i++){	
			if (MLCptr->blocks[i].is_hot == NOT_THAT_HOT_BLOCK){			//not that hot -> non hot
				release++;													//release a new space for hot blocks
				counter++;
				MLCptr->blocks[i].is_hot = NON_HOT_BLOCK;

			}else if(MLCptr->blocks[i].is_hot == HOT_BLOCK){				//hot -> not that hot
				counter++;
				MLCptr->blocks[i].is_hot = NOT_THAT_HOT_BLOCK;

			}else if(MLCptr->blocks[i].is_hot == NON_HOT_BLOCK){			//just for debug
				
			}else{
				printf("in MLCwriteSector(), wrong MLCptr->blocks[i].is_hot value\n");
				exit(0);
			}
		}
		MLCptr->config.HotBlockCounter += release;
		// printf("	counter = %lld, release = %lld\n", counter, release);
		assert(counter == TOTAL_HOT_BLOCK || counter == TOTAL_HOT_BLOCK+1);
		assert(MLCptr->config.HotBlockCounter >= 0);
		// getchar();
	} 

	if(MLCptr->blocks[invBlock].is_hot == NON_HOT_BLOCK){
		MLCptr->blocks[invBlock].is_hot = HOT_BLOCK;
		MLCptr->config.HotBlockCounter--;

	}else if (MLCptr->blocks[invBlock].is_hot ==NOT_THAT_HOT_BLOCK){
		// 不需要做 HotBlockCounter--; 所以我也沒有檢查 HotBlockCounter==0 的必要
		MLCptr->blocks[invBlock].is_hot = HOT_BLOCK;
	}

	//debug code
	// if (MLCptr->config.HotBlockCounter <0 ){
		// DWORD i, hot = 0, middle = 0;
		// for (i=0; i<MLCptr->config.PsizeBlock; i++){
		// 	if (MLCptr->blocks[i].is_hot == HOT_BLOCK){
		// 		hot++;
		// 	}else if (MLCptr->blocks[i].is_hot == NOT_THAT_HOT_BLOCK){
		// 		middle++;
		// 	}
		// }
		// printf("----hot = %lld, middle = %lld, MLCptr->config.HotBlockCounter = %d \n", hot, middle, MLCptr->config.HotBlockCounter );
	// }
	// getchar();
	assert(MLCptr->config.HotBlockCounter >= -1);


	//依 GCflag 把該 block 從 unRefList 或 GClist[舊的 validPages] 重新掛到新的 GClist 桶（因為 valid page 數改變）。
	// adjust the invalidated block to corresponding LRU list
	if(MLCptr->blocks[invBlock].GCflag == 1){		
		MLCremoveList(MLCptr, &MLCptr->GClist[MLCptr->blocks[invBlock].validPages], invBlock, invBlockPred);
	}
	// mark this physical block as accessed (from flag 3 to flag 1), and move this block to corresponding LRI list
	else if(MLCptr->blocks[invBlock].GCflag == 3){
		MLCremoveList(MLCptr, &MLCptr->unRefList, invBlock, invBlockPred);	
		MLCptr->blocks[invBlock].GCflag = 1;
	}

	MLCptr->blocks[invBlock].validPages--;								// 因為 invPage 被 invalidate 所以 invBlock 中的合法 page 數量 --
	// 因為 valid page 數量改變，所以如果這個 block 在 GClist 裡面(GCflag == 1)，就要移動到正確的 list 去 
	// (前面已經從 list[validpage] 中移除了，所以這邊只要加進去 list[validpage-1] 即可)
	if(MLCptr->blocks[invBlock].GCflag == 1){							
		MLCinsertList(MLCptr, &MLCptr->GClist[MLCptr->blocks[invBlock].validPages], invBlock);
	}



	//valid page 為 0 且 flag 為 1 代表此 block 可回收，就從 GCList 里移除並放回 Sparelist
	//empty first (invalid candidate : GC block, )
	if(MLCptr->blocks[invBlock].validPages == 0 && MLCptr->blocks[invBlock].GCflag == 1){
		
		//從對應的 GClist 中刪除 block。因為這邊跟 stable greedy 一樣把 block 按照 valid page 的數量下去分類
		//所以會是第 "MLCptr->blocks[invBlock].validPages" 個 GClist
		invBlockPred = MLCptr->predTable[invBlock];
		MLCremoveList(MLCptr, &MLCptr->GClist[MLCptr->blocks[invBlock].validPages], invBlock, invBlockPred);
		assert(MLCptr->repTable[invBlock] == MLC_LOGBLOCK_NULL && MLCptr->predTable[invBlock] == MLC_LOGBLOCK_NULL); 

		
		//把這個空的 block 放回到 "spare block" 的 list，然後更新數據
		MLCptr->blocks[invBlock].GCflag = 0;
		MLCinsertList(MLCptr, &MLCptr->Sparelist, invBlock);
		MLCptr->regionBlockCounter[MLCptr->blocks[invBlock].regionNo]--;
		MLCptr->stat.regGCBlockCount[MLCptr->blocks[invBlock].regionNo]++;
		MLCptr->stat.blockErase++;
		MLCptr->blocks[invBlock].ECs++;
	}


	//4. ========================== emulate writing (processing the new page)  ============================
	//把新資料寫進 current log block 的下一頁，更新 mapTable[LPA] 指向新 (block,page)
	MLCptr->blocks[logBlock].pages[pageIndex].sector	= offsetSector;
	MLCptr->blocks[logBlock].pages[pageIndex].valid		= 1;			//原本一定是 valid == 0
	MLCptr->blocks[logBlock].validPages++;
	MLCptr->blocks[logBlock].blockTimeStamp = MLCptr->systemTime;
	MLCptr->mapTable[LPA].pageTimeStamp = MLCptr->systemTime;

	// update statistics
	MLCptr->stat.pageWrite++;
	MLCptr->stat.userPageWrite++;

	// update the map table
	MLCptr->mapTable[LPA].blockNo = logBlock;
	MLCptr->mapTable[LPA].pageNo  = pageIndex;
	MLCptr->mapTable[LPA].regionNo = wrt_regNo;

	// update the regionTable
	pageIndex++;
	MLCptr->regionTable[wrt_regNo].activePageNo++;

	// if this logBlock is full, link it to the GClist
	// 為甚麼可以保證所有的 MLCptr->regionTable[wrt_regNo].activePageNo 都是屬於同一個 logBlock ?? -> 我寫在notion了
	if(MLCptr->regionTable[wrt_regNo].activePageNo == MLCptr->config.blockSizePage){		
		assert(MLCptr->blocks[logBlock].GCflag == 0);					//為甚麼GCflag會是 0 -> 寫在 notion 了
		MLCptr->blocks[logBlock].GCflag = 1;

		assert(MLCptr->repTable[MLCptr->regionTable[wrt_regNo].activeBlockNo] == MLC_LOGBLOCK_NULL 
			&& MLCptr->predTable[MLCptr->regionTable[wrt_regNo].activeBlockNo] == MLC_LOGBLOCK_NULL);
		MLCinsertList(MLCptr, &MLCptr->GClist[MLCptr->blocks[logBlock].validPages], logBlock);

		// clear the log block for this region，下次進到 MLCwriteSector() 裡面時才會重新選一個 logBlock
		MLCptr->regionTable[wrt_regNo].activeBlockNo = MLC_LOGBLOCK_NULL;
		MLCptr->regionTable[wrt_regNo].activePageNo  = MLC_PAGE_FREE;
	}



	return;
}