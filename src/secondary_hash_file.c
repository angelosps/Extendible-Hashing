/* ===== SECONDARY INDEX EXTENDIBLE HASHING ===== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "primary_hash_file.h"
#include "secondary_hash_file.h"

int NoOfDirsInBlock_S = (BF_BLOCK_SIZE - sizeof(int)) / sizeof(Sdirectory);
int NoOfRecInBucket_S = 4; // For full use of memory set NoOfRecInBucket_S = (BF_BLOCK_SIZE - sizeof(SbucketBlock)) / sizeof(Secondary_Record*); 
int K_S = 6;               // separates buffer blocks into dirblocks and buckets (datablocks)

HT_ErrorCode SHT_Init() { return HT_OK; }

int find_indexDesc(char *fileName){
    for(int i=0; i<MAX_OPEN_FILES; ++i){
        if(strcmp(allFiles[i].fileName, fileName) == 0)
            return allFiles[i].fileDesc;
    }
    return -1;
}

HT_ErrorCode SHT_CreateSecondaryIndex(const char *sfileName, char *attrName, int attrLength, int depth, char *fileName) {
    if (depth != 1) {
        printf("Sorry, can't create index with initial depth != 1. I am creating index with depth = 1..\n");
    }
    CALL_BF(BF_CreateFile(sfileName));  // Create a file with name filename.
    int fileDesc; 
    CALL_BF(BF_OpenFile(sfileName, &fileDesc));   // Open the file with name filename.
    BF_Block* infoBlock;
    BF_Block_Init(&infoBlock);
    // allocate memory for the first block which will be the info block
    CALL_BF(BF_AllocateBlock(fileDesc, infoBlock));  
    char* data;
    data = BF_Block_GetData(infoBlock); // take the data of the first block (initially empty).
    int GlobalDepth = 1;
    InfoStruct infoStruct;
    infoStruct.attrName = malloc(sizeof(char) * strlen(attrName));
    strcpy(infoStruct.attrName, attrName);
    infoStruct.attrLength = attrLength;
    infoStruct.depth = GlobalDepth;
    infoStruct.fileName = malloc(sizeof(char) * strlen(fileName));
    strcpy(infoStruct.fileName, fileName);
    memcpy(data, &infoStruct, sizeof(InfoStruct)); // write the GlobalDepth to data
    
    BF_Block_SetDirty(infoBlock); // because we changed the (initially empty) data of the first block
    CALL_BF(BF_UnpinBlock(infoBlock));   // unpin the block because we don't want it anymore

    // ===== ALLOCATE K DIRECTORY BLOCKS ====== //
    for (int i = 1; i <= K_S; i++) {
        BF_Block* block;
        BF_Block_Init(&block);
        CALL_BF(BF_AllocateBlock(fileDesc, block));
        if (i == 1) { // initially we 'll have just two directories only in the first Directory Block
        data = BF_Block_GetData(block);
        SdirectoryBlock dirBlock;
        Sdirectory* directories = malloc(sizeof(Sdirectory) * NoOfDirsInBlock_S);
        dirBlock.numOfDir = 2;
        dirBlock.directories = directories;
        for (int j = 0; j < 2; j++) {
            dirBlock.directories[j].hashId = j; // hashID gia to kathe directory
            dirBlock.directories[j].PosOfBucketBlock = K_S + j + 1;
        }
        memcpy(data, &dirBlock, sizeof(SdirectoryBlock));
        BF_Block_SetDirty(block); // Because we changed the data of the first block.
        }
        CALL_BF(BF_UnpinBlock(block));
    }

    // ===== ALLOCATE 2 BUCKET BLOCKS (INITIALLY) ====== //
    for (int i = K_S+1; i <= K_S+2; i++) {
        BF_Block* bucket_block;
        BF_Block_Init(&bucket_block);
        CALL_BF(BF_AllocateBlock(fileDesc, bucket_block));
        SbucketBlock* bucket = malloc(sizeof(SbucketBlock));
        bucket->secondaryRecords = malloc(NoOfRecInBucket_S * sizeof(SecondaryRecord*));
        bucket->localDepth = 1;
        bucket->cntOfRecords = 0; // initially there are 0 records in it

        char *data = BF_Block_GetData(bucket_block);
        memcpy(data, bucket, sizeof(SbucketBlock));
        
        BF_Block_SetDirty(bucket_block); 
        CALL_BF(BF_UnpinBlock(bucket_block)); 
    }

    CALL_BF(BF_CloseFile(fileDesc));
    return HT_OK;
}

HT_ErrorCode SHT_OpenSecondaryIndex(const char *sfileName, int *indexDesc) {
    CALL_BF(BF_OpenFile(sfileName, indexDesc));
    int pos_allFiles = next_file_idx();
    allFiles[pos_allFiles].hasFile = true;
    allFiles[pos_allFiles].fileDesc = *indexDesc;
    allFiles[pos_allFiles].fileName = malloc((strlen(sfileName)) * sizeof(char));
    strcpy(allFiles[pos_allFiles].fileName, sfileName);

    printf("\nFile with descriptor %d has been opened.\n", *indexDesc);
    return HT_OK;
}

HT_ErrorCode SHT_CloseSecondaryIndex(int indexDesc) {
    char * filename;
    int fileIdx = -1;

    for (int i = 0; i < MAX_OPEN_FILES; ++i) 
        if (allFiles[i].fileDesc == indexDesc) {
            fileIdx = i;
            break;
        }

    if (fileIdx == -1) return HT_ERROR;
    
    allFiles[fileIdx].fileName = NULL;
    allFiles[fileIdx].fileDesc = -1;
    allFiles[fileIdx].hasFile = false;

    CALL_BF(BF_CloseFile(indexDesc));
    printf("\nFile with descriptor %d has been closed.\n\n", indexDesc);

    return HT_OK;
}

/* djb2 string hash function */
unsigned long string_hash(unsigned char *str) {
   unsigned long hash = 5381;
   int c;

   while (c = *str++)
       hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

   return hash;
}

int findDirBlockNum_S(int hash) { return hash / NoOfDirsInBlock_S + 1; }

HT_ErrorCode SHT_SecondaryInsertEntry (int indexDesc, SecondaryRecord record) {
    // ===== GET THE INFO-BLOCK ===== //
    // First, get the infoBlock
    BF_Block* infoBlock;
    BF_Block_Init(&infoBlock);
    CALL_BF(BF_GetBlock(indexDesc, 0, infoBlock));
    char *dataI = BF_Block_GetData(infoBlock); 
    CALL_BF(BF_UnpinBlock(infoBlock));
    InfoStruct infoBlockData;
    memcpy(&infoBlockData, dataI, sizeof(infoBlockData));
    int globalDepth = infoBlockData.depth;
    // ===== END OF INFO-BLOCK ===== //

    // find in which directory belongs the given record
    unsigned long id = string_hash(record.index_key);
    int hash = id % (1 << globalDepth); 
    int numOfDirBlock = findDirBlockNum_S(hash);
    BF_Block* blockOfDirBlock;
    BF_Block_Init(&blockOfDirBlock);

    // get the block with the directory we want
    CALL_BF(BF_GetBlock(indexDesc, numOfDirBlock, blockOfDirBlock));
    // this is a directory block, so its data is a directory block struct
    char* data = BF_Block_GetData(blockOfDirBlock);

    // get that directory block's data into our variable dirBlock
    SdirectoryBlock dirBlock; 
    memcpy(&dirBlock, data, sizeof(SdirectoryBlock));

    // find the position of the wanted directory in the list of directories of the directory block
    int CurrentDirIdx = hash - dirBlock.directories[0].hashId;

    // find the position of block containing the wanted bucket to pace this record
    int PosOfBucketBlock = dirBlock.directories[CurrentDirIdx].PosOfBucketBlock;

    CALL_BF(BF_UnpinBlock(blockOfDirBlock));
    
    BF_Block* BlockOfBucketBlock;
    BF_Block_Init(&BlockOfBucketBlock);
    CALL_BF(BF_GetBlock(indexDesc, PosOfBucketBlock, BlockOfBucketBlock));
    char* BucketBlockdata = BF_Block_GetData(BlockOfBucketBlock);
    SbucketBlock BucketBlock;
    memcpy(&BucketBlock, BucketBlockdata, sizeof(SbucketBlock));
    int localDepth = BucketBlock.localDepth;
        
    // ========= THE EXTENDIBLE HASHING ALGORITHM ========= //
    if(BucketBlock.cntOfRecords < NoOfRecInBucket_S) {
        SecondaryRecord* newrecord = malloc(sizeof(SecondaryRecord));
        memcpy(newrecord, &record, sizeof(SecondaryRecord));
        BucketBlock.secondaryRecords[BucketBlock.cntOfRecords++] = newrecord;   
        memcpy(BucketBlockdata, &BucketBlock, sizeof(SbucketBlock));
        BF_Block_SetDirty(BlockOfBucketBlock);
        CALL_BF(BF_UnpinBlock(BlockOfBucketBlock));
    }
    else { // OVERFLOW
        if (localDepth == globalDepth) { // Perform BUCKET SPLIT AND DIRECTORY EXPANSION //
            // ==== DIRECTORY EXPANSION ==== //
            int NoOfDirs = (1 << globalDepth); // how many directories we have as now
            int lastDirBlockPos = NoOfDirs / NoOfDirsInBlock_S + 1; // last directory block
            for(int d = 0; d < NoOfDirs;) {
                Sdirectory newDirectory;
                newDirectory.hashId = NoOfDirs + d;
                int oldHash = newDirectory.hashId % (1 << globalDepth); // find hash using the d-1 bits (assume now we use d bits after expansion)
                int numOfDirBlock = findDirBlockNum_S(oldHash);
                BF_Block* blockOfDirBlock;
                BF_Block_Init(&blockOfDirBlock);
                CALL_BF(BF_GetBlock(indexDesc, numOfDirBlock, blockOfDirBlock));
                data = BF_Block_GetData(blockOfDirBlock);
                CALL_BF(BF_UnpinBlock(blockOfDirBlock));
                SdirectoryBlock dirBlockOfOldHash; 
                memcpy(&dirBlockOfOldHash, data, sizeof(SdirectoryBlock));
                int CurrentDirIdx = oldHash - dirBlockOfOldHash.directories[0].hashId; // find in which position of dir array is the damn directory
                newDirectory.PosOfBucketBlock = dirBlock.directories[CurrentDirIdx].PosOfBucketBlock; // POINTS TO THE SAME BUCKET AS THE OLD HASH
                
                BF_Block* lastDirBlock;
                BF_Block_Init(&lastDirBlock);  
                CALL_BF(BF_GetBlock(indexDesc, lastDirBlockPos, lastDirBlock));
                SdirectoryBlock curDirBlock;
                char *data = BF_Block_GetData(lastDirBlock);
                memcpy(&curDirBlock, data, sizeof(SdirectoryBlock));

                if(curDirBlock.numOfDir < NoOfDirsInBlock_S) { // if directory fits in current dirBlock, then place it
                    curDirBlock.directories[curDirBlock.numOfDir].hashId = newDirectory.hashId;
                    curDirBlock.directories[curDirBlock.numOfDir++].PosOfBucketBlock = newDirectory.PosOfBucketBlock;
                    memcpy(data, &curDirBlock, sizeof(SdirectoryBlock));
                    BF_Block_SetDirty(lastDirBlock);
                    d++; // directory written in memory, go to the next one
                } 
                else {   
                    lastDirBlockPos++;
                    if (lastDirBlockPos == K_S + 1) { 
                    perror("\nMemory limit reached.\n");
                    return HT_ERROR;  
                    }
                    BF_Block* newBlock;
                    BF_Block_Init(&newBlock);
                    CALL_BF(BF_GetBlock(indexDesc, lastDirBlockPos, newBlock));    
                    char * newBlockData = BF_Block_GetData(newBlock);
                    //if enough space for them
                    SdirectoryBlock newDirBlock;
                    Sdirectory* newDirectories = malloc(sizeof(Sdirectory) * NoOfDirsInBlock_S);
                    newDirBlock.numOfDir = 0;
                    newDirBlock.directories = newDirectories;            
                    memcpy(newBlockData, &newDirBlock, sizeof(SdirectoryBlock));
                    BF_Block_SetDirty(newBlock);
                    CALL_BF(BF_UnpinBlock(newBlock));
                }
                CALL_BF(BF_UnpinBlock(lastDirBlock));
            }
        
            infoBlockData.depth++;
            memcpy(dataI, &infoBlockData, sizeof(InfoStruct));

            BF_Block_SetDirty(infoBlock);
            CALL_BF(BF_UnpinBlock(infoBlock));
            CALL_BF(SHT_SecondaryInsertEntry(indexDesc, record)); // go to case (a)
        }
        else { // LOCAL DEPTH < GLOBAL DEPTH -- Perform BUCKET SPLIT || case (a) from jj
            // CREATE THE NEW BLOCK FOR BUCKET BLOCK
            BF_Block* newBucket;
            BF_Block_Init(&newBucket);
            CALL_BF(BF_AllocateBlock(indexDesc, newBucket));
            SbucketBlock newBucketBlock;
            newBucketBlock.secondaryRecords = malloc(NoOfRecInBucket_S * sizeof(SecondaryRecord*));
            newBucketBlock.localDepth = ++BucketBlock.localDepth;
            newBucketBlock.cntOfRecords = 0; // initially there are 0 records in it
            // END OF BLOCK FOR BUCKET BLOCK

            // ===== CHANGE THE POS OF PROBLEMATIC DIR ===== //
            unsigned long int oldHash = string_hash(record.index_key) % (1 << (globalDepth - 1));
            long long int hashOfProblematicDir = oldHash + (1 << (globalDepth - 1));
            int posOfProblematicDirBlock = findDirBlockNum_S(hashOfProblematicDir);

            BF_Block* blockOfProblematicDirBlock;
            BF_Block_Init(&blockOfProblematicDirBlock);
            int num_blocks;
            BF_GetBlockCounter(indexDesc, &num_blocks);
            CALL_BF(BF_GetBlock(indexDesc, posOfProblematicDirBlock, blockOfProblematicDirBlock));
            data = BF_Block_GetData(blockOfProblematicDirBlock);
            SdirectoryBlock problematicDirBlock; 
            memcpy(&problematicDirBlock, data, sizeof(SdirectoryBlock));
            int problematicDirIdx = hashOfProblematicDir - problematicDirBlock.directories[0].hashId; 
            int blockCounter;
            CALL_BF(BF_GetBlockCounter(indexDesc, &blockCounter));
            problematicDirBlock.directories[problematicDirIdx].PosOfBucketBlock = blockCounter - 1;
            CALL_BF(BF_UnpinBlock(blockOfProblematicDirBlock));
            // ===== END OF PROBLEMATIC DIR ===== //

            // ===== REHASH OF RECORDS OF OVERFLOWED BUCKET ===== //
            int cnt = 0;
            int cnt_ua = 0;
            for(int i = 0; i < BucketBlock.cntOfRecords; ++i) {
                SecondaryRecord* record = BucketBlock.secondaryRecords[i];
                unsigned long int oldHash = string_hash(record->index_key) % (1 << (BucketBlock.localDepth - 1));
                unsigned long int newHash = string_hash(record->index_key) % (1 << (BucketBlock.localDepth));
                if(newHash == oldHash){
                    BucketBlock.secondaryRecords[cnt++] = record;
                }
                else {
                    newBucketBlock.secondaryRecords[newBucketBlock.cntOfRecords++] = record;
                }
            } 
            // ===== FINALLY INSERT THE RECORD ===== //
            BF_Block_SetDirty(infoBlock);
            CALL_BF(BF_UnpinBlock(infoBlock));
            BucketBlock.cntOfRecords = cnt;
            data = BF_Block_GetData(newBucket);
            memcpy(data, &newBucketBlock, sizeof(SbucketBlock));
            BF_Block_SetDirty(newBucket);
            CALL_BF(BF_UnpinBlock(newBucket));
            memcpy(BucketBlockdata, &BucketBlock, sizeof(SbucketBlock));
            BF_Block_SetDirty(BlockOfBucketBlock);
            CALL_BF(BF_UnpinBlock(BlockOfBucketBlock)); 
            CALL_BF(SHT_SecondaryInsertEntry(indexDesc, record));
            // ===== END OF INSERTING THE RECORD ===== //
        } 
    }
    CALL_BF(BF_UnpinBlock(infoBlock));
    return HT_OK;
}

HT_ErrorCode update_Record_S(int indexDesc_S, int old_tupleID, int new_tupleID, char *indexKey) {
    // First, get the infoBlock
    BF_Block* infoBlock;
    BF_Block_Init(&infoBlock);
    CALL_BF(BF_GetBlock(indexDesc_S, 0, infoBlock));
    char *data = BF_Block_GetData(infoBlock); 
    CALL_BF(BF_UnpinBlock(infoBlock));
    InfoStruct infoBlockData;
    memcpy(&infoBlockData, data, sizeof(infoBlockData));

    unsigned long int hash = string_hash(indexKey) % (1 << infoBlockData.depth);
    int dir = findDirBlockNum_S(hash);

    BF_Block* dirBlock;
    BF_Block_Init(&dirBlock);
    CALL_BF(BF_GetBlock(indexDesc_S, dir, dirBlock));
    char *BucketData = BF_Block_GetData(dirBlock); 
    CALL_BF(BF_UnpinBlock(dirBlock));
    SdirectoryBlock dirB;
    memcpy(&dirB, BucketData, sizeof(SdirectoryBlock));

    int dirIdx = hash - dirB.directories[0].hashId; 
    int posBucket = dirB.directories[dirIdx].PosOfBucketBlock;
    
    BF_Block* bucketOfDir;
    BF_Block_Init(&bucketOfDir);
    CALL_BF(BF_GetBlock(indexDesc_S, posBucket, bucketOfDir));
    BucketData = BF_Block_GetData(bucketOfDir); 
    SbucketBlock currentBucketBlock;
    memcpy(&currentBucketBlock, BucketData, sizeof(SbucketBlock));
    
    bool flag = false;
    for(int i=0; i<currentBucketBlock.cntOfRecords; ++i){
        if(strcmp(currentBucketBlock.secondaryRecords[i]->index_key, indexKey) == 0 && currentBucketBlock.secondaryRecords[i]->tupleId == old_tupleID){
            currentBucketBlock.secondaryRecords[i]->tupleId = new_tupleID;
            memcpy(BucketData, &currentBucketBlock, sizeof(SbucketBlock));
            BF_Block_SetDirty(bucketOfDir);
            CALL_BF(BF_UnpinBlock(bucketOfDir));
            flag = true;
        }
    }
    if(!flag) {
        printf("The record with ID = %s could not be found.\n", indexKey);
        return HT_ERROR;
    }
    return HT_OK;
}                                                                                 

HT_ErrorCode SHT_SecondaryUpdateEntry (int indexDesc, UpdateRecordArray *updateArray) {
    int i = 0;
    while(updateArray[i].newTupleId != -1) {
        int newTupleId = updateArray[i].newTupleId;
        int oldTupleId = updateArray[i].oldTupleId;
        CALL_BF(update_Record_S(indexDesc, oldTupleId, newTupleId, updateArray[i].indexKey));
        i++;
    }
    return HT_OK;
}

HT_ErrorCode SHT_PrintAllEntries(int indexDesc_S, char *index_key) {
    printf("\n=========================== S E C O N D A R Y  I N D E X  P R I N T ===========================\n");
    // First, get the infoBlock
    BF_Block* infoBlock;
    BF_Block_Init(&infoBlock);
    CALL_BF(BF_GetBlock(indexDesc_S, 0, infoBlock));
    char *data = BF_Block_GetData(infoBlock); 
    CALL_BF(BF_UnpinBlock(infoBlock));
    InfoStruct infoBlockData;
    memcpy(&infoBlockData, data, sizeof(infoBlockData));
    if (index_key == NULL) {
        printf("\nPrinting all entries on file with descriptor = %d\n\n", indexDesc_S);
        for (int i = 1; i <= K_S; i++) { // DIRECTORY BLOCKS
            BF_Block* block;
            BF_Block_Init(&block);
            CALL_BF(BF_GetBlock(indexDesc_S, i, block));
            char *data = BF_Block_GetData(block); 
            CALL_BF(BF_UnpinBlock(block));
            SdirectoryBlock currentDirBlock;
            memcpy(&currentDirBlock, data, sizeof(SdirectoryBlock));
            printf("Directory Block[%d] has %d directories\n", i - 1, currentDirBlock.numOfDir);
            for (int d = 0; d < currentDirBlock.numOfDir; d++) { // iterate through directories list'
                printf(" - Directory [%d] has Hash ID: %d and PosOfBucketBlock: %d\n", d, 
                            currentDirBlock.directories[d].hashId, currentDirBlock.directories[d].PosOfBucketBlock);
                BF_Block* bucketOfDir;
                BF_Block_Init(&bucketOfDir);
                CALL_BF(BF_GetBlock(indexDesc_S, currentDirBlock.directories[d].PosOfBucketBlock, bucketOfDir));
                char *BucketData = BF_Block_GetData(bucketOfDir); 
                CALL_BF(BF_UnpinBlock(bucketOfDir));
                SbucketBlock currentBucketBlock;
                memcpy(&currentBucketBlock, BucketData, sizeof(SbucketBlock));
                for (int r = 0; r < currentBucketBlock.cntOfRecords; r++) {
                    SecondaryRecord * currentRecord;
                    currentRecord = currentBucketBlock.secondaryRecords[r];
                    int tupleID = currentRecord->tupleId;
                    int indexDesc = find_indexDesc(infoBlockData.fileName);
                    Record searched_record;
                    CALL_BF(findRecord(indexDesc, tupleID, &searched_record));
                    printf("   -> Record # %d:\n", r);
                    printf("      Tuple ID: %-3d |  ID: %-3d |  Name: %-15s |  Surname: %-13s |  City: %-15s |  \n\n", 
                            tupleID, searched_record.id, searched_record.name, searched_record.surname, searched_record.city);
                }
                printf("     -------------------------------------------------------------------------------------------------------\n\n");
            }
        }
    } 
    else {
        printf("\nSearching for record with surname = '%s' on file with descriptor = '%d'\n\n", index_key, indexDesc_S);
        // find the record with specifiied ID
        unsigned long int hash = string_hash(index_key) % (1 << infoBlockData.depth);
        int dir = findDirBlockNum_S(hash);

        BF_Block* dirBlock;
        BF_Block_Init(&dirBlock);
        CALL_BF(BF_GetBlock(indexDesc_S, dir, dirBlock));
        char *BucketData = BF_Block_GetData(dirBlock); 
        CALL_BF(BF_UnpinBlock(dirBlock));
        SdirectoryBlock dirB;
        memcpy(&dirB, BucketData, sizeof(SdirectoryBlock));

        int dirIdx = hash - dirB.directories[0].hashId; // find in which position of dir array is the fckin directory
        int posBucket = dirB.directories[dirIdx].PosOfBucketBlock;
        
        BF_Block* bucketOfDir;
        BF_Block_Init(&bucketOfDir);
        CALL_BF(BF_GetBlock(indexDesc_S, posBucket, bucketOfDir));
        BucketData = BF_Block_GetData(bucketOfDir); 
        CALL_BF(BF_UnpinBlock(bucketOfDir));
        SbucketBlock currentBucketBlock;
        memcpy(&currentBucketBlock, BucketData, sizeof(SbucketBlock));
        
        bool flag = false;
        for(int i=0; i<currentBucketBlock.cntOfRecords; ++i){
            SecondaryRecord* record = currentBucketBlock.secondaryRecords[i];
            if(strcmp(record->index_key, index_key) == 0) {
                int tupleID = record->tupleId;
                int indexDesc = find_indexDesc(infoBlockData.fileName);
                Record searched_record;
                CALL_BF(findRecord(indexDesc, tupleID, &searched_record));
                printf("   -> Record:\n");
                printf("      ID: %-3d |  Name: %-15s |  Surname: %-13s |  City: %-15s |\n\n", 
                        searched_record.id, searched_record.city, searched_record.name, searched_record.surname);
                flag = true;
            }
        }
        if(!flag) printf("The record with surname = '%s' could not be found.\n", index_key);
    } 
    return HT_OK;
}

HT_ErrorCode SHT_HashStatistics(char *filename) {
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
    int minRecords = NoOfRecInBucket_S;

    for (int i = K_S + 1; i < blockCounter; i++) {
        BF_Block* BucketBlock;
        BF_Block_Init(&BucketBlock);
        CALL_BF(BF_GetBlock(fileDesc, i, BucketBlock));
        char * bucketData = BF_Block_GetData(BucketBlock); 
        SbucketBlock currentBucket;
        memcpy(&currentBucket, bucketData, sizeof(SbucketBlock));   
        CALL_BF(BF_UnpinBlock(BucketBlock));
        printf("\n====== B U C K E T [%d]  S T A T I S T I C S ======\n", i-(K_S+1));
        printf("* Local Depth: %d\n", currentBucket.localDepth);
        printf("* Number Of Records: %d\n", currentBucket.cntOfRecords);
        totalRecords += currentBucket.cntOfRecords;
        maxRecords = max(maxRecords, currentBucket.cntOfRecords);
        minRecords = min(minRecords, currentBucket.cntOfRecords);
    }
    double avgRecords = (double)totalRecords / (double)(blockCounter - (K_S + 1));
    printf("\n\n* Minimum number of records in buckets: %.d\n", minRecords);
    printf("* Maximum number of records in buckets: %.d\n", maxRecords);
    printf("* Average number of records in buckets: %.1f\n", avgRecords);
    return HT_OK;
}

HT_ErrorCode SHT_InnerJoin(int sindexDesc1, int sindexDesc2, char *index_key) {
    if (index_key == NULL) printf("\n==================== J O I N  A L L  S U R N A M E S ====================\n\n"); 
    else printf("\n==================== J O I N  S U R N A M E S  O N  '%s' ====================\n\n", index_key);

    BF_Block* infoBlock1;
    BF_Block_Init(&infoBlock1);
    CALL_BF(BF_GetBlock(sindexDesc1, 0, infoBlock1));

    char *data1 = BF_Block_GetData(infoBlock1); 
    CALL_BF(BF_UnpinBlock(infoBlock1));
    InfoStruct infoBlockData1;
    memcpy(&infoBlockData1, data1, sizeof(infoBlockData1));

    BF_Block* infoBlock2;
    BF_Block_Init(&infoBlock2);
    CALL_BF(BF_GetBlock(sindexDesc2, 0, infoBlock2));

    char *data2 = BF_Block_GetData(infoBlock2); 
    CALL_BF(BF_UnpinBlock(infoBlock2));
    InfoStruct infoBlockData2;
    memcpy(&infoBlockData2, data2, sizeof(infoBlockData2));
    
    int blockCounter;
    CALL_BF(BF_GetBlockCounter(sindexDesc1, &blockCounter));
    for (int i = K_S + 1; i < blockCounter; i++) {
        BF_Block* BucketBlock;
        BF_Block_Init(&BucketBlock);
        CALL_BF(BF_GetBlock(sindexDesc1, i, BucketBlock));
        char * bucketData = BF_Block_GetData(BucketBlock); 

        SbucketBlock currentBucketBlock;
        memcpy(&currentBucketBlock, bucketData, sizeof(SbucketBlock));
        CALL_BF(BF_UnpinBlock(BucketBlock));

        for (int r = 0; r < currentBucketBlock.cntOfRecords; r++) {
            SecondaryRecord * currentRecord;
            currentRecord = currentBucketBlock.secondaryRecords[r];
            int tupleID = currentRecord->tupleId;
            int indexDesc = find_indexDesc(infoBlockData1.fileName);
            Record searched_record;
            CALL_BF(findRecord(indexDesc, tupleID, &searched_record));

            if(index_key != NULL && strcmp(searched_record.surname, index_key) != 0) continue;

            unsigned long int hash = string_hash(currentRecord->index_key) % (1 << infoBlockData2.depth);
            int dir = findDirBlockNum_S(hash);
            BF_Block* dirBlock;
            BF_Block_Init(&dirBlock);
            
            CALL_BF(BF_GetBlock(sindexDesc2, dir, dirBlock));
            char *BucketData = BF_Block_GetData(dirBlock); 
            CALL_BF(BF_UnpinBlock(dirBlock));
            SdirectoryBlock dirB;
            memcpy(&dirB, BucketData, sizeof(SdirectoryBlock));
            
            int dirIdx = hash - dirB.directories[0].hashId; 
            int posBucket = dirB.directories[dirIdx].PosOfBucketBlock;
            
            BF_Block* bucketOfDir;
            BF_Block_Init(&bucketOfDir);

            CALL_BF(BF_GetBlock(sindexDesc2, posBucket, bucketOfDir));
            BucketData = BF_Block_GetData(bucketOfDir); 
            CALL_BF(BF_UnpinBlock(bucketOfDir));
            SbucketBlock currentBucketBlock;
            memcpy(&currentBucketBlock, BucketData, sizeof(SbucketBlock));
            for(int i=0; i<currentBucketBlock.cntOfRecords; ++i) {
                SecondaryRecord* record = currentBucketBlock.secondaryRecords[i];
                if(strcmp(record->index_key, searched_record.surname) == 0) {
                    int tupleID = record->tupleId;
                    int indexDesc2 = find_indexDesc(infoBlockData2.fileName);
                    Record searched_record2;
                    CALL_BF(findRecord(indexDesc2, tupleID, &searched_record2));
                    printf("{ ID: %-3d | Name: %-10s | Surname: %-13s | City: %-13s }  --  { ID: %-3d | Name: %-10s | Surname: %-13s | City: %-13s }\n", 
                        searched_record.id, searched_record.name, searched_record.surname, searched_record.city,
                        searched_record2.id, searched_record2.name, searched_record2.surname, searched_record2.city);
                }
            }
        } 
    }
    return HT_OK;
}