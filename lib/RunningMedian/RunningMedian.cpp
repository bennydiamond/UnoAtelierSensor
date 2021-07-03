//
//    FILE: RunningMedian.cpp
//  AUTHOR: Rob Tillaart
// VERSION: 0.3.3
// PURPOSE: RunningMedian library for Arduino
//
//  HISTORY:
//  0.1.00  2011-02-16  initial version
//  0.1.01  2011-02-22  added remarks from CodingBadly
//  0.1.02  2012-03-15  added
//  0.1.03  2013-09-30  added _sorted flag, minor refactor
//  0.1.04  2013-10-17  added getAverage(uint8_t) - kudo's to Sembazuru
//  0.1.05  2013-10-18  fixed bug in sort; removes default constructor; dynamic memory
//  0.1.06  2013-10-19  faster sort, dynamic arrays, replaced sorted T array with indirection array
//  0.1.07  2013-10-19  add correct median if _count is even.
//  0.1.08  2013-10-20  add getElement(), add getSottedElement() add predict()
//  0.1.09  2014-11-25  T to double (support ARM)
//  0.1.10  2015-03-07  fix clear
//  0.1.11  2015-03-29  undo 0.1.10 fix clear
//  0.1.12  2015-07-12  refactor constructor + const
//  0.1.13  2015-10-30  fix getElement(n) - kudos to Gdunge
//  0.1.14  2017-07-26  revert double to T - issue #33
//  0.1.15  2018-08-24  make runningMedian Configurable #110
//  0.2.0   2020-04-16  refactor.
//  0.2.1   2020-06-19  fix library.json
//  0.2.2   2021-01-03  add Arduino-CI + unit tests
//  0.3.0   2021-01-04  malloc memory as default storage
//  0.3.1   2021-01-16  Changed size parameter to 255 max
//  0.3.2   2021-01-21  replaced bubbleSort by insertionSort 
//                      --> better performance for large arrays.
//  0.3.3   2021-01-22  better insertionSort (+ cleanup test code)


#include "RunningMedian.h"

template<typename T>
RunningMedian<T>::RunningMedian(const uint8_t size)
{
  _size = size;
  if (_size < MEDIAN_MIN_SIZE) _size = MEDIAN_MIN_SIZE;
  // if (_size > MEDIAN_MAX_SIZE) _size = MEDIAN_MAX_SIZE;

#ifdef RUNNING_MEDIAN_USE_MALLOC
  _values = (T *) malloc(_size * sizeof(T));
  _sortIdx  = (uint8_t *) malloc(_size * sizeof(uint8_t));
#endif
  clear();
}

template<typename T>
RunningMedian<T>::~RunningMedian()
{
  #ifdef RUNNING_MEDIAN_USE_MALLOC
  free(_values);
  free(_sortIdx);
  #endif
}


// resets all internal counters
template<typename T>
void RunningMedian<T>::clear()
{
  _count = 0;
  _index = 0;
  _sorted = false;
  for (uint8_t i = 0; i < _size; i++)
  {
    _sortIdx[i] = i;
  }
}


// adds a new value to the data-set
// or overwrites the oldest if full.
template<typename  T>
void RunningMedian<T>::add(T value)
{
  _values[_index++] = value;
  if (_index >= _size) _index = 0; // wrap around
  if (_count < _size) _count++;
  _sorted = false;
}

template<typename T>
T RunningMedian<T>::getMedian()
{
  if (_count == 0) return 0;

  if (_sorted == false) sort();

  if (_count & 0x01)  // is it odd sized?
  {
    return _values[_sortIdx[_count / 2]];
  }
  return (_values[_sortIdx[_count / 2]] + _values[_sortIdx[_count / 2 - 1]]) / 2;
}

template<>
float RunningMedian<float>::getMedian()
{
  if (_count == 0) return NAN;

  if (_sorted == false) sort();

  if (_count & 0x01)  // is it odd sized?
  {
    return _values[_sortIdx[_count / 2]];
  }
  return (_values[_sortIdx[_count / 2]] + _values[_sortIdx[_count / 2 - 1]]) / 2;
}

template<typename T>
T RunningMedian<T>::getAverage()
{
  if (_count == 0) return 0;

  T sum = 0;
  for (uint8_t i = 0; i < _count; i++)
  {
    sum += _values[i];
  }
  return sum / _count;
}

template<>
float RunningMedian<float>::getAverage()
{
  if (_count == 0) return NAN;

  float sum = 0;
  for (uint8_t i = 0; i < _count; i++)
  {
    sum += _values[i];
  }
  return sum / _count;
}

template<typename T>
void RunningMedian<T>::sort()
{
  // insertSort 
  for (uint16_t i = 1; i < _count; i++)
  {
    uint16_t z = i;
    uint16_t temp = _sortIdx[z];
    while ((z > 0) && (_values[temp] < _values[_sortIdx[z - 1]]))
    {
      _sortIdx[z] = _sortIdx[z - 1];
      z--;
    }
    _sortIdx[z] = temp;
  }
  _sorted = true;
}

template class RunningMedian<uint8_t>;

// -- END OF FILE --
