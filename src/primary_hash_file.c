/* ===== PRIMARY INDEX EXTENDIBLE HASHING ===== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "primary_hash_file.h"

int NoOfDirsInBlock = (BF_BLOCK_SIZE - sizeof(int)) / sizeof(directory);
int NoOfRecInBucket = 3;  // For full use of memory set NoOfRecInBucket = (BF_BLOCK_SIZE - sizeof(bucketBlock)) / sizeof(Record*); 
int K = 6;                // separates buffer blocks into Directory Blocks and Buckets (Data Blocks)

HT_ErrorCode findRecord(int indexDesc, int tupleID, Record *record) {
    BF_Block *idBlock;
    BF_Block_Init(&idBlock);
    int block_num = K + 1 + (tupleID / NoOfRecInBucket); 
    CALL_BF(BF_GetBlock(indexDesc, block_num, idBlock));
    char *blockData = BF_Block_GetData(idBlock);
    CALL_BF(BF_UnpinBlock(idBlock));
    bucketBlock BucketBlock;
    memcpy(&BucketBlock, blockData, sizeof(bucketBlock));
    memcpy(record, BucketBlock.records[tupleID % NoOfRecInBucket], sizeof(Record));
    return HT_OK;
}

HT_ErrorCode HT_Init() {
    for(int i = 0; i < MAX_OPEN_FILES; i++) {
        allFiles[i].hasFile = false;
        allFiles[i].fileDesc = -1;
        allFiles[i].fileName = NULL;
    }
    return HT_OK;
}

HT_ErrorCode HT_CreateIndex(const char *filename) {
    CALL_BF(BF_CreateFile(filename));  // Create a file with name filename.
    int fileDesc; 
    CALL_BF(BF_OpenFile(filename, &fileDesc));   // Open the file with name filename.

    BF_Block *infoBlock;
    BF_Block_Init(&infoBlock);
  
    // allocate memory for the first block which will be the info block
    CALL_BF(BF_AllocateBlock(fileDesc, infoBlock));  
    char *data;
    
    data = BF_Block_GetData(infoBlock); // take the data of the first block (initially empty).
    int GlobalDepth = 1;
    memcpy(data, &GlobalDepth, sizeof(int)); // write the GlobalDepth to data
    
    BF_Block_SetDirty(infoBlock); // because we changed the (initially empty) data of the first block
    CALL_BF(BF_UnpinBlock(infoBlock));   // unpin the block because we don't want it anymore

    // ===== ALLOCATE K DIRECTORY BLOCKS ====== //
    for (int i = 1; i <= K; i++) {
        BF_Block *block;
        BF_Block_Init(&block);
        CALL_BF(BF_AllocateBlock(fileDesc, block));
        if (i == 1) { // initially we 'll have just two directories only in the first Directory Block
            data = BF_Block_GetData(block);
            directoryBlock dirBlock;
            directory* directories = malloc(sizeof(directory) * NoOfDirsInBlock);
            dirBlock.numOfDir = 2;
            dirBlock.directories = directories;
            for (int j = 0; j < 2; j++) {
                dirBlock.directories[j].hashId = j; // hashID gia to kathe directory
                dirBlock.directories[j].PosOfBucketBlock = K + j + 1;
            }
            memcpy(data, &dirBlock, sizeof(directoryBlock));
            BF_Block_SetDirty(block); // Because we changed the data of the first block.
        }
        CALL_BF(BF_UnpinBlock(block));
    }

    // ===== ALLOCATE 2 BUCKET BLOCKS (INITIALLY) ====== //
    for (int i = K+1; i <= K+2; i++) {
        BF_Block* bucket_block;
        BF_Block_Init(&bucket_block);
        CALL_BF(BF_AllocateBlock(fileDesc, bucket_block));
        bucketBlock* bucket = malloc(sizeof(bucketBlock));
        bucket->records = malloc(NoOfRecInBucket * sizeof(Record*));
        bucket->localDepth = 1;
        bucket->cntOfRecords = 0; // initially there are 0 records in it

        char *data = BF_Block_GetData(bucket_block);
        memcpy(data, bucket, sizeof(bucketBlock));
        
        BF_Block_SetDirty(bucket_block); 
        CALL_BF(BF_UnpinBlock(bucket_block)); 
    }

    CALL_BF(BF_CloseFile(fileDesc));

    return HT_OK;
}

HT_ErrorCode HT_OpenIndex(const char *fileName, int *indexDesc) {
    CALL_BF(BF_OpenFile(fileName, indexDesc));
    int pos_allFiles = next_file_idx();
    allFiles[pos_allFiles].hasFile = true;
    allFiles[pos_allFiles].fileDesc = *indexDesc;
    allFiles[pos_allFiles].fileName = malloc((strlen(fileName)) * sizeof(char));
    strcpy(allFiles[pos_allFiles].fileName, fileName);
    printf("\nFile with descriptor %d has been opened.\n", *indexDesc);
    return HT_OK;
}

HT_ErrorCode HT_CloseFile(int indexDesc) {
    char *filename;
    int fileIdx = -1;

    for (int i = 0; i < MAX_OPEN_FILES; ++i) 
        if (allFiles[i].fileDesc == indexDesc) {
            fileIdx = i;
            break;
        }

    if (fileIdx == -1) 
        return HT_ERROR;
  
    allFiles[fileIdx].fileName = NULL;
    allFiles[fileIdx].fileDesc = -1;
    allFiles[fileIdx].hasFile = false;

    CALL_BF(BF_CloseFile(indexDesc));
    printf("\nFile with descriptor %d has been closed.\n\n", indexDesc);

    return HT_OK;
}

int findDirBlockNum(int hash) { return hash / NoOfDirsInBlock + 1; }

HT_ErrorCode HT_InsertEntry(int indexDesc, Record record, int * tupleId, UpdateRecordArray *UpdateArray) {
    // ===== GET THE INFO-BLOCK ===== //
    BF_Block *infoBlock;
    BF_Block_Init(&infoBlock);
    CALL_BF(BF_GetBlock(indexDesc, 0, infoBlock));
    char *infoBlockData = BF_Block_GetData(infoBlock);
    int globalDepth; 
    memcpy(&globalDepth, infoBlockData, sizeof(int));
    // ===== END OF INFO-BLOCK ===== //

    // find in which directory belongs the given record
    int hash = record.id % (1 << globalDepth);
    int numOfDirBlock = findDirBlockNum(hash);
    BF_Block *blockOfDirBlock;
    BF_Block_Init(&blockOfDirBlock);

    // get the block with the directory we want
    CALL_BF(BF_GetBlock(indexDesc, numOfDirBlock, blockOfDirBlock));
    // this is a directory block, so its data is a directory block struct
    char *data = BF_Block_GetData(blockOfDirBlock);

    // get that directory block's data into our variable dirBlock
    directoryBlock dirBlock; 
    memcpy(&dirBlock, data, sizeof(directoryBlock));

    // find the position of the wanted directory in the list of directories of the directory block
    int CurrentDirIdx = hash - dirBlock.directories[0].hashId;

    // find the position of block containing the wanted bucket to pace this record
    int PosOfBucketBlock = dirBlock.directories[CurrentDirIdx].PosOfBucketBlock;

    CALL_BF(BF_UnpinBlock(blockOfDirBlock));
    
    BF_Block *BlockOfBucketBlock;
    BF_Block_Init(&BlockOfBucketBlock);
    CALL_BF(BF_GetBlock(indexDesc, PosOfBucketBlock, BlockOfBucketBlock));
    char *BucketBlockdata = BF_Block_GetData(BlockOfBucketBlock);
  
    bucketBlock BucketBlock;
    memcpy(&BucketBlock, BucketBlockdata, sizeof(bucketBlock));
    int localDepth = BucketBlock.localDepth;
    
    // ========= THE EXTENDIBLE HASHING ALGORITHM ========= //
    if(BucketBlock.cntOfRecords < NoOfRecInBucket) {
        Record *newrecord = malloc(sizeof(Record));
        memcpy(newrecord, &record, sizeof(Record));
        BucketBlock.records[BucketBlock.cntOfRecords++] = newrecord;
        *tupleId = ((PosOfBucketBlock - K - 1) * NoOfRecInBucket) + (BucketBlock.cntOfRecords - 1); 
        memcpy(BucketBlockdata, &BucketBlock, sizeof(bucketBlock));
        BF_Block_SetDirty(BlockOfBucketBlock);
        CALL_BF(BF_UnpinBlock(BlockOfBucketBlock));
    }
    else { // OVERFLOW
        if (localDepth == globalDepth) { // BUCKET SPLIT AND DIRECTORY EXPANSION //
            // ==== DIRECTORY EXPANSION ==== //
            int NoOfDirs = (1 << globalDepth); // how many directories we have as now
            int lastDirBlockPos = NoOfDirs / NoOfDirsInBlock + 1; // last directory block
            for(int d = 0; d < NoOfDirs;) {
                directory newDirectory;
                newDirectory.hashId = NoOfDirs + d;
                int oldHash = newDirectory.hashId % (1 << globalDepth); // find hash using the d-1 bits (assume now we use d bits after expansion)
                int numOfDirBlock = findDirBlockNum(oldHash);
                BF_Block *blockOfDirBlock;
                BF_Block_Init(&blockOfDirBlock);
                CALL_BF(BF_GetBlock(indexDesc, numOfDirBlock, blockOfDirBlock));
                data = BF_Block_GetData(blockOfDirBlock);
                CALL_BF(BF_UnpinBlock(blockOfDirBlock));
                directoryBlock dirBlockOfOldHash; 
                memcpy(&dirBlockOfOldHash, data, sizeof(directoryBlock));
                int CurrentDirIdx = oldHash - dirBlockOfOldHash.directories[0].hashId; // find in which position of dir array is the directory
                newDirectory.PosOfBucketBlock = dirBlock.directories[CurrentDirIdx].PosOfBucketBlock; // POINTS TO THE SAME BUCKET AS THE OLD HASH
                
                BF_Block *lastDirBlock;
                BF_Block_Init(&lastDirBlock);  
                CALL_BF(BF_GetBlock(indexDesc, lastDirBlockPos, lastDirBlock));
                directoryBlock curDirBlock;
                char *data = BF_Block_GetData(lastDirBlock);
                memcpy(&curDirBlock, data, sizeof(directoryBlock));

                if(curDirBlock.numOfDir < NoOfDirsInBlock) { // if directory fits in current dirBlock, then place it
                    curDirBlock.directories[curDirBlock.numOfDir].hashId = newDirectory.hashId;
                    curDirBlock.directories[curDirBlock.numOfDir++].PosOfBucketBlock = newDirectory.PosOfBucketBlock;
                    memcpy(data, &curDirBlock, sizeof(directoryBlock));
                    BF_Block_SetDirty(lastDirBlock);
                    d++; // directory written in memory, go to the next one
                } else {   
                    lastDirBlockPos++;
                    if (lastDirBlockPos == K+1) { 
                        perror("\nMemory limit reached.\n");
                        return HT_ERROR;  
                    }
                    BF_Block *newBlock;
                    BF_Block_Init(&newBlock);
                    CALL_BF(BF_GetBlock(indexDesc, lastDirBlockPos, newBlock));    
                    char *newBlockData = BF_Block_GetData(newBlock);
                    //if enough space for them
                    directoryBlock newDirBlock;
                    directory* newDirectories = malloc(sizeof(directory) * NoOfDirsInBlock);
                    newDirBlock.numOfDir = 0;
                    newDirBlock.directories = newDirectories;            
                    memcpy(newBlockData, &newDirBlock, sizeof(directoryBlock));
                    BF_Block_SetDirty(newBlock);
                    CALL_BF(BF_UnpinBlock(newBlock));
                }
                CALL_BF(BF_UnpinBlock(lastDirBlock));
            }
            globalDepth++;
            memcpy(infoBlockData, &globalDepth, sizeof(int));
            BF_Block_SetDirty(infoBlock);
            CALL_BF(BF_UnpinBlock(infoBlock));
            CALL_BF(HT_InsertEntry(indexDesc, record, tupleId, UpdateArray)); // go to case (a)
        }
        else { // LOCAL DEPTH < GLOBAL DEPTH -- Just BUCKET SPLIT
            // CREATE THE NEW BLOCK FOR BUCKET BLOCK
            BF_Block *newBucket;
            BF_Block_Init(&newBucket);
            CALL_BF(BF_AllocateBlock(indexDesc, newBucket));
            bucketBlock newBucketBlock;
            newBucketBlock.records = malloc(NoOfRecInBucket * sizeof(Record*));
            newBucketBlock.localDepth = ++BucketBlock.localDepth;
            newBucketBlock.cntOfRecords = 0; // initially there are 0 records in it
            // END OF BLOCK FOR BUCKET BLOCK

            // ===== CHANGE THE POS OF PROBLEMATIC DIR ===== //
            int oldHash = record.id % (1 << (globalDepth - 1));
            int hashOfProblematicDir = oldHash + (1 << (globalDepth - 1));
            int posOfProblematicDirBlock = findDirBlockNum(hashOfProblematicDir);
            BF_Block *blockOfProblematicDirBlock;
            BF_Block_Init(&blockOfProblematicDirBlock);
            CALL_BF(BF_GetBlock(indexDesc, posOfProblematicDirBlock, blockOfProblematicDirBlock));
            data = BF_Block_GetData(blockOfProblematicDirBlock);
            directoryBlock problematicDirBlock; 
            memcpy(&problematicDirBlock, data, sizeof(directoryBlock));
            int problematicDirIdx = hashOfProblematicDir - problematicDirBlock.directories[0].hashId; // find in which position of dir array is the directory
            int blockCounter;
            CALL_BF(BF_GetBlockCounter(indexDesc, &blockCounter));
            problematicDirBlock.directories[problematicDirIdx].PosOfBucketBlock = blockCounter - 1;
            CALL_BF(BF_UnpinBlock(blockOfProblematicDirBlock));
            // ===== END OF PROBLEMATIC DIR ===== //
            // ===== REHASH OF RECORDS OF OVERFLOWED BUCKET ===== //
            int cnt = 0;
            int cnt_ua = 0;
            for(int i = 0; i < BucketBlock.cntOfRecords; ++i) {
                Record *record = BucketBlock.records[i];
                int cur_tupleid = (PosOfBucketBlock - K - 1) * NoOfRecInBucket + i;
                int oldHash = record->id % (1 << (BucketBlock.localDepth - 1));
                int newHash = record->id % (1 << (BucketBlock.localDepth));
                if(newHash == oldHash) {
                    BucketBlock.records[cnt++] = record;
                    int new_tupleid = (PosOfBucketBlock - K - 1) * NoOfRecInBucket + cnt - 1;
                    if(cur_tupleid != new_tupleid) {
                        UpdateArray[cnt_ua].oldTupleId = cur_tupleid;
                        UpdateArray[cnt_ua].newTupleId = new_tupleid;
                        strcpy(UpdateArray[cnt_ua++].indexKey, record->surname);
                    }
                }
                else {
                    newBucketBlock.records[newBucketBlock.cntOfRecords++] = record;
                    int new_tupleid = (blockCounter - K - 2) * NoOfRecInBucket + newBucketBlock.cntOfRecords - 1;
                    if(cur_tupleid != new_tupleid) {
                        UpdateArray[cnt_ua].oldTupleId = cur_tupleid;
                        UpdateArray[cnt_ua].newTupleId = new_tupleid;
                        strcpy(UpdateArray[cnt_ua++].indexKey, record->surname);
                    }
                }
            } 
            // ===== FINALLY INSERT THE NEW RECORD ===== //
            Record *newrecord = malloc(sizeof(Record));
            memcpy(newrecord, &record, sizeof(Record));
            int nHash = record.id % (1 << (BucketBlock.localDepth));
            int oHash = record.id % (1 << (BucketBlock.localDepth-1));
            if(oHash == nHash) {
                BucketBlock.records[cnt++] = newrecord;
                BucketBlock.cntOfRecords = cnt;
                *tupleId = ((PosOfBucketBlock - K - 1) * NoOfRecInBucket) + (BucketBlock.cntOfRecords - 1);
            }
            else {
                newBucketBlock.records[newBucketBlock.cntOfRecords++] = newrecord;
                *tupleId = ((blockCounter - K - 2) * NoOfRecInBucket) + (newBucketBlock.cntOfRecords-1);
            }

            BucketBlock.cntOfRecords = cnt;
            data = BF_Block_GetData(newBucket);
            memcpy(data, &newBucketBlock, sizeof(bucketBlock));
            BF_Block_SetDirty(newBucket);
            CALL_BF(BF_UnpinBlock(newBucket));
            memcpy(BucketBlockdata, &BucketBlock, sizeof(bucketBlock));
            BF_Block_SetDirty(BlockOfBucketBlock);
            CALL_BF(BF_UnpinBlock(BlockOfBucketBlock)); 
            // ===== END OF INSERTING THE NEW RECORD ===== //
        } 
    }
    CALL_BF(BF_UnpinBlock(infoBlock));
    return HT_OK;
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id) {
    printf("\n=========================== P R I M A R Y  I N D E X  P R I N T ===========================\n");
    if (id == NULL) {
        printf("\nPrinting all entries on file with descriptor = %d\n\n", indexDesc);
        
        for (int i = 0; i <= K; i++) {
            BF_Block *block;
            BF_Block_Init(&block);
            CALL_BF(BF_GetBlock(indexDesc, i, block));
            char *data = BF_Block_GetData(block); 
            CALL_BF(BF_UnpinBlock(block));
            if (i == 0) { // INFO BLOCK
                int globalDepth;
                memcpy(&globalDepth, data, sizeof(int));
            }
            else if (i >= 1 && i <= K) { // DIRECTORY BLOCKS
                directoryBlock currentDirBlock;
                memcpy(&currentDirBlock, data, sizeof(directoryBlock));
                printf("Directory Block[%d] has %d directories\n", i - 1, currentDirBlock.numOfDir);
                for (int d = 0; d < currentDirBlock.numOfDir; d++) { // iterate through directories list'
                    printf(" - Directory [%d] has Hash ID: %d and PosOfBucketBlock: %d\n", d, 
                                currentDirBlock.directories[d].hashId, currentDirBlock.directories[d].PosOfBucketBlock);
                    BF_Block* bucketOfDir;
                    BF_Block_Init(&bucketOfDir);
                    CALL_BF(BF_GetBlock(indexDesc, currentDirBlock.directories[d].PosOfBucketBlock, bucketOfDir));
                    char *BucketData = BF_Block_GetData(bucketOfDir); 
                    CALL_BF(BF_UnpinBlock(bucketOfDir));
                    
                    bucketBlock currentBucketBlock;
                    memcpy(&currentBucketBlock, BucketData, sizeof(bucketBlock));
                    for (int r = 0; r < currentBucketBlock.cntOfRecords; r++) {
                        Record * currentRecord;
                        currentRecord = currentBucketBlock.records[r];
                        printf("   -> Record # %d:\n", r);
                        printf("      ID: %-3d|  Name: %-12s|  Surname: %-16s|  City: %-10s\n\n", currentRecord->id, currentRecord->name, currentRecord->surname, currentRecord->city);
                    }
                    printf("     ------------------------------------------------------------------------------------------\n\n");
                }
            }
        }
    } 
    else {
        printf("\nSearching for record with ID = %d on file with descriptor = '%d'\n\n", *id, indexDesc);
        // find the record with specified ID
        BF_Block *infoBlock;
        BF_Block_Init(&infoBlock);
        CALL_BF(BF_GetBlock(indexDesc, 0, infoBlock));
        char *infoBlockData = BF_Block_GetData(infoBlock);
        CALL_BF(BF_UnpinBlock(infoBlock));
        int globalDepth; 
        memcpy(&globalDepth, infoBlockData, sizeof(int));

        int hash = (*id) % (1 << globalDepth);
        int dir = findDirBlockNum(hash);

        BF_Block *dirBlock;
        BF_Block_Init(&dirBlock);
        CALL_BF(BF_GetBlock(indexDesc, dir, dirBlock));
        char *BucketData = BF_Block_GetData(dirBlock); 
        CALL_BF(BF_UnpinBlock(dirBlock));
        directoryBlock dirB;
        memcpy(&dirB, BucketData, sizeof(directoryBlock));

        int dirIdx = hash - dirB.directories[0].hashId; // find in which position of dir array is the fckin directory
        int posBucket = dirB.directories[dirIdx].PosOfBucketBlock;
        
        BF_Block *bucketOfDir;
        BF_Block_Init(&bucketOfDir);
        CALL_BF(BF_GetBlock(indexDesc, posBucket, bucketOfDir));
        BucketData = BF_Block_GetData(bucketOfDir); 
        CALL_BF(BF_UnpinBlock(bucketOfDir));
        bucketBlock currentBucketBlock;
        memcpy(&currentBucketBlock, BucketData, sizeof(bucketBlock));
        
        bool flag = false;
        for(int i=0; i<currentBucketBlock.cntOfRecords; ++i){
            Record *record = currentBucketBlock.records[i];
            if(record->id == *id){
                if (flag) printf("     ------------------------------------------------------------------------------------------\n\n");
                printf("   -> Record # %d:\n", i);
                printf("      ID: %-3d|  Name: %-12s|  Surname: %-16s|  City: %-10s\n\n", record->id, record->name, record->surname, record->city);
                flag = true;
            }
        }
        if(!flag) printf("The record with ID = %d could not be found.\n", *id);
    } 
    return HT_OK;
}

HT_ErrorCode HT_HashStatistics(char *filename) {
    int fileDesc = -1, idx;
    for(idx = 0; idx < MAX_OPEN_FILES; idx++) {
        if (strcmp(filename, allFiles[idx].fileName) == 0) {
            fileDesc = allFiles[idx].fileDesc;
            break;  
        }
    }
    if (fileDesc == -1) {
        perror("Sorry, your file was not found.\n");
        return HT_ERROR;
    }

    int blockCounter;
    CALL_BF(BF_GetBlockCounter(fileDesc, &blockCounter));
    
    printf("\n\n========== F I L E  S T A T I S T I C S ==========\n\n");

    printf("File name: '%s'\n", filename);
    printf("File descriptor: %d\n", fileDesc);
    printf("Number of allocated blocks in file: %d\n", blockCounter);
    
    int totalRecords = 0;
    int maxRecords = 0;
    int minRecords = NoOfRecInBucket;

    for (int i = K + 1; i < blockCounter; i++) {
        BF_Block *BucketBlock;
        BF_Block_Init(&BucketBlock);
        CALL_BF(BF_GetBlock(fileDesc, i, BucketBlock));
        char *bucketData = BF_Block_GetData(BucketBlock); 
        bucketBlock currentBucket;
        memcpy(&currentBucket, bucketData, sizeof(bucketBlock));   
        CALL_BF(BF_UnpinBlock(BucketBlock));
        printf("\n====== B U C K E T [%d]  S T A T I S T I C S ======\n", i-(K+1));
        printf("* Local Depth: %d\n", currentBucket.localDepth);
        printf("* Number Of Records: %d\n", currentBucket.cntOfRecords);
        totalRecords += currentBucket.cntOfRecords;
        maxRecords = max(maxRecords, currentBucket.cntOfRecords);
        minRecords = min(minRecords, currentBucket.cntOfRecords);
    }
    double avgRecords = (double)totalRecords / (double)(blockCounter - (K + 1));
    printf("\n\n* Minimum number of records in buckets: %.d\n", minRecords);
    printf("* Maximum number of records in buckets: %.d\n", maxRecords);
    printf("* Average number of records in buckets: %.1f\n", avgRecords);
    return HT_OK;
}
