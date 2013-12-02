#include "FileContentProvider.h"

logger::LogChannel filecontentproviderlog("filecontentproviderlog", "[FileContentProvider] ");


FileContentProvider::FileContentProvider(std::string filename) :
	_filepath(filename) {

	registerOutput(_content, "content");
	startInotifyThread();
}

FileContentProvider::~FileContentProvider() {

	stopInotifyThread();
}

void
FileContentProvider::updateOutputs() {

	if (_content->is_open())
		_content->close();
	_content->open(_filepath.string().c_str());
}

void
FileContentProvider::startInotifyThread() {

	_inThread = boost::make_shared<boost::thread>(boost::bind(&FileContentProvider::watch, this));
}

void
FileContentProvider::stopInotifyThread() {

	_stopped = true;

	// interrupt select
	char c = 0;
	if (!write(_interruptFds[1], &c, 1))
		LOG_ERROR(filecontentproviderlog) << "[FileContentProvider] couldn't write to interrupt pipe" << std::endl;
	close(_interruptFds[1]);

	// wait for thread to finish
	_inThread->join();
}

void
FileContentProvider::watch() {

	boost::filesystem::path watchDir = _filepath.parent_path();

	initInotify();
	watchDirectory(watchDir);

	while (true) {

		LOG_DEBUG(filecontentproviderlog) << "[FileContentProvider] watching directory " << watchDir << std::endl;

		if (!checkEvents()) {

			LOG_DEBUG(filecontentproviderlog) << "[FileContentProvider] got an interrupt signal" << std::endl;
			break;
		}

		inotify_event* event = readInotifyEvent();

		if (event == NULL )
			break;

		LOG_ALL(filecontentproviderlog) << "directory " << watchDir << " changed!" << std::endl;
		LOG_ALL(filecontentproviderlog) << "\tmask is: " << event->mask << std::endl;
		LOG_ALL(filecontentproviderlog) << "\tname is: " << event->name << std::endl;

		if (strcmp(event->name, _filepath.leaf().c_str()) != 0) {

			LOG_ALL(filecontentproviderlog) << "file " << event->name << " changed, but I'm interested in " << _filepath.leaf() << std::endl;
			continue;
		}

		processInotifyEvent(event);
	}

	unwatchDirectory(watchDir);
	tearDownInotify();
}

void
FileContentProvider::initInotify() {

	// initialize inotify
	_inFd = inotify_init();

	if (_inFd < 0)
		LOG_ERROR(filecontentproviderlog) << "[FileContentProvider] could not init inotify" << std::endl;

	// create a buffer to read inotify events
	_inBufferSize = sizeof(struct inotify_event) + NAME_MAX + 1;
	_inBuffer = new char[_inBufferSize];

	// create interrupt pipe
	if (pipe(_interruptFds) < 0)
		LOG_ERROR(filecontentproviderlog) << "[FileContentProvider] could not create interrupt pipe" << std::endl;
}

void
FileContentProvider::watchDirectory(boost::filesystem::path dir) {

	_inWd = inotify_add_watch(
			_inFd,
			dir.string().c_str(),
			IN_CLOSE_WRITE | IN_MOVED_TO | IN_MOVED_FROM | IN_DELETE);

	if (_inWd < 0)
		LOG_ERROR(filecontentproviderlog) << "[FileContentProvider] could not add watch for " << dir << std::endl;
}

/**
 * Check for available events. Returns -1 if we got interrupted.
 */
bool
FileContentProvider::checkEvents() {

	// create a file descriptor set
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(_inFd, &readfds);
	FD_SET(_interruptFds[0], &readfds);

	// wait until either we got an inotify event from _inFd or an interrupt 
	// signal from _interruptFds[0]
	select(FD_SETSIZE, &readfds, NULL, NULL, NULL);

	return !_stopped;
}

inotify_event*
FileContentProvider::readInotifyEvent() {

	size_t result = read(_inFd, _inBuffer, _inBufferSize);

	// interupted by signal
	if (result == EINTR)
		return NULL;

	return (inotify_event*)_inBuffer;
}

void
FileContentProvider::processInotifyEvent(inotify_event* event) {

	switch (event->mask) {

		// changed
		case IN_CLOSE_WRITE:
		case IN_MOVED_TO:

			LOG_DEBUG(filecontentproviderlog) << "[FileContentProvider] the content of " << event->name << " changed!" << std::endl;
			setDirty(_content);
			break;

		// disappeared
		case IN_MOVED_FROM:
		case IN_DELETE:

			LOG_DEBUG(filecontentproviderlog) << "[FileContentProvider] the file " << event->name << " disappeared!" << std::endl;
			break;
	}
}

void
FileContentProvider::unwatchDirectory(boost::filesystem::path dir) {

	// remove watch
	if (inotify_rm_watch(_inFd, _inWd) < 0)
		LOG_ERROR(filecontentproviderlog) << "[FileContentProvider] could not remove watch for directory " << dir << std::endl;

	LOG_DEBUG(filecontentproviderlog) << "[FileContentProvider] stop observing directory " << dir << std::endl;
}

void
FileContentProvider::tearDownInotify() {

	close(_inFd);
	close(_interruptFds[0]);

	delete[] _inBuffer;
}
