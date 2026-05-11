#include "MemoryLeakCheck.h"

using namespace std;

int MemoryLeak::News[65536];
int MemoryLeak::Deletes[65536];
bool MemoryLeak::FirstNews[65536];
bool MemoryLeak::FirstDeletes[65536];
bool MemoryLeak::Initialized;
