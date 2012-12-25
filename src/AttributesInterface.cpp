/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)
Copyright (C) 2006 Daniel Knobe (daniel-knobe@web.de)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
=============================================================================*/

/* header include */
#include "AttributesInterface.h"

/* includes */
#include <map>

#include <boost/any.hpp>
#include <boost/lexical_cast.hpp>

#include "Global.h"


/* implementation */
struct AttributesInterface::Container
{
	typedef std::map<std::string, boost::any> map_type;
	map_type mMap;

	map_type::const_iterator getIteratorTo(const std::string& name) const
	{
		map_type::const_iterator it = mMap.find(name);
		if(it == mMap.end())
		{
			BOOST_THROW_EXCEPTION (AttributeNotFoundException() << AttributeNotFoundException::attribute_name_info(name));
		}

		return it;
	}

	boost::any get(const std::string& name) const
	{
		return getIteratorTo(name)->second;
	}

	template<class T>
	void set(const std::string& name, const T& val)
	{
		mMap[name] = val;
	}

};

AttributesInterface::AttributesInterface() : mContainer( new  AttributesInterface::Container() )
{

}

AttributesInterface::~AttributesInterface()
{

}

std::string AttributesInterface::getAttributeAsString(const std::string name) const
{
	AttributeType type = getAttributeType(name);
	switch(type)
	{
		case EAT_BOOLEAN:
			return getAttributeBoolean(name) ? "true" : "false";
		case EAT_FLOAT:
			return boost::lexical_cast<std::string>(getAttributeFloat(name));
		case EAT_INTEGER:
			return boost::lexical_cast<std::string>(getAttributeInteger(name));
		case EAT_STRING:
			return getAttributeString(name);
		case EAT_COLOR:
			/// \todo
			return getAttributeString(name);
	}
}

std::string AttributesInterface::getAttributeString(const std::string& name) const
{
	boost::any attrib = mContainer->get(name);
	return boost::any_cast<std::string>( attrib );
}

float AttributesInterface::getAttributeFloat(const std::string& name) const
{
	boost::any attrib = mContainer->get(name);
	return boost::any_cast<float>( attrib );
}

int AttributesInterface::getAttributeInteger(const std::string& name) const
{
	boost::any attrib = mContainer->get(name);
	return boost::any_cast<int>( attrib );
}

bool AttributesInterface::getAttributeBoolean(const std::string& name) const
{
	boost::any attrib = mContainer->get(name);
	return boost::any_cast<bool>( attrib );
}

Color AttributesInterface::getAttributeColor(const std::string& name) const
{
	boost::any attrib = mContainer->get(name);
	return boost::any_cast<Color>( attrib );
}


AttributesInterface::AttributeType AttributesInterface::getAttributeType(const std::string& name) const
{
	boost::any value = mContainer->get(name);

	if(value.type() == typeid (int))
	{
		return EAT_INTEGER;
	}
	else if( value.type() == typeid(float))
	{
		return EAT_FLOAT;
	}
	else if( value.type() == typeid(bool))
	{
		return EAT_BOOLEAN;
	}
	else if( value.type() == typeid(std::string))
	{
		return EAT_STRING;
	}
	else if( value.type() == typeid(Color))
	{
		return EAT_COLOR;
	}

	return EAT_UNKNOWN;
}

unsigned int AttributesInterface::getAttributeCount() const
{
	return mContainer->mMap.size();
}

std::string AttributesInterface::getAttributeName(unsigned int i) const
{
	if(i >= mContainer->mMap.size())
	{
		BOOST_THROW_EXCEPTION (AttributeNotFoundException());
	}
	Container::map_type::iterator it = mContainer->mMap.begin();
	std::advance(it, i);

	return it->first;
}

void AttributesInterface::setAttributeString(const std::string& name, std::string value)
{
	mContainer->set(name, value);
}

void AttributesInterface::setAttributeFloat(const std::string& name, float value)
{
	mContainer->set(name, value);
}

void AttributesInterface::setAttributeInteger(const std::string& name, int value)
{
	mContainer->set(name, value);
}

void AttributesInterface::setAttributeBoolean(const std::string& name, bool value)
{
	mContainer->set(name, value);
}

void AttributesInterface::setAttributeColor(const std::string& name, Color value)
{
	mContainer->set(name, value);
}
