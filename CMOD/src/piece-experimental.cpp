/*
CMOD (composition module)
Copyright (C) 2005  Sever Tipei (s-tipei@uiuc.edu)
Modified by Ming-ching Chiu 2013

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

//----------------------------------------------------------------------------//
//
//   Piece.cpp
//
//----------------------------------------------------------------------------//

#include "Piece.h"

#include "Libraries.h"
#include "Define.h"
#include "Note.h"
#include "Output.h"
#include "Random.h"
#include "Utilities.h"
#include <fstream>

#ifdef _WIN32
#undef SIZE
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOMINMAX
#include <windows.h>
#include <vector>
#else
#include <cstdlib>
#endif

//----------------------------------------------------------------------------//

int PieceHelper::getDirectoryList(string dir, vector<string> &files) {
  std::error_code ec;
  std::filesystem::directory_iterator it(dir, ec);
  if (ec) {
    cout << "Error(" << ec.value() << ") opening " << dir << ": "
         << ec.message() << endl;
    return ec.value();
  }

  // Note: directory_iterator skips "." and ".." automatically, unlike the
  // POSIX readdir() loop this replaced (which leaked them into the result).
  for (const auto& entry : it) {
    files.push_back(entry.path().filename().string());
  }
  return 0;
}

//----------------------------------------------------------------------------//

string PieceHelper::getFixedPath(string path) {
  if(path == "")
    return "./";
  if(path.c_str()[path.length() - 1] != '/')
    path = path + "/";
  return path;
}

//----------------------------------------------------------------------------//

string PieceHelper::getProjectName(string path) {
  string dir = string(path);
  vector<string> files = vector<string>();
  getDirectoryList(dir, files);
  string g = "";
  for(unsigned int i = 0; i < files.size(); i++) {
    string f = files[i];
    if(f.length() >= 5 && f.substr(f.length() - 3, 3) == "dat") {
      g = f.erase(f.length() - 4);
      break;
    }
  }

  if(g == "") {
    cout << endl;
    cout << "=========================================================" << endl;
    cout << "Warning: there is no CMOD project file in this directory." << endl;
    cout << "=========================================================" << endl;
    cout << "Files in '" << path << "':" << endl;

    for(unsigned int i = 0; i < files.size(); i++)
      cout << files[i] << endl;
    cout << "=========================================================" << endl;
    cout << endl;
  }

  return g;

}


//----------------------------------------------------------------------------//

int PieceHelper::getSeedNumber(string seed) {
  if(seed == "time") {
    int now = (int)time(0);
    cout << "Using current time " << now << " as seed." << endl;
    return now;
  }

  const char* seed_c = seed.c_str();
  int seedNumber = 0;
  int digits = 1;
  for(unsigned i = 0; i < seed.length(); i++)
    digits *= 10;
  for(int i = 0; seed_c[i] != 0; i++) {
    digits /= 10;
    seedNumber += digits * (int)(seed_c[i] - '0');
  }
  if(seedNumber < 0) seedNumber = -seedNumber;
  return seedNumber;
}

//----------------------------------------------------------------------------//

void PieceHelper::createSoundFilesDirectory(string path) {
  std::error_code ec;
  // create_directory is a no-op if the directory already exists, so the
  // previous "list, then mkdir if missing" dance is unnecessary.
  std::filesystem::create_directory(path + "SoundFiles", ec);
  if (ec) {
    cout << "Error creating SoundFiles directory: " << ec.message() << endl;
  }
}

//----------------------------------------------------------------------------//

void PieceHelper::createScoreFilesDirectory(string path) {
  std::error_code ec;
  std::filesystem::path scoreDir = std::filesystem::path(path) / "ScoreFiles";
  std::filesystem::create_directory(scoreDir, ec);
  if (ec) {
    cout << "Error creating ScoreFiles directory: " << ec.message() << endl;
    return;
  }

  // Clean stale .fms files (was "rm -f .../ScoreFiles/*.fms" via the shell).
  for (const auto& entry : std::filesystem::directory_iterator(scoreDir, ec)) {
    if (entry.path().extension() == ".fms") {
      std::filesystem::remove(entry.path(), ec);
    }
  }
}

//----------------------------------------------------------------------------//

bool PieceHelper::doesFileExist(string path, string filename) {
  string dir = string(path);
  vector<string> files = vector<string>();
  getDirectoryList(dir, files);
  string g = "";
  for(unsigned int i = 0; i < files.size(); i++) {
    if(files[i] == filename)
      return true;
  }
  return false;
}

//----------------------------------------------------------------------------//

string Piece::getNextSoundFile() {

  string soundPath =  "SoundFiles/";
  //cout<<"sound Path:"<<soundPath<<endl;
  for(int i = 0; i < 1000000; i++) {
    stringstream oss;
    oss << projectName << "_" << i << ".aiff";
    //cout << oss.str() << endl;

    if(PieceHelper::doesFileExist(soundPath, oss.str()))
      continue;
    else
      return "SoundFiles/" + oss.str();
  }
  return "";
}


//----------------------------------------------------------------------------//

void Piece::Print() {
  Output::beginSubLevel("Information");
    Output::beginSubLevel("General");
      Output::addProperty("Title", title);
    Output::endSubLevel();

    Output::beginSubLevel("Audio");
      Output::addProperty("Using Synthesis", soundSynthesis ? "Yes" : "No");
      Output::addProperty("Channels", numChannels);
      Output::addProperty("Sample Rate", sampleRate, "Hz");
      Output::addProperty("Sample Resolution", sampleSize, "bits");
    Output::endSubLevel();

    Output::beginSubLevel("Execution");
      Output::addProperty("Seed", "(unspecified)");
      Output::addProperty("Threads", numThreads);
      Output::addProperty("File Flags", fileFlags);
      Output::addProperty("File List", fileList);
    Output::endSubLevel();
  Output::endSubLevel();
}


Piece::Piece(string _workingPath, string _projectTitle){
  path = _workingPath;
  projectName = _projectTitle;
  //Change working directory.
  std::error_code chdirEc;
  std::filesystem::current_path(_workingPath, chdirEc);
  if (chdirEc) {
    cout << "Error changing working directory to " << _workingPath
         << ": " << chdirEc.message() << endl;
  }

  //Parse .dissco File
  pugi::xml_document disscoDoc;
  string disscoFile = _projectTitle+ ".dissco";
  disscoDoc.load_file(disscoFile.c_str());

  pugi::xml_node root = disscoDoc.document_element();
  pugi::xml_node configurations = GFEC(root);
  pugi::xml_node element = GFEC(configurations);
  title = XMLTC(element);
  element = GNES(element);
  fileFlags = XMLTC(element);
  element = GNES(element);
  fileList = XMLTC(element);
  element = GNES(element);
  pieceStartTime = XMLTC(element);
  element = GNES(element);
  pieceDuration = XMLTC(element);
  element = GNES(element);
  soundSynthesis = (XMLTC(element).compare("True")==0)?true:false;
  element = GNES(element);
  scorePrinting = (XMLTC(element).compare("True")==0)?true:false;
  element = GNES(element);

  // multistaffs
  grandStaff = (XMLTC(element).compare("True")==0)?true:false;
  cout <<"grandStaff: " << grandStaff << endl;
  element = GNES(element);

  // get the staffs number
  
  numberOfStaff = atoi(XMLTC(element).c_str());
  cout <<"numberOfStaff: " << numberOfStaff << endl;
  element = GNES(element);

  numChannels = atoi(XMLTC(element).c_str());
  cout << "Channel: " << numChannels << "\n";
  element = GNES(element);

  sampleRate = atoi(XMLTC(element).c_str());
  cout << "Sample Rate: "<< sampleRate << "\n";
  element = GNES(element);

  sampleSize = atoi(XMLTC(element).c_str());
  cout << "Sample Size: "<< sampleSize << "\n";
  element = GNES(element);
  numThreads = atoi(XMLTC(element).c_str());
  element = GNES(element);
  bool outputParticel = (XMLTC(element).compare("True")==0)?true:false;

  //check if seed exists
  string seed;
  element = GNES(element);
  if(element.first_child()){
    seed = XMLTC(element);
  }
  else{
    cout<<"Please key in the Random Seed:"<<endl;
    cin>>seed;
  }

  //Convert seed string to seed number and seed the random number generator
  int seedNumber = PieceHelper::getSeedNumber(seed);
  Random::Seed((unsigned int)seedNumber);

  //construct the utilities object
  utilities = new Utilities(root,
                            _workingPath,
                            soundSynthesis,
                            outputParticel,
                            numThreads,
                            numChannels,
                            sampleRate,
                            this);

  // setup TimeSpan and Tempo
  TimeSpan pieceSpan;
  pieceSpan.start = utilities->evaluate(pieceStartTime, NULL);
  pieceSpan.duration = utilities->evaluate(pieceDuration, NULL);
  Tempo mainTempo; //Though we supply this, "Top" will provide its own tempo.
  
  // multistaffs
  // Initialize the output score
  // Output::notation_score_ = NotationScore(_projectTitle);
  Output::notation_score_ = NotationScore(_projectTitle,grandStaff,numberOfStaff);
  

  //Initialize the output class.
  if (utilities->getOutputParticel()){
    string particelFilename = _projectTitle + ".particel";
    Output::initialize(particelFilename);
    Output::beginSubLevel("Piece");
    Print();
  }


  //Modify the XML file - Eperimental
  // string evName = utilities->topEventnames.at(0);
  // EventType evType = (EventType)0;
  //
  // for(int i = 0; i < 20; i++){
  //   geneticOptimization("redundancy", 34);
  // }
  //
  // cout<<"GA DONE"<<endl;

  //int newSeed = rand();
  //Random::Seed((unsigned int)newSeed);
//}

  //Create the Top event and recursively build its children.
  
  pugi::xml_node topElement = utilities->getEventElement(eventTop, fileList);
  utilities->currChild = 0;
  Event* topEvent = new Event(topElement,
        pieceSpan,0, mainTempo, utilities, {},{},{},{});
  topEvent->buildChildren();

  //get the final MultiTrack object and write it to disk
  if (soundSynthesis){
    cout << "Piece::Piece: " << "soundSynthesis " << endl;
    MultiTrack* renderedScore = utilities->doneCMOD();
    string soundFilename = getNextSoundFile();
    //Write to file.
    AuWriter::write(*renderedScore, soundFilename);
    delete renderedScore;
  }
  if (scorePrinting) {
    cout << "Piece::Piece: " << "Score output " << endl;

		/* for score file */

     // output score to lilypond file
     //output_score(projectName);

    Output::notation_score_.Build();
    ofstream score_file;
    string temp = projectName + ".ly";
    const char* projectNameCstr = temp.c_str();
    score_file.open(projectNameCstr);
    score_file << Output::notation_score_;
    score_file.close();

    // execute lilypond to create pdf file
    {
      std::string lilypondCmd = "lilypond " + projectName + ".ly";
#ifdef _WIN32
      // Use the native CreateProcess API instead of system() so we avoid
      // launching cmd.exe just to parse the command line.
      //
      // We resolve lilypond.exe via PATH ourselves rather than letting
      // CreateProcess do it from a NULL lpApplicationName. With NULL,
      // CreateProcess's search order includes the parent's current
      // directory (which we just chdir'd to a user-controlled project
      // path), so an attacker could drop a lilypond.exe next to the
      // .dissco file and have it execute. SearchPath under safe-search
      // mode moves CWD to the end of the lookup list, and passing the
      // resolved absolute path as lpApplicationName removes the parsing
      // ambiguity entirely. If the binary can't be found we fail closed.
      SetSearchPathMode(BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE |
                        BASE_SEARCH_PATH_PERMANENT);
      char resolvedPath[MAX_PATH];
      DWORD resolvedLen = SearchPathA(nullptr, "lilypond", ".exe",
                                      MAX_PATH, resolvedPath, nullptr);
      if (resolvedLen == 0 || resolvedLen >= MAX_PATH) {
        cout << "Could not locate lilypond.exe on PATH (Win32 error "
             << GetLastError() << ")" << endl;
      } else {
        STARTUPINFOA si{};
        PROCESS_INFORMATION pi{};
        si.cb = sizeof(si);
        std::vector<char> mutableCmd(lilypondCmd.begin(), lilypondCmd.end());
        mutableCmd.push_back('\0');
        if (CreateProcessA(resolvedPath, mutableCmd.data(), nullptr, nullptr,
                           FALSE, 0, nullptr, nullptr, &si, &pi)) {
          WaitForSingleObject(pi.hProcess, INFINITE);
          CloseHandle(pi.hProcess);
          CloseHandle(pi.hThread);
        } else {
          cout << "Failed to launch " << resolvedPath << " (Win32 error "
               << GetLastError() << ")" << endl;
        }
      }
#else
      std::system(lilypondCmd.c_str());
#endif
    }

    // generating score files with different suffixes instead of replacing the older files.
    int suffix_rank = 0;
    int exist = 1;
    string suffix = "";
    while (exist){
      suffix = "_" + Note::int_to_str(suffix_rank);
      std::ifstream infile(( "ScoreFiles/" + projectName + suffix + ".pdf").c_str());
      exist = infile.good();
      infile.close();
      suffix_rank++;
    }

    std::error_code mvEc;
    std::filesystem::rename(projectName + ".pdf",
                            "ScoreFiles/" + projectName + suffix + ".pdf",
                            mvEc);
    if (mvEc) {
      cout << "Error moving " << projectName << ".pdf into ScoreFiles/: "
           << mvEc.message() << endl;
    }

  }

  if (outputParticel){
    //Finish particel output and free up the Output class members.
    Output::endSubLevel();
    Output::free();
  }

  cout << endl;
  cout << "-----------------------------------------------------------" <<
    endl;
  cout << "Build complete." << endl;
  cout << "-----------------------------------------------------------" <<
    endl << endl;
  cout.flush();


  //clean up
  delete utilities;
  delete topEvent; //wait till the thread join

}

Piece::~Piece(){
  //do nothing
}

//Experiment 2
vector<pugi::xml_node> Piece::calcEventM(pugi::xml_node eventElement){

      vector<pugi::xml_node> childElements;
	    pugi::xml_node thisEventElement = GFEC(eventElement); //type
	    string typeString = XMLTC(thisEventElement);
	    int type = atoi(typeString.c_str());
	    double mVal = 0.0;

	    thisEventElement = GNES(thisEventElement); //name
	    string name = XMLTC(thisEventElement);

	    if(type <= 4){  //Top, High, Medium, Low, Bottom

	      thisEventElement = GNES(thisEventElement); //maxChildDur
	      float maxChildDur = (float)utilities->evaluate(XMLTC(thisEventElement), (void*)this);

	      thisEventElement = GNES(thisEventElement); //newEDUPerBeat
	      int newEDUPerBeat = (int) utilities->evaluate(XMLTC(thisEventElement),(void*)this);

	      thisEventElement = GNES(thisEventElement); //Time Signature element

	      thisEventElement = GNES(thisEventElement); //Tempo element

	      pugi::xml_node numChildrenElement = GNES(thisEventElement); //Num children element

	      pugi::xml_node childEventDefElement = GNES(numChildrenElement);
	      pugi::xml_node childStartTimeElement = GFEC(childEventDefElement);
	      pugi::xml_node childTypeElement = GNES(childStartTimeElement);
          cout << "pieceExperimental - childTypeElement=" << childTypeElement << endl;
      int sever; cin >> sever;
	      pugi::xml_node childDurationElement = GNES(childTypeElement);
	      pugi::xml_node AttackSieveElement = GNES(childDurationElement);
	      pugi::xml_node DurationSieveElement = GNES(AttackSieveElement);
	      pugi::xml_node methodFlagElement = GNES(DurationSieveElement);
	      pugi::xml_node childStartTypeFlag = GNES(methodFlagElement);
	      pugi::xml_node childDurationTypeFlag = GNES(childStartTypeFlag);

	      //Read Flag values (Needed for modification)
	      string defFlag = XMLTC(methodFlagElement);
	      int definitionVal = atoi(defFlag.c_str());

	      if(definitionVal == 0){     //Only Continuum

		//Calculating start time orignality
		string startFlag = XMLTC(childStartTypeFlag);
		int startFlagVal = atoi(startFlag.c_str());

	      //layers, initialize child names
	      thisEventElement = GNES(childEventDefElement);
	      pugi::xml_node layerElement = GFEC(thisEventElement);
	      vector<pugi::xml_node> layerElements;
	      vector<pugi::xml_node> childTypeElements;

	      while (layerElement){

		layerElements.push_back(layerElement);
		pugi::xml_node childPackage = GFEC(GNES(GFEC(layerElement)));

		while(childPackage){
		  childTypeElements.push_back(childPackage);
		  childPackage = GNES(childPackage);
		}
		layerElement = GNES(layerElement);
	      }

	      int numChildren;
	      pugi::xml_node flagElement = GFEC(numChildrenElement);
	      if (XMLTC(flagElement) =="0"){ // Continuum
		pugi::xml_node entry1Element = GNES(flagElement);
		if (XMLTC(entry1Element)==""){
		  numChildren = childTypeElements.size();
		}
		else {
		  numChildren =(int) utilities->evaluate(XMLTC(entry1Element), (void*)this);
		}
	      }
	      else if (XMLTC(flagElement) == "1"){ // Density
		pugi::xml_node densityElement = GNES(GFEC(numChildrenElement));
		pugi::xml_node areaElement = GNES(densityElement);
		pugi::xml_node underOneElement = GNES(areaElement);
		double density = utilities->evaluate( XMLTC(densityElement),(void*)this);
		double area = utilities->evaluate( XMLTC(areaElement),(void*)this);
		double underOne = utilities->evaluate( XMLTC(underOneElement),(void*)this);
		double soundsPsec = pow(2, density * area - underOne); //this can't be right..

		numChildren = (int)(soundsPsec * utilities->evaluate(pieceDuration, NULL) + underOne/area);

	      }
	      else {// by layer
	      numChildren = 0;
          for (unsigned i = 0; i < layerElements.size(); i ++){
            numChildren +=utilities->evaluate(XMLTC(GFEC(layerElements[i])),(void*)this);
          }
	      }

	      if(type == 4){ //Bottom
		pugi::xml_node extraInfo = descendantByName(eventElement, "ExtraInfo");

		//Frequency Entropy
		pugi::xml_node frequencyElement = GFEC(extraInfo);
		pugi::xml_node frequencyFlagElement = GFEC(frequencyElement);
		string flagNum = XMLTC(frequencyFlagElement);
		int flagVal = atoi(flagNum.c_str());

		if(flagVal == 0){ //Equal Temperament
		  mVal += EQUAL_TEMP * (log(1 / EQUAL_TEMP)/log(2));
		  pugi::xml_node freqEntry1 = GNES(GNES(frequencyFlagElement));
	      }

	      else if(flagVal == 1){//Fundamental
		mVal += FUNDAMENTAL * (log(1 / FUNDAMENTAL)/log(2));
		pugi::xml_node freqEntry1 = GNES(GNES(frequencyFlagElement));
		pugi::xml_node freqEntry2 = GNES(freqEntry1);
	      }

	      else if(flagVal == 2){//Continuum
	        mVal += CONTINUUM * (log(1 / CONTINUUM)/log(2));
		pugi::xml_node freqEntry1 = GNES(GNES(frequencyFlagElement));
		pugi::xml_node continuumFlagElement = GNES(frequencyFlagElement);

		if (utilities->evaluate(XMLTC(continuumFlagElement), NULL)==0) { //Hertz
		   mVal += HZ * (log(1 / HZ)/log(2));
		  /* 3rd arg is a float (baseFreq in Hz) */
		}

		else  {
		   mVal += POW2 * (log(1 / POW2)/log(2));
		  /* 3rd arg is a float (power of 2) */
		  float step = utilities->evaluate(XMLTC(freqEntry1), (void*)this);
		  double range = log10(CEILING / MINFREQ) / log10(2.); // change log base
		  double baseFreqResult = pow(2, step * range) * MINFREQ;  // equal chance for all 8vs
		}
	      }

	      /*entropy = calculateEntropyRatio(samples, "Pow2", 0, 15000); //Freq in Hz
	      currentEntropy = utilities->eventValues[name];
	      utilities->eventValues.erase(name);
	      newEntropy = (currentEntropy + entropy); // Mean of ratios
	      utilities->eventValues.insert(std::pair<string, double> (name, newEntropy));
	      //cout<<"Entropy Ratio:"<<entropy<<endl;*/

	      printf("%f\n", mVal);

	    for(int i = 0; i < numChildren; i++){

	      double childType = utilities->evaluate(XMLTC(childTypeElement),(void*)this);
	      string childName = XMLTC(GFEC(childTypeElements[childType]));
	      EventType childEventType = (EventType) utilities->evaluate(XMLTC(GNES(GFEC(childTypeElements[childType]))),(void*)this);

	      pugi::xml_node childElement = utilities->getEventElement(childEventType, childName);
	      childElements.push_back(childElement);
	  }

	   return childElements;
	  }
	}
}
return childElements;
}

//Experimental - For now only bottom events
void Piece::geneticOptimization(string fitnessFunction, double optimum){

  // Step 1: Calculating current Aesthetic of the piece - User decides function
  string evName = utilities->topEventnames.at(0);
  EventType evType = (EventType)0;
  utilities->currChild = 0;

  //for(int k = 0; k < 10; k++){
  pugi::xml_node topEvEl = utilities->getEventElement(evType, evName);
  vector<pugi::xml_node> children = calculateAesthetic(topEvEl);

  cout << "EventValues: " << utilities->eventValues.size() << endl;

  for(unsigned i = 0; i < children.size(); i++){
      vector<pugi::xml_node> currChildren = calculateAesthetic(children[i]);

      for(unsigned j = 0; j < currChildren.size(); j++){
        children.push_back(currChildren[j]);
      }

      cout<<"Piece::geneticOptimization: Child No: "<<i<<endl;
  }

    cout << "EventValues: " << utilities->eventValues.size() << endl;

  //(Only bottom events for now)
  double pieceAesthetic = 0.0;
  double fitnessSum = 0.0;
//   int numUniqueEvents = 0;
  std::map<double, string> bottoms;
  std::vector<string> names;

  for(std::map<string, double>::iterator it = utilities->eventValues.begin(); it != utilities->eventValues.end(); ++it){
    names.push_back(it->first);
    cout<<it->first<<", "<<it->second<<endl;
    if(it->second != 0.0){
      pieceAesthetic += it->second;
    //   numUniqueEvents++;
      bottoms.insert(std::pair<double, string>(it->second, it->first));
      cout<< "CHOOSEN: "<<it->first<<", "<<it->second<<endl;
      fitnessSum += it->second;
    }
  }

  //pieceAesthetic /= numUniqueEvents;
  cout<<"Piece::geneticOptimization : Piece aesthetic: "<<pieceAesthetic<<endl;

//---------------------------------------------------------------------------

// Step 2: Find suitable parents
vector<string> sortedEvents;
vector<pugi::xml_node> parents;
vector<pugi::xml_node> child;
vector<double> prange;
double runningSum = 0.0;
EventType t = (EventType)4;

for(std::map<double, string>::iterator it = bottoms.begin(); it != bottoms.end(); ++it){
  sortedEvents.push_back(it->second);
  prange.push_back(runningSum + (it->first / fitnessSum));
  runningSum += it->first / fitnessSum;
}

if(pieceAesthetic < optimum){

  double randNum = Random::Rand();

  //Selecting First Parent
  for(unsigned i = 0; i < prange.size(); i++){
    if(prange[i] >= randNum){
      cout<<"P1: "<<sortedEvents[i]<<endl;
      parents.push_back(utilities->getEventElement(t, sortedEvents[i]));
      fitnessSum -= utilities->eventValues[sortedEvents[i]];
      sortedEvents.erase(sortedEvents.begin() + i);
      runningSum = 0.0;
      prange.clear();
      break;
    }
  }

  //Recalculating Probability
  for(unsigned i = 0; i < sortedEvents.size(); i++){
    cout<<"Piece::geneticOptimization :  Recalculating Probability "<< i <<endl;
    prange.push_back(runningSum + (utilities->eventValues[sortedEvents[i]] / fitnessSum));
    runningSum += utilities->eventValues[sortedEvents[i]] / fitnessSum;
  }

  //Selecting Second Parent
  randNum = Random::Rand();

  for(unsigned i = 0; i < prange.size(); i++){
    if(prange[i] >= randNum){
      cout<<"P2: "<<sortedEvents[i]<<endl;
      parents.push_back(utilities->getEventElement(t, sortedEvents[i]));
      fitnessSum -= utilities->eventValues[sortedEvents[i]];
      sortedEvents.erase(sortedEvents.begin() + i);
      runningSum = 0.0;
      prange.clear();
      break;
    }
  }

  //Recalculating Probability
  for(unsigned i = 0; i < sortedEvents.size(); i++){
    prange.push_back(runningSum + (utilities->eventValues[sortedEvents[i]] / fitnessSum));
    runningSum += utilities->eventValues[sortedEvents[i]] / fitnessSum;
  }

  //Selecting Child
  randNum = Random::Rand();
  for(unsigned i = 0; i < prange.size(); i++){
    if(prange[i] >= randNum){
      cout<<"child: "<<sortedEvents[sortedEvents.size() - i]<<endl;
      child.push_back(utilities->getEventElement(t, sortedEvents[sortedEvents.size() - i]));
      sortedEvents.erase(sortedEvents.begin() + i);
      runningSum = 0.0;
      prange.clear();
      break;
    }
  }
}

else if(pieceAesthetic > optimum){


}

else{ //Nearly impossible for double values
  return;
}
//----------------------------------------------------------------------------

// Step 3: Crossover (For now single point crossover)
// For now, Attributes can come from either parent

crossoverMutation(parents[0], parents[1], child[0], 0.2);

//-------------------------------------------------------------------

// Step 4: Repeat Steps 1-3

//Resetting eventValues map for next iteration
utilities->eventValues.clear();
for(unsigned i = 0; i < names.size(); i++){
  utilities->eventValues.insert(std::pair<string, double>(names[i], 0.0));
}

//geneticOptimization(fitnessFunction, optimum);


}

void Piece::crossoverMutation(pugi::xml_node parent1, pugi::xml_node parent2, pugi::xml_node child, double mutationProb){

  int freqParent = Random::RandInt(0, 1);
  pugi::xml_node extraInfoList;

  if(freqParent == 0){
    extraInfoList = descendantByName(parent1, "ExtraInfo");
  }

  else{
    extraInfoList = descendantByName(parent2, "ExtraInfo");
  }

  pugi::xml_node extraInfo = extraInfoList;

  pugi::xml_node parentFrequencyElement = GFEC(extraInfo);

  pugi::xml_node extraInfoList2 = descendantByName(child, "ExtraInfo");
  pugi::xml_node extraInfo2 = extraInfoList2;

  // Replace child's frequency element with a copy of parent's.
  pugi::xml_node oldFreq = GFEC(extraInfo2);
  extraInfo2.insert_copy_before(parentFrequencyElement, oldFreq);
  extraInfo2.remove_child(oldFreq);

  /*
  pugi::xml_node childFrequencyElement = GFEC(extraInfo2);
  frequencyFlagElement = GFEC(childFrequencyElement);
  flagNum = XMLTC(frequencyFlagElement);
  flagVal = atoi(flagNum.c_str());
  cout<<flagVal<<"\t";*/

  int loudParent = Random::RandInt(0, 1);
  if(loudParent == 0){
    extraInfoList = descendantByName(parent1, "ExtraInfo");
  }

  else{
    extraInfoList = descendantByName(parent2, "ExtraInfo");
  }

  extraInfo = extraInfoList;

  pugi::xml_node parentLoudnessElement = GNES(GFEC(extraInfo));

  extraInfoList2 = descendantByName(child, "ExtraInfo");
  extraInfo2 = extraInfoList2;

  // Replace child's loudness element with a copy of parent's.
  pugi::xml_node oldLoud = GNES(GFEC(extraInfo2));
  extraInfo2.insert_copy_before(parentLoudnessElement, oldLoud);
  extraInfo2.remove_child(oldLoud);

  //Mutation - Use modifypiece method for random modification

  if(mutationProb * 10 >= Random::RandInt(1, 10)){
    cout<<"MUTATED"<<endl;
    modifyPiece(GFEC(extraInfo2)); //Mutate the new child
  }

}

//Experimental

vector<pugi::xml_node> Piece::modifyPiece(pugi::xml_node eventElement){

  //Read certain properties of this event/ Navigate the XML
  vector<pugi::xml_node> childElements;
  pugi::xml_node thisEventElement = GFEC(eventElement); //type
  string typeString = XMLTC(thisEventElement);
  int type = atoi(typeString.c_str());

  thisEventElement = GNES(thisEventElement); //name
  string name = XMLTC(thisEventElement);

     if(type == 4){ //Bottom
      pugi::xml_node extraInfo = descendantByName(eventElement, "ExtraInfo");

      //Modifying Frequency
      pugi::xml_node frequencyElement = GFEC(extraInfo);
      pugi::xml_node frequencyFlagElement = GFEC(frequencyElement);
      string flagNum = XMLTC(frequencyFlagElement);
      int flagVal = atoi(flagNum.c_str());

      if(flagVal == 0){
        pugi::xml_node freqEntry1 = GNES(GNES(frequencyFlagElement));

        if(GFEC(freqEntry1) != NULL){
          pugi::xml_node funcElement = GFEC(freqEntry1);
          functionModifier(funcElement, 80);
        }

        else{

          string freqNum = XMLTC(freqEntry1);
          int freqVal = atoi(freqNum.c_str());
          freqVal = Random::RandInt(8, 80);

          char lval[33];
          sprintf(lval, "%d", freqVal);
          freqEntry1.text().set(lval);

      }
    }

    else if(flagVal == 1){    //Didnt change Partial Number
      pugi::xml_node freqEntry1 = GNES(GNES(frequencyFlagElement));

      if(GFEC(freqEntry1) != NULL){
        pugi::xml_node funcElement = GFEC(freqEntry1);
        //functionModifier(funcElement, 100);
      }

      else{

        string freqNum = XMLTC(freqEntry1);
        int freqVal = atoi(freqNum.c_str());
        //freqVal = (freqVal + Random::RandInt(0, 10)) % 100;

        char lval[33];
        sprintf(lval, "%d", freqVal);
          freqEntry1.text().set(lval);

    }
    }

    else if(flagVal == 2){
      pugi::xml_node continuumFlagElement = GNES(frequencyFlagElement);
      string contflagNum = XMLTC(continuumFlagElement);
      int contflagVal = atoi(contflagNum.c_str());
      pugi::xml_node freqEntry1 = GNES(GNES(frequencyFlagElement));

      if(GFEC(freqEntry1) != NULL){
        pugi::xml_node funcElement = GFEC(freqEntry1);
        if(contflagVal == 0){
         functionModifier(funcElement, 15000);
      }
        else{
         functionModifier(funcElement, 14);
        }
      }

      else{

        string freqNum = XMLTC(freqEntry1);
        int freqVal = atoi(freqNum.c_str());
        if(contflagVal == 0){
         freqVal = Random::RandInt(0, 15000);
      }
        else{
         freqVal = Random::RandInt(0, 14);
        }

        char lval[33];
        sprintf(lval, "%d", freqVal);
          freqEntry1.text().set(lval);

    }
    }

      //Modifying Loudness
      pugi::xml_node loudnessElement = GNES(frequencyElement);

      if(GFEC(loudnessElement) != NULL){
        pugi::xml_node funcElement = GFEC(loudnessElement);
        functionModifier(funcElement, 225);
      }

      else{
        //char *loudnessvalue = XMLString::transcode(loudnessElement->getFirstChild()->getNodeValue());
        //cout<<"hey:"<<loudnessvalue[0]<<loudnessvalue[1]<<loudnessvalue[2]<<endl;

        string typeString = XMLTC(loudnessElement);
        int val = atoi(typeString.c_str());
        int loudnessNum = val;
        loudnessNum = Random::RandInt(0, 225);
        //cout<<"loudness = "<< loudnessNum<<endl;

        char lval[33];
        sprintf(lval, "%d", loudnessNum);
          loudnessElement.text().set(lval);

    }

      GNES(loudnessElement); // Spatialization element
      GNES(GNES(loudnessElement)); // Reverb element
      GNES(GNES(GNES(loudnessElement))); // Filters element
      GNES(GNES(GNES(GNES(loudnessElement)))); // Modifiers element
    }

    /*for(int i = 0; i < numChildren; i++){

      double childType = utilities->evaluate(XMLTC(childTypeElement),(void*)this);
      string childName = XMLTC(GFEC(childTypeElements[childType]));
      EventType childEventType = (EventType) utilities->evaluate(XMLTC(GNES(GFEC(childTypeElements[childType]))),(void*)this);

      pugi::xml_node childElement = utilities->getEventElement(childEventType, childName);
      childElements.push_back(childElement);

      //cout<<childName<<"!!!!!!!!!!!!!!!!!!!!!!"<<endl;

  }*/

   return childElements;
 }

 /*//Modifying Spectrum Class (Spectrum event has diff layout than others)
 else if(type == 5){
   thisEventElement = GNES(thisEventElement); //Num Partials
   pugi::xml_node deviationElement = GNES(thisEventElement); //Deviation

   //Modifying Deviation
   if(GFEC(deviationElement) != NULL){
     pugi::xml_node funcElement = GFEC(deviationElement);
     //functionModifier(funcElement, 1);
   }

   else{
     string typeString = XMLTC(deviationElement);
     double val = atof(typeString.c_str());
     //int devNum = (val + Random::RandInt(0, 22)) % 225;

     char lval[33];
     sprintf(lval, "%f", val);
          deviationElement.text().set(lval);
 }

 return childElements;
}*/
//}

//Experimental
void Piece::functionModifier(pugi::xml_node functionElement, int maxValue){ //Needs some range info

  string functionName = XMLTC(GFEC(functionElement));

  if(functionName.compare("RandomInt") == 0){

    // cout<<"RandomInt"<<endl;

    pugi::xml_node lowBoundElement = GNES(GFEC(functionElement));
    pugi::xml_node highBoundElement = GNES(lowBoundElement);

    if(GFEC(lowBoundElement) != NULL){
      functionModifier(GFEC(lowBoundElement), maxValue);
    }

    else{
      int lowBound = atoi(XMLTC(lowBoundElement).c_str());
      lowBound = (lowBound + Random::RandInt(0, maxValue/2)) % (maxValue/2);
      char val[33];
      sprintf(val, "%d", lowBound);
          lowBoundElement.text().set(val);

    }

    if(GFEC(highBoundElement) != NULL){
      functionModifier(GFEC(highBoundElement), maxValue);
    }

    else{
      int highBound = atoi(XMLTC(highBoundElement).c_str());
      highBound = (highBound + Random::RandInt(0, maxValue/2)) % (maxValue/2) + maxValue/2;
      char val[33];
      sprintf(val, "%d", highBound);
          highBoundElement.text().set(val);

  }
  }

  else if (functionName.compare("Random") == 0){

    pugi::xml_node lowBoundElement = GNES(GFEC(functionElement));
    pugi::xml_node highBoundElement = GNES(lowBoundElement);

    if(GFEC(lowBoundElement) != NULL){
      functionModifier(GFEC(lowBoundElement), maxValue);
    }

    else{
      double lowBound = atof(XMLTC(lowBoundElement).c_str());
      lowBound = fmod((lowBound + Random::Rand(0, maxValue/2)), maxValue/2);
      char val[33];
      sprintf(val, "%f", lowBound);
          lowBoundElement.text().set(val);

    }

    if(GFEC(highBoundElement) != NULL){
      functionModifier(GFEC(highBoundElement), maxValue);
    }

    else{
      double highBound = atof(XMLTC(highBoundElement).c_str());
      highBound = fmod((highBound + Random::Rand(0, maxValue/2)), maxValue/2) + maxValue/2;
      char val[33];
      sprintf(val, "%f", highBound);
          highBoundElement.text().set(val);

  }

  }

  else if (functionName.compare("Select") == 0){
    pugi::xml_node listElement = GNES(GFEC(functionElement));
    pugi::xml_node indexElement = GNES(listElement);

    std::vector<std::string> list = utilities->listElementToStringVector(listElement);

    if(GFEC(indexElement) != NULL){
      functionModifier(GFEC(indexElement), list.size() - 1);
    }

    else{
      int indexVal = atoi(XMLTC(indexElement).c_str());
      indexVal = (indexVal + 1)%(list.size() - 1);
      char val[33];
      sprintf(val, "%d", indexVal);
          indexElement.text().set(val);
    }
  }

  else if (functionName.compare("GetPattern") == 0){
    //Do Later

  }
  else if (functionName.compare("Randomizer") == 0){

    pugi::xml_node baseValElement = GNES(GFEC(functionElement));
    pugi::xml_node percDevElement = GNES(baseValElement);

    if(GFEC(baseValElement) != NULL){
      functionModifier(GFEC(baseValElement), maxValue);
    }

    else{
      double baseVal = atof(XMLTC(baseValElement).c_str());
      baseVal = fmod(baseVal + Random::Rand(0, baseVal/10), maxValue/2);
      char val[33];
      sprintf(val, "%f", baseVal);
          baseValElement.text().set(val);

    }

    if(GFEC(percDevElement) != NULL){
      functionModifier(GFEC(percDevElement), maxValue);
    }

    else{
      double percDev = atof(XMLTC(percDevElement).c_str());
      percDev = fmod((percDev + Random::Rand(0, percDev/10)), 1);
      char val[33];
      sprintf(val, "%f", percDev);
          percDevElement.text().set(val);

   }

  }
  else if (functionName.compare("RandomDensity") == 0) {

    pugi::xml_node envelopeNumberElement = GNES(GFEC(functionElement));
    pugi::xml_node lowBoundElement = GNES(envelopeNumberElement);
    pugi::xml_node highBoundElement = GNES(lowBoundElement);

    if(GFEC(lowBoundElement) != NULL){
      functionModifier(GFEC(lowBoundElement), maxValue);
    }

    else{
      double lowBound = atof(XMLTC(lowBoundElement).c_str());
      lowBound = fmod((lowBound + Random::Rand(0, maxValue/2)), maxValue/2);
      char val[33];
      sprintf(val, "%f", lowBound);
          lowBoundElement.text().set(val);

    }

    if(GFEC(highBoundElement) != NULL){
      functionModifier(GFEC(highBoundElement), maxValue);
    }

    else{
      double highBound = atof(XMLTC(highBoundElement).c_str());
      highBound = fmod((highBound + Random::Rand(0, maxValue/2)), maxValue/2) + maxValue/2;
      char val[33];
      sprintf(val, "%f", highBound);
          highBoundElement.text().set(val);

  }

  }
  else if (functionName.compare("ChooseL") == 0){
    //Do Later

  }

  else if (functionName.compare("ValuePick") == 0){
    //Do Later

  }

  else if (functionName.compare("Stochos") == 0){

  }
  else if (functionName.compare("Decay") == 0){

  }

  else if (functionName.compare("Fibonacci") == 0){

  }

  else if (functionName.compare("LN") == 0){

  }

  else if (functionName.compare("Inverse") == 0){

  }
}

  vector<pugi::xml_node> Piece::calculateAesthetic(pugi::xml_node eventElement){

    vector<pugi::xml_node> childElements;
    const int NUM_SAMPLES = 100;
    std::vector<double> samples;
    double entropy = 0.0;
    double currentEntropy, newEntropy;
    pugi::xml_node thisEventElement = GFEC(eventElement); //type
    string typeString = XMLTC(thisEventElement);
    int type = atoi(typeString.c_str());

    thisEventElement = GNES(thisEventElement); //name
    string name = XMLTC(thisEventElement);

    if(type <= 4){  //Top, High, Medium, Low, Bottom

      thisEventElement = GNES(thisEventElement); //maxChildDur
      float maxChildDur = (float)utilities->evaluate(XMLTC(thisEventElement), (void*)this);

      thisEventElement = GNES(thisEventElement); //newEDUPerBeat
      int newEDUPerBeat = (int) utilities->evaluate(XMLTC(thisEventElement),(void*)this);

      thisEventElement = GNES(thisEventElement); //Time Signature element

      thisEventElement = GNES(thisEventElement); //Tempo element

      pugi::xml_node numChildrenElement = GNES(thisEventElement); //Num children element

      pugi::xml_node childEventDefElement = GNES(numChildrenElement);
      pugi::xml_node childStartTimeElement = GFEC(childEventDefElement);
      pugi::xml_node childTypeElement = GNES(childStartTimeElement);
      pugi::xml_node childDurationElement = GNES(childTypeElement);
      pugi::xml_node AttackSieveElement = GNES(childDurationElement);
      pugi::xml_node DurationSieveElement = GNES(AttackSieveElement);
      pugi::xml_node methodFlagElement = GNES(DurationSieveElement);
      pugi::xml_node childStartTypeFlag = GNES(methodFlagElement);
      pugi::xml_node childDurationTypeFlag = GNES(childStartTypeFlag);

      //Read Flag values (Needed for modification)
      string defFlag = XMLTC(methodFlagElement);
      int definitionVal = atoi(defFlag.c_str());

      if(definitionVal == 0){     //Only Continuum

        //Calculating start time orignality
        string startFlag = XMLTC(childStartTypeFlag);
        int startFlagVal = atoi(startFlag.c_str());


        /*//Calculating Duration entropy

        samples.clear();
        string durationFlag = XMLTC(childDurationTypeFlag);
        int durationFlagVal = atoi(durationFlag.c_str());
        //float durationInSec = 0.0;

        for(int i = 0; i < NUM_SAMPLES; i++){
          double rawChildDuration = utilities->evaluate(XMLTC(childDurationElement),(void*)this);

          // pre-quantize the duration in case "EDU" is used
          int rawChildDurationInt = (int)rawChildDuration;
          int maxChildDurInt = (int)maxChildDur;
          if(rawChildDurationInt > maxChildDurInt)
              rawChildDurationInt = maxChildDurInt;

          // how to process duration: EDU, SECONDS or PERCENTAGE
          if (durationFlagVal == 1) {//EDU (Not sure how to do this)
            //durationInSec = // convert to seconds
              //(float)rawChildDurationInt * tempo.getEDUDurationInSeconds().To<float>();

          } else if (durationFlagVal == 2) {//seconds
            samples.push_back(rawChildDuration);

          } else if (durationFlagVal == 0) {//fraction

          }
        }

        entropy = calculateEntropyRatio(samples, "Unit", 0.0, atoi(pieceDuration.c_str())); //duration
        currentEntropy = utilities->eventValues[name];
        utilities->eventValues.erase(name);
        newEntropy = (currentEntropy + entropy)/2.0; // Mean of ratios
        utilities->eventValues.insert(std::pair<string, double> (name, newEntropy));
        //cout<<"Entropy Ratio:"<<entropy<<endl;*/

      //layers, initialize child names
      thisEventElement = GNES(childEventDefElement);
      pugi::xml_node layerElement = GFEC(thisEventElement);
      vector<pugi::xml_node> layerElements;
      vector<pugi::xml_node> childTypeElements;

      while (layerElement){

        layerElements.push_back(layerElement);
        pugi::xml_node childPackage = GFEC(GNES(GFEC(layerElement)));

        while(childPackage){
          childTypeElements.push_back(childPackage);
          childPackage = GNES(childPackage);
        }
        layerElement = GNES(layerElement);
      }

      int numChildren;
      pugi::xml_node flagElement = GFEC(numChildrenElement);
      if (XMLTC(flagElement) =="0"){ // Continuum
        pugi::xml_node entry1Element = GNES(flagElement);
        if (XMLTC(entry1Element)==""){
          numChildren = childTypeElements.size();
        }
        else {
          numChildren =(int) utilities->evaluate(XMLTC(entry1Element), (void*)this);
        }
      }
      else if (XMLTC(flagElement) == "1"){ // Density
        pugi::xml_node densityElement = GNES(GFEC(numChildrenElement));
        pugi::xml_node areaElement = GNES(densityElement);
        pugi::xml_node underOneElement = GNES(areaElement);
        double density = utilities->evaluate( XMLTC(densityElement),(void*)this);
        double area = utilities->evaluate( XMLTC(areaElement),(void*)this);
        double underOne = utilities->evaluate( XMLTC(underOneElement),(void*)this);
        double soundsPsec = pow(2, density * area - underOne); //this can't be right..

        numChildren = (int)(soundsPsec * utilities->evaluate(pieceDuration, NULL) + underOne/area);

      }
      else {// by layer
        numChildren = 0;
        for (unsigned i = 0; i < layerElements.size(); i ++){
          numChildren +=utilities->evaluate(XMLTC(GFEC(layerElements[i])),(void*)this);
        }
      }

      if(type == 4){ //Bottom
        pugi::xml_node extraInfo = descendantByName(eventElement, "ExtraInfo");

        //Frequency Entropy
        samples.clear();
        pugi::xml_node frequencyElement = GFEC(extraInfo);
        pugi::xml_node frequencyFlagElement = GFEC(frequencyElement);
        string flagNum = XMLTC(frequencyFlagElement);
        int flagVal = atoi(flagNum.c_str());

        if(flagVal == 0){
          pugi::xml_node freqEntry1 = GNES(GNES(frequencyFlagElement));

          for(int i = 0; i < NUM_SAMPLES; i++){
            double wellTempPitch = utilities->evaluate(XMLTC(freqEntry1), (void*)this);
            double baseFreqResult = C0 * pow(WELL_TEMP_INCR, wellTempPitch);
            samples.push_back(baseFreqResult);
          }
      }

      else if(flagVal == 1){
        pugi::xml_node freqEntry1 = GNES(GNES(frequencyFlagElement));
        pugi::xml_node freqEntry2 = GNES(freqEntry1);

        for(int i = 0; i < NUM_SAMPLES; i++){
          float fund_freq = utilities->evaluate(XMLTC(freqEntry1), (void*)this);
          int overtone_step = utilities->evaluate(XMLTC(freqEntry2), (void*)this);
          double baseFreqResult = fund_freq * overtone_step;
          samples.push_back(baseFreqResult);
        }
      }

      else if(flagVal == 2){
        pugi::xml_node freqEntry1 = GNES(GNES(frequencyFlagElement));
        pugi::xml_node continuumFlagElement = GNES(frequencyFlagElement);

        for(int i = 0; i < NUM_SAMPLES; i++){
        if (utilities->evaluate(XMLTC(continuumFlagElement), NULL)==0) { //Hertz
          samples.push_back(utilities->evaluate(XMLTC(freqEntry1), (void*)this));
          /* 3rd arg is a float (baseFreq in Hz) */
        }
        else  {
          /* 3rd arg is a float (power of 2) */
          float step = utilities->evaluate(XMLTC(freqEntry1), (void*)this);
          double range = log10(CEILING / MINFREQ) / log10(2.); // change log base
          double baseFreqResult = pow(2, step * range) * MINFREQ;  // equal chance for all 8vs
          samples.push_back(baseFreqResult);
        }
      }
      }

      entropy = calculateEntropyRatio(samples, "Pow2", 0, 15000); //Freq in Hz
      currentEntropy = utilities->eventValues[name];
      utilities->eventValues.erase(name);
      newEntropy = (currentEntropy + entropy); // Mean of ratios
      utilities->eventValues.insert(std::pair<string, double> (name, newEntropy));
      //cout<<"Entropy Ratio:"<<entropy<<endl;

      //LOUDNESS Entropy

      pugi::xml_node loudnessElement = GNES(frequencyElement);
      samples.clear();

      for(int i = 0; i < NUM_SAMPLES;){
        double loudval = utilities->evaluate(XMLTC(loudnessElement), (void*)this);
        if(loudval >= 1.0){
          samples.push_back(loudval);
          i++;
        }
      }

      entropy = calculateEntropyRatio(samples, "Pow2", 0, 256); //Loudness
      currentEntropy = utilities->eventValues[name];
      utilities->eventValues.erase(name);
      newEntropy = (currentEntropy + entropy); // Mean of ratios
      utilities->eventValues.insert(std::pair<string, double> (name, newEntropy));
      //cout<<"Entropy Ratio:"<<entropy<<endl;
    }

    for(int i = 0; i < numChildren; i++){

      double childType = utilities->evaluate(XMLTC(childTypeElement),(void*)this);
      string childName = XMLTC(GFEC(childTypeElements[childType]));
      EventType childEventType = (EventType) utilities->evaluate(XMLTC(GNES(GFEC(childTypeElements[childType]))),(void*)this);

      pugi::xml_node childElement = utilities->getEventElement(childEventType, childName);
      childElements.push_back(childElement);
  }

   return childElements;
  }
}
return childElements;

}

  double Piece::calculateEntropyRatio(vector<double> sampleData, string partitionMethod, double min, double max){

    if(min == 0 && partitionMethod.compare("Pow2") == 0){
      min++; //For log to work
    }
    int numPartitions = 100; //Arbitrary
    vector<double> probs(numPartitions, 0.0);
    double shannonEntropy = 0.0, maxEntropy, redundancy;

    std::sort(sampleData.begin(), sampleData.end());

    double sampleRange = sampleData[sampleData.size() - 1] - sampleData[0];

    for(unsigned i = 0; i < sampleData.size(); i++){
      if(partitionMethod.compare("Pow2") == 0){
        probs[(int)(log(sampleData[i]) / log(2))] += 1.0 / sampleData.size();
      }
      else if(partitionMethod.compare("Unit") == 0){
        probs[(int)sampleData[i]] += 1.0 / sampleData.size();
      }
    }

    for(unsigned i = 0; i < probs.size(); i++){

      if(0.0 < probs[i])
        shannonEntropy += -1 * probs[i] * (log(probs[i]) / log(2));

  }

  if(shannonEntropy < 0.0)    // C++ log stuff error?
    shannonEntropy = 0.0;

  //cout<<"Shannon: "<<shannonEntropy;

  if(partitionMethod.compare("Pow2") == 0){

    numPartitions = ceil(log(max)/log(2)) - floor(log(min)/log(2));
  }

  else if(partitionMethod.compare("Unit") == 0){

    numPartitions = max - min;
  }

  maxEntropy = log(numPartitions)/log(2);
  //redundancy = 1 - (shannonEntropy/maxEntropy);
  redundancy = maxEntropy - shannonEntropy;
  double relativeShannonEntropy = shannonEntropy/maxEntropy;
  double benseOriginality = relativeShannonEntropy/redundancy;
  cout<<shannonEntropy<<endl;

  return redundancy;
  }
