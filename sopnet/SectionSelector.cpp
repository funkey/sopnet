#include "SectionSelector.h"

logger::LogChannel sectionselectorlog("sectionselectorlog", "[SectionSelector] ");

SectionSelector::SectionSelector(int firstSection, int lastSection) :
	_firstSection(firstSection),
	_lastSection(lastSection) {

	registerInput(_stack, "stack");
	registerOutput(_subStack, "stack");
}

void
SectionSelector::updateOutputs() {

	_subStack->clear();

	LOG_ALL(sectionselectorlog)
			<< "selecting sections from stack of size "
			<< _stack->size() << std::endl;

	LOG_ALL(sectionselectorlog)
			<< "first section is " << _firstSection
			<< ", last section is " << _lastSection
			<< std::endl;

	unsigned int lastSection = (_lastSection < 0 ? _stack->size() - 1 : _lastSection);

	LOG_ALL(sectionselectorlog)
			<< "set last section to " << lastSection
			<< std::endl;

	if (lastSection > _stack->size() - 1) {

		LOG_ERROR(sectionselectorlog)
				<< "parameter last section (" << lastSection << ") "
				<< "is bigger than number of sections in given stack -- "
				<< "will use " << (_stack->size() - 1) << " instead"
				<< std::endl;

		lastSection = _stack->size() - 1;
	}

	for (int i = _firstSection; i <= lastSection; i++)
		_subStack->add((*_stack)[i]);
}
