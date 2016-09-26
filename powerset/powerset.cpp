/*
 * Reading a list of integers from a file and form a set.
 *
 * Calculate the powerset of this set.
 *
 *
 */

#include <iostream>
#include <fstream>
#include <iomanip>
#include <set>
#include <assert.h>

using namespace std;

// overload operator+ to add 2 powersets
set< set<long> > operator + (const set< set<long> > a, const set< set<long> > b)
{
    set< set<long> > result = a;

    //debug
    //cout << __FUNCTION__ << endl;

    for(set< set<long> >::iterator i=b.begin(); i!=b.end(); ++i) {
       result.insert(*i); 
    }

    return result;
}


set< set<long> > generatePowerSet( const set<long> inputSet)
{
    set< set<long> > powerSet;
    set<long> subSet = inputSet;

#if 0
    static long level = 0;

    // debugging
    cout << __FUNCTION__ << "[" << level++ << "]" <<": ";
    for(set<long>::iterator i=inputSet.begin(); i!=inputSet.end(); ++i) {
        cout << *i << " ";
    }
    cout << endl;
#endif //0

    powerSet.insert(inputSet); // add to result

    // in each loop, only one element of inputSet is removed to create 
    // a subset, which is added to the result and also passed down 
    // the recursion.
    for(set<long>::iterator i=inputSet.begin(); i!=inputSet.end(); ++i) {

        // remove only one element 
        set<long>::iterator t = subSet.find(*i);
        long tv = *t;

        assert(t!=subSet.end()); // should not happend
        subSet.erase(tv);

        powerSet.insert(subSet); // add to result

        set< set<long> > rset = generatePowerSet(subSet);
        powerSet = powerSet + rset; // add to result

        // restore this element
        // in the next loop we will remove the next element
        subSet.insert(tv);
    }

    return powerSet;
}

int main(int argc, char **argv)
{
    const char *fileName = "test.txt";
    set<long> inputSet;
    set< set<long> > powerSet;

    ifstream inFile(fileName);
    if(!inFile) {
        cout << "cannot open " << fileName << " for reading." << endl;
        return 1;
    }

    long n;
    long count=0;
    cout << "reading from file" << endl;
    while(true) {
        
        inFile >> n;
        
        // eof() is only true when a read reaches end of file.
        // this is why eof() must be tested after a read.
        if(inFile.eof()) {
            break;        
        }

        // reading into long integer failed
        if(inFile.fail()) {
            cout << endl << "reading failed, stop" << endl;
            break;
        }

        inputSet.insert(n);
        cout << n << " ";
        count++;
    }
    cout << endl << "read " << count << " numbers" << endl;

    // printing the input set
    count=0;
    cout << "iterate the input set" << endl;
    for(set<long>::iterator i=inputSet.begin(); i!=inputSet.end(); ++i) {
        cout << *i << " ";
        count++;
    }
    cout << endl << "set size: " << count << endl;

    // generate and print power set
    cout << "generate powerset...." << endl;
    powerSet = generatePowerSet(inputSet);

    cout << "printing elements of powerset" << endl;
    for(set< set<long> >::iterator i=powerSet.begin(); i!=powerSet.end(); ++i) {
        cout << "{";
        for(set<long>::iterator j=i->begin(); j!=i->end(); ++j) {
            if(j!=i->begin()) cout << ", ";
            cout << *j;
        }
        cout << "}" << endl;
    }

    return 0;
}

