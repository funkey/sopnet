#ifndef FILE_CONTENT_PROVIDER_H__
#define FILE_CONTENT_PROVIDER_H__

#include <string>
#include <fstream>
#include <sys/inotify.h>

#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

#include <pipeline/SimpleProcessNode.h>
#include <util/Logger.h>

class FileContentProvider : public pipeline::SimpleProcessNode<> {

public:

	FileContentProvider(std::string filename);

	~FileContentProvider();

private:

	void updateOutputs();

	void startInotifyThread();

	void stopInotifyThread();

	/**
	 * Listen for inotify events for the specified file.
	 */
	void watch();

	void initInotify();

	/**
	 * Register dir for observation.
	 */
	void watchDirectory(boost::filesystem::path dir);

	/**
	 * Check for available events. Returns -1 if we got interrupted.
	 */
	bool checkEvents();

	/**
	 * Read one inotify event from the inotify file descriptor.
	 */
	inotify_event* readInotifyEvent();

	void processInotifyEvent(inotify_event* event);

	/**
	 * Unregister dir for observation.
	 */
	void unwatchDirectory(boost::filesystem::path dir);

	void tearDownInotify();

	pipeline::Output<std::ifstream> _content;

	boost::filesystem::path _filepath;

	// for inotify

	boost::shared_ptr<boost::thread> _inThread;

	int    _inFd;
	int    _inWd;
	char*  _inBuffer;
	size_t _inBufferSize;

	int  _interruptFds[2];
	bool _stopped;
};

#endif // FILE_CONTENT_PROVIDER_H__

