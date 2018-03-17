#include "stdafx.h"
#include "HeroCard.h"

CHeroCard::CHeroCard(int hero_id) :
hero_template_{ CHeroTP::GetHeroTP(hero_id) }
{
	quality_ = 1;
	trains_.fill(0);
	runes_.fill(0);
}

CHeroCard::~CHeroCard()
{

}
