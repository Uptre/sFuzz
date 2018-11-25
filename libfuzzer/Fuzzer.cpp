#include <thread>
#include <unistd.h>
#include <fstream>
#include "boost/filesystem.hpp"
#include "Fuzzer.h"
#include "Mutation.h"
#include "Util.h"
#include "ContractABI.h"
#include "Dictionary.h"

using namespace dev;
using namespace eth;
using namespace std;
using namespace fuzzer;

/* Setup virgin byte to 255 */
Fuzzer::Fuzzer(FuzzParam fuzzParam): fuzzParam(fuzzParam){
  fuzzStat.idx = 0;
  fuzzStat.totalExecs = 0;
  fuzzStat.clearScreen = false;
  fuzzStat.queueCycle = 0;
  fuzzStat.coveredTuples = 0;
  fuzzStat.maxdepth = 0;
  fuzzStat.numTest = 0;
  fuzzStat.numException = 0;
  fill_n(fuzzStat.stageFinds, 32, 0);
}

/* Detect new exception */
u8 Fuzzer::hasNewExceptions(unordered_map<string, unordered_set<u64>> uexps) {
  int orginExceptions = 0;
  int newExceptions = 0;
  for (auto it : uniqExceptions) orginExceptions += it.second.size();
  for (auto it : uexps) {
    if (!uniqExceptions.count(it.first)) {
      uniqExceptions[it.first] = it.second;
    } else {
      for (auto v : it.second) {
        uniqExceptions[it.first].insert(v);
      }
    }
  }
  for (auto it : uniqExceptions) newExceptions += it.second.size();
  return newExceptions - orginExceptions;
}
/* Detect new branch by comparing tracebits to virginbits */
u8 Fuzzer::hasNewBits(unordered_set<uint64_t> singleTracebits) {
  auto originSize = tracebits.size();
  for (auto it : singleTracebits) tracebits.insert(it);
  auto newSize = tracebits.size();
  return newSize - originSize;
}

void Fuzzer::showStats(Mutation mutation, CFG cfg) {
  int numLines = 19, i = 0;
  if (!fuzzStat.clearScreen) {
    for (i = 0; i < numLines; i++) cout << endl;
    fuzzStat.clearScreen = true;
  }
  int totalTuples = cfg.totalCount();
  double duration = timer.elapsed();
  double fromLastNewPath = timer.elapsed() - fuzzStat.lastNewPath;
  for (i = 0; i < numLines; i++) cout << "\x1b[A";
  auto nowTrying = padStr(mutation.stageName, 20);
  auto stageExecProgress = to_string(mutation.stageCur) + "/" + to_string(mutation.stageMax);
  auto stageExecPercentage = to_string((int)((float) (mutation.stageCur) / mutation.stageMax * 100));
  auto stageExec = padStr(stageExecProgress + " (" + stageExecPercentage + "%)", 20);
  auto allExecs = padStr(to_string(fuzzStat.totalExecs), 20);
  auto execSpeed = padStr(to_string((int)(fuzzStat.totalExecs / duration)), 20);
  auto cyclePercentage = (int)((float)(fuzzStat.idx + 1) / queues.size() * 100);
  auto cycleProgress = padStr(to_string(fuzzStat.idx + 1) + " (" + to_string(cyclePercentage) + "%)", 20);
  auto cycleDone = padStr(to_string(fuzzStat.queueCycle), 15);
  int expCout = 0;
  for (auto exp: uniqExceptions) expCout+= exp.second.size();
  auto exceptionCount = padStr(to_string(expCout), 15);
  auto tupleCountMethod = cfg.extraEstimation > 0 ? "Est" : "Exa";
  auto coveredTuplePercentage = totalTuples ? to_string((int)((float)fuzzStat.coveredTuples/ totalTuples * 100)) + "% " : "n/a ";
  auto coveredTupleStr = padStr(to_string(fuzzStat.coveredTuples) + " (" + coveredTuplePercentage + tupleCountMethod + ")", 15);
  auto typeExceptionCount = padStr(to_string(uniqExceptions.size()), 15);
  auto tupleSpeed = fuzzStat.coveredTuples ? mutation.dataSize * 8 / fuzzStat.coveredTuples : mutation.dataSize * 8;
  auto bitPerTupe = padStr(to_string(tupleSpeed) + " bits", 15);
  auto flip1 = to_string(fuzzStat.stageFinds[STAGE_FLIP1]) + "/" + to_string(mutation.stageCycles[STAGE_FLIP1]);
  auto flip2 = to_string(fuzzStat.stageFinds[STAGE_FLIP2]) + "/" + to_string(mutation.stageCycles[STAGE_FLIP2]);
  auto flip4 = to_string(fuzzStat.stageFinds[STAGE_FLIP4]) + "/" + to_string(mutation.stageCycles[STAGE_FLIP4]);
  auto bitflip = padStr(flip1 + ", " + flip2 + ", " + flip4, 30);
  auto byte1 = to_string(fuzzStat.stageFinds[STAGE_FLIP8]) + "/" + to_string(mutation.stageCycles[STAGE_FLIP8]);
  auto byte2 = to_string(fuzzStat.stageFinds[STAGE_FLIP16]) + "/" + to_string(mutation.stageCycles[STAGE_FLIP16]);
  auto byte4 = to_string(fuzzStat.stageFinds[STAGE_FLIP32]) + "/" + to_string(mutation.stageCycles[STAGE_FLIP32]);
  auto byteflip = padStr(byte1 + ", " + byte2 + ", " + byte4, 30);
  auto arith1 = to_string(fuzzStat.stageFinds[STAGE_ARITH8]) + "/" + to_string(mutation.stageCycles[STAGE_ARITH8]);
  auto arith2 = to_string(fuzzStat.stageFinds[STAGE_ARITH16]) + "/" + to_string(mutation.stageCycles[STAGE_ARITH16]);
  auto arith4 = to_string(fuzzStat.stageFinds[STAGE_ARITH32]) + "/" + to_string(mutation.stageCycles[STAGE_ARITH32]);
  auto arithmetic = padStr(arith1 + ", " + arith2 + ", " + arith4, 30);
  auto int1 = to_string(fuzzStat.stageFinds[STAGE_INTEREST8]) + "/" + to_string(mutation.stageCycles[STAGE_INTEREST8]);
  auto int2 = to_string(fuzzStat.stageFinds[STAGE_INTEREST16]) + "/" + to_string(mutation.stageCycles[STAGE_INTEREST16]);
  auto int4 = to_string(fuzzStat.stageFinds[STAGE_INTEREST32]) + "/" + to_string(mutation.stageCycles[STAGE_INTEREST32]);
  auto knownInts = padStr(int1 + ", " + int2 + ", " + int4, 30);
  auto dict1 = to_string(fuzzStat.stageFinds[STAGE_EXTRAS_UO]) + "/" + to_string(mutation.stageCycles[STAGE_EXTRAS_UO]);
  auto dictionary = padStr(dict1, 30);
  auto hav1 = to_string(fuzzStat.stageFinds[STAGE_HAVOC]) + "/" + to_string(mutation.stageCycles[STAGE_HAVOC]);
  auto havoc = padStr(hav1, 30);
  auto random1 = to_string(fuzzStat.stageFinds[STAGE_RANDOM]) + "/" + to_string(mutation.stageCycles[STAGE_RANDOM]);
  auto random = padStr(random1, 30);
  auto pending = padStr(to_string(queues.size() - fuzzStat.idx - 1), 5);
  auto fav = count_if(queues.begin() + fuzzStat.idx + 1, queues.end(), [](FuzzItem item) {
    return !item.wasFuzzed;
  });
  auto pendingFav = padStr(to_string(fav), 5);
  auto maxdepthStr = padStr(to_string(fuzzStat.maxdepth), 5);
  printf(cGRN Bold "%sAFL Solidity v0.0.1 (%s)" cRST "\n", padStr("", 10).c_str(), fuzzParam.fuzzContract.contractName.substr(0, 20).c_str());
  printf(bTL bV5 cGRN " processing time " cRST bV20 bV20 bV5 bV2 bV2 bV5 bV bTR "\n");
  printf(bH "      run time : %s " bH "\n", formatDuration(duration).data());
  printf(bH " last new path : %s " bH "\n",formatDuration(fromLastNewPath).data());
  printf(bLTR bV5 cGRN " stage progress " cRST bV5 bV10 bV2 bV bTTR bV2 cGRN " overall results " cRST bV2 bV5 bV2 bV2 bV bRTR "\n");
  printf(bH "  now trying : %s" bH " cycles done : %s" bH "\n", nowTrying.c_str(), cycleDone.c_str());
  printf(bH " stage execs : %s" bH "      tuples : %s" bH "\n", stageExec.c_str(), coveredTupleStr.c_str());
  printf(bH " total execs : %s" bH " except type : %s" bH "\n", allExecs.c_str(), typeExceptionCount.c_str());
  printf(bH "  exec speed : %s" bH "  bit/tuples : %s" bH "\n", execSpeed.c_str(), bitPerTupe.c_str());
  printf(bH "  cycle prog : %s" bH " uniq except : %s" bH "\n", cycleProgress.c_str(), exceptionCount.c_str());
  printf(bLTR bV5 cGRN " fuzzing yields " cRST bV5 bV5 bV5 bV2 bV bBTR bV10 bV bTTR bV cGRN " path geometry " bV2 bV2 cRST bRTR "\n");
  printf(bH "   bit flips : %s" bH "     pending : %s" bH "\n", bitflip.c_str(), pending.c_str());
  printf(bH "  byte flips : %s" bH " pending fav : %s" bH "\n", byteflip.c_str(), pendingFav.c_str());
  printf(bH " arithmetics : %s" bH "   max depth : %s" bH "\n", arithmetic.c_str(), maxdepthStr.c_str());
  printf(bH "  known ints : %s" bH "                    " bH "\n", knownInts.c_str());
  printf(bH "  dictionary : %s" bH "                    " bH "\n", dictionary.c_str());
  printf(bH "       havoc : %s" bH "                    " bH "\n", havoc.c_str());
  printf(bH "      random : %s" bH "                    " bH "\n", random.c_str());
  printf(bBL bV50 bV5 bV bBTR bV20 bV2 bV2 bBR "\n");
}

void Fuzzer::writeStats(Mutation, CFG cfg) {
  double speed = fuzzStat.totalExecs / (double) timer.elapsed();
  ofstream stats(fuzzParam.fuzzContract.contractName + "/stats.csv");
  stats << "Testcases,Speed,Execs,Tuples,Coverage" << endl;
  stats << queues.size();
  stats << "," << speed;
  stats << "," << fuzzStat.totalExecs;
  stats << "," << fuzzStat.coveredTuples;
  stats << "," << fuzzStat.coveredTuples * 100 / cfg.totalCount();
  stats << endl;
  stats.close();
}

void Fuzzer::writeTestcase(bytes data) {
  ContractABI ca(fuzzParam.fuzzContract.abiJson);
  ca.updateTestData(data);
  fuzzStat.numTest ++;
  string ret = ca.toStandardJson();
  ofstream test(fuzzParam.fuzzContract.contractName + "/__TEST__" + to_string(fuzzStat.numTest) + "__.json");
  test << ret;
  test.close();
}

void Fuzzer::writeException(bytes data) {
  ContractABI ca(fuzzParam.fuzzContract.abiJson);
  ca.updateTestData(data);
  fuzzStat.numException ++;
  string ret = ca.toStandardJson();
  ofstream exp(fuzzParam.fuzzContract.contractName + "/__EXCEPTION__" + to_string(fuzzStat.numException) + "__.json");
  exp << ret;
  exp.close();
}
/* Save data if interest */
FuzzItem Fuzzer::saveIfInterest(TargetContainer& container, bytes data, int depth) {
  auto revisedData = ContractABI::preprocessTestData(data);
  FuzzItem item(revisedData);
  item.res = container.exec(revisedData);
  item.wasFuzzed = false;
  fuzzStat.totalExecs ++;
  if (hasNewBits(item.res.tracebits)) {
    if (depth + 1 > fuzzStat.maxdepth) fuzzStat.maxdepth = depth + 1;
    item.depth = depth + 1;
    queues.push_back(item);
    fuzzStat.lastNewPath = timer.elapsed();
    fuzzStat.coveredTuples = tracebits.size();
    writeTestcase(revisedData);
  }
  if (hasNewExceptions(item.res.uniqExceptions)) {
    writeException(revisedData);
  }
  return item;
}

/* Start fuzzing */
void Fuzzer::start() {
  auto fuzzContract = fuzzParam.fuzzContract;
  boost::filesystem::remove_all(fuzzContract.contractName);
  boost::filesystem::create_directory(fuzzContract.contractName);
  Dictionary dict(fromHex(fuzzContract.bin));
  ContractABI ca(fuzzContract.abiJson);
  CFG cfg(fuzzContract.bin, fuzzContract.binRuntime);
  TargetContainer container(fromHex(fuzzContract.bin), ca);
  /* Load assets */
  for (auto assetInfo : fuzzParam.assetContracts) {
    ContractABI assetCa(assetInfo.abiJson);
    container.loadAsset(fromHex(assetInfo.bin), assetCa);
  }
  /* First test case */
  timer.restart();
  saveIfInterest(container, ca.randomTestcase(), 0);
  int origHitCount = queues.size();
  double lastSpeed = 0;
  int refreshRate = 100;
  while (true) {
    FuzzItem curItem = queues[fuzzStat.idx];
    Mutation mutation(curItem, dict);
    auto save = [&](bytes data) {
      auto item = saveIfInterest(container, data, curItem.depth);
      double speed = fuzzStat.totalExecs / (double) timer.elapsed();
      if (speed > lastSpeed) {
        refreshRate += 10;
        lastSpeed = speed;
      }
      if (refreshRate > speed) refreshRate = speed;
      if (fuzzStat.totalExecs % refreshRate == 0) showStats(mutation, cfg);
      /* Stop program */
      if (timer.elapsed() > fuzzParam.duration) {
        writeStats(mutation, cfg);
        exit(0);
      }
      return item;
    };
    switch (fuzzParam.mode) {
      case AFL: {
        if (!curItem.wasFuzzed) {
          mutation.singleWalkingBit(save);
          fuzzStat.stageFinds[STAGE_FLIP1] += queues.size() - origHitCount;
          origHitCount = queues.size();
          mutation.twoWalkingBit(save);
          fuzzStat.stageFinds[STAGE_FLIP2] += queues.size() - origHitCount;
          origHitCount = queues.size();
          mutation.fourWalkingBit(save);
          fuzzStat.stageFinds[STAGE_FLIP4] += queues.size() - origHitCount;
          origHitCount = queues.size();
          mutation.singleWalkingByte(save);
          fuzzStat.stageFinds[STAGE_FLIP8] += queues.size() - origHitCount;
          origHitCount = queues.size();
          mutation.twoWalkingByte(save);
          fuzzStat.stageFinds[STAGE_FLIP16] += queues.size() - origHitCount;
          origHitCount = queues.size();
          mutation.fourWalkingByte(save);
          fuzzStat.stageFinds[STAGE_FLIP32] += queues.size() - origHitCount;
          origHitCount = queues.size();
          mutation.singleArith(save);
          fuzzStat.stageFinds[STAGE_ARITH8] += queues.size() - origHitCount;
          origHitCount = queues.size();
          mutation.twoArith(save);
          fuzzStat.stageFinds[STAGE_ARITH16] += queues.size() - origHitCount;
          origHitCount = queues.size();
          mutation.fourArith(save);
          fuzzStat.stageFinds[STAGE_ARITH32] += queues.size() - origHitCount;
          origHitCount = queues.size();
          mutation.singleInterest(save);
          fuzzStat.stageFinds[STAGE_INTEREST8] += queues.size() - origHitCount;
          origHitCount = queues.size();
          mutation.twoInterest(save);
          fuzzStat.stageFinds[STAGE_INTEREST16] += queues.size() - origHitCount;
          origHitCount = queues.size();
          mutation.fourInterest(save);
          fuzzStat.stageFinds[STAGE_INTEREST32] += queues.size() - origHitCount;
          origHitCount = queues.size();
          mutation.overwriteWithDictionary(save);
          fuzzStat.stageFinds[STAGE_EXTRAS_UO] += queues.size() - origHitCount;
          origHitCount = queues.size();
          mutation.havoc(tracebits, save);
          fuzzStat.stageFinds[STAGE_HAVOC] += queues.size() - origHitCount;
          origHitCount = queues.size();
          queues[fuzzStat.idx].wasFuzzed = true;
        } else {
          mutation.havoc(tracebits, save);
          fuzzStat.stageFinds[STAGE_HAVOC] += queues.size() - origHitCount;
          origHitCount = queues.size();
          if (mutation.splice(queues)) {
            mutation.havoc(tracebits, save);
            fuzzStat.stageFinds[STAGE_HAVOC] += queues.size() - origHitCount;
            origHitCount = queues.size();
          };
        }
        break;
      }
      case RANDOM: {
        mutation.random(save);
        fuzzStat.stageFinds[STAGE_RANDOM] += queues.size() - origHitCount;
        origHitCount = queues.size();
        break;
      }
    }
    fuzzStat.idx = (fuzzStat.idx + 1) % queues.size();
    if (fuzzStat.idx == 0) fuzzStat.queueCycle ++;
  }
}
