//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "NAME";
const char *studentID   = "PID";
const char *email       = "EMAIL";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here
//

// Data Structures for Tournament branch predictor
struct GlobalPredictor {
  uint32_t historyRegister; 
  // TODO: Find a way to use only two bits
  // Global Prediction Table: 4096 x 2 bits
  // uint8_t pht[4096]; // Pattern History Table
  uint8_t *pht; 
}; 

struct LocalPredictor {
  // TODO: Find a way to use only 10 bits
  // uint16_t lht[1024]; // Local History Table
  uint32_t *lht; 
  // TODO: Find a way to use only 3 bits
  // uint8_t lpt[1024]; // Local Prediction Table
  uint8_t *lpt; 
}; 

// gshare
uint32_t g_history_reg;
// uint8_t *g_PHT; //globalPHT

// tournament predictor
uint32_t globalHistoryRegister; 
uint8_t *globalPHT = NULL;
uint32_t *localLHT = NULL; 
uint8_t *localLPT = NULL;   
uint8_t *choicePredictor = NULL; 

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void
init_predictor()
{
  //
  //TODO: Initialize Branch Predictor Data Structures
  //
  size_t gpSize = pow(2, ghistoryBits); 
  size_t lhSize = pow(2, pcIndexBits); 
  size_t lpSize = pow(2, lhistoryBits); 
  // Init global predictor
  globalPHT = (uint8_t*)malloc(sizeof(uint8_t)*gpSize); 
  memset(globalPHT, 0, sizeof(uint8_t)*gpSize);
  // globalHistoryRegister = ghistoryBits == 32 ? 0 : (1 << ghistoryBits) - 1; 
  globalHistoryRegister = 0;
  
  // init for gshare
  g_history_reg = 0;


  // Init local predictor
  localLHT = (uint32_t*)malloc(sizeof(uint32_t)*lhSize);
  localLPT = (uint8_t*)malloc(sizeof(uint8_t)*lpSize);
  // init local history to not taken
  memset(localLHT, 0, sizeof(uint32_t)*lhSize); 
  memset(localLPT, 0, sizeof(uint8_t)*lpSize); 

  // Init chioce predictor
  choicePredictor = (uint8_t*)malloc(sizeof(uint8_t)*gpSize); 
  // Prefer to choose result from local predictor in the beginning
  memset(choicePredictor, 0, sizeof(uint8_t)*gpSize); 
}

// Local Prediction
uint8_t local_prediction(uint32_t pc) {
  if(pcIndexBits != 32) {
    uint32_t pcMask = (1 << pcIndexBits) - 1;
    //printf("PC: %d, pcMask: %d\n", pc, pcMask); 
    pc &= pcMask; 
  }
  uint32_t indexLPT = localLHT[pc]; 
  // printf("indexLPT: %d\n", indexLPT); 
  // Use the first bit as the prediction
  uint8_t localPrediction = localLPT[indexLPT] & 2; 
  // printf("indexLPT: %d\n", indexLPT); 
  
  return localPrediction == 2; 
}

// Global Prediction
uint8_t global_prediction() {
  uint8_t globalPrediction = globalPHT[globalHistoryRegister] & 2; 
  return globalPrediction == 2; 
}

uint8_t tournament_prediction(uint32_t pc) {
  uint8_t choice = choicePredictor[globalHistoryRegister] & 2;
  // printf("Chioce: %d\n", choice); 
  if(choice == 0) { 
    return local_prediction(pc); 
  } 
  return global_prediction(); 
}

uint32_t gshare_get_index(uint32_t pc) {
  // XOR n-bit GHR and n-bit pc
  uint32_t g_xor = (g_history_reg ^ pc);
  uint32_t table_entries = 1 << ghistoryBits;
  // use & to get n LSBs
  uint32_t g_index = g_xor & (table_entries - 1);
  return g_index;
}

// GSHARE branch predictor
uint8_t gshare_prediction(uint32_t pc) {
  uint32_t g_index = gshare_get_index(pc);
  uint8_t g_prediction = globalPHT[g_index];
  return (g_prediction >= 2) ? 1 : 0;
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
uint8_t
make_prediction(uint32_t pc)
{
  //
  //TODO: Implement prediction scheme
  //

  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      return gshare_prediction(pc);
    case TOURNAMENT:
      return tournament_prediction(pc); 
    case CUSTOM:
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

void train_gshare(uint32_t pc, uint8_t outcome) {
  uint32_t g_index = gshare_get_index(pc);
  uint8_t g_prediction = globalPHT[g_index];
  // updates history register with outcome
  g_history_reg = g_history_reg << 1;
  g_history_reg = g_history_reg | outcome;
  // update prediction based on the actual outcome
  if (outcome == 0 && g_prediction != SN) {
    // if not strong not taken
    globalPHT[g_index]--;
  }
  if (outcome == 1 && g_prediction != ST) {
    globalPHT[g_index]++;
  }
}

void train_tournament(uint32_t pc, uint8_t outcome) {
  // Update tournament predictor
  // 1. local predictor
  if(pcIndexBits != 32) {
    uint32_t pcMask = (1 << pcIndexBits) - 1;
    pc &= pcMask; 
  }
  localLHT[pc] = localLHT[pc] >> 1; 
  // Set the first bit to 1 when outcome is TAKEN
  if(outcome == TAKEN){
    localLHT[pc] = localLHT[pc] | (1 << (pcIndexBits - 1)); 
  }
  uint32_t localHistory = localLHT[pc]; 
  if(outcome == TAKEN && localLPT[localHistory] != ST){
    localLPT[localHistory]++; 
  }
  if(outcome == NOTTAKEN && localLPT[localHistory] != SN) {
    localLPT[localHistory]--; 
  }

  // 2. global predictor
  globalHistoryRegister = globalHistoryRegister >> 1; 
  if(outcome == TAKEN) {
    globalHistoryRegister = globalHistoryRegister | (1 << (ghistoryBits - 1)); 
  }
  if(outcome == TAKEN && globalPHT[globalHistoryRegister] != ST){
    globalPHT[globalHistoryRegister]++; 
  }
  if(outcome == NOTTAKEN && globalPHT[globalHistoryRegister] != SN) {
    globalPHT[globalHistoryRegister]--; 
  }

  // 3. choice predictor
  uint8_t choice = choicePredictor[globalHistoryRegister] & 2;
  uint8_t localPrediction = local_prediction(pc); 
  uint8_t globalPrediction = global_prediction(); 
  if(choice == 0 && localPrediction != outcome && globalPrediction == outcome) {
    choicePredictor[globalHistoryRegister]++; 
  }else if(choice == 1 && localPrediction == outcome && globalPrediction != outcome) {
    choicePredictor[globalHistoryRegister]--; 
  }
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint8_t outcome)
{
  //
  //TODO: Implement Predictor training
  //
  switch (bpType) {
    case GSHARE:
      train_gshare(pc, outcome); 
      return;
    case TOURNAMENT:
      train_tournament(pc, outcome); 
    case CUSTOM:
    default:
      break;
  }

  
}

void clean_predictor() {
  if(globalPHT != NULL) {
    free(globalPHT); 
  }
  if(localLHT != NULL) {
    free(localLHT); 
  }
  if(localLPT != NULL) {
    free(localLPT); 
  }
  if(choicePredictor != NULL){
    free(choicePredictor); 
  }
}
