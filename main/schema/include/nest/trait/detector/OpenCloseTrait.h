
/**
 *    Copyright (c) 2019 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 *    SOURCE TEMPLATE: trait.cpp.h
 *    SOURCE PROTO: nest/trait/detector/open_close_trait.proto
 *
 */
#ifndef _NEST_TRAIT_DETECTOR__OPEN_CLOSE_TRAIT_H_
#define _NEST_TRAIT_DETECTOR__OPEN_CLOSE_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>



namespace Schema {
namespace Nest {
namespace Trait {
namespace Detector {
namespace OpenCloseTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x235aU << 16) | 0x208U
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
    //  open_close_state                    OpenCloseState                       int               NO              NO
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
    // Enum for last handle
    //
    kLastSchemaHandle = 4,
};

//
// Events
//
struct OpenCloseEvent
{
    int32_t openCloseState;
    int32_t priorOpenCloseState;

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x235aU << 16) | 0x208U,
        kEventTypeId = 0x1U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct OpenCloseEvent_array {
    uint32_t num;
    OpenCloseEvent *buf;
};


//
// Enums
//

enum OpenCloseState {
    OPEN_CLOSE_STATE_CLOSED = 1,
    OPEN_CLOSE_STATE_OPEN = 2,
    OPEN_CLOSE_STATE_UNKNOWN = 3,
    OPEN_CLOSE_STATE_INVALID_CALIBRATION = 4,
};

} // namespace OpenCloseTrait
} // namespace Detector
} // namespace Trait
} // namespace Nest
} // namespace Schema
#endif // _NEST_TRAIT_DETECTOR__OPEN_CLOSE_TRAIT_H_
