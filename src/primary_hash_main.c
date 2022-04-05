/* ============ MAIN FUNCTION FOR DEMONSTRATING THE USE OF PRIMARY INDEX EXTENDIBLE HASHING ============ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "primary_hash_file.h"

#define FILE_NAME "data.db"

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

#define CALL_OR_DIE(call)       \
{                               \
    HT_ErrorCode code = call;   \
    if (code != HT_OK) {        \
        printf("Error\n");      \
        exit(code);             \
    }                           \
}

int main(void) {
    BF_Init(LRU);
    CALL_OR_DIE(HT_Init());
    CALL_OR_DIE(HT_CreateIndex(FILE_NAME));
    int indexDesc; CALL_OR_DIE(HT_OpenIndex(FILE_NAME, &indexDesc)); 
    
    int numOfRecords = sizeof(names) / sizeof(char*);
    printf("\nInserting entries into file the file '%s'..\n\n", FILE_NAME);
    
    srand(12345);

    for (int id = 0; id < numOfRecords; ++id) {
        Record record;
        record.id = id;

        int r = rand() % numOfRecords;
        memcpy(record.name, names[r], strlen(names[r]) + 1);
        printf("INSERTING RECORD: ID: %-3d|  Name: %-12s|  ", id, names[r]);
        
        r = rand() % numOfRecords;
        memcpy(record.surname, surnames[r], strlen(surnames[r]) + 1);
        printf("Surname: %-16s|  ", surnames[r]);
        
        r = rand() % numOfRecords;
        memcpy(record.city, cities[r], strlen(cities[r]) + 1);
        printf("City: %-10s\n\n", cities[r]);

        int tupleId;
        struct UpdateRecordArray *UpdateArray;
        UpdateArray = malloc(numOfRecords * sizeof(UpdateRecordArray));
        CALL_OR_DIE(HT_InsertEntry(indexDesc, record, &tupleId, UpdateArray));
        //CALL_OR_DIE(HT_PrintAllEntries(indexDesc, NULL));
    }

    // int id = rand() % numOfRecords;
    // CALL_OR_DIE(HT_PrintAllEntries(indexDesc, &id));
    
    CALL_OR_DIE(HT_PrintAllEntries(indexDesc, NULL));
    //CALL_OR_DIE(HT_HashStatistics(FILE_NAME));

    CALL_OR_DIE(HT_CloseFile(indexDesc));
    BF_Close();

    return 0;
}