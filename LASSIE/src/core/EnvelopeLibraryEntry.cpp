#include "EnvelopeLibraryEntry.hpp"

/**
 * @brief Default constructor: sets up a linear, flexible segment
 */
EnvLibEntrySeg::EnvLibEntrySeg()
  : leftNode(nullptr),
    rightNode(nullptr),
    segmentType(envSegmentTypeLinear),
    segmentProperty(envSegmentPropertyFlexible)
{}

/**
 * @brief Construct a node at coordinates (_x, _y)
 * @param _x  x-coordinate
 * @param _y  y-coordinate
 */
EnvLibEntryNode::EnvLibEntryNode(double _x, double _y)
  : x(_x),
    y(_y),
    leftSeg(nullptr),
    rightSeg(nullptr)
{}

/**
 * @brief Count this node plus all nodes reachable to the right
 * @return number of nodes from this to list end
 */
int EnvLibEntryNode::countNumOfNodes() {
  // If no outgoing segment, just this one; otherwise 1 + nodes to the right
  return (rightSeg == nullptr)
         ? 1
         : 1 + rightSeg->rightNode->countNumOfNodes();
}

/**
 * @brief Create a fresh envelope with given index
 * @param _number  index for this entry
 */
EnvelopeLibraryEntry::EnvelopeLibraryEntry(int _number)
  : number(_number),
    next(nullptr),
    prev(nullptr)
{
  // initialize a simple two-node envelope: (0,0) → (1,0)
  head = new EnvLibEntryNode(0.0, 0.0);
  head->rightSeg = new EnvLibEntrySeg();
  head->rightSeg->leftNode = head;

  EnvLibEntryNode* tail = new EnvLibEntryNode(1.0, 0.0);
  head->rightSeg->rightNode = tail;
  tail->leftSeg = head->rightSeg;
}

/**
 * @brief Copy-construct an entry from an existing one
 * @param _originalEnvelope  entry to duplicate
 * @param _number            new index
 */
EnvelopeLibraryEntry::EnvelopeLibraryEntry(EnvelopeLibraryEntry* _originalEnvelope, int _number)
  : number(_number),
    next(nullptr),
    prev(nullptr)
{
  // duplicate linked nodes/segments from original
  EnvLibEntryNode* origNode = _originalEnvelope->head;
  head = new EnvLibEntryNode(origNode->x, origNode->y);
  EnvLibEntryNode* dupNode = head;

  while (origNode->rightSeg != nullptr) {
    // copy segment properties
    EnvLibEntrySeg* newSeg = new EnvLibEntrySeg();
    newSeg->segmentType     = origNode->rightSeg->segmentType;
    newSeg->segmentProperty = origNode->rightSeg->segmentProperty;

    dupNode->rightSeg = newSeg;
    newSeg->leftNode  = dupNode;

    // step forward in original
    origNode = origNode->rightSeg->rightNode;

    // create new node
    EnvLibEntryNode* newNode = new EnvLibEntryNode(origNode->x, origNode->y);
    newSeg->rightNode = newNode;
    newNode->leftSeg  = newSeg;
    dupNode = newNode;
  }
}

/**
 * @brief Build from an LASSIE Envelope object
 * @param _envelope  source Envelope
 * @param _number    new index
 */
EnvelopeLibraryEntry::EnvelopeLibraryEntry(Envelope* _envelope, int _number)
  : number(_number),
    next(nullptr),
    prev(nullptr)
{
  auto segments = _envelope->getSegments();
  EnvLibEntrySeg*  prevSeg  = nullptr;

  // build all but last segment
  int lastIndex = segments->size() - 1;
  for (int i = 0; i < lastIndex; ++i) {
    // create node
    EnvLibEntryNode* node = new EnvLibEntryNode(segments->at(i).x, segments->at(i).y);
    if (i == 0) head = node;
    if (prevSeg) {
      prevSeg->rightNode = node;
      node->leftSeg      = prevSeg;
    }

    // create segment
    EnvLibEntrySeg* seg = new EnvLibEntrySeg();
    node->rightSeg = seg;
    seg->leftNode  = node;

    // set fixed/flexible
    seg->segmentProperty = 
        (_envelope->getSegmentLengthType(i) == FIXED)
        ? envSegmentPropertyFixed
        : envSegmentPropertyFlexible;

    // set interpolation type
    switch (_envelope->getSegmentInterpolationType(i)) {
      case EXPONENTIAL:  seg->segmentType = envSegmentTypeExponential; break;
      case CUBIC_SPLINE: seg->segmentType = envSegmentTypeSpline;      break;
      default:            seg->segmentType = envSegmentTypeLinear;      break;
    }

    prevSeg  = seg;
  }

  // final node
  EnvLibEntryNode* finalNode = new EnvLibEntryNode(
    segments->at(lastIndex).x,
    segments->at(lastIndex).y
  );
  prevSeg->rightNode = finalNode;
  finalNode->leftSeg = prevSeg;
}

/**
 * @brief Destructor (cleanup segments/nodes if desired)
 */
EnvelopeLibraryEntry::~EnvelopeLibraryEntry() {
  // any cleanup logic would go here (delete nodes/segments) if needed
}

/**
 * @brief Count how many entries in the doubly-linked list
 * @return total entries including this
 */
int EnvelopeLibraryEntry::count() {
  return (next == nullptr) ? 1 : 1 + next->count();
}

/**
 * @brief Append a new, empty envelope entry after this one
 * @return pointer to the new entry
 */
EnvelopeLibraryEntry* EnvelopeLibraryEntry::createNewEnvelope() {
  if (next)
    return next->createNewEnvelope();

  next       = new EnvelopeLibraryEntry(number + 1);
  next->prev = this;
  return next;
}

/**
 * @brief Append a duplicated envelope entry after this one
 * @param _originalEnvelope  which entry to duplicate
 * @return pointer to the new duplicate
 */
EnvelopeLibraryEntry* EnvelopeLibraryEntry::duplicateEnvelope(EnvelopeLibraryEntry* _originalEnvelope) {
  if (next)
    return next->duplicateEnvelope(_originalEnvelope);

  next       = new EnvelopeLibraryEntry(_originalEnvelope, number + 1);
  next->prev = this;
  return next;
}

/**
 * @brief Get this entry's number as a QString
 * @return number converted to QString
 */
QString EnvelopeLibraryEntry::getNumberString() const {
  return QString::number(number);
} 