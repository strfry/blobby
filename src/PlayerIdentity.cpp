#include "PlayerIdentity.h"

PlayerIdentity::PlayerIdentity(std::string name) : mName(name)
{
	
}

PlayerIdentity::~PlayerIdentity()
{
	
}

std::string PlayerIdentity::getName() const
{
	return mName;
}

Color PlayerIdentity::getStaticColor() const
{
	return mStaticColor;
}

bool PlayerIdentity::getOscillating() const
{
	return mOscillating;
}

void PlayerIdentity::setStaticColor(Color c)
{
	mStaticColor = c;
}

void PlayerIdentity::setOscillating(bool oc)
{
	mOscillating = oc;
}
