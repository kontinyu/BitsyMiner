/* 
 * BitsyMiner Open Source
 * Copyright (c) 2025 Justin Williams
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include "miner.h"
#include "utils.h"
#include "defines_n_types.h"
#include "stratum.h"
#include "MinerSha256.h"
#include "monitor.h"
#include "soc/hwcrypto_reg.h"
#ifndef ESP32C3
  #include "soc/dport_reg.h"
#endif
#include "esp32/rom/ets_sys.h"
#include "soc/i2s_reg.h"
#include "soc/i2s_struct.h"

#define MAX_DIFFICULTY 0x1d00ffff
#define MAX_COINBASE_LENGTH 512
#define MAX_EXTRA_NONCE_LENGTH 16


extern SetupData settings;
extern QueueHandle_t stratumMessageQueueHandle;
extern QueueHandle_t appMessageQueueHandle;

unsigned char blockTarget[32];
double poolDifficulty = 1.0;
unsigned char poolTarget[32];

size_t extraNonce2Size = 0;
unsigned long extraNonce2 = 1;

hash_block pendingMiningJobBlock;


unsigned long startNonce[2] = {0, 0x80000000};
volatile bool isMining = false;
char currentJobId[MAX_JOB_ID_LENGTH];

volatile bool core0Mining = false;
volatile bool core1Mining = false;

extern MonitorData monitorData;


// Encode an extra nonce value as a hexadecimal string
void encodeExtraNonce(char *dest, size_t len, unsigned long en) {
  static const char *tbl= "0123456789ABCDEF";  
  dest += len * 2;
  *dest-- = '\0';
  while( len-- ) {
      *dest-- = tbl[en & 0x0f];
      *dest-- = tbl[(en >> 4) & 0x0f];
      en >>= 8;
  }
}


void divide_256bit_by_double(uint64_t* target, double divisor) {
    for (int i = 0; i < 4; i++) {
        target[i] = (uint64_t)((double)target[i] / divisor);
        if (i < 3) {
            double rem = (double)target[i] - ((double)target[i] * divisor);
            target[i + 1] += (uint64_t)(rem * 1e18); // Scale remainder to next higher part
        }
    }
}

// Function to adjust target based on difficulty, using max difficulty / pool difficulty
void adjust_target_for_difficulty(uint8_t* pt, uint8_t* bt, double difficulty) {
    uint64_t target_parts[4];

    // Convert target to uint64_t array for easy arithmetic
    for (int i = 0; i < 4; i++) {
       target_parts[i] = ((uint64_t)bt[i * 8 + 0]) |
                          ((uint64_t)bt[i * 8 + 1] << 8)  |
                          ((uint64_t)bt[i * 8 + 2] << 16) |
                          ((uint64_t)bt[i * 8 + 3] << 24) |
                          ((uint64_t)bt[i * 8 + 4] << 32) |
                          ((uint64_t)bt[i * 8 + 5] << 40) |
                          ((uint64_t)bt[i * 8 + 6] << 48) |
                          ((uint64_t)bt[i * 8 + 7] << 56);
    }

    // Divide target by difficulty
    divide_256bit_by_double(target_parts, difficulty);

    // Convert back to byte array
    for (int i = 0; i < 4; i++) {
        pt[i * 8 + 0] = target_parts[i] & 0xff;
        pt[i * 8 + 1] = (target_parts[i] >> 8) & 0xff;
        pt[i * 8 + 2] = (target_parts[i] >> 16) & 0xff;
        pt[i * 8 + 3] = (target_parts[i] >> 24) & 0xff;
        pt[i * 8 + 4] = (target_parts[i] >> 32) & 0xff;
        pt[i * 8 + 5] = (target_parts[i] >> 40) & 0xff;
        pt[i * 8 + 6] = (target_parts[i] >> 48) & 0xff;
        pt[i * 8 + 7] = (target_parts[i] >> 56) & 0xff;
    }
}

void bits_to_target(uint32_t nBits, uint8_t* target) {
    uint32_t exponent = nBits >> 24;
    uint32_t mantissa = nBits & 0x007fffff;

    if (nBits & 0x00800000) {
        mantissa |= 0x00800000;
    }

    memset(target, 0, 32);

    if (exponent <= 3) {
        mantissa >>= 8 * (3 - exponent);
        memcpy(target, &mantissa, 4);
    } else {
        int shift = (exponent - 3) * 8;
        uint32_t* target_ptr = (uint32_t*)(target + shift / 8);
        *target_ptr = mantissa;
    }
}

// Checks hash against a target value
//__attribute__((section(".fastcode")))
int check_target(const unsigned char* hash, const unsigned char* target) {
    int i, j;
    for (i = 31; i >0; i--) {
        if (hash[i] < target[i]) {
            return 1;  // Valid
        } else if (hash[i] > target[i]) {
            return 0;  // Invalid
        }
    }
    return 1;  // Equal is also valid
}

void convert_string_to_bytes(unsigned char*out, const char *in, size_t len) {
    size_t b = 0;
    for(int i = 0; i < len; i+=2) {
        out[b++] = (unsigned char) (decodeHexChar(in[i]) << 4) + decodeHexChar(in[i+1]);
    }
}

void double_sha256_merkle(unsigned char *dest, unsigned char* buf64) {
  
  miner_sha256_hash ctx, ctx1;

  sha256(&ctx, buf64, 64); // get hash of pair
  sha256(&ctx1, ctx.bytes, 32); // double hash

  // Copy hash into original position
  memcpy(dest, ctx1.bytes, 32);
}

void calculateMerkleRoot(unsigned char *root, unsigned char* coinbaseHash, JsonArray& merkleBranch) {

  int i, j;
  unsigned char merklePair[64];

  // Add coinbase hash to tree at position 0
  memcpy(merklePair, coinbaseHash, 32);

  for(i = 0; i < merkleBranch.size(); i++) {
    convert_string_to_bytes(&merklePair[32], (const char*) merkleBranch[i], 64);
    double_sha256_merkle(merklePair, merklePair);
  }
  memcpy(root, merklePair, 32);

}


void createCoinbaseHash(unsigned char* hash, stratum_block* sb, const char* cb1, const char* cb2) {

  char buffer[400];
  unsigned char cbin[MAX_COINBASE_LENGTH];
  size_t cbSize = ((strlen(cb1) + strlen(cb2) + sb->extraNonce1.length()) / 2) + sb->extraNonce2Size;

  // Build coinbase string of hex
  if( cbSize <= MAX_COINBASE_LENGTH ) {
    size_t clen = 0;
    // Start with coinbase 1
    
    convert_string_to_bytes(cbin, (const char*) cb1, strlen(cb1));
    clen += strlen(cb1) / 2;

    // Add extra nonce 1
    convert_string_to_bytes(&cbin[clen], (const char*) sb->extraNonce1.c_str(), sb->extraNonce1.length());
    clen += sb->extraNonce1.length() / 2;

    size_t en2len = sb->extraNonce2Size;
    if( en2len > 8 ) {
      dbg("Bad extra nonce 2 length\n");
      en2len = 8;
    }
    // Add extra nonce 2
    extraNonce2Size = en2len;

    encodeExtraNonce(buffer, en2len, extraNonce2);

    convert_string_to_bytes(&cbin[clen], (const char*) buffer, strlen(buffer));
    clen += en2len;

    // Finish with coinbase 2
    convert_string_to_bytes(&cbin[clen], (const char*) cb2, strlen(cb2));
    clen += strlen(cb2) / 2;

    // Produce the double hash
    miner_sha256_hash ctx, ctx1;
    sha256(&ctx, cbin, clen);
    sha256(&ctx1, ctx.bytes, 32);

    memcpy(hash, ctx1.bytes, 32);    
  }

}

// Swap bytes on merkle 
void longSwap(unsigned long* val) {
  for(int i = 0; i < 8; i++) {
    val[i] = BYTESWAP32(val[i]);
  }
}

// Calculates hash difficulty and compares it to best achieved
//__attribute__((section(".fastcode")))
void compareBestDifficulty(miner_sha256_hash *ctx) {
  static const double maxTarget = 26959535291011309493156476344723991336010898738574164086137773096960.0;
  double hashValue = 0.0;

  int i,j;
  for(i = 0, j = 31; i < 32; i++, j--) {
    hashValue = hashValue * 256 + ctx->bytes[j];
  }

  double difficulty = maxTarget / hashValue;
  if( ! isnan(difficulty) && ! isinf(difficulty) && 
    (isnan(monitorData.bestDifficulty) || isinf(monitorData.bestDifficulty) || difficulty >= monitorData.bestDifficulty) ) 
  {
    monitorData.bestDifficulty = difficulty;
  }
}

//__attribute__((section(".fastcode")))
static double getDifficulty(miner_sha256_hash *ctx) {
  static const double maxTarget = 26959535291011309493156476344723991336010898738574164086137773096960.0;
  double hashValue = 0.0;

  int i,j;
  for(i = 0, j = 31; i < 32; i++, j--) {
    hashValue = hashValue * 256 + ctx->bytes[j];
  }

  double difficulty = maxTarget / hashValue;
  if( isnan(difficulty) &&  isinf(difficulty) ) {
    difficulty = 0.0;
  }
  return difficulty;
}

void getBlockHeight(const char* cb) {
  if( strlen(cb) >= 92 && cb[84] == '0' && cb[85] == '3' ) {
    uint32_t bh = 0;
    bh |= ((decodeHexChar(cb[90]) << 4 ) + decodeHexChar(cb[91])) << 16;
    bh |= ((decodeHexChar(cb[88]) << 4 ) + decodeHexChar(cb[89])) << 8;
    bh |= ((decodeHexChar(cb[86]) << 4 ) + decodeHexChar(cb[87]));
    monitorData.blockHeight = bh;
    dbg("Block Height: %lu\n", bh);
  }
}


// Set the pool target
void setPoolTarget() {
    // Set the target for the pool based on the pool difficulty
    unsigned char maxDifficulty[32];
    bits_to_target(MAX_DIFFICULTY, maxDifficulty);
    adjust_target_for_difficulty(poolTarget, maxDifficulty, poolDifficulty);
}


// Copy everything to a local structure and get ready to roll
void startMiningJob(stratum_block* sb) {

  char buffer[65];
  unsigned char coinbaseHash[32];

  while( isMining || core0Mining || core1Mining ) {
    isMining = false;
    vTaskDelay(10/portTICK_PERIOD_MS);    
  }

  // Define a random Extra Nonce 2
  extraNonce2 = esp_random();

  // Build the block
  pendingMiningJobBlock.version = strtoul(sb->version.c_str(), NULL, 16);
  convert_string_to_bytes(pendingMiningJobBlock.prev_hash, (const char*) sb->prevHash.c_str(), 64);
  createCoinbaseHash(coinbaseHash, sb, sb->coinBase1.c_str(), sb->coinBase2.c_str());
  calculateMerkleRoot(pendingMiningJobBlock.merkle_root, coinbaseHash, sb->merkleBranch);
  pendingMiningJobBlock.timestamp = strtoul(sb->nTime.c_str(), NULL, 16);
  pendingMiningJobBlock.difficulty = strtoul(sb->difficulty.c_str(), NULL, 16);
  safeStrnCpy(currentJobId, sb->jobId.c_str(), MAX_JOB_ID_LENGTH);

  dbg("Job ID first: %s\n", currentJobId);

  getBlockHeight(sb->coinBase1.c_str());

  // Play with time stamp
  if( settings.randomizeTimestamp ) {
    pendingMiningJobBlock.timestamp += (esp_random() & 0xff); // Up to 255 seconds in the future
  }

  // Do some swaps
  longSwap((unsigned long*) &pendingMiningJobBlock.prev_hash);

  
  // Make our nonces random but without overlap
  startNonce[0] = esp_random();
  startNonce[1] = startNonce[0] + 0x80000000;

  //Serial.println("Begin new mining job...");
  monitorData.totalJobs++;

  // Set up the target
  bits_to_target(pendingMiningJobBlock.difficulty, blockTarget);

  // Just make sure this is done before any mining occurs
  setPoolTarget();

  isMining = true;

}



// Sets the pool difficulty as recalculates the target
void setPoolDifficulty(double pDiff) {

  if( ! isnan(pDiff) && ! isinf(pDiff) ) {
    monitorData.poolDifficulty = pDiff;
    poolDifficulty = pDiff;
    setPoolTarget();

    ApplicationMessage am;
    am.action = MAIN_ACTION_SEND_DIFFICULTY;
    xQueueSend(appMessageQueueHandle, &am, pdMS_TO_TICKS(100));
  }
}




// Submit the job back to Stratum pool via a messaging queue
//__attribute__((section(".fastcode")))
void submitJob(const char* jobId, uint32_t timestamp, uint32_t nonce, bool topPriority, uint32_t submitFlags, double difficulty) {
  
  //submit(jobId, extraNonce2, hb->timestamp, hb->nonce);
  jobSubmitQueueEntry qe;

  // Make the extra nonce 2 a formatted text field
  encodeExtraNonce(qe.extraNonce2, extraNonce2Size, extraNonce2);

  safeStrnCpy(qe.jobId, jobId, MAX_JOB_ID_LENGTH);
  qe.timestamp = timestamp;
  qe.nonce = nonce;
  qe.callback = NULL;
  qe.versionBits = 0;
  qe.submitflags = submitFlags;
  qe.difficulty = difficulty;

  if( ! topPriority ) {
    xQueueSendToFront(stratumMessageQueueHandle, &qe, pdMS_TO_TICKS(100));
  } else {
    xQueueSend(stratumMessageQueueHandle, &qe, pdMS_TO_TICKS(100));
  }
}


//__attribute__((section(".fastcode")))
void hashCheck(char* jobId, miner_sha256_hash *ctx, uint32_t timestamp, uint32_t nonce) {

  uint32_t submitFlags = 0;

  if( check_target(ctx->bytes, poolTarget) ) {
      
    if( ! ctx->hash[7] ) {
      dbg("32-bit match\n");
      submitFlags |= SUBMIT_FLAG_32BIT;
    }
          
    if(check_target(ctx->bytes, blockTarget) ) {

      dbg("Met block target with nonce %x.\n", nonce); 
      submitFlags |= SUBMIT_FLAG_BLOCK_SOLUTION;
   
    } else {
      dbg("Met pool target with nonce: %x\n", nonce);
    }

    double difficulty = getDifficulty(ctx);

    submitJob(jobId, timestamp, nonce, false, submitFlags, difficulty);
    monitorData.poolSubmissions++;
  }

}


//__attribute__((section(".fastcode")))
void minerTask(void *task_id) {

  hash_block hb;
  miner_sha256_hash ctx;
  miner_sha256_hash midstate;

  unsigned int miner_id = (uint32_t)task_id;
   
  char jobId[MAX_JOB_ID_LENGTH];

  dbg("Starting Miner %lu on core %d\n", task_id, xPortGetCoreID());

  // Subscribe to watchdog for Core 0 miner (PlatformIO compatibility)
  esp_task_wdt_add(NULL);
  dbg("Miner %lu subscribed to watchdog\n", task_id);

  while(1) {

    esp_task_wdt_reset();

    if( isMining ) {

      // Check to see if we have core 0 mining turned off
      if( settings.coreZeroDisabled ) {
        taskYIELD();
        continue;
      }

      core0Mining = true;

      // For now, use the block difficulty
      memcpy(&hb, &pendingMiningJobBlock, sizeof(hash_block)); 
      sha256midstate(&midstate, &hb);

      safeStrnCpy(jobId, currentJobId, MAX_JOB_ID_LENGTH);

      dbg("Miner Task 1: %s\n", jobId);

      
      //Set the starting nonce for this task
      hb.nonce = startNonce[miner_id];

      unsigned short yieldCounter = 0;

      while(isMining) {

        yieldCounter++;
        if( (yieldCounter & 0xff) == 0 ){  // Yield and feed watchdog every 256 iterations
          esp_task_wdt_reset();  // Feed the watchdog
          vTaskDelay(1);  // Yield for 1ms
        }

        if( sha256header(&midstate, &ctx, &hb) ) {
          hashCheck(jobId, &ctx, hb.timestamp, hb.nonce);
        }

        hb.nonce += 1;        
        monitorData.internalHashes += 1;        
        
      } // isMining

      core0Mining = false;
      
    } // while isMining
 
    // Leave this delay in case we're not mining
    vTaskDelay(20 / portTICK_PERIOD_MS);

  } 

}



typedef struct {
    uint32_t dw0; // Flags and length
    uint32_t dw1; // Unused (usually)
    uint32_t buffer_address; // Pointer to sha_dma_buffer
    uint32_t next_desc_address; // Pointer to next descriptor (or NULL)
} dma_desc_t;


//__attribute__((section(".fastcode")))
void IRAM_ATTR miner1Task(void *task_id) {

  uint32_t id = (uint32_t) task_id;
  miner_sha256_hash ctx, midstate;
  char jobId[MAX_JOB_ID_LENGTH];

  //hash_block hb __attribute__((aligned(4)));
  hash_block hb __attribute__((aligned(4)));
  hash_block hbCheck __attribute__((aligned(4)));


  /* Set up DMA business */
  // Allocate the descriptor in DMA-capable memory
  dma_desc_t dma_descriptor __attribute__((aligned(4)));  
  
  // 3. Configure the single DMA descriptor
  dma_descriptor.dw0 = (64 << 12) | 0x80000000; // Total length (64) | EOF flag
  dma_descriptor.dw1 = 0; // Reserved
  dma_descriptor.buffer_address = (uint32_t) &hb; // Source buffer
  dma_descriptor.next_desc_address = 0; // End of list
  

  while(1) {

    if(! isMining ) {
      vTaskDelay(20 / portTICK_PERIOD_MS);
      continue;
    }

    // Creates a copy of the pending block that we will byte reverse
    memcpy(&hb, &pendingMiningJobBlock, sizeof(hash_block));

    // And another copy that we will use for block verifications
    memcpy(&hbCheck, &pendingMiningJobBlock, sizeof(hash_block)); 

    sha256midstate(&midstate, &hbCheck);

    hb.nonce = startNonce[id];

    safeStrnCpy(jobId, currentJobId, MAX_JOB_ID_LENGTH);
    dbg("Miner Task 2: %s\n", jobId);


    core1Mining = true;

    // Swap all the bytes we can up front instead of every time
    uint32_t* data = (uint32_t*) &hb;
    for(int i = 0; i < 20; i++) {
      data[i] = BYTESWAP32(data[i]);
    }

    volatile uint32_t *sha_base = (volatile uint32_t*) HASH_AREA_SHA256;

    const uint32_t shaPad    = 0x80000000u; // word 8 for second SHA
    const uint32_t firstShaBitLen = 0x00000280u;
    const uint32_t secondShaBitLen = 0x00000100u; // 256 bits
    

    INIT_HARDWARE_SHA256

    while(isMining) {

      // Assumes sha_base, data, internalHashes, hashBlock1, hb.nonce are 4-byte aligned.

        __asm__ __volatile__(

        "l32i.n   a2,  %[nonce], 0 \n" /* Store nonce in a register */
        "addi     a5,  %[sb], 0x90 \n" /* a5 = sb_ctl = sb + 0x90 */

      "proc_start: \n"

        /* Move data from hash block into SHA registers */
        "l32i.n    a3,  %[IN],  0 \n"
        "s32i.n    a3,  %[sb],  0 \n"
        "l32i.n    a3,  %[IN],  4 \n"
        "s32i.n    a3,  %[sb],  4 \n"

        "l32i.n    a3,  %[IN],  8 \n"
        "s32i.n    a3,  %[sb],  8 \n"        
        "l32i.n    a3,  %[IN],  12 \n"
        "s32i.n    a3,  %[sb],  12 \n"

        "l32i.n    a3,  %[IN],  16 \n"
        "s32i.n    a3,  %[sb],  16 \n"        
        "l32i.n    a3,  %[IN],  20 \n"
        "s32i.n    a3,  %[sb],  20 \n"

        "l32i.n    a3,  %[IN],  24 \n"
        "s32i.n    a3,  %[sb],  24 \n"        
        "l32i.n    a3,  %[IN],  28 \n"
        "s32i.n    a3,  %[sb],  28 \n"

        "l32i.n    a3,  %[IN],  32 \n"
        "s32i.n    a3,  %[sb],  32 \n"
        "l32i.n    a3,  %[IN],  36 \n"
        "s32i.n    a3,  %[sb],  36 \n"

        "l32i.n    a3,  %[IN],  40 \n"
        "s32i.n    a3,  %[sb],  40 \n"        
        "l32i.n    a3,  %[IN],  44 \n"
        "s32i.n    a3,  %[sb],  44 \n"

        "l32i.n    a3,  %[IN],  48 \n"
        "s32i.n    a3,  %[sb],  48 \n"        
        "l32i.n    a3,  %[IN],  52 \n"
        "s32i.n    a3,  %[sb],  52 \n"

        "l32i.n    a3,  %[IN],  56 \n"
        "s32i.n    a3,  %[sb],  56 \n"        
        "l32i.n    a3,  %[IN],  60 \n"
        "s32i.n    a3,  %[sb],  60 \n"

        /* 1) start SHA */
        "movi.n  a3, 1\n"
        "s32i.n  a3, a5, 0\n" /* 0x90 */

        "memw   \n" // Publish change

        /* Start preparing next block */
        "l32i    a3,  %[IN],   64 \n"
        "s32i.n  a3,  %[sb],    0 \n"
        "l32i    a3,  %[IN],   68 \n"
        "s32i.n  a3,  %[sb],    4 \n"
        "l32i    a3,  %[IN],   72 \n"
        "s32i.n  a3,  %[sb],    8 \n"

        "s32i.n    a2, %[sb], 12 \n" /* Nonce */
        "s32i.n    %[pad2], %[sb], 16 \n" /* Termininating bit */
        "s32i.n    %[len1], %[sb], 60 \n" /* Bit length */

        // Zero sb[20..56]
        "movi.n  a4,  0            \n" 

        "addi    a8, %[sb], 20  \n"  /* ptr = &sb[5] */
        "movi.n  a3, 10         \n"  /* 10 words */

        "loop    a3, 1f         \n"
        "s32i.n  a4, a8, 0      \n"
        "addi.n  a8, a8, 4      \n"

      "1:\n"
        /* 3) busy-wait */
        "l32i.n  a3, a5, 12 \n" /* 0x9C */
        "bnez.n  a3, 1b\n"

        /* Continue hash */
        "movi.n  a3, 1\n"
        "s32i.n  a3, a5, 4\n" /* 0x94 */
        "memw \n"

      "2:\n"
        /* 3) busy-wait */
        "l32i.n  a4, a5, 12 \n" /* 0x9C */
        "bnez.n  a4, 2b\n"

        /* Do load */
        "movi.n  a4, 1\n"
        "s32i.n  a4, a5, 8\n"  

        "memw \n"

        "addi.n     a2, a2, 1\n"        /* Increment nonce */
        

      "3:\n"
        /* 3) busy-wait */
        "l32i.n  a4, a5, 12\n"
        "bnez.n  a4, 3b\n" 

        /* Set terminating bit and length */
        "s32i.n   %[pad2], %[sb], 32 \n"
        "s32i.n   %[len2], %[sb], 60 \n"        

        /* 1) start SHA */
        "movi.n  a4, 1\n"
        "s32i.n  a4, a5, 0\n"

        "memw\n"
       
        /* 2) internalHashes++ (64-bit) */
        "l32i.n  a3, %[ih], 0\n"   /* load low */
        "addi.n  a3, a3, 1  \n"
        "s32i.n  a3, %[ih], 0\n"   /* store low */
     
        "bnez.n  a3, 4f\n"         /* if low≠0 skip high */
        "l32i.n  a4, %[ih], 4\n"   /* load high */
        "addi.n  a4, a4, 1\n"
        "s32i.n  a4, %[ih], 4\n"   /* store high */

      "4:\n"
        /* 3) busy-wait */
        "l32i.n  a4, a5, 12\n"
        "bnez.n  a4, 4b\n" 

        /* Do load */
        "movi.n  a3, 1\n"
        "s32i.n  a3, a5, 8\n" /* 0x98\ */  

        "memw \n"

      "5:\n"
        /* 3) busy-wait */
        "l32i.n  a4, a5, 12\n"
        "bnez.n  a4, 5b\n" 

        /* bail on isMining if false */
        "l8ui   a3, %[flag], 0     \n"
        "beqz.n a3, proc_end     \n"

        /* early-continue if (sb_buf[7]&0xFFFF)!=0 */
        "l16ui  a3, %[sb], 28         \n"
        "beqz.n a3, proc_end          \n"
        "j proc_start                 \n"

      "proc_end:\n"

        "s32i.n    a2, %[nonce], 0\n" /* Put the nonce back in the hash block */
        :
        : [sb] "r"(sha_base),
          [IN] "r"(data),
          [ih] "r" (&monitorData.internalHashes),
          [nonce] "r"(&hb.nonce),
          [flag] "r"(&isMining),
          [pad2]  "r" (shaPad),
          [len2]  "r" (secondShaBitLen),
          [len1] "r" (firstShaBitLen)
        : "a2", "a3", "a4", "a5", "a8", "memory"
      );
      
      // See if we have a hash worth checking
      if( (sha_base[7] & 0xffff) != 0 ) continue; 

      // I had some issues where the sha module would become disabled
      // likely due to power management by the CPU
      // so I started validating that it's still running before 
      // I even think about submitting anything. No real performance penalty
      // so better safe than sorry.
      bool shaEnabled = DPORT_REG_READ(DPORT_PERI_CLK_EN_REG) & DPORT_PERI_EN_SHA;
      if( ! shaEnabled ) {
        dbg("*************** SHA module not enabled. **********************\n");
        INIT_HARDWARE_SHA256
        continue;
      }
      
      // We really shouldn't see a bad hash, but better safe that sorry
      hbCheck.nonce = BYTESWAP32(hb.nonce - 1);
      if( sha256header(&midstate, &ctx, &hbCheck) ) {
        hashCheck(jobId, &ctx, hbCheck.timestamp, hbCheck.nonce);
      } else {
        dbg("Invalid hash\n");
        INIT_HARDWARE_SHA256
      }
      
    }
    core1Mining = false;
    
    // Leave this delay in case we're not mining
    vTaskDelay(20 / portTICK_PERIOD_MS);
    
  }
  
}
