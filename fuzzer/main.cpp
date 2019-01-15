#include <iostream>
#include <vector>
#include <libfuzzer/Fuzzer.h>
#include "Utils.h"

using namespace std;
using namespace fuzzer;

static int DEFAULT_MODE = AFL;
static int DEFAULT_DURATION = 120; // 2 mins
static int DEFAULT_REPORTER = CSV_FILE;
static int DEFAULT_CSV_INTERVAL = 5; // 5 sec
static int DEFAULT_LOG_OPTION = 0;
static string DEFAULT_CONTRACTS_FOLDER = "contracts/";
static string DEFAULT_ASSETS_FOLDER = "assets/";
static string DEFAULT_ATTACKER = "ReentrancyAttacker";

int main(int argc, char* argv[]) {
  /* Run EVM silently */
  dev::LoggingOptions logOptions;
  logOptions.verbosity = VerbositySilent;
  dev::setupLogging(logOptions);
  /* Program options */
  int mode = DEFAULT_MODE;
  int duration = DEFAULT_DURATION;
  int reporter = DEFAULT_REPORTER;
  int logOption = DEFAULT_LOG_OPTION;
  string contractsFolder = DEFAULT_CONTRACTS_FOLDER;
  string assetsFolder = DEFAULT_ASSETS_FOLDER;
  string jsonFile = "";
  string contractName = "";
  string attackerName = DEFAULT_ATTACKER;
  po::options_description desc("Allowed options");
  po::variables_map vm;
  
  desc.add_options()
    ("help,h", "produce help message")
    ("contracts,c", po::value(&contractsFolder), "contract's folder path")
    ("generate,g", "g fuzzMe script")
    ("assets,a", po::value(&assetsFolder), "asset's folder path")
    ("file,f", po::value(&jsonFile), "fuzz a contract")
    ("name,n", po::value(&contractName), "contract name")
    ("mode,m", po::value(&mode), "choose mode: 0 - Random | 1 - AFL ")
    ("reporter,r", po::value(&reporter), "choose reporter: 0 - TERMINAL | 1 - CSV")
    ("duration,d", po::value(&duration), "fuzz duration")
    ("log,l", po::value(&logOption), "write log: 0 - false | 1 - true")
    ("attacker", po::value(&attackerName), "default is ReentrancyAttacker");
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  /* Show help message */
  if (vm.count("help")) showHelp(desc);
  /* Generate working scripts */
  if (vm.count("generate")) {
    std::ofstream fuzzMe("fuzzMe");
    fuzzMe << "#!/bin/bash" << endl;
    fuzzMe << compileSolFiles(contractsFolder);
    fuzzMe << compileSolFiles(assetsFolder);
    fuzzMe << fuzzJsonFiles(contractsFolder, assetsFolder, duration, mode, reporter, logOption, attackerName);
    fuzzMe.close();
    showGenerate();
    return 0;
  }
  /* Fuzz a single contract */
  if (vm.count("file") && vm.count("name")) {
    FuzzParam fuzzParam;
    auto contractInfo = parseAssets(assetsFolder);
    contractInfo.push_back(parseJson(jsonFile, contractName, true));
    fuzzParam.contractInfo = contractInfo;
    fuzzParam.mode = (FuzzMode) mode;
    fuzzParam.duration = duration;
    fuzzParam.reporter = (Reporter) reporter;
    fuzzParam.csvInterval = DEFAULT_CSV_INTERVAL;
    fuzzParam.log = logOption > 0;
    fuzzParam.attackerName = attackerName;
    Fuzzer fuzzer(fuzzParam);
    cout << ">> Fuzz " << contractName << endl;
    fuzzer.start();
    return 0;
  }
  return 0;
}
