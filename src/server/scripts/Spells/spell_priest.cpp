/*
 * Copyright (C) 2008 - 2011 Trinity <http://www.trinitycore.org/>
 *
 * Copyright (C) 2010 - 2015 Myth Project <http://mythprojectnetwork.blogspot.com/>
 *
 * Myth Project's source is based on the Trinity Project source, you can find the
 * link to that easily in Trinity Copyrights. Myth Project is a private community.
 * To get access, you either have to donate or pass a developer test.
 * You may not share Myth Project's sources! For personal use only.
 */

#include "ScriptPCH.h"
#include "SpellAuraEffects.h"

enum PriestSpells
{
    PRIEST_SPELL_GUARDIAN_SPIRIT_HEAL           = 48153,
    PRIEST_SPELL_PENANCE_R1                     = 47540,
    PRIEST_SPELL_PENANCE_R1_DAMAGE              = 47758,
    PRIEST_SPELL_PENANCE_R1_HEAL                = 47757,
    PRIEST_SPELL_REFLECTIVE_SHIELD_TRIGGERED    = 33619,
    PRIEST_SPELL_REFLECTIVE_SHIELD_R1           = 33201,
};

// Guardian Spirit
class spell_pri_guardian_spirit : public SpellScriptLoader
{
public:
    spell_pri_guardian_spirit() : SpellScriptLoader("spell_pri_guardian_spirit") { }

    class spell_pri_guardian_spirit_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_pri_guardian_spirit_AuraScript);

        uint32 healPct;

        bool Validate(SpellEntry const* /*pSpellEntry*/)
        {
            return sSpellStore.LookupEntry(PRIEST_SPELL_GUARDIAN_SPIRIT_HEAL) != NULL;
        }

        bool Load()
        {
            healPct = SpellMgr::CalculateSpellEffectAmount(GetSpellProto(), EFFECT_1);
            return true;
        }

        void CalculateAmount(AuraEffect const* /*aurEff*/, int32 & amount, bool & /*canBeRecalculated*/)
        {
            // Set absorbtion amount to unlimited
            amount = -1;
        }

        void Absorb(AuraEffect* /*aurEff*/, DamageInfo & dmgInfo, uint32 & absorbAmount)
        {
            Unit* target = GetTarget();
            if(dmgInfo.GetDamage() < target->GetHealth())
                return;

            int32 healAmount = int32(target->CountPctFromMaxHealth(healPct));
            // remove the aura now, we don't want 40% healing bonus
            Remove(AURA_REMOVE_BY_ENEMY_SPELL);
            target->CastCustomSpell(target, PRIEST_SPELL_GUARDIAN_SPIRIT_HEAL, &healAmount, NULL, NULL, true);
            absorbAmount = dmgInfo.GetDamage();
        }

        void Register()
        {
            DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_pri_guardian_spirit_AuraScript::CalculateAmount, EFFECT_1, SPELL_AURA_SCHOOL_ABSORB);
            OnEffectAbsorb += AuraEffectAbsorbFn(spell_pri_guardian_spirit_AuraScript::Absorb, EFFECT_1);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_pri_guardian_spirit_AuraScript();
    }
};

class spell_pri_mana_burn : public SpellScriptLoader
{
public:
    spell_pri_mana_burn() : SpellScriptLoader("spell_pri_mana_burn") { }

    class spell_pri_mana_burn_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_pri_mana_burn_SpellScript);

        void HandleAfterHit()
        {
            Unit* unitTarget = GetHitUnit();
            if(!unitTarget)
                return;

            unitTarget->RemoveAurasWithMechanic((1 << MECHANIC_FEAR) | (1 << MECHANIC_POLYMORPH));
        }

        void Register()
        {
            AfterHit += SpellHitFn(spell_pri_mana_burn_SpellScript::HandleAfterHit);
        }
    };

    SpellScript * GetSpellScript() const
    {
        return new spell_pri_mana_burn_SpellScript;
    }
};

class spell_pri_mind_sear : public SpellScriptLoader
{
public:
    spell_pri_mind_sear() : SpellScriptLoader("spell_pri_mind_sear") { }

    class spell_pri_mind_sear_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_pri_mind_sear_SpellScript);

        void FilterTargets(std::list<Unit*>& unitList)
        {
            unitList.remove(GetTargetUnit());
        }

        void Register()
        {
            OnUnitTargetSelect += SpellUnitTargetFn(spell_pri_mind_sear_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_AREA_ENEMY_DST);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_pri_mind_sear_SpellScript();
    }
};

class spell_pri_pain_and_suffering_proc : public SpellScriptLoader
{
public:
    spell_pri_pain_and_suffering_proc() : SpellScriptLoader("spell_pri_pain_and_suffering_proc") { }

    // 47948 Pain and Suffering (proc)
    class spell_pri_pain_and_suffering_proc_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_pri_pain_and_suffering_proc_SpellScript);

        void HandleEffectScriptEffect(SpellEffIndex /*effIndex*/)
        {
            // Refresh Shadow Word: Pain on target
            if(Unit* unitTarget = GetHitUnit())
                if(AuraEffect* aur = unitTarget->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_PRIEST, 0x8000, 0, 0, GetCaster()->GetGUID()))
                    aur->GetBase()->RefreshDuration();
        }

        void Register()
        {
            OnEffect += SpellEffectFn(spell_pri_pain_and_suffering_proc_SpellScript::HandleEffectScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_pri_pain_and_suffering_proc_SpellScript;
    }
};

class spell_pri_penance : public SpellScriptLoader
{
public:
    spell_pri_penance() : SpellScriptLoader("spell_pri_penance") { }

    class spell_pri_penance_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_pri_penance_SpellScript);

        bool Load()
        {
            return GetCaster()->GetTypeId() == TYPEID_PLAYER;
        }

        bool Validate(SpellEntry const* spellEntry)
        {
            if(!sSpellStore.LookupEntry(PRIEST_SPELL_PENANCE_R1))
                return false;
            // can't use other spell than this penance due to spell_ranks dependency
            if(sSpellMgr->GetFirstSpellInChain(PRIEST_SPELL_PENANCE_R1) != sSpellMgr->GetFirstSpellInChain(spellEntry->Id))
                return false;

            uint8 rank = sSpellMgr->GetSpellRank(spellEntry->Id);
            if(!sSpellMgr->GetSpellWithRank(PRIEST_SPELL_PENANCE_R1_DAMAGE, rank, true))
                return false;
            if(!sSpellMgr->GetSpellWithRank(PRIEST_SPELL_PENANCE_R1_HEAL, rank, true))
                return false;

            return true;
        }

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            Unit* unitTarget = GetHitUnit();
            if(!unitTarget || !unitTarget->isAlive())
                return;

            Unit* caster = GetCaster();

            uint8 rank = sSpellMgr->GetSpellRank(GetSpellInfo()->Id);

            if(caster->IsFriendlyTo(unitTarget))
            {
                Unit* spellTarget = unitTarget;
                if(unitTarget->GetTypeId() == TYPEID_PLAYER && unitTarget->ToPlayer()->duel != NULL)
                    spellTarget = caster;

                caster->CastSpell(spellTarget, sSpellMgr->GetSpellWithRank(PRIEST_SPELL_PENANCE_R1_HEAL, rank), false, 0);
            }
            else
                caster->CastSpell(unitTarget, sSpellMgr->GetSpellWithRank(PRIEST_SPELL_PENANCE_R1_DAMAGE, rank), false, 0);
        }

        SpellCastResult CheckCast()
        {
            Player* caster = GetCaster()->ToPlayer();
            if(GetTargetUnit())
                if(Unit* target = GetTargetUnit())
                    if(!caster->IsFriendlyTo(target) && !caster->IsHostileTo(target))
                        return SPELL_FAILED_BAD_TARGETS;
            return SPELL_CAST_OK;
        }

        void Register()
        {
            // add dummy effect spell handler to Penance
            OnEffect += SpellEffectFn(spell_pri_penance_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            OnCheckCast += SpellCheckCastFn(spell_pri_penance_SpellScript::CheckCast);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_pri_penance_SpellScript;
    }
};

// Reflective Shield
class spell_pri_reflective_shield_trigger : public SpellScriptLoader
{
public:
    spell_pri_reflective_shield_trigger() : SpellScriptLoader("spell_pri_reflective_shield_trigger") { }

    class spell_pri_reflective_shield_trigger_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_pri_reflective_shield_trigger_AuraScript);

        bool Validate(SpellEntry const* /*pSpellEntry*/)
        {
            return sSpellStore.LookupEntry(PRIEST_SPELL_REFLECTIVE_SHIELD_TRIGGERED) && sSpellStore.LookupEntry(PRIEST_SPELL_REFLECTIVE_SHIELD_R1);
        }

        void Trigger(AuraEffect* aurEff, DamageInfo & dmgInfo, uint32 & absorbAmount)
        {
            Unit* target = GetTarget();
            if(dmgInfo.GetAttacker() == target)
                return;
            Unit* caster = GetCaster();
            if(!caster)
                return;
            if(AuraEffect* talentAurEff = target->GetAuraEffectOfRankedSpell(PRIEST_SPELL_REFLECTIVE_SHIELD_R1, EFFECT_0))
            {
                int32 bp = CalculatePctN(absorbAmount, talentAurEff->GetAmount());
                target->CastCustomSpell(dmgInfo.GetAttacker(), PRIEST_SPELL_REFLECTIVE_SHIELD_TRIGGERED, &bp, NULL, NULL, true, NULL, aurEff);
            }
        }

        void Register()
        {
             AfterEffectAbsorb += AuraEffectAbsorbFn(spell_pri_reflective_shield_trigger_AuraScript::Trigger, EFFECT_0);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_pri_reflective_shield_trigger_AuraScript();
    }
};

void AddSC_priest_spell_scripts()
{
    new spell_pri_guardian_spirit;
    new spell_pri_mana_burn;
    new spell_pri_pain_and_suffering_proc;
    new spell_pri_penance;
    new spell_pri_reflective_shield_trigger;
    new spell_pri_mind_sear;
}
