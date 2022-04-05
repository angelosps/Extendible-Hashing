/* ============ MAIN FUNCTION FOR DEMONSTRATING THE USE OF SECONDARY INDEX EXTENDIBLE HASHING ============ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "primary_hash_file.h"
#include "secondary_hash_file.h"

#define FILE_NAME "data.db"
#define FILE_NAME_S "data_s.db" 
#define FILE_NAME_2 "data_2.db"
#define FILE_NAME_S_2 "data_s_2.db"
#define INDEX_KEY "Surname"
#define INDEX_KEY_LEN 20
#define GLOBAL_DEPTH 1  // we always begin with global depth = 1 //

#define CALL_OR_DIE(call)       \
{                               \
    HT_ErrorCode code = call;   \
    if (code != HT_OK) {        \
        printf("Error\n");      \
        exit(code);             \
    }                           \
}

const char* names[] = {
    "Will",
    "Gabriel",
    "Angelina",
    "Giannis",
    "Ed",
    "Angelos",
    "Dimitrios",
    "Dimitrios",
    "Tom",
};

const char* surnames[] = {
    "Smith",
    "Macht",
    "Jolie",
    "Antetokounmpo",
    "Sheeran",
    "Poulis",
    "Kyriakopoulos",
    "Rontogiannis",
    "Hardy",
};

const char* cities[] = {
    "Philadeplhia",
    "New York",
    "Los Angeles",
    "Milwaukee",
    "Halifax",
    "Athens",
    "Athens",
    "Athens",
    "London",
};

int main(void) {
    BF_Init(LRU);
    CALL_OR_DIE(HT_Init());
    CALL_OR_DIE(HT_CreateIndex(FILE_NAME));
    int indexDesc; CALL_OR_DIE(HT_OpenIndex(FILE_NAME, &indexDesc)); 
    CALL_OR_DIE(SHT_Init());
    SHT_CreateSecondaryIndex(FILE_NAME_S, INDEX_KEY, INDEX_KEY_LEN, GLOBAL_DEPTH, FILE_NAME);
    int SindexDesc; SHT_OpenSecondaryIndex(FILE_NAME_S, &SindexDesc);
  
    int numOfRecords = sizeof(names) / sizeof(char*);
    
    srand(12345);

    UpdateRecordArray *UpdateArray;
    UpdateArray = malloc(numOfRecords * sizeof(UpdateRecordArray));
    UpdateRecordArray NullRec; NullRec.newTupleId = -1; NullRec.oldTupleId = -1;
    for(int i=0; i < numOfRecords; i++) UpdateArray[i] = NullRec;
  
    printf("\n");
    for (int id = 0; id < numOfRecords; ++id) {
        Record record;
        record.id = id;

        int r = rand() % numOfRecords;
        memcpy(record.name, names[r], strlen(names[r]) + 1);
        printf("Inserting record: {%d, %s, ", id, names[r]);
        
        r = rand() % numOfRecords;
        memcpy(record.surname, surnames[r], strlen(surnames[r]) + 1);
        printf("%s, ", surnames[r]);
        
        r = rand() % numOfRecords;
        memcpy(record.city, cities[r], strlen(cities[r]) + 1);
        printf("%s}\n", cities[r]);
        
        int tupleId;
        for(int i=0; i<numOfRecords; i++) UpdateArray[i] = NullRec;

        CALL_OR_DIE(HT_InsertEntry(indexDesc, record, &tupleId, UpdateArray));
        
        SecondaryRecord newSrecord;
        strcpy(newSrecord.index_key, record.surname);
        newSrecord.tupleId = tupleId;    
        CALL_OR_DIE(SHT_SecondaryInsertEntry(SindexDesc, newSrecord));
        CALL_OR_DIE(SHT_SecondaryUpdateEntry(SindexDesc, UpdateArray));
    } printf("\n");

    /* ======= PRINT ALL ENTRIES FOR FIRST HT ======= */
    CALL_OR_DIE(HT_PrintAllEntries(indexDesc, NULL));
    int id = rand() % numOfRecords; CALL_OR_DIE(HT_PrintAllEntries(indexDesc, &id));
    
    /* ======= PRINT HASH STATISTICS FOR FIRST HT ======= */
    CALL_OR_DIE(HT_HashStatistics(FILE_NAME));

    /* ======= PRINT ALL ENTRIES FOR FIRST SHT ======= */
    CALL_OR_DIE(SHT_PrintAllEntries(SindexDesc, NULL));

    srand(time(NULL));
    int randIndex = rand() % numOfRecords;
    char * indexKey = malloc(strlen((surnames[randIndex])+1) * sizeof(char));
    strcpy(indexKey, surnames[randIndex]);

    CALL_OR_DIE(SHT_PrintAllEntries(SindexDesc, indexKey));
  
    /* ======= PRINT HASH STATISTICS FOR 1ST SHT ======= */
    CALL_OR_DIE(SHT_HashStatistics(FILE_NAME_S));

    printf("\n=============== E N D  O F  P R I N T I N G  F I R S T  H T & S H T  F I L E S ===============\n");
    
    /* ========== E N D  O F  F I R S T  F I L E ========== */

    /* ========== S T A R T  O F  S E C O N D  F I L E ========== */
    CALL_OR_DIE(HT_CreateIndex(FILE_NAME_2));
    int indexDesc_2; CALL_OR_DIE(HT_OpenIndex(FILE_NAME_2, &indexDesc_2)); 
    CALL_OR_DIE(SHT_Init());
    SHT_CreateSecondaryIndex(FILE_NAME_S_2, INDEX_KEY, INDEX_KEY_LEN, GLOBAL_DEPTH, FILE_NAME_2);
    int SindexDesc_2; SHT_OpenSecondaryIndex(FILE_NAME_S_2, &SindexDesc_2);
      
    srand(123456);
    
    UpdateRecordArray *UpdateArray_2;
    UpdateArray_2 = malloc(numOfRecords * sizeof(UpdateRecordArray));
    for(int i=0; i < numOfRecords; i++) UpdateArray_2[i] = NullRec;

    printf("\n");
    for (int id = 0; id < numOfRecords; ++id) {
        Record record;
        record.id = id;

        int r = rand() % numOfRecords;
        memcpy(record.name, names[r], strlen(names[r]) + 1);
        printf("Inserting record: {%d, %s, ", id, names[r]);
        
        r = rand() % numOfRecords;
        memcpy(record.surname, surnames[r], strlen(surnames[r]) + 1);
        printf("%s, ", surnames[r]);
        
        r = rand() % numOfRecords;
        memcpy(record.city, cities[r], strlen(cities[r]) + 1);
        printf("%s}\n", cities[r]);
        
        int tupleId;
        for(int i=0; i<numOfRecords; i++) UpdateArray_2[i] = NullRec;
        CALL_OR_DIE(HT_InsertEntry(indexDesc_2, record, &tupleId, UpdateArray_2));
        
        SecondaryRecord newSrecord;
        strcpy(newSrecord.index_key, record.surname);
        newSrecord.tupleId = tupleId;    
        CALL_OR_DIE(SHT_SecondaryInsertEntry(SindexDesc_2, newSrecord));
        CALL_OR_DIE(SHT_SecondaryUpdateEntry(SindexDesc_2, UpdateArray_2));
    } printf("\n");

    /* ======= PRINT ALL ENTRIES FOR 2ND HT ======= */
    CALL_OR_DIE(HT_PrintAllEntries(indexDesc_2, NULL));
    id = rand() % numOfRecords; CALL_OR_DIE(HT_PrintAllEntries(indexDesc_2, &id));

    /* ======= PRINT HASH STATISTICS FOR 2ND HT ======= */
    CALL_OR_DIE(HT_HashStatistics(FILE_NAME_2));

    /* ======= PRINT ALL ENTRIES FOR 2ND SHT ======= */
    CALL_OR_DIE(SHT_PrintAllEntries(SindexDesc_2, NULL));
    srand(time(NULL));
    randIndex = rand() % numOfRecords;
    indexKey = malloc(strlen((surnames[randIndex])+1) * sizeof(char));
    strcpy(indexKey, surnames[randIndex]);
    CALL_OR_DIE(SHT_PrintAllEntries(SindexDesc_2, indexKey));

    /* ======= PRINT HASH STATISTICS FOR 2ND SHT ======= */
    CALL_OR_DIE(SHT_HashStatistics(FILE_NAME_S_2));

    /* ===== SEARCH WITH INNER JOIN ===== */
    SHT_InnerJoin(SindexDesc, SindexDesc_2, NULL);

    srand(time(NULL));
    randIndex = rand() % numOfRecords;
    strcpy(indexKey, surnames[randIndex]); 
    SHT_InnerJoin(SindexDesc, SindexDesc_2, indexKey);

    // Inserting new record on the second HT file 
    Record record;
    record.id = numOfRecords;

    int r = rand() % numOfRecords;
    memcpy(record.name, names[r], strlen(names[r]) + 1);

    printf("\n\nInserting record: {%d, %s, ", record.id, names[r]);
        
    r = rand() % numOfRecords;
    memcpy(record.surname, surnames[r], strlen(surnames[r]) + 1);
    printf("%s, ", surnames[r]);
    
    r = rand() % numOfRecords;
    memcpy(record.city, cities[r], strlen(cities[r]) + 1);
    printf("%s}\n", cities[r]);
  
    int tupleId;
    for(int i=0; i<numOfRecords; i++) UpdateArray_2[i] = NullRec;
    CALL_OR_DIE(HT_InsertEntry(indexDesc_2, record, &tupleId, UpdateArray_2));
    
    SecondaryRecord newSrecord;
    strcpy(newSrecord.index_key, record.surname);
    newSrecord.tupleId = tupleId;    
    CALL_OR_DIE(SHT_SecondaryInsertEntry(SindexDesc_2, newSrecord));
    CALL_OR_DIE(SHT_SecondaryUpdateEntry(SindexDesc_2, UpdateArray_2));

    /* ======= PRINT ALL ENTRIES FOR 2ND HT ======= */
    CALL_OR_DIE(HT_PrintAllEntries(indexDesc_2, NULL));
    id = rand() % numOfRecords; CALL_OR_DIE(HT_PrintAllEntries(indexDesc_2, &id));

    /* ======= PRINT HASH STATISTICS FOR 2ND HT ======= */
    CALL_OR_DIE(HT_HashStatistics(FILE_NAME_2));

    /* ======= PRINT ALL ENTRIES FOR 2ND SHT ======= */
    CALL_OR_DIE(SHT_PrintAllEntries(SindexDesc_2, NULL));
    srand(time(NULL));
    randIndex = rand() % numOfRecords;
    indexKey = malloc(strlen((surnames[randIndex])+1) * sizeof(char));
    strcpy(indexKey, surnames[randIndex]);
    CALL_OR_DIE(SHT_PrintAllEntries(SindexDesc_2, indexKey));

    /* ======= PRINT HASH STATISTICS FOR 2ND SHT ======= */
    CALL_OR_DIE(SHT_HashStatistics(FILE_NAME_S_2));

    /* ===== SEARCH WITH INNER JOIN ===== */
    SHT_InnerJoin(SindexDesc, SindexDesc_2, NULL);

    srand(time(NULL));
    randIndex = rand() % numOfRecords;
    strcpy(indexKey, surnames[randIndex]); 
    SHT_InnerJoin(SindexDesc, SindexDesc_2, indexKey);
    /* ============ E N D  O F  S E C O N D  F I L E ============ */


    /* ============ C L O S E  F I L E S ============ */
    CALL_OR_DIE(HT_CloseFile(indexDesc)); // close 1
    CALL_OR_DIE(SHT_CloseSecondaryIndex(SindexDesc)); // close s_1

    CALL_OR_DIE(HT_CloseFile(indexDesc_2)); // close 2
    CALL_OR_DIE(SHT_CloseSecondaryIndex(SindexDesc_2)); // close s_2

    BF_Close(); 
  
    return 0;
}