#include "simu.h"
#include "MLC.h"


//greedy : choose the block with most invalid pages
DWORD MLCgetGCvictimGreedy(MLC *MLCptr){
	DWORD i, Top;

	Top = MLCptr->config.blockSizePage;
	for(i = 0; i <= MLCptr->config.blockSizePage; i++){
		if(MLCptr->GClist[i].Count != 0){
			Top = i;
			break;
		}
	}

	return MLCptr->GClist[Top].Head;
}

//我跟宥毅跟登豪提出的方法，先在 simulator 這邊實作
//use our method, but not just greedy, to choose victim
//lookup from the block with most invalid pages, but this block also need not to be a hot block
DWORD MLCgetGCvictimOurMethod(MLC *MLCptr){
	DWORD i, j, CurBlock, victim = NULL, victim_VC = MLCptr->config.blockSizePage+1;//victim_VC 先設為 int max 的感覺

	for(i = 0; i <= MLCptr->config.blockSizePage; i++){
		if(MLCptr->GClist[i].Count == 0){
			continue;
		}
		CurBlock = MLCptr->GClist[i].Head;
		for (j = 1; j<MLCptr->GClist[i].Count; j++){
			if (MLCptr->blocks[CurBlock].validPages <= VAILD_PAGE_THRESHOLD){
				// 一個避免過度計算的停損點，不管 hot nonhot，只要 invalid data 夠多我就直接選
				victim = CurBlock;
				break;
			}else if(MLCptr->blocks[CurBlock].is_hot == NON_HOT_BLOCK && MLCptr->blocks[CurBlock].validPages < victim_VC){
				//否則就選擇 non hot block 中 VC 最小的
				victim = CurBlock;
				victim_VC = MLCptr->blocks[CurBlock].validPages;
			}
			CurBlock = MLCptr->repTable[CurBlock];
		}
		if (victim != NULL){
			break;
		}
	}
	// if (victim == NULL){
	// 	printf("\nMLCptr->config.blockSizePage = %lld\n", MLCptr->config.blockSizePage);
	// 	for (i = 0; i<=MLCptr->config.blockSizePage; i++){
	// 		CurBlock = MLCptr->GClist[i].Head;
	// 		for (j=1; j<MLCptr->GClist[i].Count; j++){
	// 			printf("block vaild page = %lld, is_hot = %lld\n", MLCptr->blocks[CurBlock].validPages, (MLCptr->blocks[CurBlock].is_hot));
	// 		}
	// 	}
	// }
	// assert(victim != NULL);
	return victim;

}
