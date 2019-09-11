
/**
 *    Copyright (c) 2019 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 *    SOURCE TEMPLATE: trait.cpp.h
 *    SOURCE PROTO: nest/trait/security/security_open_close_trait.proto
 *
 */
#ifndef _NEST_TRAIT_SECURITY__SECURITY_OPEN_CLOSE_TRAIT_H_
#define _NEST_TRAIT_SECURITY__SECURITY_OPEN_CLOSE_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>

#include <nest/trait/detector/OpenCloseTrait.h>


namespace Schema {
namespace Nest {
namespace Trait {
namespace Security {
namespace SecurityOpenCloseTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x235aU << 16) | 0x20aU
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
    //  open_close_state                    nest.trait.detector.OpenCloseTrait.OpenCloseState int               NO              NO
    //
    kPropertyHandle_OpenCloseState = 2,

    //
    //  first_observed_at                   google.protobuf.Timestamp            uint32 seconds    YES             YES
    //
    kPropertyHandle_FirstObservedAt = 3,

    //
    //  first_observed_at_ms                google.protobuf.Timestamp            int64 millisecondsYES             YES
    //
    kPropertyHandle_FirstObservedAtMs = 4,

    //
    //  bypass_requested                    bool                                 bool              NO              NO
    //
    kPropertyHandle_BypassRequested = 5,

    //
    // Enum for last handle
    //
    kLastSchemaHandle = 5,
};

//
// Events
//
struct SecurityOpenCloseEvent
{
    int32_t openCloseState;
    int32_t priorOpenCloseState;
    bool bypassRequested;

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x235aU << 16) | 0x20aU,
        kEventTypeId = 0x1U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct SecurityOpenCloseEvent_array {
    uint32_t num;
    SecurityOpenCloseEvent *buf;
};


} // namespace SecurityOpenCloseTrait
} // namespace Security
} // namespace Trait
} // namespace Nest
} // namespace Schema
#endif // _NEST_TRAIT_SECURITY__SECURITY_OPEN_CLOSE_TRAIT_H_
