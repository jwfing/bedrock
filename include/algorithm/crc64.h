#ifndef HYPERBIRDER_COMMON_CRC64_INCLUDE_H_
#define HYPERBIRDER_COMMON_CRC64_INCLUDE_H_

#include "comm_def.h"

DECLARE_HB_NAMESPACE(common)

/** 
  * Processes a multiblock of a CRC64 calculation. 
  * 
  * @returns Intermediate CRC64 value. 
  * @param   uCRC64  Current CRC64 intermediate value. 
  * @param   pv      The data block to process. 
  * @param   cb      The size of the data block in bytes. 
  */ 
uint64_t calculate_crc64(uint64_t uCRC64, const void *pv, size_t cb) ;

/** 
  * Calculate CRC64 for a memory block. 
  * 
  * @returns CRC64 for the memory block. 
  * @param   pv      Pointer to the memory block. 
  * @param   cb      Size of the memory block in bytes. 
  */ 
uint64_t calculate_crc64(const void *pv, size_t cb);
/**
  * Get the static CRC64 table. This function is only used for testing purpose.
  *
  */
const uint64_t * get_crc64_table();

END_DECLARE_HB_NAMESPACE(common)

#endif
