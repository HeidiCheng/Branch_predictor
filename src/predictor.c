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

// gshare
uint32_t g_history_reg;
// uint8_t *g_PHT; //globalPHT

// tournament predictor
uint8_t *globalPHT = NULL;
uint32_t *localLHT = NULL; 
uint8_t *localLPT = NULL;   
uint8_t *choicePredictor = NULL; 

// tage predictor
typedef struct {
  int8_t counter; 
  uint16_t tag; 
  uint8_t usage; 
} Entry; 

Entry *taggedPT_0 = NULL; 
Entry *taggedPT_1 = NULL; 
Entry *taggedPT_2 = NULL; 

// custom
uint32_t totalWeights = 20; 
uint32_t weight = 5; 

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
  // init for gshare
  globalPHT = (uint8_t*)malloc(sizeof(uint8_t)*gpSize); 
  memset(globalPHT, 0, sizeof(uint8_t)*gpSize);
  g_history_reg = 0;

  // Init local predictor
  localLHT = (uint32_t*)malloc(sizeof(uint32_t)*lhSize);
  localLPT = (uint8_t*)malloc(sizeof(uint8_t)*lpSize);
  // init local history to not taken
  memset(localLHT, 0, sizeof(uint32_t)*lhSize); 
  memset(localLPT, 0, sizeof(uint8_t)*lpSize); 

  // Init chioce predictor
  choicePredictor = (uint8_t*)malloc(sizeof(uint8_t)*gpSize); 
  // Prefer to choose result from global predictor in the beginning
  memset(choicePredictor, 0, sizeof(uint8_t)*gpSize); 

  // Init custom predictor 
  taggedPT_0 = malloc(TAG_CONST * sizeof(Entry)); 
  taggedPT_1 = malloc(TAG_CONST * 2 * sizeof(Entry)); 
  taggedPT_2 = malloc(TAG_CONST * 4 * sizeof(Entry)); 
  memset(taggedPT_0, 0, TAG_CONST * sizeof(Entry)); 
  memset(taggedPT_1, 0, TAG_CONST * 2 * sizeof(Entry)); 
  memset(taggedPT_2, 0, TAG_CONST * 4 * sizeof(Entry)); 
}

// Local Prediction
uint8_t local_prediction(uint32_t pc) {
  if(pcIndexBits != 32) {
    uint32_t pcMask = (1 << pcIndexBits) - 1;
    pc &= pcMask; 
  }
  uint32_t indexLPT = localLHT[pc]; 
  // Use the first bit as the prediction
  uint8_t localPrediction = localLPT[indexLPT] & 2;  
  return localPrediction == 2; 
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

uint32_t choice_get_index() {
  uint32_t table_entries = 1 << ghistoryBits;
  // use & to get n LSBs
  uint32_t c_index = g_history_reg & (table_entries - 1);
  return c_index;
}

uint8_t tournament_prediction(uint32_t pc) {
  uint8_t choice = choicePredictor[choice_get_index()] & 2;
  if(choice == 1) { 
    return local_prediction(pc); 
  } 
  return gshare_prediction(pc); 
}



// TAGE branch predictor
uint32_t get_num_of_digits(uint32_t num) {
  uint32_t count = 0; 
  while(num != 0) {
    count++; 
    num /= 10; 
  }
  return count; 
} 

uint32_t get_folded(uint32_t num, uint32_t numOfDigits){
  uint32_t result = 0; 
  uint32_t tmp = 0; 
  uint32_t count = 0; 
  while(num != 0) {
    tmp += (num%10) * pow(10, count); 
    num /= 10; 
    count++; 
    if(count == numOfDigits){
      result += tmp; 
      tmp = 0; 
      count = 0;
    }
  }
  uint32_t upperBound = pow(10, numOfDigits) - 1; 
  while(result > upperBound) {
    result /= 10; 
  }
  return result; 
}

uint32_t fold_hash(uint32_t h, uint32_t m) {
  uint32_t numOfDigits = get_num_of_digits(m); 
  uint32_t foldedNum = get_folded(h, numOfDigits); 
  return foldedNum % m; 
}

uint8_t get_tag_index(uint8_t g_bits, uint32_t pc) {
  uint32_t tableSize = pow(2, g_bits); 
  uint32_t hashed = fold_hash(g_history_reg, tableSize); 
  uint32_t tableEntries = (1 << g_bits) - 1;
  // TODO: Not complete
  return (pc ^ hashed) & tableEntries; 
}

uint8_t get_tag(uint8_t g_bits, uint32_t pc) {
  uint32_t tableSize = pow(2, g_bits); 
  uint32_t hashed = fold_hash(g_history_reg, tableSize); 
  uint32_t hashedShift = fold_hash(g_history_reg, tableSize - 1) << 1; 
  uint32_t tableEntries = (1 << g_bits) - 1;
  return (pc ^ hashed ^ hashedShift) & tableEntries; 
}

uint8_t tage_prediction(uint32_t pc) {
  uint32_t index0 = get_tag_index(TAG_CONST, pc); 
  uint32_t index1 = get_tag_index(2 * TAG_CONST, pc);
  uint32_t index2 = get_tag_index(4 * TAG_CONST, pc);

  return TAKEN; 
}

uint8_t custom_prediction(uint32_t pc) {

  uint8_t choice = choicePredictor[gshare_get_index(pc)] & 2;
  if(choice == 1) { 
    return local_prediction(pc); 
  } 
  return gshare_prediction(pc); 
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
      return custom_prediction(pc);
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

void train_local(uint32_t pc, uint8_t outcome) {
  uint32_t pcMask = 0; 
  if(pcIndexBits != 32) {
    uint32_t pcMask = (1 << pcIndexBits) - 1;
    pc &= pcMask; 
  }
  uint32_t localUpdate = localLHT[pc]; 
  localUpdate = localUpdate << 1; 
  localUpdate |= outcome;   
  localLHT[pc] = localUpdate & pcMask; 

  uint32_t localHistory = localLHT[pc]; 
  if(outcome == TAKEN && localLPT[localHistory] != ST){
    localLPT[localHistory]++; 
  }
  if(outcome == NOTTAKEN && localLPT[localHistory] != SN) {
    localLPT[localHistory]--; 
  }
}

void train_choice(uint32_t pc, uint8_t outcome) {
  uint32_t index = choice_get_index(); 
  uint8_t choice = choicePredictor[index] & 2;
  uint8_t localPrediction = local_prediction(pc); 
  uint8_t globalPrediction = gshare_prediction(pc); 
  if(choice == 0 && localPrediction != outcome && globalPrediction == outcome) {
    choicePredictor[index]++; 
  }else if(choice == 1 && localPrediction == outcome && globalPrediction != outcome) {
    choicePredictor[index]--; 
  }
}

void train_tournament(uint32_t pc, uint8_t outcome) {
  // Update tournament predictor
  // 1. local predictor
  train_local(pc, outcome); 

  // 2. global predictor
  train_gshare(pc, outcome); 

  // 3. choice predictor
  train_choice(pc, outcome); 
 
}

void train_choice_custom(uint32_t pc, uint8_t outcome) {
  uint32_t index = gshare_get_index(pc);
  uint8_t choice = choicePredictor[index] & 2;
  uint8_t localPrediction = local_prediction(pc); 
  uint8_t globalPrediction = gshare_prediction(pc); 
  if(choice == 0 && localPrediction != outcome && globalPrediction == outcome) {
    choicePredictor[index]++; 
  }else if(choice == 1 && localPrediction == outcome && globalPrediction != outcome) {
    choicePredictor[index]--; 
  }
}

void train_custom(uint32_t pc, uint8_t outcome) {
  // 1. local predictor
  train_local(pc, outcome); 

  // 2. global predictor
  train_gshare(pc, outcome); 

  // 3. choice predictor
  train_choice_custom(pc, outcome);
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
      train_custom(pc, outcome);
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
