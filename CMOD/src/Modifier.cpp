/*
CMOD (composition module)
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

//----------------------------------------------------------------------------//
//
//   modifier.cpp
//
//----------------------------------------------------------------------------//

#include "Modifier.h"
#include "Random.h"

using namespace std;

//----------------------------------------------------------------------------//

Modifier::Modifier() {
  type = "";
  applyHow = "";
  probEnv = NULL;
  checkPt = 0;
  partialNum = 0;
  partialResultString = "";
}

//----------------------------------------------------------------------------//

Modifier::Modifier(string modType, Envelope* prob, string modApplyHow, int modPartialNum) {
  type = modType;
  if (prob != NULL) {
      probEnv = new Envelope(*prob);
  }
  applyHow = modApplyHow;
  partialNum = modPartialNum;
  checkPt = 0;
}

//----------------------------------------------------------------------------//

Modifier::Modifier(const Modifier& orig) {
  type = orig.type;
  applyHow = orig.applyHow;
  checkPt = orig.checkPt;
  partialNum = orig.partialNum;
  partialResultString = orig.partialResultString;

  if (orig.probEnv != NULL) {
    probEnv = new Envelope(*orig.probEnv);
  } else {
    probEnv = NULL;
  }

  for (unsigned i = 0; i < orig.env_values.size(); i++) {
    if (orig.env_values[i] != NULL) {
      env_values.push_back(new Envelope(*orig.env_values[i]));
    } else {
      env_values.push_back(NULL);
    }
  }
}

//----------------------------------------------------------------------------//

Modifier& Modifier::operator=(const Modifier& rhs) {
  if (this == &rhs) {
    return *this;
  }
  type = rhs.type;
  applyHow = rhs.applyHow;
  checkPt = rhs.checkPt;
  partialNum = rhs.partialNum;
  partialResultString = rhs.partialResultString;

  delete probEnv;
  if (rhs.probEnv != NULL) {
    probEnv = new Envelope(*rhs.probEnv);
  } else {
    probEnv = NULL;
  }
  for (auto* env : env_values) {
    delete env;  
  }
  env_values.clear();
  for (unsigned i = 0; i < rhs.env_values.size(); i++) {
    if (rhs.env_values[i] != NULL) {
      env_values.push_back(new Envelope(*rhs.env_values[i]));
    } else {
      env_values.push_back(NULL);
    }
  }
  return *this;
}

//----------------------------------------------------------------------------//

Modifier::~Modifier() {
  if (probEnv != NULL) {
    delete probEnv;
  }

  for (unsigned i = 0; i < env_values.size(); i++) {
    delete env_values[i];
  }
}

//----------------------------------------------------------------------------//

void Modifier::addValueEnv( Envelope* env ) {
  env_values.push_back( new Envelope(*env) );
}

//----------------------------------------------------------------------------//

void Modifier::addSpread(double spread_){
  spread = spread_;
}

//----------------------------------------------------------------------------//

void Modifier::addDirection(double dir_){
  direction = dir_;
}

//----------------------------------------------------------------------------//

 void Modifier::addVelocity(double vel){
  velocity = vel;
 }

//----------------------------------------------------------------------------//

float Modifier::getProbability(double checkPoint) {
  if (checkPoint < 0 || checkPoint > 1) {
    cerr << "Modifier::getProbability -- Error: checkPt out of bounds;" << endl;
    cerr << "        checkPoint = " << checkPoint << endl;
    return -1;
  }
  checkPt = checkPoint;
  return probEnv->getValue(checkPoint, 1);
}

//----------------------------------------------------------------------------//

string Modifier::getModName() {
  return type;
}

//----------------------------------------------------------------------------//

int Modifier::getPartialNum() {
  return partialNum;
}

//----------------------------------------------------------------------------//

string Modifier::getPartialResultString() {
  return partialResultString;
}

//----------------------------------------------------------------------------//

bool Modifier::willOccur(double checkPoint) {
  bool result = false;
  double rand  = Random::Rand();

  if (rand > 0.0 && rand <= getProbability(checkPoint)) {
    result = true;
  }
  return result;
}

//----------------------------------------------------------------------------//

void Modifier::applyModifier(Sound* snd) {
  if (applyHow == "SOUND") {
    applyModSound(snd);
  } else if (applyHow == "PARTIAL") {
    applyModPartial(snd);
  }
}

//----------------------------------------------------------------------------//

void Modifier::applyModSound(Sound* snd) {
  if (type == "FREQUENCY" || type == "GLISSANDO"
      || type == "BEND") {
    snd->setPartialParam(FREQ_ENV, *(env_values[0]));
  } else if (type == "TREMOLO") {
    snd->setPartialParam(TREMOLO_AMP, *(env_values[0]));
    snd->setPartialParam(TREMOLO_RATE, *(env_values[1]));
  } else if (type == "VIBRATO") {
    snd->setPartialParam(VIBRATO_AMP, *(env_values[0]));
    snd->setPartialParam(VIBRATO_RATE, *(env_values[1]));
  } else if (type == "PHASE") {
    snd->setPartialParam(PHASE, *(env_values[0]));
  } else if (type == "AMPTRANS") {
    snd->setPartialParam(AMPTRANS_AMP_ENV, *(env_values[0]));
    snd->setPartialParam(AMPTRANS_RATE_ENV, *(env_values[1]));
    snd->setPartialParam(AMPTRANS_WIDTH, *(env_values[2]));
  } else if (type == "FREQTRANS") {
    snd->setPartialParam(FREQTRANS_AMP_ENV, *(env_values[0]));
    snd->setPartialParam(FREQTRANS_RATE_ENV, *(env_values[1]));
    snd->setPartialParam(FREQTRANS_WIDTH, *(env_values[2]));
  } else if (type == "WAVE_TYPE") {
    snd->setPartialParam(WAVE_TYPE, env_values[0]->getValue(checkPt, 1));
  } else if (type == "DETUNE"){
    snd->setDetune(direction, spread, velocity);
  }else {
    cerr << "ERROR: Modifier given an invalid type: " << type << endl;
    exit(1);
  }
}

//----------------------------------------------------------------------------//

void Modifier::applyModPartial(Sound* snd) {
  if (type == "FREQUENCY" || type == "GLISSANDO" || type == "BEND") {
//      || type == "DETUNE" || type == "BEND") {
    snd->get(partialNum).setParam(FREQ_ENV, *(env_values.front()));
    delete env_values.front();
    env_values.pop_front();
//		-----------------------added by Sever 2/4/23
  } else if (type == "DETUNE") {
    snd->get(partialNum).setParam(DETUNING_ENV, *(env_values.front()));
    delete env_values.front();
    env_values.pop_front();
//	----------------------------------------------------
  } else if (type == "TREMOLO") {
    snd->get(partialNum).setParam(TREMOLO_AMP, *(env_values.front()));
    delete env_values.front();
    env_values.pop_front();
    snd->get(partialNum).setParam(TREMOLO_RATE, *(env_values.front()));
    delete env_values.front();
    env_values.pop_front();
  } else if (type == "VIBRATO") {
    snd->get(partialNum).setParam(VIBRATO_AMP, *(env_values.front()));
    delete env_values.front();
    env_values.pop_front();
    snd->get(partialNum).setParam(VIBRATO_RATE, *(env_values.front()));
    delete env_values.front();
    env_values.pop_front();
  } else if (type == "PHASE") {
    snd->get(partialNum).setParam(PHASE, *(env_values.front()));
    delete env_values.front();
    env_values.pop_front();
  } else if (type == "AMPTRANS") {
    snd->get(partialNum).setParam(AMPTRANS_AMP_ENV, *(env_values.front()));
    delete env_values.front();
    env_values.pop_front();
    snd->get(partialNum).setParam(AMPTRANS_RATE_ENV, *(env_values.front()));
    delete env_values.front();
    env_values.pop_front();
    snd->get(partialNum).setParam(AMPTRANS_WIDTH, *(env_values.front()));
    delete env_values.front();
    env_values.pop_front();
  } else if (type == "FREQTRANS") {
    snd->get(partialNum).setParam(FREQTRANS_AMP_ENV, *(env_values.front()));
    delete env_values.front();
    env_values.pop_front();
    snd->get(partialNum).setParam(FREQTRANS_RATE_ENV, *(env_values.front()));
    delete env_values.front();
    env_values.pop_front();
    snd->get(partialNum).setParam(FREQTRANS_WIDTH, *(env_values.front()));
    delete env_values.front();
    env_values.pop_front();
  } else if (type == "WAVE_TYPE") {
    snd->get(partialNum).setParam(WAVE_TYPE, env_values.front()->getValue(checkPt, 1));
    delete env_values.front();
    env_values.pop_front();
  } else  {
    cerr << "ERROR: Modifier given an invalid type: " << type << endl;
    exit(1);
  }
}

//----------------------------------------------------------------------------//
