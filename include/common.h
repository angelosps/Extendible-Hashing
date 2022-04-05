#ifndef COMMON_H
#define COMMON_H

#include "stdbool.h"
#include "bf.h"

#define MAX_OPEN_FILES 20

#define CALL_BF(call)         \
{                             \
  BF_ErrorCode code = call;   \
  if (code != BF_OK) {        \
    BF_PrintError(code);      \
    return HT_ERROR;          \
  }                           \
}

#define min(a,b)              \
({ __typeof__ (a) _a = (a);   \
  __typeof__ (b) _b = (b);    \
  _a < _b ? _a : _b; })

#define max(a,b)              \
({ __typeof__ (a) _a = (a);   \
    __typeof__ (b) _b = (b);  \
  _a > _b ? _a : _b; })


int next_file_idx();    // Finds the index of the next available file in the "allFiles" array 

typedef enum HT_ErrorCode {
	HT_OK,
	HT_ERROR
} HT_ErrorCode;

typedef struct hashFileIdx {
	int fileDesc;		
	bool hasFile;		
	char *fileName;
} hashFileIdx; 

hashFileIdx allFiles[MAX_OPEN_FILES]; // Declaration of a global array (for opening files).

typedef struct Record {
	int id;
	char name[15];
	char surname[20];
	char city[20];
} Record;

typedef struct UpdateRecordArray {  // μπορειτε να αλλαξετε τη δομη συμφωνα με τις ανάγκες σας
	char indexKey[20];
	int oldTupleId; // η παλια θέση της εγγραφής πριν την εισαγωγή της νέας
	int newTupleId; // η νέα θέση της εγγραφής που μετακινήθηκε μετα την εισαγωγή της νέας εγγραφής 
} UpdateRecordArray; 

#endif