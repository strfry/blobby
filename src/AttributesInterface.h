#ifndef ATTRIBUTESINTERFACE_H_INCLUDED
#define ATTRIBUTESINTERFACE_H_INCLUDED

#include <string>
#include <exception>

#include <boost/scoped_ptr.hpp>
#include <boost/exception/all.hpp>

class Color;

class AttributesInterface
{
	public:
		// exception classes
		class AttributeNotFoundException : public virtual boost::exception, public virtual std::exception
		{

		};

		typedef boost::error_info<struct attribute_name, std::string> attribute_name_info;

		AttributesInterface();
		~AttributesInterface();


		enum AttributeType
		{
			EAT_STRING,
			EAT_FLOAT,
			EAT_INTEGER,
			EAT_BOOLEAN,
			EAT_COLOR,
			EAT_UNKNOWN		//!< this value should never occur and indicates a serious error
		};

		// -------------------------------------------------------------------------------------------
		// reading interface
		// -------------------------------------------------------------------------------------------

		/// \brief Gets attribute value converted into a string
		std::string getAttributeAsString(const std::string name) const;

		//	querying attribute values
		///	\brief Gets a string attribute.
		///	\details Returns the attribute if it is of type string, otherwise throws
		///				boost::bad_any_cast
		std::string getAttributeString(const std::string& name) const;

		///	\brief Gets a float attribute.
		///	\details Returns the attribute if it is of type float, otherwise throws
		///				boost::bad_any_cast
		float getAttributeFloat(const std::string& name) const;

		///	\brief Gets an integer attribute.
		///	\details Returns the attribute if it is of type integer, otherwise throws
		///				boost::bad_any_cast
		int getAttributeInteger(const std::string& name) const;

		///	\brief Gets a boolean attribute.
		///	\details Returns the attribute if it is of type boolean, otherwise throws
		///				boost::bad_any_cast
		bool getAttributeBoolean(const std::string& name) const;

		///	\brief Gets a color attribute.
		///	\details Returns the attribute if it is of type color, otherwise throws
		///				boost::bad_any_cast
		Color getAttributeColor(const std::string& name) const;

		/// \brief Gets the type of the specified attribute
		AttributeType getAttributeType(const std::string& name) const;

		// -------------------------------------------------------------------------------------------
		// writing interface
		// -------------------------------------------------------------------------------------------

		void setAttributeString(const std::string& name, std::string value);

		void setAttributeFloat(const std::string& name, float value);

		void setAttributeInteger(const std::string& name, int value);

		void setAttributeBoolean(const std::string& name, bool value);

		void setAttributeColor(const std::string& name, Color value);

		// iterate over all attribute names
		unsigned int getAttributeCount() const;
		std::string getAttributeName(unsigned int i) const;

	private:
		struct Container;

		boost::scoped_ptr<Container> mContainer;
};

#endif // ATTRIBUTESINTERFACE_H_INCLUDED
