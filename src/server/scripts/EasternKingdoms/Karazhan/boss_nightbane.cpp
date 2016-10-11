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
#include "karazhan.h"

//phase 1
#define SPELL_BELLOWING_ROAR        39427
#define SPELL_CHARRED_EARTH         30129
#define SPELL_DISTRACTING_ASH       30130
#define SPELL_SMOLDERING_BREATH     30210
#define SPELL_TAIL_SWEEP            25653
//phase 2
#define SPELL_RAIN_OF_BONES         37098
#define SPELL_SMOKING_BLAST         37057
#define SPELL_FIREBALL_BARRAGE      30282
#define SPELL_SEARING_CINDERS       30127
#define SPELL_SUMMON_SKELETON       30170

#define EMOTE_SUMMON                "An ancient being awakens in the distance..."
#define YELL_AGGRO                  "What fools! I shall bring a quick end to your suffering!"
#define YELL_FLY_PHASE              "Miserable vermin. I shall exterminate you from the air!"
#define YELL_LAND_PHASE_1           "Enough! I shall land and crush you myself!"
#define YELL_LAND_PHASE_2           "Insects! Let me show you my strength up close!"
#define EMOTE_BREATH                "takes a deep breath."

float IntroWay[8][3] =
{
    {-11053.37f, -1794.48f, 149.00f},
    {-11141.07f, -1841.40f, 125.00f},
    {-11187.28f, -1890.23f, 125.00f},
    {-11189.20f, -1931.25f, 125.00f},
    {-11153.76f, -1948.93f, 125.00f},
    {-11128.73f, -1929.75f, 125.00f},
    {-11140.00f, -1915.00f, 122.00f},
    {-11163.00f, -1903.00f, 91.473f}
};

class boss_nightbane : public CreatureScript
{
public:
    boss_nightbane() : CreatureScript("boss_nightbane") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_nightbaneAI(pCreature);
    }

    struct boss_nightbaneAI : public ScriptedAI
    {
        boss_nightbaneAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            pInstance = pCreature->GetInstanceScript();
            Intro = true;
        }

        InstanceScript* pInstance;

        uint32 Phase;

        bool RainBones;
        bool Skeletons;

        uint32 BellowingRoarTimer;
        uint32 CharredEarthTimer;
        uint32 DistractingAshTimer;
        uint32 SmolderingBreathTimer;
        uint32 TailSweepTimer;
        uint32 RainofBonesTimer;
        uint32 SmokingBlastTimer;
        uint32 FireballBarrageTimer;
        uint32 SearingCindersTimer;

        uint32 FlyCount;
        uint32 FlyTimer;

        bool Intro;
        bool Flying;
        bool Movement;
        bool AllowWaitTimer;

        uint32 WaitTimer;
        uint32 MovePhase;

        void Reset()
        {
            BellowingRoarTimer = 30000;
            CharredEarthTimer = 15000;
            DistractingAshTimer = 20000;
            SmolderingBreathTimer = 10000;
            TailSweepTimer = 12000;
            RainofBonesTimer = 10000;
            SmokingBlastTimer = 20000;
            FireballBarrageTimer = 13000;
            SearingCindersTimer = 14000;
            WaitTimer = 1000;
            AllowWaitTimer = true;

            Phase = 1;
            FlyCount = 0;
            MovePhase = 0;

            me->SetSpeed(MOVE_RUN, 2.0f);
            me->AddUnitMovementFlag(MOVEMENTFLAG_LEVITATING);
            me->RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);
            me->setActive(true);

            if(pInstance)
            {
                pInstance->SetData64(DATA_NIGHTBANE, me->GetGUID());

                if(pInstance->GetData(TYPE_NIGHTBANE) == DONE || pInstance->GetData(TYPE_NIGHTBANE) == IN_PROGRESS)
                    me->DisappearAndDie();
                else
                    pInstance->SetData(TYPE_NIGHTBANE, NOT_STARTED);
            }

            HandleTerraceDoors(true);

            Flying = false;
            Movement = false;

            if(!Intro)
            {
                me->SetHomePosition(IntroWay[7][0], IntroWay[7][1], IntroWay[7][2], 0);
                me->GetMotionMaster()->MoveTargetedHome();
            }
        }

        void HandleTerraceDoors(bool open)
        {
            if(pInstance)
            {
                pInstance->HandleGameObject(pInstance->GetData64(DATA_MASTERS_TERRACE_DOOR_1), open);
                pInstance->HandleGameObject(pInstance->GetData64(DATA_MASTERS_TERRACE_DOOR_2), open);
            }
        }

        void EnterCombat(Unit* /*pWho*/)
        {
            if(pInstance)
                pInstance->SetData(TYPE_NIGHTBANE, IN_PROGRESS);

            HandleTerraceDoors(false);
            me->MonsterYell(YELL_AGGRO, LANG_UNIVERSAL, 0);
        }

        void AttackStart(Unit* who)
        {
            if(!Intro && !Flying)
                ScriptedAI::AttackStart(who);
        }

        void JustDied(Unit* /*pKiller*/)
        {
            if(pInstance)
                pInstance->SetData(TYPE_NIGHTBANE, DONE);

            HandleTerraceDoors(true);
        }

        void MoveInLineOfSight(Unit* who)
        {
            if(!Intro && !Flying)
                ScriptedAI::MoveInLineOfSight(who);
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if(type != POINT_MOTION_TYPE)
                    return;

            if(Intro)
            {
                if(id >= 8)
                {
                    Intro = false;
                    me->SetHomePosition(IntroWay[7][0], IntroWay[7][1], IntroWay[7][2], 0);
                    return;
                }

                WaitTimer = 1;
            }

            if(Flying)
            {
                if(id == 0)
                {
                    me->MonsterTextEmote(EMOTE_BREATH, 0, true);
                    Flying = false;
                    Phase = 2;
                    return;
                }

                if(id == 3)
                {
                    MovePhase = 4;
                    WaitTimer = 1;
                    return;
                }

                if(id == 8)
                {
                    Flying = false;
                    Phase = 1;
                    Movement = true;
                    return;
                }

                WaitTimer = 1;
            }
        }

        void JustSummoned(Creature* summoned)
        {
            summoned->AI()->AttackStart(me->getVictim());
        }

        void TakeOff()
        {
            me->MonsterYell(YELL_FLY_PHASE, LANG_UNIVERSAL, 0);

            me->InterruptSpell(CURRENT_GENERIC_SPELL);
            me->HandleEmoteCommand(EMOTE_ONESHOT_LIFTOFF);
            me->AddUnitMovementFlag(MOVEMENTFLAG_LEVITATING);
            (*me).GetMotionMaster()->Clear(false);
            (*me).GetMotionMaster()->MovePoint(0, IntroWay[2][0], IntroWay[2][1], IntroWay[2][2]);

            Flying = true;

            FlyTimer = urand(45000, 60000); //timer wrong between 45 and 60 seconds
            ++FlyCount;

            RainofBonesTimer = 5000; //timer wrong (maybe)
            RainBones = false;
            Skeletons = false;
         }

        void UpdateAI(const uint32 diff)
        {
            if(WaitTimer <= diff)
            {
                if(Intro)
                {
                    if(MovePhase >= 7)
                    {
                        me->RemoveUnitMovementFlag(MOVEMENTFLAG_LEVITATING);
                        me->HandleEmoteCommand(EMOTE_ONESHOT_LAND);
                        me->GetMotionMaster()->MovePoint(8, IntroWay[7][0], IntroWay[7][1], IntroWay[7][2]);
                        AllowWaitTimer = false;
                    }
                    else
                    {
                        me->GetMotionMaster()->MovePoint(MovePhase, IntroWay[MovePhase][0], IntroWay[MovePhase][1], IntroWay[MovePhase][2]);
                        ++MovePhase;
                    }
                }
                if(Flying)
                {
                    if(MovePhase >= 7)
                    {
                        me->RemoveUnitMovementFlag(MOVEMENTFLAG_LEVITATING);
                        me->HandleEmoteCommand(EMOTE_ONESHOT_LAND);
                        me->GetMotionMaster()->MovePoint(8, IntroWay[7][0], IntroWay[7][1], IntroWay[7][2]);
                        AllowWaitTimer = false;
                    }
                    else
                    {
                        me->GetMotionMaster()->MovePoint(MovePhase, IntroWay[MovePhase][0], IntroWay[MovePhase][1], IntroWay[MovePhase][2]);
                        ++MovePhase;
                    }
                }

                WaitTimer = 0;
            } else WaitTimer -= diff;

            if(!UpdateVictim())
                return;

            if(Flying)
                return;

            //  Phase 1 "GROUND FIGHT"
            if(Phase == 1)
            {
                if(Movement)
                {
                    DoStartMovement(me->getVictim());
                    Movement = false;
                }

                if(BellowingRoarTimer <= diff)
                {
                    DoCast(me->getVictim(), SPELL_BELLOWING_ROAR);
                    BellowingRoarTimer = urand(30000, 40000);
                } else BellowingRoarTimer -= diff;

                if(SmolderingBreathTimer <= diff)
                {
                    DoCast(me->getVictim(), SPELL_SMOLDERING_BREATH);
                    SmolderingBreathTimer = 20000;
                } else SmolderingBreathTimer -= diff;

                if(CharredEarthTimer <= diff)
                {
                    if(Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                        DoCast(target, SPELL_CHARRED_EARTH);
                    CharredEarthTimer = 20000;
                } else CharredEarthTimer -= diff;

                if(TailSweepTimer <= diff)
                {
                    if(Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                        if(!me->HasInArc(M_PI, target))
                            DoCast(target, SPELL_TAIL_SWEEP);
                    TailSweepTimer = 15000;
                } else TailSweepTimer -= diff;

                if(SearingCindersTimer <= diff)
                {
                    if(Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                        DoCast(target, SPELL_SEARING_CINDERS);
                    SearingCindersTimer = 10000;
                } else SearingCindersTimer -= diff;

                uint32 Prozent = uint32(me->GetHealthPct());

                if(Prozent < 75 && FlyCount == 0) // first take off 75%
                    TakeOff();

                if(Prozent < 50 && FlyCount == 1) // secound take off 50%
                    TakeOff();

                if(Prozent < 25 && FlyCount == 2) // third take off 25%
                    TakeOff();

                DoMeleeAttackIfReady();
            }

            //Phase 2 "FLYING FIGHT"
            if(Phase == 2)
            {
                if(!RainBones)
                {
                    if(!Skeletons)
                    {
                        for(uint8 i = 0; i <= 3; ++i)
                        {
                            DoCast(me->getVictim(), SPELL_SUMMON_SKELETON);
                            Skeletons = true;
                        }
                    }

                    if(RainofBonesTimer < diff && !RainBones) // only once at the beginning of phase 2
                    {
                        DoCast(me->getVictim(), SPELL_RAIN_OF_BONES);
                        RainBones = true;
                        SmokingBlastTimer = 20000;
                    } else RainofBonesTimer -= diff;

                    if(DistractingAshTimer <= diff)
                    {
                        if(Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                            DoCast(target, SPELL_DISTRACTING_ASH);
                        DistractingAshTimer = 2000; //timer wrong
                    } else DistractingAshTimer -= diff;
                }

                if(RainBones)
                {
                    if(SmokingBlastTimer <= diff)
                     {
                        DoCast(me->getVictim(), SPELL_SMOKING_BLAST);
                        SmokingBlastTimer = 1500; //timer wrong
                     } else SmokingBlastTimer -= diff;
                }

                if(FireballBarrageTimer <= diff)
                {
                    if(Unit* target = SelectTarget(SELECT_TARGET_FARTHEST, 0))
                        DoCast(target, SPELL_FIREBALL_BARRAGE);
                    FireballBarrageTimer = 20000;
                } else FireballBarrageTimer -= diff;

                if(FlyTimer <= diff) //landing
                {
                    me->MonsterYell(RAND(*YELL_LAND_PHASE_1, *YELL_LAND_PHASE_2), LANG_UNIVERSAL, 0);

                    me->GetMotionMaster()->Clear(false);
                    me->GetMotionMaster()->MovePoint(3, IntroWay[3][0], IntroWay[3][1], IntroWay[3][2]);

                    Flying = true;
                } else FlyTimer -= diff;
            }
        }
    };
};

void AddSC_boss_nightbane()
{
    new boss_nightbane;
}
