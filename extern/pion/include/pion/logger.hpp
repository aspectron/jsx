#ifndef _CUSTOM_PION_LOGGER_HPP_
#define _CUSTOM_PION_LOGGER_HPP_

#include <pion/config.hpp>

#include <boost/enable_shared_from_this.hpp>
#include <sstream>

#define HAVE_PION_LOGGER_DELEGATE 1

namespace pion
{

enum log_priority
{
	LOG_LEVEL_TRACE, LOG_LEVEL_DEBUG, LOG_LEVEL_INFO,
	LOG_LEVEL_WARN, LOG_LEVEL_ERROR, LOG_LEVEL_FATAL,

	LOG_LEVEL_MIN = LOG_LEVEL_TRACE, LOG_LEVEL_MAX = LOG_LEVEL_FATAL
};

class PION_API logger_delegate: public boost::enable_shared_from_this<logger_delegate>
{
public:
	virtual void output(log_priority level, const char *msg) = 0;
};

class PION_API logger
{
public:
	typedef log_priority priority;

	logger()
		: level_(LOG_LEVEL_FATAL)
	{
	}

	void set_delegate(boost::shared_ptr<logger_delegate> delegate) { delegate_ = delegate; }
	void reset_delegate() { delegate_.reset(); }

	priority  level() const { return level_; }
	void set_level(priority level)
	{
		if ( level >= LOG_LEVEL_MIN && level <= LOG_LEVEL_MAX )
		{
			level_ = level;
		}
	}

	void set_level_up()
	{
		if ( level_ < LOG_LEVEL_MAX )
		{
			level_ = static_cast<priority>(level_ + 1);
		}
	}

	void set_level_down()
	{
		if ( level_ > LOG_LEVEL_MIN )
		{
			level_ = static_cast<priority>(level_ - 1);
		}
	}

	void output(priority level, std::string const& str)
	{
		if ( delegate_ )
		{
			delegate_->output(level, str.c_str());
		}
	}

	static logger& generic()
	{
		static logger generic_;
		return generic_;
	}

	static void shutdown() { generic().reset_delegate(); }
	
private:
	boost::shared_ptr<logger_delegate> delegate_;
	priority level_;

};

#define PION_LOG_CONFIG_BASIC {}
#define PION_LOG_CONFIG(FILE) {}
#define PION_GET_LOGGER(NAME) pion::logger::generic()

#define PION_LOG_SETLEVEL_DEBUG(LOG) { LOG.set_level(pion::LOG_LEVEL_DEBUG); }
#define PION_LOG_SETLEVEL_INFO(LOG)  { LOG.set_level(pion::LOG_LEVEL_INFO); }
#define PION_LOG_SETLEVEL_WARN(LOG)  { LOG.set_level(pion::LOG_LEVEL_WARN); }
#define PION_LOG_SETLEVEL_ERROR(LOG) { LOG.set_level(pion::LOG_LEVEL_ERROR); }
#define PION_LOG_SETLEVEL_FATAL(LOG) { LOG.set_level(pion::LOG_LEVEL_FATAL); }
#define PION_LOG_SETLEVEL_UP(LOG)    { LOG.set_level_up(); }
#define PION_LOG_SETLEVEL_DOWN(LOG)  { LOG.set_level_down(); }

#define PION_LOG_DEBUG(LOG, MSG) if (LOG.level() <= pion::LOG_LEVEL_DEBUG) { std::stringstream oss; oss << MSG; LOG.output(pion::LOG_LEVEL_DEBUG,oss.str()); }
#define PION_LOG_INFO(LOG, MSG)  if (LOG.level() <= pion::LOG_LEVEL_INFO)  { std::stringstream oss; oss << MSG; LOG.output(pion::LOG_LEVEL_INFO,oss.str()); }
#define PION_LOG_WARN(LOG, MSG)  if (LOG.level() <= pion::LOG_LEVEL_WARN)  { std::stringstream oss; oss << MSG; LOG.output(pion::LOG_LEVEL_WARN,oss.str()); }
#define PION_LOG_ERROR(LOG, MSG) if (LOG.level() <= pion::LOG_LEVEL_ERROR) { std::stringstream oss; oss << MSG; LOG.output(pion::LOG_LEVEL_ERROR,oss.str()); }
#define PION_LOG_FATAL(LOG, MSG) if (LOG.level() <= pion::LOG_LEVEL_FATAL) { std::stringstream oss; oss << MSG; LOG.output(pion::LOG_LEVEL_FATAL,oss.str()); }

}

#endif // _CUSTOM_PION_LOGGER_HPP_
