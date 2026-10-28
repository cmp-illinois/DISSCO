/*
CMOD (Composition MODule)
Copyright (C) 2005  Sever Tipei (s-tipei@uiuc.edu)

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

//---------------------------------------------------------------------------//
//
//	Sieve.cpp
//
//---------------------------------------------------------------------------//

#include <iterator>
#include "Sieve.h"
#include "Random.h"
#include "ModParser.h"
#include "CmodError.h"
#include <cmath>
//---------------------------------------------------------------------------//

Sieve::Sieve() {
  skip = 0;
}

//---------------------------------------------------------------------------//

Sieve::Sieve(string aFile) {
  skip = 0;
  fileName = aFile;
}

//---------------------------------------------------------------------------//

Sieve::~Sieve() {
}

//---------------------------------------------------------------------------//

string Sieve::getFileName() {
  return fileName;
}


//---------------------------------------------------------------------------//

void Sieve::BuildFromExpr(int minVal, int maxVal,
                          const char *, const char *wMethod,
                          std::string expr, vector<int> wArgVect, vector<int> offsetVect) {
  ModParser mp(offsetVect);
  mp.parseExpr(expr, minVal, maxVal);
  eList = mp.getElements();
  if (eList.empty()) {
    throw CmodError(CmodError::Kind::Project,
                    "Sieve has no elements within range " + std::to_string(minVal)
                        + " through " + std::to_string(maxVal) + ".",
                    "Sieve -> MODS expression: " + expr,
                    "Widen the range or adjust the moduli and offsets so the sieve contains at least one element.");
  }
//Sieve::print_eList();
  Sieve::Weights(mp.getMods(), wMethod, wArgVect, mp.getOffsets());
}


//---------------------------------------------------------------------------//

void Sieve::Build(int minVal, int maxVal,
                  const char *eMethod, const char *wMethod,
                  vector<int> eArgVect, vector<int> wArgVect, vector<int> offsetVect) {

  Sieve::Elements(minVal, maxVal, eMethod, eArgVect, offsetVect);
  if (eList.empty()) {
    throw CmodError(CmodError::Kind::Project,
                    "Sieve has no elements within range " + std::to_string(minVal)
                        + " through " + std::to_string(maxVal) + ".",
                    "Sieve -> Elements",
                    "Widen the range or adjust the element list and offsets so the sieve contains at least one element.");
  }
  Sieve::Weights(eArgVect, wMethod, wArgVect, offsetVect);
}


//---------------------------------------------------------------------------//

void Sieve::FillInVectors(vector<int>& intVect, vector<double>& doubleVect) {
  if (eList.size() != wList.size()) {
    throw CmodError(CmodError::Kind::Project,
                    "Sieve has " + std::to_string(eList.size()) + " elements but "
                        + std::to_string(wList.size()) + " weights.",
                    "Sieve -> Weights",
                    "Provide a weight for each retained element, or use PERIODIC weights.");
  }

//cout << "Sieve::FillInVectors - eList.size()=" << eList.size() << " wList.size()=" << wList.size() << endl;

  list<int>::iterator eIter = eList.begin();
  list<double>::iterator wIter = wList.begin();

  if ( eList.size() >= wList.size() ) {
    while (eIter != eList.end()) {
      intVect.push_back( *eIter );
      doubleVect.push_back( *wIter );
      eIter++;
      wIter++;
    }
  } else if ( wList.size() > eList.size() ) {
    while (wIter != wList.end()) {
      intVect.push_back( *eIter );
      doubleVect.push_back( *wIter );
      eIter++;
      wIter++;
    }
  }
}


//---------------------------------------------------------------------------//

int Sieve::GetNumItems() {
  int result = 0;

  if (eList.size() >= wList.size()) {
    result = static_cast<int>(eList.size());
  } else {
    result = static_cast<int>(wList.size());
  }
  return result;
}

//---------------------------------------------------------------------------//

int Sieve::Modify(Envelope *env, string method) {
  Sieve::AddEnvelope(env, method);
  Sieve::CumulWeights();

  return Sieve::ChooseL();
}


//---------------------------------------------------------------------------//

int Sieve::ChooseL() {
  if (eList.empty() || eList.size() != wList.size()) {
    throw CmodError(CmodError::Kind::Project,
                    "Sieve cannot choose from " + std::to_string(eList.size()) + " elements and "
                        + std::to_string(wList.size()) + " weights.",
                    "Function: ChooseL -> Sieve",
                    "Provide at least one element and a weight for each element.");
  }
  double randomNumber = Random::Rand();

  list<int>::iterator eIter = eList.begin();
  list<double>::iterator wIter = wList.begin();

  while (eIter != eList.end() && (randomNumber > *wIter)) {
    eIter++;
    wIter++;
  }
  if (eIter == eList.end() || !std::isfinite(*wIter) || *wIter < 0) {
    throw CmodError(CmodError::Kind::Project,
                    "Sieve weights do not provide a valid selection for random value "
                        + std::to_string(randomNumber) + ".",
                    "Function: ChooseL -> Sieve weights",
                    "Use finite nonnegative weights that cover the full selection range; a cumulative weight list must end at 1 or greater.");
  }

  return *eIter;
}


//---------------------------------------------------------------------------//

void Sieve::Elements(int minVal, int maxVal,
                     const char *method,
                     vector<int> eArgVect, std::vector<int> offsetVect) {
  if ((strcmp(method, "MEANINGFUL") == 0 || strcmp(method, "MODS") == 0)
      && offsetVect.size() < eArgVect.size()) {
    throw CmodError(CmodError::Kind::Project,
                    "Sieve has " + std::to_string(eArgVect.size()) + " elements/moduli but only "
                        + std::to_string(offsetVect.size()) + " offsets.",
                    "Sieve -> Offsets",
                    "Provide one offset per element or modulus (use 0 when no offset is needed).");
  }
  if(strcmp(method, "MEANINGFUL") == 0) {		//only meaningful elem.
    Sieve::Meaningful(minVal, maxVal, eArgVect, offsetVect);
  } else if(strcmp(method, "MODS") == 0) {		//uses moduli
    Sieve::Multiples(minVal, maxVal, eArgVect, offsetVect);
  } else if(strcmp(method, "FAKE") == 0) {		//all elem, same weight
    Sieve::Fake(minVal, maxVal);
  } else {
    throw CmodError(CmodError::Kind::Project,
                    "Sieve element method '" + string(method) + "' is not supported.",
                    "Sieve -> Method",
                    "Choose MEANINGFUL, MODS, or FAKE in the sieve editor.");
  }
}


//---------------------------------------------------------------------------//

void Sieve::Weights(std::vector<int> eArgVect,
		    const char *method,
                    std::vector<int> wArgVect,
		    std::vector<int> offsetVect) {
  if(strcmp(method, "PERIODIC") == 0) {
    // Bug fix: cycle through the whole list as given. (It used to be cut or
    // padded to exactly 2 values, so "1" became 1, 0, 1, 0.) -- Tony Chai, 2026
    if (wArgVect.empty()) {
      throw CmodError(CmodError::Kind::Project,
                      "Sieve PERIODIC weights list is empty.",
                      "Sieve -> PERIODIC weights",
                      "Enter at least one weight to repeat across the sieve's elements.");
    }
    Sieve::PeriodicWeights(wArgVect);
  } else if(strcmp(method, "HIERARCHIC") == 0) {
    Sieve::HierarchicWeights(eArgVect, wArgVect, offsetVect);
  } else if(strcmp(method, "INCLUDE") == 0) {
    Sieve::IncludeWeights(wArgVect);
  } else {
    throw CmodError(CmodError::Kind::Project,
                    "Sieve weight method '" + string(method) + "' is not supported.",
                    "Sieve -> WeightMethod",
                    "Choose PERIODIC, HIERARCHIC, or INCLUDE in the sieve editor.");
  }
}


//---------------------------------------------------------------------------//

void Sieve::Meaningful(int minVal, int maxVal, vector<int> eArgVect, std::vector<int> offsetVect) {
  skip = 0;

  for (unsigned i = 0; i < eArgVect.size(); i++) {
    if( (eArgVect[i]+ offsetVect[i]) >= minVal && (eArgVect[i] + offsetVect[i]) <= maxVal) {
/*
    cout << "Sieve::Meaningful - eArgVect[" << i << "]=" << eArgVect[i]
         << " offsetVect[" << i << "]=" << offsetVect[i] << endl;
*/
      eList.push_back( eArgVect[i] + offsetVect[i] );
    }

    if(eArgVect[i] + offsetVect[i]< minVal) {
      skip++;
    }
  }

  // sort the list, ascending
  eList.sort();
/*
  for(list<int>::iterator iter = eList.begin(); iter != eList.end(); iter++) {
	cout << *iter << endl;
  }
*/
}

//---------------------------------------------------------------------------//

void Sieve::Multiples(int minVal, int maxVal, vector<int> numMods, std::vector<int> offsetVect) {
  eList.clear();

  skip = 0;

 /* cout << " --- SIEVE::MULTIPLES == " << endl;
  cout << "numMods" << endl;
  for (int q = 0; q < numMods.size(); q++){
    cout << numMods[q] << ", ";
  }
  cout << endl;
*/
  for (unsigned i = 0; i < numMods.size(); i++) {
    if (minVal == 0 && offsetVect[i] >=0) { //put 0 in the list if minVal ==0 and at least one of the offset is 0
      eList.push_back(offsetVect[i]);
    }

    int newElement = numMods[i] + offsetVect[i];

    while (newElement <= maxVal) {
      if ( newElement >= minVal ) {
        // note: we don't have to check for duplicates before adding, because
        //     we'll sort and remove consecutive duplicates at the end
        eList.push_back(newElement);
      } else if (newElement < minVal) {
        skip++;
      }
      newElement += numMods[i]; // increment to the next multiple
    }
  }

  eList.sort(); // sort into ascending order
/*
  cout << "Sieve::Multiples - eList" << endl;
  for (list<int>::iterator it = eList.begin(); it != eList.end(); ++it) {
    cout << *it << ", ";
  }
  cout << endl;
  int sever; cin >> sever;
*/
  eList.unique();  //remove consecutive duplicate values
}

//---------------------------------------------------------------------------//

void Sieve::Fake(int minVal, int maxVal) {
  skip = 0;
  eList.clear();

  for (int i = minVal; i <= maxVal; i++) {
    eList.push_back(i);
  }

}

//---------------------------------------------------------------------------//

void Sieve::PeriodicWeights(const vector<int>& wArgVect) {
/*
=>changed by Sever: elist is >= wArgVect.size; loop should go to eList.size
  for (int count = 0; count < wArgVect.size(); count++) {
    if (count < eList.size()) {
      wList.push_back(wArgVect[count % wArgVect.size()]);
    }
  }
*/
  for (unsigned count = 0; count < eList.size(); count++) {
    wList.push_back(wArgVect[count % wArgVect.size()]);
  }
}

//---------------------------------------------------------------------------//

void Sieve::HierarchicWeights(const std::vector<int>& eArgVect,
				std::vector<int> wArgVect,
				std::vector<int> offsetVect) {
  if (wArgVect.size() > eArgVect.size() || wArgVect.size() > offsetVect.size()) {
    throw CmodError(CmodError::Kind::Project,
                    "Sieve HIERARCHIC weights do not have matching moduli and offsets.",
                    "Sieve -> HIERARCHIC weights",
                    "Provide a modulus and offset for every hierarchical weight.");
  }
  for (size_t i = 0; i < wArgVect.size(); ++i) {
    if (eArgVect[i] == 0) {
      throw CmodError(CmodError::Kind::Project,
                      "Sieve HIERARCHIC modulus " + std::to_string(i + 1) + " is 0.",
                      "Sieve -> HIERARCHIC weights",
                      "Use nonzero moduli, or choose PERIODIC/INCLUDE weights for an element list containing 0.");
    }
  }
  unsigned whichMod;
  double probability;

  list<int>::iterator eIter = eList.begin();
  list<double>::iterator wIter = wList.begin();

/*
cout << "Sieve::Hierarchic - eList.end=" << eList.end() << " eArgVect.size()="
     << eArgVect.size() << " wArgVect.size=" << wArgVect.size() << endl;
  print_eList();
*/

  while (eIter != eList.end()) {
    whichMod = 0;
    probability = 0;

    while (whichMod < wArgVect.size()) {
      if((*eIter - offsetVect[whichMod]) % eArgVect[whichMod] == 0) {
         probability += wArgVect[whichMod];
      }

      whichMod++;
    }

    wList.push_back(probability);

    eIter++;
    wIter++;
  }
}

//---------------------------------------------------------------------------//

void Sieve::IncludeWeights(const vector<int>& wArgVect) {
  if (wArgVect.size() < eList.size()) {
    throw CmodError(CmodError::Kind::Project,
                    "Sieve INCLUDE has " + std::to_string(wArgVect.size()) + " weights for "
                        + std::to_string(eList.size()) + " elements.",
                    "Sieve -> INCLUDE weights",
                    "Provide a weight for every retained element, or choose PERIODIC weights.");
  }
//for(int i = 0; i < wArgVect.size(); i++) {
  for(unsigned i = 0; i < eList.size(); i++) {
    if((int)i >= skip && i < eList.size() + skip) {
      wList.push_back(wArgVect[i]);
    }
  }
}

//---------------------------------------------------------------------------//

void Sieve::AddEnvelope(Envelope *env, string method) {
  if (method != "CONSTANT" && method != "VARIABLE") {
    throw CmodError(CmodError::Kind::Project,
                    "Sieve envelope method '" + method + "' is not supported.",
                    "Sieve -> Envelope Method",
                    "Choose CONSTANT or VARIABLE for the sieve's envelope method.");
  }
  float value;
  double checkPoint;
  double probability;

  NormalizewList();

  int index = 0;
  list<double>::iterator iter = wList.begin();
  while (iter != wList.end()) {
    if (wList.size() > 1) {
      checkPoint = (float)index / (float)(wList.size()-1);
      // check for valid checkpoint
      if (checkPoint < 0 || checkPoint > 1) {
        cerr << "Sieve::AddEnvelope -- checkpoint error:" << endl;
        cerr << "   checkPoint = " << checkPoint << endl;
      }
    } else {
      checkPoint = 0;
    }
    value = env->getValue(static_cast<m_value_type>(checkPoint), 1.);
    if (method == "VARIABLE") {
      probability = Random::PreferedValueDistribution(value, checkPoint);
    } else {
      probability = value;
    }

    //update this element
    *iter = *iter + probability;

    index++;
    iter++;
  }
}

//---------------------------------------------------------------------------//

void Sieve::CumulWeights() {
    double cumul = 0;

    NormalizewList();

    list<double>::iterator iter = wList.begin();
    while (iter != wList.end()) {
      cumul += *iter;
      *iter = cumul;
      iter++;
    }
}

//---------------------------------------------------------------------------//

void Sieve::NormalizewList() {
  double wListSum = 0;

  // get the sum of all the items
  list<double>::iterator iter = wList.begin();
  while (iter != wList.end()) {
    wListSum += *iter;
    iter++;
  }

  // go through the list again, normalizing all elements
  iter = wList.begin();
  while (iter != wList.end()) {
    *iter = *iter / wListSum;
    iter++;
  }
}

//---------------------------------------------------------------------------//

void Sieve::print_eList(){
  list<int>::iterator iter = eList.begin();
  while (iter != eList.end()) {
    cout << *iter << " ";
    iter++;
  }
  cout << endl;
}

//---------------------------------------------------------------------------//

void Sieve::print_wList(){
  list<double>::iterator iter = wList.begin();
  while (iter != wList.end()) {
    cout << *iter << " ";
    iter++;
  }
  cout << endl;
}

