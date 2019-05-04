
/**
 *    Copyright (c) 2019 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 *    SOURCE TEMPLATE: trait.cpp.h
 *    SOURCE PROTO: weave/trait/security/bolt_lock_settings_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_SECURITY__BOLT_LOCK_SETTINGS_TRAIT_H_
#define _WEAVE_TRAIT_SECURITY__BOLT_LOCK_SETTINGS_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>



namespace Schema {
namespace Weave {
namespace Trait {
namespace Security {
namespace BoltLockSettingsTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x0U << 16) | 0xe08U
};

//
// Properties
//

enum {
    kPropertyHandle_Root = 1,

    //---------------------------------------------------------------------------------------------------------------------------//
    //  Name                                IDL Type                            TLV Type           Optional?       Nullable?     //
    //---------------------------------------------------------------------------------------------------------------------------//

    //
    //  auto_relock_on                      bool                                 bool              NO              NO
    //
    kPropertyHandle_AutoRelockOn = 2,

    //
    //  auto_relock_duration                google.protobuf.Duration             uint32 seconds    NO              NO
    //
    kPropertyHandle_AutoRelockDuration = 3,

    //
    // Enum for last handle
    //
    kLastSchemaHandle = 3,
};

} // namespace BoltLockSettingsTrait
} // namespace Security
} // namespace Trait
} // namespace Weave
} // namespace Schema
#endif // _WEAVE_TRAIT_SECURITY__BOLT_LOCK_SETTINGS_TRAIT_H_
