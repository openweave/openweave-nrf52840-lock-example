#ifndef BOLT_LOCK_SETTINGS_TRAIT_DATA_SINK_H
#define BOLT_LOCK_SETTINGS_TRAIT_DATA_SINK_H

#include <Weave/Profiles/data-management/DataManagement.h>

class BoltLockSettingsTraitDataSink : public nl::Weave::Profiles::DataManagement::TraitDataSink
{
public:
    BoltLockSettingsTraitDataSink();

private:
    WEAVE_ERROR SetLeafData(nl::Weave::Profiles::DataManagement::PropertyPathHandle aLeafHandle, nl::Weave::TLV::TLVReader &aReader);

    bool mAutoRelockOn;
    uint32_t mAutoLockDurationSecs;
};

#endif /* BOLT_LOCK_SETTINGS_TRAIT_DATA_SINK_H */
