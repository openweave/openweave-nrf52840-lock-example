#ifndef BOLT_LOCK_TRAIT_DATA_SOURCE_H
#define BOLT_LOCK_TRAIT_DATA_SOURCE_H

#include <Weave/Profiles/data-management/DataManagement.h>

class BoltLockTraitDataSource : public nl::Weave::Profiles::DataManagement::TraitDataSource
{
public:
    BoltLockTraitDataSource();

    bool IsLocked();
    void InitiateLock(int32_t aLockActor);
    void InitiateUnlock(int32_t aLockActor);

    void LockingSuccessful(void);
    void UnlockingSuccessful(void);

private:
    WEAVE_ERROR GetLeafData(::nl::Weave::Profiles::DataManagement_Current::PropertyPathHandle aLeafHandle, uint64_t aTagToWrite,
                    ::nl::Weave::TLV::TLVWriter & aWriter);

    void OnCustomCommand(nl::Weave::Profiles::DataManagement::Command * aCommand,
            const nl::Weave::WeaveMessageInfo * aMsgInfo,
            nl::Weave::PacketBuffer * aPayload,
            const uint64_t & aCommandType,
            const bool aIsExpiryTimeValid,
            const int64_t & aExpiryTimeMicroSecond,
            const bool aIsMustBeVersionValid,
            const uint64_t & aMustBeVersion,
            nl::Weave::TLV::TLVReader & aArgumentReader);

    int32_t mLockedState;
    int32_t mLockActor;
    int32_t mActuatorState;
    int32_t mState;
};

#endif /* BOLT_LOCK_TRAIT_DATA_SOURCE_H */
