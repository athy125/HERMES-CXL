#ifndef CXL_COMMON_H
#define CXL_COMMON_H

// Common definitions for HERMES-CXL

// CXL command status codes
#define CXL_CMD_STATUS_ACTIVE     0
#define CXL_CMD_STATUS_COMPLETED  1
#define CXL_CMD_STATUS_ERROR      2
#define CXL_CMD_STATUS_INVALID    3

// CXL Memory commands
#define CXL_MEM_SEND_COMMAND     0x1001
#define CXL_MEM_QUERY_CMD        0x1002

#endif // CXL_COMMON_H