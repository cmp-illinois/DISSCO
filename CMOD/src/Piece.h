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
// all codes with the comment "multistaffs" are added by xiaoyi han
//----------------------------------------------------------------------------//
//
//  Piece.h
//
//----------------------------------------------------------------------------//


#ifndef PIECE_H
#define PIECE_H
#include "CMOD.h"

#include "Libraries.h"
#include "Note.h"
#include <stdlib.h>
#include <stdio.h>

//----------------------------------------------------------------------------//

struct PieceHelper {

  ///Lists contents of directory.
  static int getDirectoryList(std::string dir, std::vector<std::string> &files);

  ///Ensures a path has a backslash on the end.
  static std::string getFixedPath(std::string path);

  ///Determines the name of the project in the given directory.
  static std::string getProjectName(std::string path);

  ///Converts a seed std::string into a seed number.
  static int getSeedNumber(std::string seed);

  ///Creates the sound files directory if it does not exist.
  static void createSoundFilesDirectory(std::string path);

  ///Creates the score files directory if it does not exist.
  static void createScoreFilesDirectory(std::string path);

  ///Checks to see if a file exists.
  static bool doesFileExist(std::string path, std::string filename);

  ///Gets the next available sound file.
  static std::string getNextSoundFile(std::string path, std::string projectName);
};

//----------------------------------------------------------------------------//
class FileValue;
class Utilities;

class Piece {

  public:


  /**
  * Constructor
  **/
  Piece(std::string _workingPath, std::string _projectTitle);

  /**
  * Destructor
  **/
  ~Piece();

  /**
  * Prints information about the piece.
  **/
  void Print();

  ///Gets the next available sound file.
  std::string getNextSoundFile();

  //Modifies the Piece by writing to the DOMDocument. Returns std::vector of its children.
  // Experimental
  std::vector<DOMElement*> modifyPiece(DOMElement* eventElement);

  //Identifies function and modifies their numbers. Experimental
  void functionModifier(DOMElement* functionElement, int maxValue);

  //Calculates Aesthetic value for every event. Returns std::vector of events children.
  //Experimental
  std::vector<DOMElement*> calculateAesthetic(DOMElement* eventElement);

  //Calculates Shannon Entropy ratio based on sampled values
  double calculateEntropyRatio(std::vector<double> sampleData, std::string partitionMethod,
                                double min, double max);

  // Genetic Evolution Algorithm.
  void geneticOptimization(std::string fitnessFunction, double optimum);

  // Crossover and Mutation Function
  void crossoverMutation(DOMElement* parent1, DOMElement* parent2, DOMElement* child,
                         double mutationProb);

  // Experiment 2: Calculate M val for an event
  std::vector<DOMElement *> calcEventM(DOMElement* eventElement);

  // Exp 2: List

  private:

  std::string path;
  std::string projectName;
  std::string title;
  std::string fileFlags;
  std::string fileList;
  std::string pieceStartTime;
  std::string pieceDuration;

  Utilities* utilities;

  bool soundSynthesis;
  bool scorePrinting;
  bool grandStaff;
  int numberOfStaff;
  int numChannels;
  int sampleRate;
  int sampleSize;
  int numThreads;

};

//----------------------------------------------------------------------------//
#endif
