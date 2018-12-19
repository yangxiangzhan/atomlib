#ifndef _VIRSUAL_FILE_SYSTTEM_
#define _VIRSUAL_FILE_SYSTTEM_

// File open flags
enum OPEN_FLAG {
    // open flags
    RDONLY = 1,        // Open a file as read only
    WRONLY = 2,        // Open a file as write only
    RDWR   = 3,        // Open a file as read and write
    CREAT  = 0x0100,   // Create a file if it does not exist
    EXCL   = 0x0200,   // Fail if a file already exists
    TRUNC  = 0x0400,   // Truncate the existing file to zero size
    APPEND = 0x0800,   // Move to end of file on every write
};



#endif

