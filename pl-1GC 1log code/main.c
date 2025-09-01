#include "simu.h"
#include "MLC.h"

MLC MLCflash;
DWORD	totalReq, writeReq;



int main(int argc,char *argv[]){

	//====================================  1.inititalize MLC，讀入 config file 並呼叫 MLCinit() 設定好 MLC===============================================
	
	BYTE	*config = argv[1];
	FILE	*traceFile, *ResultFile, *configFile;
	BYTE	lineBuffer[1024];
	BYTE	op[100], time[100], ampm[100];
	BYTE	ResultFileName[500], InitFileName[100], ConfigFileName[100];
	DWORD	sector_nr, len, i, r, m, offsetSector, lenSector, j, n, k, listPtr;			

	m = 0;

	// LRU GC
	writeReq = 0;					
	MLCflash.REGION_NUMBER = 1;	 //設定為單一 region
	MLCflash.DEMOTE_THRESHOLD = 0xffffffff;
	MLCflash.PROMOTE_THRESHOLD = 0;	
	errno = 0;

	// config file setting
	sprintf(ConfigFileName, "%s", config);
	// note that data path under windows & linux are different
	sprintf(InitFileName, "../Config File/my_config/%s", ConfigFileName);	//in linux 
	

	printf("Init MLC flash(%s)...\nMLCflash.STABLE_TIME = %d\n", InitFileName, MLCflash.STABLE_TIME);
	configFile = fopen(InitFileName, "rt");
	if ( errno != 0 ){
		perror("Error occurred while opening config file");
		exit(1);
	}
	MLCinit(&MLCflash, configFile);
	printf("Init done.\n");









	//====================================  2.run 6 trace file  ===============================================
	BYTE *trace_list[6];
	trace_list[0] = "rawfile952G_simple_LUN0.txt";
    trace_list[1] = "rawfile952G_simple_LUN1.txt";
    trace_list[2] = "rawfile952G_simple_LUN2.txt";
    trace_list[3] = "rawfile952G_simple_LUN3.txt";
    trace_list[4] = "rawfile952G_simple_LUN4.txt";
    trace_list[5] = "rawfile952G_simple_LUN6.txt";

	for (int traceCount = 0; traceCount <6; traceCount++){
		BYTE *trace = trace_list[traceCount];
		BYTE TraceFileName[100], TraceFilePath[100];
		sprintf(TraceFileName, "%s", trace);
		sprintf(TraceFilePath, "../trace/%s", TraceFileName);	//linux


		printf("------------------------Config file %s, Doing trace file :%s ---------------------------\n",ConfigFileName, TraceFileName);
		traceFile = fopen(TraceFilePath, "rt");
		if ( errno != 0 ){
			perror("Error occurred while opening trace file");
			exit(1);
		}
		fseek(traceFile, 0, SEEK_SET);
		
		for(r = 1; r <= 1; r++){		
			while(!feof(traceFile)){
				sector_nr = 0;
				len = 0;
				fgets(lineBuffer, 1024, traceFile);

				// for IO meter
				sscanf(lineBuffer, "%d %s %d %d",	&i, op, &sector_nr, &len);

				//因為SSD大小是我們自己讀 config file 來的，跟 trace 無關，所以有可能 trace 裡面 access 的範圍超過我們的設定大小，這種情況就直接忽略
				if(sector_nr + len >= MLCflash.config.LsizeSector || len == 0) continue;// out of range

				if(op[0] == 'R') totalReq++; // read operation 不是重點，沒有實做
				if(op[0] == 'W'){
					if(writeReq % 50000 == 0){
						printf("write request = %d\n", writeReq);//100000
						DWORD i, hot = 0, middle = 0;
						for (i=0; i<MLCflash.config.PsizeBlock; i++){
							if (MLCflash.blocks[i].is_hot == HOT_BLOCK){
								hot++;
							}else if (MLCflash.blocks[i].is_hot == NOT_THAT_HOT_BLOCK){
								middle++;
							}
						}
						printf("	hot = %lld, middle = %lld, HotBlockCounter = %d \n", hot, middle, MLCflash.config.HotBlockCounter );
						assert(MLCflash.config.HotBlockCounter >= -1);
					
					}
					offsetSector = sector_nr;
					lenSector = len;
					MLCconvertReq(&MLCflash, &offsetSector, &lenSector);

					while(1){					
						MLCwriteSector(&MLCflash, offsetSector, lenSector);
						offsetSector += MLCflash.config.pageSizeSector;
						lenSector -= MLCflash.config.pageSizeSector;
						if(lenSector <= 0) break;
					}
					totalReq++;
					writeReq++;
					MLCflash.stat.reqCount++;
					MLCflash.systemTime++;
				}
			}
			fseek(traceFile, 0, SEEK_SET);
		}
		fclose(traceFile);

	}
	printf("simulation done\n");


	







	//====================================  3. print log file ===============================================
	printf("-----------------------printing ResultFile----------------------\n");
	sprintf(ResultFileName,"../result/under_greedy+32victim+1000VC+400hot/128G+%s", ConfigFileName);//wsl 
	ResultFile = fopen(ResultFileName, "w+t");
	
	// Over Provisioning 
	printf("0. *Over Provisioning* = %f\n", ((float)MLCflash.config.PsizeBlock - (float)MLCflash.config.LsizeBlock)/(float)MLCflash.config.LsizeBlock);
	fprintf(ResultFile, "*Over Provisioning* = %f\n", ((float)MLCflash.config.PsizeBlock - (float)MLCflash.config.LsizeBlock)/(float)MLCflash.config.LsizeBlock);
	//page write
	printf("1. user page write = %d\n", MLCflash.stat.userPageWrite);
	fprintf(ResultFile, "user page write = %d\n", MLCflash.stat.userPageWrite);
	printf("   total page write = %d\n", MLCflash.stat.pageWrite);
	fprintf(ResultFile, "total page write = %d\n", MLCflash.stat.pageWrite);
	//write amplification
	printf("   *write amplification* = %f\n", (float)MLCflash.stat.pageWrite/(float)MLCflash.stat.userPageWrite);
	fprintf(ResultFile, "*write amplification* = %f\n", (float)MLCflash.stat.pageWrite/(float)MLCflash.stat.userPageWrite);
	//Erase Count
	printf("2. totalEC = %lu\n", MLCflash.stat.blockErase);
	fprintf(ResultFile, "totalEC = %lu\n", MLCflash.stat.blockErase);
	//page Copy
	printf("3. Max page copy = %d\n", MLCflash.stat.maxCopy);
	fprintf(ResultFile, "Max page copy = %d\n", MLCflash.stat.maxCopy);
	printf("   AVG page copy = %f\n", (double)(MLCflash.stat.pageWrite - MLCflash.stat.userPageWrite) / MLCflash.stat.blockErase);
	fprintf(ResultFile, "AVG page copy = %f\n", (double)(MLCflash.stat.pageWrite - MLCflash.stat.userPageWrite) / MLCflash.stat.blockErase);




	fclose(ResultFile);
	MLCfree(&MLCflash);

	// pause(); 		//in wsl
	// system("pause");	//in windows
}