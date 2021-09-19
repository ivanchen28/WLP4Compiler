#include <string>
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <list>
#include "merl.h"

/*
Starter code for the linker you are to write for CS241 Assignment 10 Problem 5.
The code uses the MERL library defined in "merl.h". This library depends on the object file
"merl.o" and this program must be compiled with those object files.

The starter code includes functionality for processing command-line arguments, reading the input 
MERL files, calling the linker, and producing output on standard output.

You need to implement the "linking constructor" for MERL files, which takes two MERL file objects
as input and produces a new MERL file object representing the result of linking the files:

MERL::MERL(MERL& m1, MERL& m2)

A definition of this function is at the end of the file. The default implementation creates a
dummy MERL file to demonstrate the use of the MERL library. You should replace it with code that
links the two input files.

You are free to modify the existing code as needed, but it should not be necessary.
You are free to add your own helper functions to this file.

The functions in the MERL library will throw std::runtime_error if an error occurs. The starter
code is set up to catch these exceptions and output an error message to standard error. You may
wish to use this functionality for your own error handling.
*/

// Links all the MERL objects in the given list, by recursively
// calling the "linking constructor" that links two MERL objects.
// You should not need to modify this function.
MERL link(std::list<MERL>& merls) {
	if (merls.size() == 1) {
		return merls.front();
	}
	else if (merls.size() == 2) {
		return MERL(merls.front(),merls.back()); 
	} 
	MERL first = merls.front();
	merls.pop_front();
	MERL linkedRest = link(merls);
	return MERL(first,linkedRest);
}

// Main function, which reads the MERL files and passes them into
// the link function, then outputs the linked MERL file. 
// You should not need to modify this function.
int main (int argc, char* argv[]) {
	if (argc == 1) { 
		std::cerr << "Usage: " << argv[0] << " <file1.merl> <file2.merl> ..."  << std::endl;
		return 1;
	}
	std::list<MERL> merls;
	try {
		for (int i = 1; i < argc; i++) {
			std::ifstream file;
			file.open(argv[i]);
			merls.emplace_back(file);
			file.close();
		}
		// Link all the MERL files read from the command line.
		MERL linked = link(merls);
		// Print a readable representation of the linked MERL file to standard error for debugging.
		linked.print(std::cerr);
		// Write the binary representation of the linked MERL file to standard output.
		std::cout << linked;
	} catch(std::runtime_error &re) {
		std::cerr << "ERROR: " << re.what() << std::endl;
		return 1;
	}
	return 0;
}

// g++ -g -std=c++14 -Wall linker.cc merl.o

// Linking constructor for MERL objects.
// Implement this, which constructs a new MERL object by linking the two given MERL objects. 
// The function is allowed to modify the inputs m1 and m2. It does not need to leave them in 
// the same state as when the function was called. The default implementation creates a dummy 
// MERL file that does not depend on either input, to demonstrate the use of the MERL library.
// You should not output anything to standard output here; the main function handles output.
MERL::MERL(MERL& m1, MERL& m2) {
	// Step 1: Check for duplicate export errors
	for (auto & entry1 : m1.table) {
		if (entry1.type == Entry::Type::ESD) {
			for (auto & entry2 : m2.table) {
				if (entry2.type == Entry::Type::ESD) {
					if (entry1.name == entry2.name) {
						std::cerr << "ERROR: duplicate export " << entry1.name << std::endl;
					}
				}
			}
		}
	}

	// Step 2: Combine the code segments for the linked file
	code.reserve(m1.code.size() + m2.code.size());
	code.insert(code.end(), m1.code.begin(), m1.code.end());
	code.insert(code.end(), m2.code.begin(), m2.code.end());

	// Step 3: Relocate m2's table entries
	int offset = m1.endCode - 12;
	for (auto & entry : m2.table) entry.location = entry.location + offset;

	// Step 4: Relocate m2.code
	for (auto & entry : m2.table) {
		if (entry.type == Entry::Type::REL) {
			int index = (entry.location - 12) / 4;
			code[index] = code[index] + offset;
		}
	}

	// Step 5: Resolve imports for m1
	for (auto & entry1 : m1.table) {
		if (entry1.type == Entry::Type::ESR) {
			for (auto & entry2 : m2.table) {
				if (entry2.type == Entry::Type::ESD) {
					if (entry1.name == entry2.name) {
						int index = (entry1.location - 12) / 4;
						code[index] = entry2.location;
						entry1.type = Entry::Type::REL;
					}
				}
			}
		}
	}
	
	// Step 6: Resolve imports for m2
	for (auto & entry2 : m2.table) {
		if (entry2.type == Entry::Type::ESR) {
			for (auto & entry1 : m1.table) {
				if (entry1.type == Entry::Type::ESD) {
					if (entry2.name == entry1.name) {
						int index = (entry2.location - 12) / 4;
						code[index] = entry1.location;
						entry2.type = Entry::Type::REL;
					}
				}
			}
		}
	}

	// Step 7: Combine the tables for the linked file
	table.reserve(m1.table.size() + m2.table.size() );
	table.insert(table.end(), m1.table.begin(), m1.table.end());
	table.insert(table.end(), m2.table.begin(), m2.table.end());

	// Step 8: Compute the header information
	endCode = 12 + code.size() * 4;
	// obtain size of linked table
	int tableSize = 0;
	for (auto & entry : table) tableSize = tableSize + entry.size();
	endModule = endCode + tableSize;

	// Step 9: Output the MERL file
	// Output handled by main function
}
