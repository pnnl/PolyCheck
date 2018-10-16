#include <iostream>
#include <Processor.hpp>
#include <Orchestrator.hpp>
#include <DebugComputer.hpp>
#include <CodeParser.hpp>
using namespace std;

int main(int argc, char **argv)
{
  char *source ;  // for source program
  char *target ;  // for transformed program
 
  if (argc == 3)
  {
    source = argv[1];
    target = argv[2];
    //cout << "source : " << source << endl;
    //cout << "target : " << source << endl;
  }
  else if(argc != 3)
  {
    cout << "\nUsage: ./PolyCheck source target \n";
  }

  string fileNameStr(source);

  string outputFileName = Processor::GetOutputFileName(fileNameStr);
  string output = "./"+outputFileName;
  //cout << "Output file name: " << output << endl;

  //*****************Analyzing*********************
  
  Orchestrator fTOrchestrator; 
  ReturnValues ftReturnValues = fTOrchestrator.drive(fileNameStr);

  //*****************Inserting********************
  ParseScop(target, ftReturnValues, output);

  cout << "-------------------------SUCCESS---------------------------\n";

  return 0;
}
