#include "simu.h"
#include "MLC.h"


void MLCinsertList(MLC *MLCptr, MLClist *list, DWORD block){

	// MLCinit 裡面有用來把 block i (in flash) 塞進 unRefList 裡面， MLCinsertList(MLCptr, &MLCptr->unRefList, i);
	// MLCwriteSector 裡面也有用到

	assert(list->Count >= 0);

	// list is empty
	if(list->Head == MLC_LOGBLOCK_NULL && list->Tail == MLC_LOGBLOCK_NULL){
		list->Head = list->Tail = block;
		MLCptr->predTable[list->Head] = MLCptr->repTable[list->Head] = MLC_LOGBLOCK_NULL;
		list->Count++;
		assert(list->Count == 1);
	}else {
		MLCptr->repTable[list->Tail] = block;
		MLCptr->predTable[block]     = list->Tail;
		list->Tail = block;
		list->Count++;
	}

	if(list->Count >= 1){
		if(MLCptr->predTable[list->Head] != 0xffffffff){
			printf("ins:non empty head(%d) pred\n", list->Head);
			system("pause");
		}
		if(MLCptr->repTable[list->Tail] != 0xffffffff){
			printf("ins:non empty tail(%d) suc\n", list->Tail); 
			system("pause");
		}
		assert(MLCptr->predTable[list->Head] == 0xffffffff);
		assert(MLCptr->repTable[list->Tail] == 0xffffffff);
	}
}

void MLCremoveList(MLC *MLCptr, MLClist *list, DWORD block, DWORD pred){
	//從 list 中移除掉 block
	
	DWORD	oldListHead;

	assert(list->Count >= 0);
	if(pred == MLC_NOT_FOUNT){
		printf("pred not fount, nothing is removed from this list!\n");
		system("pause");
		return;
	}


	if(list->Head == list->Tail == 0xffffffff){
		printf("empty list\n");
	}
	// list 只有一個 element
	else if(list->Head == list->Tail){
		assert(list->Count == 1);
		assert(MLCptr->predTable[list->Head] == 0xffffffff && MLCptr->repTable[list->Tail] == 0xffffffff);

		list->Head = list->Tail = MLC_LOGBLOCK_NULL;
		MLCptr->predTable[block] = MLCptr->repTable[block] = MLC_LOGBLOCK_NULL;		
		list->Count = 0;
	}
	// remove from list head
	else if(pred == MLC_LOGBLOCK_NULL && block == list->Head && list->Count != 1){
		oldListHead = list->Head;
		list->Head = MLCptr->repTable[oldListHead];					//下一個來當新的 head 
		MLCptr->repTable[oldListHead] = MLC_LOGBLOCK_NULL;
		MLCptr->predTable[list->Head] = MLC_LOGBLOCK_NULL;
		list->Count--;
	}
	// remove from list tail
	else if(MLCptr->repTable[block] == MLC_LOGBLOCK_NULL && list->Count != 1){
		list->Tail = pred;
		MLCptr->repTable[list->Tail] = MLC_LOGBLOCK_NULL;
		MLCptr->predTable[block] = MLC_LOGBLOCK_NULL;
		list->Count--;
	}
	// remove from the middle of the list
	else{
		MLCptr->repTable[pred] = MLCptr->repTable[block];
		MLCptr->predTable[MLCptr->repTable[block]] = MLCptr->predTable[block];
		MLCptr->predTable[block] = MLCptr->repTable[block] = MLC_LOGBLOCK_NULL;
		list->Count--;
	}

	if(list->Count >= 1){
		if(MLCptr->predTable[list->Head] != 0xffffffff){
			printf("rmv:non empty head(%d) pred\n",list->Head);
			system("pause");
		}if(MLCptr->repTable[list->Tail] != 0xffffffff){
			printf("rmv:non empty tail(%d) suc\n",list->Tail); 
			system("pause");
		}
		assert(MLCptr->predTable[list->Head] == 0xffffffff);
		assert(MLCptr->repTable[list->Tail] == 0xffffffff);
	}
	return;
}

DWORD MLCfindPred(MLC *MLCptr, MLClist *list, DWORD block){
	DWORD	i, listPtr;
	listPtr = list->Head;

	// empty list
	if(list->Head == list->Tail == MLC_LOGBLOCK_NULL && list->Count == 0){
		printf("empty list, pred is not found!\n");
		system("pause");
		return MLC_NOT_FOUNT;
	}
	// list with 1 member, but the member is not me
	if(list->Head == list->Tail && list->Count == 1 && list->Head != block){
		printf("list only has 1 member, and pred is not fount!\n");
		system("pause");
		return MLC_NOT_FOUNT;
	}


	// I am list head
	if(list->Head == block)return MLC_LOGBLOCK_NULL;

	for(i = 0; i < list->Count; i++){
		if(MLCptr->repTable[listPtr] == block) break;		//跑到 list 底都沒有出去就會是 tail 的下一個，也就是 MLC_LOGBLOCK_NULL
		listPtr = MLCptr->repTable[listPtr];				//往下一個一直跑
	}

	// block is not in this list, return error message
	if(listPtr == MLC_LOGBLOCK_NULL){
		return MLC_NOT_FOUNT;
	}
	
	else return listPtr;
}