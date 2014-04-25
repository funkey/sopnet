#include "MinimalImpactTEDWriter.h" 
#include <iostream>
#include <fstream>
#include <algorithm>

MinimalImpactTEDWriter::MinimalImpactTEDWriter()
{

	registerInput(_goldStandard, "gold standard");

}

void
MinimalImpactTEDWriter::write(std::string filename) {

	updateInputs();

}

