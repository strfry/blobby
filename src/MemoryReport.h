#ifndef MEMORYREPORT_H_INCLUDED
#define MEMORYREPORT_H_INCLUDED

#include <boost/scoped_ptr.hpp>

class MemoryReport
{
	public:
		MemoryReport();
		MemoryReport(const MemoryReport& other);
		~MemoryReport();

		void subtract(const MemoryReport& other);

		int getInstanceCount(const std::type_info& type) const;
		void print(std::ostream& stream) const;

		static MemoryReport createReport();

	private:
		struct MRImpl;
		boost::scoped_ptr<MRImpl> mImpl;
};

#endif // MEMORYREPORT_H_INCLUDED
