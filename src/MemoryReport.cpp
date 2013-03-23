#include "MemoryReport.h"
#include "BlobbyDebug.h"
#include <map>

struct MemoryReport::MRImpl
{
	std::map<std::string, CountingReport> map;
};


MemoryReport::MemoryReport() : mImpl( new MRImpl )
{

}

MemoryReport::MemoryReport(const MemoryReport& other): mImpl( new MRImpl )
{
	mImpl->map = other.mImpl->map;
}

MemoryReport::~MemoryReport()
{

}

MemoryReport& MemoryReport::operator=(const MemoryReport& other)
{
	mImpl->map = other.mImpl->map;
	return *this;
}

int MemoryReport::getInstanceCount(const std::type_info& type) const
{
	return mImpl->map[type.name()].alive;
}

int MemoryReport::getInstanceCount(const std::string& type) const
{
	return mImpl->map[type].alive;
}


void MemoryReport::subtract(const MemoryReport& other)
{
	for(std::map<std::string, CountingReport>::iterator i = mImpl->map.begin(), j = other.mImpl->map.begin(); i != mImpl->map.end() && j != other.mImpl->map.end(); ++i, ++j)
	{
		i->second.alive -= j->second.alive;
		i->second.created -= j->second.created;
	}

	/// delete all unchanged totals
	std::map<std::string, CountingReport>::iterator testit = mImpl->map.begin();
	std::map<std::string, CountingReport>::iterator old = testit;
	do
	{
		if(testit->second.alive <=0 )
		{
			if(testit != mImpl->map.begin())
			{
				mImpl->map.erase(testit);
				testit = old;
			}
			else
			{
				mImpl->map.erase(testit);
				testit = mImpl->map.begin();
			}
		}

		old = testit;
		++testit;

	}while(testit != mImpl->map.end());
}

void MemoryReport::print(std::ostream& stream) const
{
	int sum = 0;
	for(std::map<std::string, CountingReport>::iterator i = mImpl->map.begin(); i != mImpl->map.end(); ++i)
	{
		stream << i->first << "\n- - - - - - - - - -\n";
		stream << " alive:   " << i->second.alive << "\n";
		stream << " created: " << i->second.created << "\n\n";
		sum += i->second.alive;
	}
	stream << "TOTAL: " << sum << std::endl;
}

std::map<std::string, CountingReport>& GetCounterMap();

MemoryReport MemoryReport::createReport()
{
	MemoryReport report;
	report.mImpl->map = GetCounterMap();

	return report;
}
