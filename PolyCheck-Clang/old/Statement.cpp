#include <Statement.hpp>
#include <iostream>
#include <isl/aff.h>
#include <isl/constraint.h>

using namespace std;

Statement::Statement(isl_ctx *ctx_in) {
  ctx = ctx_in;
  stmtVarIds = new vector<string>();
}

Statement::~Statement() {
  delete stmtVarIds;
}

void Statement::setCtx(isl_ctx* ctx) {
  this->ctx = ctx;
}
isl_ctx* Statement::getCtx() {
  return ctx;
}

void Statement::setDomainSet(isl_set* domainSet) {
  this->domainSet = domainSet;
}
isl_set* Statement::getDomainSet() {
  return domainSet;
}

void Statement::setReadReferenceMaps(vector<isl_map*> read_referenceMaps) {
  this->read_referenceMaps = read_referenceMaps;
}
vector<isl_map*> Statement::getReadReferenceMaps() {
  return read_referenceMaps;
}
void Statement::addToReadReferenceMaps (isl_map* read_referenceMap) {
  (this->read_referenceMaps).push_back(read_referenceMap);
}

void Statement::setWriteReferenceMaps(isl_map* write_referenceMap) {
  this->write_referenceMap = write_referenceMap;
}
isl_map* Statement::getWriteReferenceMap() {
  return write_referenceMap;
}

void Statement::setScheduleMap(isl_map* scheduleMap) {
  this->scheduleMap = scheduleMap;
}
isl_map *Statement::getScheduleMap() {
  return scheduleMap;
}

vector<string> Statement::getIterators() {
  return iterators;
}
void Statement::setIterators(vector<string> inIterators) {
  this->iterators = inIterators;
}

string* Statement::GetDebugCode() {
  return debugCode;
}
void Statement::SetDebugCode(string* inDebugCode) {
  this->debugCode = inDebugCode;
}

isl_id* Statement::getId() {
  return Id;
}
void Statement::setId(isl_id* stmtId) {
  this->Id = stmtId;
}

string Statement::getWriteArrayName() {
  return writeArrayName;
}
void Statement::setWriteArrayName(string writeArray) {
  writeArrayName = writeArray;
}

int Statement::getStatementNumber() {
  return stmtNum;
}
void Statement::setStatementNumber(int num) {
  stmtNum = num;
}

vector<int>* Statement::getIndex() {
  return index;
}
void Statement::setIndex(vector<int>* inIndex) {
  this->index = inIndex;
}

vector<string>* Statement::getStmtVarIds() {
  return stmtVarIds;
}
void Statement::setStmtVarIds(vector<string>* inStmtVarIds) {
  this->stmtVarIds = inStmtVarIds;
}
