/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "CreatureAI.h"
#include "HostileRefMgr.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "ThreatMgr.h"
#include "Unit.h"

HostileRefMgr::~HostileRefMgr()
{
    deleteReferences();
}

//=================================================
// send threat to all my hateres for the victim
// The victim is hated than by them as well
// use for buffs and healing threat functionality

void HostileRefMgr::threatAssist(Unit* victim, float baseThreat, SpellInfo const* threatSpell)
{
    if (getSize() == 0)
        return;

    HostileReference* ref = getFirst();
    float threat = ThreatCalcHelper::calcThreat(victim, iOwner, baseThreat, (threatSpell ? threatSpell->GetSchoolMask() : SPELL_SCHOOL_MASK_NORMAL), threatSpell);
    threat /= getSize();
    while (ref)
    {
        if (ThreatCalcHelper::isValidProcess(victim, ref->GetSource()->GetOwner(), threatSpell))
            ref->GetSource()->doAddThreat(victim, threat);

        ref = ref->next();
    }
}

//=================================================

void HostileRefMgr::addTempThreat(float threat, bool apply)
{
    HostileReference* ref = getFirst();

    while (ref)
    {
        if (apply)
        {
            if (ref->getTempThreatModifier() == 0.0f)
                ref->addTempThreat(threat);
        }
        else
            ref->resetTempThreat();

        ref = ref->next();
    }
}

//=================================================

void HostileRefMgr::addThreatPercent(int32 percent)
{
    HostileReference* ref = getFirst();
    while (ref)
    {
        ref->addThreatPercent(percent);
        ref = ref->next();
    }
}

//=================================================
// The online / offline status is given to the method. The calculation has to be done before

void HostileRefMgr::setOnlineOfflineState(bool isOnline)
{
    HostileReference* ref = getFirst();
    while (ref)
    {
        ref->setOnlineOfflineState(isOnline);
        ref = ref->next();
    }
}

//=================================================
// The online / offline status is calculated and set

void HostileRefMgr::updateThreatTables()
{
    HostileReference* ref = getFirst();
    while (ref)
    {
        ref->updateOnlineStatus();
        ref = ref->next();
    }
}

//=================================================
// The references are not needed anymore
// tell the source to remove them from the list and free the mem

void HostileRefMgr::deleteReferences(bool removeFromMap /*= false*/)
{
    HostileReference* ref = getFirst();
    while (ref)
    {
        HostileReference* nextRef = ref->next();
        ref->removeReference();

        if (removeFromMap)
        {
            if (ThreatMgr const* threatMgr = ref->GetSource())
            {
                if (threatMgr->areThreatListsEmpty())
                {
                    if (Creature* creature = threatMgr->GetOwner()->ToCreature())
                    {
                        creature->AI()->EnterEvadeMode();
                    }
                }
            }
        }

        delete ref;
        ref = nextRef;
    }
}

//=================================================
// delete one reference, defined by faction

void HostileRefMgr::deleteReferencesForFaction(uint32 faction)
{
    HostileReference* ref = getFirst();
    while (ref)
    {
        HostileReference* nextRef = ref->next();
        if (ref->GetSource()->GetOwner()->GetFactionTemplateEntry()->faction == faction)
        {
            ref->removeReference();
            delete ref;
        }
        ref = nextRef;
    }
}

//=================================================
// delete one reference, defined by Unit

void HostileRefMgr::deleteReference(Unit* creature)
{
    HostileReference* ref = getFirst();
    while (ref)
    {
        HostileReference* nextRef = ref->next();
        if (ref->GetSource()->GetOwner() == creature)
        {
            ref->removeReference();
            delete ref;
            break;
        }
        ref = nextRef;
    }
}

//=================================================
// delete all references out of specified range

void HostileRefMgr::deleteReferencesOutOfRange(float range)
{
    HostileReference* ref = getFirst();
    range = range * range;
    while (ref)
    {
        HostileReference* nextRef = ref->next();
        Unit* owner = ref->GetSource()->GetOwner();
        if (!owner->isActiveObject() && owner->GetExactDist2dSq(GetOwner()) > range)
        {
            ref->removeReference();
            delete ref;
        }
        ref = nextRef;
    }
}

//=================================================
// set state for one reference, defined by Unit

void HostileRefMgr::setOnlineOfflineState(Unit* creature, bool isOnline)
{
    HostileReference* ref = getFirst();
    while (ref)
    {
        HostileReference* nextRef = ref->next();
        if (ref->GetSource()->GetOwner() == creature)
        {
            ref->setOnlineOfflineState(isOnline);
            break;
        }
        ref = nextRef;
    }
}

//=================================================

void HostileRefMgr::UpdateVisibility(bool checkThreat)
{
    HostileReference* ref = getFirst();
    while (ref)
    {
        HostileReference* nextRef = ref->next();
        if ((!checkThreat || ref->GetSource()->getThreatList().size() <= 1) &&
                !ref->GetSource()->GetOwner()->CanSeeOrDetect(GetOwner()))
        {
            nextRef = ref->next();
            ref->removeReference();
            delete ref;
        }
        ref = nextRef;
    }
}
