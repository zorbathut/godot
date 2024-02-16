/**************************************************************************/
/*  logger.h                                                              */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef LOGGER_H
#define LOGGER_H

#include "core/io/file_access.h"
#include "core/string/ustring.h"
#include "core/templates/vector.h"

#include <stdarg.h>

namespace core_bind {
class LogManager;
}

class Logger {
protected:
	bool should_log(bool p_err);

	static bool _flush_stdout_on_print;

public:
	enum ErrorType {
		ERR_ERROR,
		ERR_WARNING,
		ERR_SCRIPT,
		ERR_SHADER
	};

	static void set_flush_stdout_on_print(bool value);

	virtual void logv(const char *p_format, va_list p_list, bool p_err) _PRINTF_FORMAT_ATTRIBUTE_2_0 = 0;
	virtual void log_error(const char *p_function, const char *p_file, int p_line, const char *p_code, const char *p_rationale, bool p_editor_notify = false, ErrorType p_type = ERR_ERROR);

	void logf(const char *p_format, ...) _PRINTF_FORMAT_ATTRIBUTE_2_3;
	void logf_error(const char *p_format, ...) _PRINTF_FORMAT_ATTRIBUTE_2_3;

	virtual ~Logger() {}
};

/**
 * Writes messages to stdout/stderr.
 */
class StdLogger : public Logger {
public:
	virtual void logv(const char *p_format, va_list p_list, bool p_err) override _PRINTF_FORMAT_ATTRIBUTE_2_0;
	virtual ~StdLogger() {}
};

/**
 * Writes messages to the specified file. If the file already exists, creates a copy (backup)
 * of it with timestamp appended to the file name. Maximum number of backups is configurable.
 * When maximum is reached, the oldest backups are erased. With the maximum being equal to 1,
 * it acts as a simple file logger.
 */
class RotatedFileLogger : public Logger {
	String base_path;
	int max_files;

	Ref<FileAccess> file;

	void clear_old_backups();
	void rotate_file();

public:
	explicit RotatedFileLogger(const String &p_base_path, int p_max_files = 10);

	virtual void logv(const char *p_format, va_list p_list, bool p_err) override _PRINTF_FORMAT_ATTRIBUTE_2_0;
};

class CompositeLogger : public Logger {
	Vector<Logger *> loggers;

public:
	explicit CompositeLogger(Vector<Logger *> p_loggers);

	virtual void logv(const char *p_format, va_list p_list, bool p_err) override _PRINTF_FORMAT_ATTRIBUTE_2_0;
	virtual void log_error(const char *p_function, const char *p_file, int p_line, const char *p_code, const char *p_rationale, bool p_editor_notify, ErrorType p_type = ERR_ERROR) override;

	void add_logger(Logger *p_logger);

	virtual ~CompositeLogger();
};

class UserLogManagerLogger : public Logger {
	static UserLogManagerLogger *singleton;

	Vector<Dictionary> buffered_logs;
	
	// Stored as Vector so we can iterate over it in a threadsafe manner without holding a lock
	// Deleted items are replaced by Callable(), with the slot later reused, to avoid problems with in-flight iterators
	Vector<Callable> captures_nonthreadsafe;
	Vector<Callable> captures_buffered;

	enum State {
		STATE_OFF,	// log messages are not processed in any way
		STATE_PASSTHROUGH,	// log messages are processed but sent straight to nonthreadsafe
		STATE_BUFFERING,	// log messages are processed and buffered until the next flush (and maybe sent straight to nonthreadsafe also)
	};
	std::atomic<State> state;

	// This is expected to be called from multiple threads.
	// Right now we have a single mutex that applies to any access of any collection, as well as `state` transitions.
	// It's possible this should be split into multiple contextual states, but, man, good luck figuring out the details on that one.
	// You *must* release this mutex before calling any of the callables!
	// They might add log messages or add/remove callables of their own!
	Mutex mutex;

public:
	UserLogManagerLogger();
	~UserLogManagerLogger();

	static UserLogManagerLogger *get_singleton() { return singleton; }

	// Log input interface
	virtual void logv(const char *p_format, va_list p_list, bool p_err) override _PRINTF_FORMAT_ATTRIBUTE_2_0;
	virtual void log_error(const char *p_function, const char *p_file, int p_line, const char *p_code, const char *p_rationale, bool p_editor_notify, ErrorType p_type = ERR_ERROR) override;

	// Callback registration/unregistration (via LogManager)
	// Must be called from the main thread
	void register_log_capture_nonthreadsafe(const Callable &p_callable);
	void unregister_log_capture_nonthreadsafe(const Callable &p_callable);
	void register_log_capture_buffered(const Callable &p_callable);
	void unregister_log_capture_buffered(const Callable &p_callable);

	// Buffer flush notification (currently on a per-frame basis)
	// Must be called from the main thread
	void flush();

private:
	void process(const Dictionary &p_message);

	static void dispatch_message(const Dictionary &p_message, const Callable &p_callable);

	// General "figure out what you should be doing" callback.
	// `mutex` *must* be held when calling this.
	void recalculate_state();

	// small utility functions for our sparse vector behavior, assumes the mutex is already held
	static void register_callable(Vector<Callable> &p_vector, const Callable &p_callable);
	static void unregister_callable(Vector<Callable> &p_vector, const Callable &p_callable);

	// small utility functions for common functionality
	uint64_t get_frames_drawn_safe() const;
};

#endif // LOGGER_H
