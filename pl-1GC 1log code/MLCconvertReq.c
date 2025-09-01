#include "simu.h"
#include "MLC.h"

// write request 是 (offset, len) 形式，MLCconvertReq() 讓他對齊 page
// Convert (align) a write to sectors to a write to pages
//將偏移量和長度從扇區數轉換為頁數，然後再轉換回扇區數，並確保在進行這些轉換時處理了向上取整，
//也就是確保每次寫入都是整數 page 數，才可以降低讀取次數

// if the request is not aligned (starting address or ending address)
// note: we have to prevent the following condition occurs:
// [--00][0000][00--]
// [----][----][--00][0000][00--]
//
// [0000][0000][0000]
// [----][----][0000][0000][0000]
// 
// this causes very strange behaviors, since each sequential &bulk write invalidates *one* AU.
// The correct way to simulate this is:
//
// [--00][0000][00--]
// [----][----][--00][0000][00--]
//
// [0000][0000][----]
// [----][----][0000][0000][----]
// 

void MLCconvertReq(MLC *MLCptr, DWORD *offsetSector, DWORD *lenSector){

	*offsetSector = *offsetSector / (MLCptr->config.pageSizeSector);

	//計算我要寫幾個 page?
	//給定我要寫幾個 sector，如果剛好可以填滿幾個 page 那OK直接轉成 page 數
	//如果會有 page 沒被填滿，也要當作她會被填滿，也就是 page 層級的 align，所以要 +1
	if((*lenSector) % MLCptr->config.pageSizeSector != 0){
		*lenSector = 1 + *lenSector / MLCptr->config.pageSizeSector;
	}
	else{
		*lenSector = *lenSector / MLCptr->config.pageSizeSector;
	} 

	*offsetSector	*= (MLCptr->config.pageSizeSector);
	*lenSector		*= (MLCptr->config.pageSizeSector);

}